# Récap session — IA Zappy & correctif serveur (2026-06-26)

Document à destination de l'équipe : résumé de **toutes** les modifications de cette
session, avec le pourquoi. Cible : faire monter l'IA jusqu'au niveau 8 (victoire) et
corriger un bug de protocole côté serveur fait-main.

> ⚠️ Tests : les tests unitaires passent (24/24) ; les validations « jeu réel » ont été
> faites sur des fenêtres courtes/moyennes. À re-tester en conditions réelles.

---

## TL;DR des changements

| Domaine | Fichier(s) | Résumé |
|---|---|---|
| Lancement | `launch.sh` | Utilise `zappy_team` (pas `zappy_ai`) ; **freq 20 → 100** ; `-c 18`. |
| Peuplement | `ai/src/launcher.py` | **Démarre à 1 bot/équipe** ; peuplement par fork jusqu'à 12 ; respawn anti-extinction. |
| Coordination | `ai/src/strategy.py` | Broadcast scopé par équipe ; incant-and-retry ; collecte par rang ; économie de nourriture anti-churn ; **fork anti-blocage**. |
| Navigation | `ai/src/pathfinding.py` | Homing pivot+avance (convergence ~2× plus rapide). |
| Boucle réseau | `ai/src/bot.py` | **Fix deadlock d'incantation** + watchdog ; dépose/ramasse anti-vol ; grab opportuniste (thystame). |
| Serveur | `server/src/commands/CommandExecutor.cpp` | **Fix convention de direction K** du Broadcast/Eject (gauche/droite étaient inversés vs référence). |
| Tests | `ai/tests/` | `test_strategy.py` (20), `test_bot_resync.py` (4). |

---

## 1. Bugs critiques corrigés

### 1.2 `Connect_nbr` jamais appelé (condition morte)
La condition de sondage des forks était `level==1 && state==LEAD`, jamais vraie (niveau 1
est solo, donc jamais LEAD). Les bots ne forkaient jamais. **Corrigé** (sondage périodique).

### 1.3 Deadlock d'incantation (cause du « les bots se figent et meurent »)
**Cause prouvée empiriquement** (`scratchpad/probe_freeze.py`) : quand un bot est figé
comme **participant** à l'incantation d'un autre, le serveur **annule sa commande en vol
sans envoyer de réponse**. Côté IA, `pending_commands` reste bloqué à 1 → plus aucune
commande envoyée → famine. Probabiliste (selon qu'on soit au milieu d'une commande au
moment du gel), d'où l'apparition « au bout d'un moment ».

**Fix (`bot.py`)** :
- Flag `awaiting_incant_result` posé sur `Elevation underway` ; au résultat
  (`Current level`/`ko`) on purge le suivi des commandes, remet `pending=0`, et repart.
- **Watchdog** : si aucune activité serveur pendant `ZAPPY_WATCHDOG_SEC` (déf. 20 s) hors
  incantation légitime → resynchronisation automatique (filet de sécurité général).
- Handler `eject:` durci pareil.
- Régression couverte par `test_bot_resync.py`.

### 1.4 Convention de direction K inversée (serveur fait-main)
`soundDirection` calculait **droite = K3 / gauche = K7**, alors que le binaire de
**référence** utilise **droite = K7 / gauche = K3** (vérifié empiriquement,
`scratchpad/k_test3.py`, 3 runs cohérents avec contrôle de cohérence). L'IA, elle, était
**correcte** (calibrée sur la référence). Donc un GUI/IA branché sur notre serveur recevait
des directions miroir pour les côtés → échec de validation croisée.

---

## 2. Refonte de la stratégie IA (modèle essaim)

### 2.1 Broadcast scopé par équipe
Format : `TAG:équipe:niveau:rang` (avant : `TAG:niveau:rang`). Les messages d'une autre
équipe sont ignorés → fini les level-up « avec l'adversaire » et la convergence croisée.
Tags : `P` (présence/élection), `Z` (forage), `G` (assemblée).

> Note protocole : le serveur ne filtre PAS les participants d'incantation par équipe ;
> un ennemi de même niveau sur la case monte gratuitement. Inévitable sauf `Eject`.

### 2.2 Incant-and-retry (au lieu du lockstep par comptage)
`Look` est **aveugle** (les jetons `player` ne portent ni niveau ni équipe). Plutôt que de
compter les corps, le leader **tente** l'incantation dès que le set est au sol + `needed`
corps présents ; un `ko` est instantané et gratuit (pierres conservées) → on retente.

### 2.3 Leader figé tôt + suiveurs qui apportent les pierres
Le leader se **fige tôt** (grâce d'élection + nourriture) et balise, **sans** attendre le
set complet (sinon, aux paliers hauts à 7+ pierres, il ne baliserait jamais). Les suiveurs
foragent puis convergent déposer (modèle 2 phases Z/G).

### 2.4 Collecte distribuée par rang + gap-fill
`my_share()` répartit le set du palier en round-robin selon la position du bot dans les
pairs triés (déterministe, sans dialogue point-à-point — impossible dans le protocole).
Les trous (désaccord/mort d'un porteur) sont comblés par un **gap-fill** au rendez-vous.
Plus le **grab opportuniste** : tout bot ramasse une pierre requise sous ses pieds même
hors de sa part — décisif pour le **thystame** (palier 7→8), la ressource la plus rare.

### 2.5 Économie de nourriture anti-churn
Symptôme : au palier 4, un bot repartait manger pendant qu'un autre arrivait → la cohorte
n'atteignait jamais la masse critique. Fix :
- Gros **stock de nourriture** constitué AVANT de rejoindre le rendez-vous (`FOOD_GO`).
- En attente (`WAIT_TILE`), on **reste** sur la case (on ne part qu'à `FOOD_PANIC`, et on
  mange ce qui réapparaît sur place).
- Invariant : `FOOD_PANIC < FOOD_HUNGRY <= FOOD_TARGET <= FOOD_RESERVE` (sinon zone morte
  de foraging qui bloquait même la montée solo niveau 1).

### 2.6 Homing pivot+avance
Avant, un bot sur le côté **pivotait sur place** (un beacon « gâché »). Désormais il pivote
ET avance dans le même cycle → convergence ~2× plus rapide.

---

## 3. Démarrage à 1 bot + reproduction anti-blocage (nouveau)

### 3.1 Démarrer seul
`launcher.py` : `INITIAL_BOTS = 1`. Chaque équipe démarre avec **un seul** individu qui
peuple ensuite l'équipe par fork. `TEAM_TARGET = 12` (plafond de bots vivants).

### 3.2 Fork quand la progression est bloquée
Problème observé : 4 bots niveau 6, 3 niveau 5, 1 niveau 2 → personne ne peut évoluer
(aucun niveau n'a l'effectif requis) et **plus personne ne pond**. Les bots figés en
`LEAD`/`WAIT_TILE` ne sondaient jamais `Connect_nbr`.

Fix :
- `is_progress_stalled()` : palier de groupe + pas assez de pairs de mon niveau +
  ça dure (`level_age`). Dans ce cas on autorise le fork **même figé** en LEAD/WAIT_TILE.
- `want_fork()` : pond si l'effectif perçu de mon niveau est sous le minimum du palier,
  throttlé par un **cooldown** (`FORK_COOLDOWN`) et un **budget par palier**
  (`MAX_FORKS`, réinitialisé à chaque montée). Forker produit des bébés niveau 1 qui
  alimentent le pipeline ; à terme un niveau accumule assez de joueurs et débloque
  (l'incantation élève **tous** les présents du bon niveau).

## Limites connues / pistes

-- reproduction directe a 12 au lieu de faire automatiquement le nombre necessaire pour atteindre le palier final. Il faudrait automatiquement detecter la situation entiere de l'essaim et forker le nombre exact qu'il manque. 
-- faire en sorte que les bots se regroupent facilement poour l'incantation. Peut etre faire moins de stock si on nous appelle pour une invocation.
-- si on a du sureffectif a cause d'une erreur, d'un probleme. Un bot qui n'a pas d'objectif pourrait se suicider ou aller embeter d'autres equipes.
-- pas encore de gestion des autres equipes, potentiel pvp a ajouter / incrustation aux incantations. (mise en place avant mais remove pour le moment).