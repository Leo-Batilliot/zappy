# Documentation — IA Zappy

Documentation complète de la partie IA : comment compiler et lancer les binaires,
l'architecture du code, la stratégie de jeu, le protocole utilisé, le réglage des
paramètres et les limites connues.

---

## 1. Lancer les binaires

### 1.1 Compilation

Tout se compile depuis la racine du dépôt avec un seul `make` :

```bash
make            # compile server + gui + ai (et le launcher zappy_team)
make ai         # compile uniquement la partie IA
make re         # fclean + rebuild complet
```

`make ai` génère **deux** exécutables à la racine du dépôt :

| Binaire       | Rôle                                                                                  |
|---------------|---------------------------------------------------------------------------------------|
| `zappy_ai`    | **un** client/drone par exécution (binaire conforme au sujet, lancé un par terminal). |
| `zappy_team`  | **launcher** : lance un bot puis en spawn d'autres à chaque ponte.                     |

Les deux ne sont que des wrappers shell qui appellent `python3 ai/src/main.py`
(resp. `ai/src/launcher.py`). **Aucune dépendance externe** : Python 3 standard
suffit (sockets, pas de pip).

### 1.2 Lancement d'une partie

Dans **3 terminaux séparés**, depuis la racine :

```bash
# 1) le serveur
./zappy_server -p 4242 -x 20 -y 20 -n team1 team2 -c 6 -f 100

# 2) le GUI
./zappy_gui -p 4242 -h localhost

# 3) L'IA
./zappy_team -p 4242 -n team1 -h localhost
```

Paramètres serveur utiles :
- `-c 6` : nombre de slots (œufs) par équipe au départ. Il en faut au minimum 6
  pour viser le niveau 8.
- `-f 100` : fréquence du jeu (plus élevé = plus rapide). On teste à `-f 100`.

### 1.3 Les deux binaires

Le projet fournit **deux exécutables distincts**, utilisables indépendamment.
Voici l'usage de chacun.

#### `zappy_ai` — UN client, lancé manuellement dans son propre terminal

C'est le binaire **conforme au sujet** : `./zappy_ai -p port -n name -h machine`
connecte **un seul drone**. Pour avoir une équipe de N drones, on lance
**N fois `zappy_ai`, chacun dans un terminal séparé** :

```bash
# Terminal A
./zappy_ai -p 4242 -n team1 -h localhost
# Terminal B
./zappy_ai -p 4242 -n team1 -h localhost
# Terminal C
./zappy_ai -p 4242 -n team1 -h localhost
# ... autant de terminaux que de drones voulus
```

Chaque instance est un **process indépendant** qui rejoint la même équipe et se
coordonne par broadcast. En mode `zappy_ai` pur, **personne n'occupe
automatiquement les œufs** : un `Fork` crée bien un œuf côté serveur, mais aucun
process ne vient le remplir tant qu'un nouveau `zappy_ai` n'est pas relancé dans
un autre terminal. C'est le mode « un terminal = un drone » du sujet.

#### `zappy_team` — le launcher qui peuple l'équipe automatiquement

`./zappy_team` lance un bot, surveille sa sortie, et **dès qu'un bot pond**
(`##FORK##` imprimé sur stdout) il **spawn automatiquement un nouveau process**
pour occuper l'œuf. **Un seul terminal suffit** pour faire grandir toute l'équipe :

```bash
./zappy_team -p 4242 -n team1 -h localhost   # un terminal, l'équipe se peuple seule
```

#### Récapitulatif

| Binaire      | Combien de drones   | Lancement                          | Reproduction auto |
|--------------|---------------------|------------------------------------|-------------------|
| `zappy_ai`   | **1 par exécution** | **manuel, un terminal par drone**  | non               |
| `zappy_team` | toute l'équipe      | un seul terminal                   | oui (`##FORK##`)  |

Les deux acceptent exactement les flags du sujet : `-p` (port), `-n` (nom
d'équipe), `-h` (machine). `-h` vaut `localhost`/`127.0.0.1` par défaut.

### 1.4 Afficher l'usage : `-help`

Les deux binaires acceptent `-help`, qui affiche la ligne d'usage et quitte
immédiatement (sans se connecter au serveur) :

```bash
./zappy_ai -help
# USAGE: ./zappy_ai -p port -n name [-h machine]
        -p port      port number
        -n name      name of the team
        -h machine   hostname of the server (localhost by default)

./zappy_team -help
# USAGE: ./zappy_team -p port -n name [-h machine]
        -p port      port number
        -n name      name of the team
        -h machine   hostname of the server (localhost by default)
```

`zappy_ai` affiche aussi cet usage et sort en erreur si `-p` ou `-n` est manquant.

### 1.5 `-p` vs `-h`

- `-p` = **port** TCP du serveur.
- `-h` = **host** (machine) du serveur.

Le serveur bind sur `INADDR_ANY` (`server/src/server/Server.cpp`), il écoute donc
sur toutes les interfaces : un client sur une autre machine peut s'y connecter en
passant l'IP de la machine serveur via `-h`.

---

## 2. Architecture du code (`ai/src/`)

| Fichier          | Responsabilité                                                            |
|------------------|--------------------------------------------------------------------------|
| `main.py`        | Point d'entrée d'**un** bot. Parse `-p/-n/-h`, instancie `ZappyBot`.      |
| `launcher.py`    | Binaire `zappy_team`. Spawn/surveille les process bots, écoute `##FORK##`.|
| `network.py`     | Client TCP : `connect`, `send_line`, lecture non-bloquante par lignes.    |
| `bot.py`         | Boucle réseau + cadencement des commandes + traduction état → commandes.  |
| `strategy.py`    | **Le cerveau** : FSM de décision, élection, collecte, fork, deadlock.     |
| `state.py`       | Constantes du jeu : exigences d'élévation, inventaire, états FSM.          |
| `pathfinding.py` | Parsing du `Look`, conversion index→coordonnées, déplacement vers balise.  |

Séparation clé : **`bot.py` = le « comment » réseau** (une commande en vol à la
fois, routage des réponses serveur), **`strategy.py` = le « quoi faire »** (état
pur, sans I/O). `bot.py` appelle `strategy.update()` après chaque `Inventory`,
lit `strategy.state`, et émet les commandes correspondantes.

### 2.1 Le launcher (`zappy_team`) en détail

`main.py` et `launcher.py` ont chacun leur propre fonction `main()` parce que ce
sont **deux programmes distincts** :

- `main.py` → `main()` = **un drone** (`zappy_ai`). Il se connecte au serveur et
  **joue**.
- `launcher.py` → `main()` = le **superviseur** (`zappy_team`). Il **ne joue pas** :
  il lance des drones et en relance un à chaque reproduction.

Avoir deux `main()` est normal : chaque exécutable a besoin de son propre point
d'entrée. Le launcher est un **parent** qui lance plusieurs **enfants** `zappy_ai`.

Déroulé :

1. `./zappy_team` démarre : son `main()` localise le binaire `zappy_ai` sur le disque.
2. `spawn_bot()` lance un `zappy_ai` comme **process enfant** (équivalent d'ouvrir
   un terminal et taper `./zappy_ai ...` à ta place) et capture tout ce qu'il affiche.
3. Un thread `monitor` lit la sortie de l'enfant ligne par ligne et la ré-affiche
   (tous les logs des bots arrivent ainsi dans un seul terminal).
4. Quand un drone se reproduit avec succès (`Fork` ok), il imprime la ligne spéciale
   `##FORK##`.
5. Le `monitor` voit `##FORK##` → rappelle `spawn_bot()` → un **nouveau** `zappy_ai`
   démarre, se connecte, et occupe l'œuf que le serveur vient de créer.
6. Ça se répète, l'équipe grandit, plafonnée par `TEAM_TARGET`.

```
zappy_team  (launcher.py : main)         superviseur, ne joue pas
   │  lance
   ├── zappy_ai #0  (main.py : main)      joue, connecté au serveur
   │        └─ Fork ok → imprime "##FORK##"
   │                         │
   ├── zappy_ai #1  ◄────────┘            (relancé par le launcher)
   ├── zappy_ai #2
   └── ...                                (jusqu'à TEAM_TARGET)
```

---

## 3. La stratégie de jeu

### 3.1 Vue d'ensemble

Objectif : amener **6 joueurs au niveau 8** (condition de victoire) le plus vite
possible. Le principe : se regrouper sur une case commune et lancer l'incantation
**dès que le nombre requis de joueurs pour le palier est présent** (`needed`) —
**et pas forcément toute l'équipe**. Aux premiers paliers (1, puis 2, 2, 4, 4
joueurs requis), ce sont donc de **petits groupes** qui montent dès qu'ils sont au
complet, indépendamment du reste de l'équipe ; le besoin de réunir **6 joueurs**
sur une même case n'apparaît qu'aux paliers **6→7 et 7→8**. Ce n'est donc pas un
lockstep strict de toute l'équipe : c'est un enchaînement de regroupements de
taille `needed`.

Contrainte forte du protocole : le `Look` est **aveugle** — un jeton `player` ne
dit ni son niveau ni son équipe ; et le `Broadcast` n'accepte qu'**un seul token
sans espace**. Toute la coordination passe donc par un protocole de balises
encodées dans le broadcast, pas par du dialogue point-à-point.

### 3.2 Élection de leader (rang aléatoire)

À un palier donné, plusieurs bots doivent se retrouver sur **une même case** pour
incanter. Mais ils ne peuvent pas se parler directement (le broadcast est anonyme
et limité à un seul token) : si chacun choisissait son coin, ils ne se croiseraient
jamais. Il faut donc désigner **un seul** bot dont la position fait office de point
de rendez-vous : le **leader**. L'élection est entièrement décentralisée et ne
repose que sur des broadcasts.

Comment les bots tombent d'accord sans se parler :

1. À sa naissance, chaque bot tire un **grand nombre au hasard** (`self.idx`, dans
   un intervalle énorme, donc pratiquement unique) : c'est son **rang**.
2. Chaque bot **diffuse ce rang** en continu (`P:team:niveau:idx`).
3. Règle commune à tous : **le leader est le plus petit rang vivant**. Chaque bot
   retient le plus petit rang entendu (le sien compris) et l'adopte comme leader ;
   s'il en entend un plus petit, il change.
4. Les broadcasts étant **globaux** (tout le monde entend tout le monde) et tout le
   monde appliquant la **même règle**, les bots d'un palier convergent **tous vers
   le même** leader — celui de plus petit rang — **sans négociation ni vote**.
5. Un bot se considère leader tant que `leader_idx == self.idx` (personne de plus
   petit n'existe).

Deux sécurités :

- **`ELECTION_GRACE`** : un leader fraîchement élu attend quelques cycles avant de
  se figer comme point de rendez-vous, le temps d'entendre s'il existe un rang plus
  petit. Sans ça, deux bots pourraient se croire leader au démarrage et se figer à
  deux endroits différents (*split*).
- **`LEADER_SILENCE_TIMEOUT`** : si le leader meurt, il cesse d'émettre. Un suiveur
  qui ne l'entend plus pendant trop longtemps « l'oublie » et relance l'élection
  (le rang suivant devient leader). C'est la bascule automatique en cas de mort.

### 3.3 Niveau 1 = SOLO

Le palier 1→2 ne demande qu'**un seul joueur**. Chaque bot monte donc seul
(linemate + un peu de food), sans rendez-vous : démarrage parallèle immédiat, pas
de temps perdu à se regrouper.

### 3.4 Niveaux ≥ 2 = GATHER (rendez-vous à 2 phases)

Dès qu'un palier exige plusieurs joueurs, on bascule sur un **rendez-vous point
fixe** mené par le leader :

1. **Le leader se fige tôt** (dès qu'il a la grâce d'élection + un minimum de
   food `LEADER_COMMIT_FOOD`), sans attendre d'avoir tout le set de pierres. Sinon,
   aux paliers hauts (7 pierres au niveau 6), il errerait sans jamais baliser et
   personne ne convergerait. Une fois figé, il **reste sur place** : c'est le
   point de rendez-vous stable.

2. **Phase Z (forage)** : tant que le set complet de pierres n'est pas sur la case
   du leader, chaque suiveur forage **sa part** assignée du set (split par rang),
   puis vient la **déposer** (`Set`) sur la case du leader. Le leader balise `Z`.

3. **Phase G (assemblée)** : quand le set complet est au sol, le leader balise `G`.
   Il suffit alors aux suiveurs d'**être présents** (plus besoin de pierres) :
   les corps convergent sur la case.

4. **Incantation** : dès que **le nombre requis** (`needed`) de joueurs est présent
   avec le set au sol — **pas toute l'équipe** — le leader lance le rituel. Seuls
   les joueurs **présents sur la case** montent d'un niveau ; les autres restent à
   leur palier et continuent (ils formeront le groupe suivant). À niveau égal,
   plusieurs sous-groupes peuvent donc progresser en parallèle aux premiers paliers.

### 3.5 Collecte distribuée par rang

Pour ne pas que tous les bots collectent la même pierre, le set requis est étendu
par quantité en une liste et **distribué en round-robin** selon la position du bot
dans la liste triée des participants (`my_share`). Comme chaque bot calcule la
même liste à partir des rangs entendus, la répartition est cohérente **sans
dialogue** (impossible dans le protocole). Les trous éventuels (désaccord sur les
pairs, mort d'un porteur) sont rattrapés par un **gap-fill** au rendez-vous
(`_fetch_missing_for_tile`).

### 3.6 Incant-and-retry

Le `Look` étant aveugle, compter les corps sur la case ne garantit pas qu'on a
`needed` joueurs **du bon niveau**. Plutôt qu'attendre un comptage parfait, on
**tente** l'incantation dès que le set est au sol et qu'assez de corps sont là.
Le serveur arbitre : un `ko` est instantané et **sans coût** (pierres conservées),
on temporise alors (`INCANT_RETRY_COOLDOWN`) et on retente. Le rituel part donc à
la milliseconde où assez de bons joueurs ont convergé.

### 3.7 Reproduction (Fork) et contrôle de population

Un bot pond (`Fork`) pour **peupler** quand l'effectif manque, dans la limite d'un
budget par palier (`MAX_FORKS`) et d'un cooldown (`FORK_COOLDOWN`). Chaque ponte
réussie émet `##FORK##`, capté par le launcher qui spawn un nouveau process. La
logique des plafonds de population est détaillée dans la section
« Contrôle de population ».

### 3.8 Anti-deadlock

Un **vrai deadlock** survient quand un niveau manque d'effectif depuis longtemps
(`HARD_DEADLOCK_DELAY`) **et** qu'aucun coéquipier n'est à un niveau inférieur pour
remonter combler le trou (`_lower_level_peers() == 0`). C'est le seul cas où la
seule issue est de **forker des bébés** qui remonteront le pipeline. On distingue
ce vrai deadlock du *stall* transitoire normal de la montée.

### 3.9 Résilience (survie)

- **Faim** : sous `FOOD_PANIC`, le bot lâche tout et part manger (état `SURVIVE`).
- **Stock avant engagement** : un suiveur ne rejoint le rendez-vous qu'avec assez
  de food (`FOOD_GO`) pour tenir l'attente sans repartir manger — c'est ce qui a
  supprimé le churn qui bloquait au niveau 4.
- **Effectif fenêtré** : la taille d'équipe est estimée sur une fenêtre temporelle
  (`PEER_WINDOW`) ; un bot mort cesse d'être compté, ce qui **réabaisse la cible**
  et débloque une incantation qui attendait un mort.
- **Watchdog** (`bot.py`) : si plus aucun message serveur n'arrive pendant
  `ZAPPY_WATCHDOG_SEC`, le bot vide ses files et renvoie un `Inventory` pour se
  resynchroniser.

---

## 4. Contrôle de population : les caps

### 4.1 Le nombre magique = 6

Exigences en joueurs par palier (`ELEVATION_REQUIREMENTS`) :

| Incantation | Joueurs requis |
|-------------|----------------|
| 1 → 2       | 1              |
| 2 → 3       | 2              |
| 3 → 4       | 2              |
| 4 → 5       | 4              |
| 5 → 6       | 4              |
| 6 → 7       | **6**          |
| 7 → 8       | **6**          |

Le maximum est **6**, et la victoire exige **6 joueurs au niveau 8**. Une
incantation élève **tous les joueurs présents** sur la case dès que le compte
atteint le requis, et les **pierres sont partagées** (consommées une seule fois).
En théorie, **6 bots suffisent** donc à monter de 1 à 8.

En pratique, l'IA déclenche le rituel dès que **`needed` joueurs** sont là (le
requis du palier), **et non toute l'équipe** : aux premiers paliers, des
sous-groupes peuvent monter séparément. Le point dur reste de réunir **6 joueurs
de niveau 7** sur une case. Toute population au-delà de 6 n'est qu'une
**assurance** contre la mort, pas une nécessité.

### 4.2 Les deux caps, à deux étages différents

| Constante                 | Fichier       | Rôle                                                        |
|---------------------------|---------------|-------------------------------------------------------------|
| `POP_TARGET` (= 8)        | `strategy.py` | Cible normale : un bot cesse d'**envoyer** `Fork` à ce seuil.|
| `DEADLOCK_POP_CAP` (= 12) | `strategy.py` | Même rôle, mais en situation de deadlock avéré.             |
| `TEAM_TARGET` (= 18)      | `launcher.py` | **Mur dur** : le launcher refuse de **spawner** au-delà.    |

**Règle d'or : `TEAM_TARGET ≥ DEADLOCK_POP_CAP`.** Sinon le cap anti-deadlock est
du code mort : les bots *veulent* monter à 12, mais le launcher bloque avant et
les bébés de backfill ne sont jamais lancés.

Nuance : `POP_TARGET`/`DEADLOCK_POP_CAP` sont des **cibles souples** (estimation
par bot, fenêtrée, qui peut déborder un peu), tandis que `TEAM_TARGET` est le
**plafond ferme** réel.

### 4.3 Configs cohérentes

| Config                | DEADLOCK_POP_CAP | TEAM_TARGET | Comportement                            |
|-----------------------|------------------|-------------|-----------------------------------------|
| Actuelle              | 12               | 18          | vise 12, peut déborder jusqu'à 18       |
| Plafond ferme à 12    | 12               | 12          | vrai mur à 12, backfill OK              |
| Lean (optimal math)   | 9                | 9           | mur à 9 ; suffisant (6 gagnants + marge)|

**Augmenter le nombre de bots n'améliore pas la robustesse.** Le risque principal
est la **famine** : la consommation de nourriture est linéaire et la nourriture de
la carte est finie. Trop de joueurs entraîne plus de morts, l'effectif repasse
sous 6 et la partie est perdue ; un effectif dispersé ralentit aussi le
regroupement. La marge de sécurité tient dans l'**économie de nourriture** et le
maintien des **6 joueurs groupés**, pas dans le nombre. Valeur de référence :
**6 joueurs gagnants + 1 à 2 d'assurance, soit environ 8**.

---

## 5. Protocole de balises (encodage broadcast)

Le `Broadcast` n'accepte qu'**un token sans espace**. On encode donc toute
l'information de coordination dans une chaîne `:` :

```
<TAG>:<team>:<level>:<rank>
```

| TAG | Sens                                                                       |
|-----|----------------------------------------------------------------------------|
| `P` | **Ping** : présence + participation à l'élection (rang).                    |
| `Z` | Le leader est figé, **phase forage** (set pas encore au sol).               |
| `G` | Le leader est figé, **phase assemblée** (set au sol, venez vous poser).     |

- `team` permet d'ignorer les balises adverses (`if team != self.team_name`).
- `level` : on n'agit que sur les balises de **son propre niveau** (mais on
  enregistre quand même le rang/niveau de l'émetteur dans le roster global, ce qui
  alimente `total_team_size()` et `_lower_level_peers()`).
- La **direction** `K` du broadcast (1=devant, 3=gauche, 5=derrière, 7=droite,
  0=même case) sert à se rapprocher du leader (`pathfinding.moves_toward_broadcast`).

> Note serveur : `Broadcast` renvoie un token unique (sans espace), et la
> direction `K` suit la convention front=1 / gauche=3 / arrière=5 / droite=7,
> case courante=0.

---

## 6. Variables d'environnement (debug / tuning)

| Variable               | Effet                                                              |
|------------------------|-------------------------------------------------------------------|
| `ZAPPY_DEBUG`          | Logs détaillés `[IN]/[OUT]/[STRAT]` de chaque bot.                 |
| `ZAPPY_HEARTBEAT`      | Ligne d'état périodique (`idx`, niveau, état, food…) toutes les 2s.|
| `ZAPPY_TS`             | Préfixe chaque ligne du launcher d'un timestamp relatif.          |
| `ZAPPY_WATCHDOG_SEC`   | Délai du watchdog de resynchro (défaut 20s).                       |
| `ZAPPY_START_LEVEL`    | Niveau de départ d'un bot (debug d'un palier précis).             |

Exemple de run instrumenté :

```bash
ZAPPY_TS=1 ZAPPY_DEBUG=1 ./zappy_team -p 4242 -n team1 -h localhost
```

---

## 7. Principales constantes de réglage (`strategy.py`)

| Constante              | Val. | Rôle                                                       |
|------------------------|------|------------------------------------------------------------|
| `FOOD_PANIC`           | 7    | Sous ce seuil : tout lâcher et manger.                     |
| `FOOD_HUNGRY`          | 28   | En dessous : prioriser la food dans le `Look`.             |
| `FOOD_GO`              | 32   | Stock min pour qu'un suiveur rejoigne le rendez-vous.      |
| `FOOD_RESERVE`         | 42   | Stock de food visé en mode regroupement.                   |
| `LEADER_COMMIT_FOOD`   | 42   | Food min pour que le leader se fige.                       |
| `FOOD_MIN_INCANT`      | 14   | Food min pour tenter une incantation (solo niv 1).         |
| `LEADER_SILENCE_TIMEOUT`| 30  | Silence du leader avant réélection.                        |
| `INCANT_RETRY_COOLDOWN`| 6    | Temporisation après un `ko` d'incantation.                 |
| `MAX_FORKS`            | 6    | Budget de pontes par palier.                               |
| `FORK_COOLDOWN`        | 18   | Cycles min entre deux pontes.                              |
| `POP_TARGET`           | 8    | Cible de population normale (cap souple du fork).          |
| `DEADLOCK_POP_CAP`     | 12   | Cible de population en deadlock avéré.                     |
| `HARD_DEADLOCK_DELAY`  | 50   | Ancienneté de palier avant de déclarer un deadlock.        |
| `ELECTION_GRACE`       | 12   | Grâce anti-split avant que le leader se fige.              |
| `PEER_WINDOW`          | 80   | Fenêtre (cycles) de comptage des pairs vivants.            |

---

## 8. Limites connues

1. **Regroupement des 6.** L'assemblée des 6 joueurs de niveau 7 sur une même case
   est l'étape la plus **lente et variable**. 'est le principal point d'amélioration restant.

2. **Pas de PvP offensif.** L'IA ne cherche pas à gêner les équipes adverses.

3. **Population pilotée en cible, pas garantie.** Les caps de fork sont des cibles
   souples ; un léger dépassement est possible avant ue le mur dur du launcher ne s'applique.

---

## 9. Test rapide

```bash
make re
# Map 20x20
./zappy_server -p 4242 -x 20 -y 20 -n team1 -c 6 -f 100 &
ZAPPY_TS=1 ./zappy_team -p 4242 -n team1 -h localhost
# Attendre la ligne : "RITUEL RÉUSSI ! Current level: 8"
```

Le temps d'atteinte du niveau 8 varie selon la charge de la machine : prévoir une
fenêtre de plusieurs minutes avant de conclure à un échec.
