# Zappy — Documentation du serveur

Documentation technique de l'exécutable `zappy_server` tel qu'implémenté
actuellement. Elle décrit l'architecture, le modèle de temps, le protocole réseau
(IA et GUI), les mécaniques de jeu et l'état d'avancement des commandes.

---

## 1. Vue d'ensemble

`zappy_server` est l'arbitre d'une partie Zappy. Il :

- accueille deux types de clients sur un même port TCP : les **IA** (un client par
  joueur) et les **GUI** (clients graphiques observateurs, équipe réservée
  `GRAPHIC`) ;
- maintient l'état du monde (carte torique, ressources, joueurs, œufs, équipes) ;
- exécute les commandes des IA avec un **coût en temps** et applique les
  mécaniques périodiques (faim, réapparition des ressources) ;
- diffuse en continu l'état du jeu aux GUI ;
- décide de la fin de partie.

Le serveur est **mono-thread** et **piloté par événements** : un unique `poll()`
gère à la fois les entrées/sorties réseau et l'échéance du prochain événement de
jeu. Il n'y a **aucune attente active** ni `nanosleep`.

### Arborescence des sources

```
src/
├── main.cpp              Point d'entrée, gestion des erreurs et de --help
├── Error.hpp             Hiérarchie d'exceptions (Error / ArgError / NetworkError)
├── args/                 Analyse de la ligne de commande
│   ├── Parser.hpp/.cpp   Parse + validation des arguments → Config
├── time/                 Modèle de temps et ordonnancement
│   ├── Clock.hpp         Horloge, conversions unités de temps ↔ secondes/ms
│   ├── Event.hpp         Événement planifié (id, échéance, callback)
│   └── Scheduler.hpp/.cpp File d'événements (min-heap) à annulation paresseuse
├── game/                 Modèle de jeu
│   ├── Types.hpp         Ressources, orientation, constantes, inventaire
│   ├── Tile.hpp          Contenu d'une case (ressources, joueurs, œufs)
│   ├── Player.hpp        État d'un joueur
│   ├── Egg.hpp           État d'un œuf
│   ├── Team.hpp          État d'une équipe (slots libres, joueurs, œufs)
│   ├── World.hpp/.cpp    Carte torique, vision, ressources, géométrie
│   └── GameState.hpp/.cpp Agrégat monde + joueurs + œufs + équipes
├── protocol/             Sérialisation du protocole
│   ├── AiProtocol.hpp    Réponses textuelles vers les IA
│   └── GuiProtocol.hpp/.cpp Événements vers les GUI
├── server/               Réseau et boucle principale
│   ├── Client.hpp/.cpp   Connexion (état, buffers de lecture/écriture)
│   └── Server.hpp/.cpp   Socket, boucle poll, handshake, mécaniques de jeu
└── commands/             Exécution des commandes IA
    └── CommandExecutor.hpp/.cpp Table de dispatch, file par joueur, minuterie
```

---

## 2. Compilation et lancement

```bash
make            # construit ./zappy_server (g++ -Wall -Wextra -std=c++20)
make re         # reconstruction complète
```

### Ligne de commande

```
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
```

| Drapeau | Obligatoire | Description |
|---------|-------------|-------------|
| `-p`    | oui | Port TCP d'écoute (1..65535) |
| `-x`    | oui | Largeur de la carte (> 0) |
| `-y`    | oui | Hauteur de la carte (> 0) |
| `-n`    | oui | Liste des noms d'équipes (un ou plusieurs) |
| `-c`    | oui | Nombre de clients (slots) par équipe (> 0) |
| `-f`    | non | Fréquence ; inverse de l'unité de temps (défaut `100`, > 0) |

`--help` affiche l'usage et quitte avec le code `0`.

### Validation des arguments (`Parser::validate`)

- Chaque drapeau entier doit consommer toute sa valeur (`std::from_chars` strict).
- Le nom d'équipe `GRAPHIC` est **réservé** (utilisé par les GUI) et refusé.
- Les noms d'équipe en double sont refusés.
- Toute erreur lève une `ArgError` ; `main` l'attrape, écrit le message sur
  `stderr` et quitte avec le code **84**.

---

## 3. Modèle de temps

Défini dans `time/Clock.hpp`. L'unité de temps vaut `1 / freq` seconde. Une action
qui coûte `n` unités dure donc `n / freq` secondes en temps réel.

- `secondsFor(units, freq)` → durée en secondes.
- `deadlineFor(units, freq)` → `TimePoint` absolu (`now + units/freq`) dans la
  résolution de `std::chrono::steady_clock`.
- `toMilliseconds(duration)` → entier ≥ 0 (utilisé comme timeout de `poll`).

### Ordonnanceur (`time/Scheduler`)

File de priorité (min-heap sur l'échéance) d'`Event { id, due, run, cancelled }`.

- `schedule(due, fn)` → renvoie un `id` (≥ 1) et empile l'événement.
- `cancel(id)` → **annulation paresseuse** : l'id est mémorisé dans un ensemble ;
  l'événement est réellement écarté quand il remonte en tête du tas (`pruneTop`).
- `nextDeadline()` → échéance du prochain événement valide, ou `nullopt`.
- `popReady(now)` → extrait le prochain événement dont l'échéance est passée, ou
  `nullopt`.

Les événements sont la **seule** source de progression temporelle du jeu (faim,
fin de commandes, réapparition des ressources).

---

## 4. Boucle principale (`Server::run`)

```
scheduleRespawn()                       // planifie la 1re réapparition de ressources
tant que la partie n'est pas finie :
    timeout = computePollTimeout()      // ms jusqu'à la prochaine échéance, ou -1
    poll(_pollFds, timeout)             // dort jusqu'à une I/O ou l'échéance
    pour chaque fd signalé :
        POLLHUP/POLLERR/POLLNVAL → removeClient
        POLLOUT                  → flushWrites
        POLLIN                   → acceptClient (socket d'écoute) ou handleClient
    exécuter tous les événements échus (scheduler.popReady)
    pumpPlayerQueues()                  // démarre les commandes IA en attente
    checkVictory()
    réarmer POLLOUT pour tout client ayant des écritures en attente
```

Points clés :

- `computePollTimeout()` renvoie `-1` (blocage jusqu'à une I/O) quand aucun
  événement n'est planifié, sinon le délai en millisecondes jusqu'à la prochaine
  échéance. **Pas d'attente active.**
- Le réarmage final de `POLLOUT` est nécessaire car un événement ou un handler de
  commande peut écrire dans le buffer d'un **autre** client (typiquement les
  diffusions GUI). Sans ce réarmage, ces écritures ne partiraient jamais sur des
  clients par ailleurs inactifs.
- La boucle de parcours des `pollfd` itère **à l'envers** pour pouvoir supprimer
  un client (et son entrée) en cours d'itération.

---

## 5. Couche réseau

### Socket d'écoute (`Server::setupSocket`)

`AF_INET` / `SOCK_STREAM`, `SO_REUSEADDR`, `bind` sur le port, `listen`,
puis passage en **non bloquant** (`O_NONBLOCK`). `SIGPIPE` est ignoré dans `main`.

### Connexion (`server/Client`)

Chaque connexion possède un état et deux buffers :

- **État** : `AwaitingTeam` (par défaut) → `Ai` ou `Gui` après handshake.
- **Lecture** : les octets reçus sont accumulés ; `extractLine()` découpe sur
  `\n` et retire un éventuel `\r` final. Si le buffer de lecture dépasse
  **65536 octets** sans ligne complète, la connexion est fermée (anti-flood).
- **Écriture** : `queueWrite` empile, `flushWrites` envoie en `MSG_NOSIGNAL` et
  consomme ce qui a réellement été émis ; `POLLOUT` est armé tant qu'il reste des
  octets en attente.

À l'acceptation, le serveur envoie immédiatement `WELCOME\n`.

### Handshake (`Server::doHandshake`)

La première ligne envoyée par un client est interprétée comme un nom d'équipe :

- **`GRAPHIC`** → la connexion passe en `Gui` et reçoit l'**état initial complet**
  (voir `sendInitialState`, §7).
- **Nom d'équipe valide avec un slot libre** → un œuf de l'équipe est tiré
  (`hatchEgg`), le joueur **naît sur la case de cet œuf**, l'œuf est retiré, le
  compteur de slots est décrémenté. Le serveur envoie à l'IA :
  1. `<slots restants>\n` (le **CLIENT-NUM**, après décrément) ;
  2. `<width> <height>\n`.
  Puis il prévient les GUI (`ebo` + `pnw` + `pin`) et planifie la faim du joueur.
- **Sinon** (équipe inconnue, plus de slot, ou plus d'œuf) → la connexion est
  fermée.

Le couple **slot ⇔ œuf** est maintenu cohérent : un slot libre correspond toujours
à un œuf disponible (création conjointe au démarrage et via `Fork`).

---

## 6. Modèle de jeu

### Types et constantes (`game/Types.hpp`)

- `enum class Resource { Food, Linemate, Deraumere, Sibur, Mendiane, Phiras, Thystame }`
  (`RESOURCE_COUNT = 7`).
- `DENSITY[7]` : densité cible de chaque ressource par case.
- `FOOD_LIFE_UNITS = 126` : unités de vie par ration ; `INITIAL_FOOD_UNITS = 1260` ;
  `INITIAL_FOOD_COUNT = 10` (rations affichées à l'IA et au GUI).
- `enum class Orientation { North=1, East=2, South=3, West=4 }`.
- `Inventory = std::array<int, 7>`, `Pos { x, y }`, `resourceName(idx)`.

### Carte torique (`game/World`)

- `wrap(x, y)` : coordonnées ramenées dans `[0,width) × [0,height)`.
- `at(x, y)` : accès à la `Tile` (toujours via `wrap`).
- `step(pos, orient, n)` : avance de `n` cases dans l'orientation (tore).
- `placePlayer/removePlayer/movePlayer`, `placeEgg/removeEgg` : indexation des
  occupants par case.
- Ressources : `initResources()` distribue chaque ressource à hauteur de
  `max(1, width*height*DENSITY[r])` ; `refill()` recomplète jusqu'à la cible et
  renvoie la liste des cases modifiées.
- `shortestDelta(from, to)` : plus court vecteur torique (préparé pour le calcul
  de direction du `Broadcast`).
- `visibleTiles(pos, orient, level)` : cône de vision ; ligne `k` (de `0` à
  `level`) contient `2k+1` cases, l'ensemble est renvoyé dans l'ordre attendu par
  la commande `Look`.

### Joueur (`game/Player`)

`id`, `teamName`, `connFd`, `pos`, `orient` (North par défaut), `level` (1),
`inventory[7]`, `lifeUnits` (1260), `state ∈ {Idle, Busy, Incanting, Dead}`,
`cmdQueue` (file de commandes), `currentEventId`, `foodEventId`.

### Œuf / Équipe

- `Egg { id, team, pos, layerId }` — `layerId` est l'id du joueur qui a pondu
  l'œuf (`-1` pour les œufs initiaux).
- `Team { name, freeSlots, playerIds, eggIds }`.

### État global (`game/GameState`)

Agrège le monde, les joueurs, les œufs et les équipes, et la table `fd → id` de
joueur. Au démarrage, pour chaque équipe : `freeSlots = clients` et autant d'œufs
sont créés à des positions aléatoires.

- **Numérotation des joueurs et des œufs à partir de 0**, jamais réutilisée
  (`_nextPlayerId`, `_nextEggId`).
- `createPlayer` initialise l'inventaire avec `food = INITIAL_FOOD_COUNT` (10).
- `hatchEgg(team)` choisit un œuf de l'équipe au hasard et renvoie son id.

---

## 7. Protocole

### 7.1 Réponses vers l'IA (`protocol/AiProtocol`)

| Fonction | Sortie |
|----------|--------|
| `ok()` / `ko()` / `dead()` | `ok\n` / `ko\n` / `dead\n` |
| `look(tiles)` | `[ <case0>, <case1>, ... ]\n` |
| `inventory(player)` | `[ food N, linemate N, ..., thystame N ]\n` |
| `connectNbr(n)` | `<n>\n` |
| `message(k, text)` | `message <k>, <text>\n` |
| `eject(k)` | `eject: <k>\n` |
| `elevationUnderway()` | `Elevation underway\n` |
| `currentLevel(k)` | `Current level: <k>\n` |

**Format `Look`** (calibré sur le binaire de référence) : espace après `[` et
avant `]`, cases séparées par `,`. Le contenu d'une case est, dans l'ordre :
un jeton `player` par joueur présent, puis chaque ressource (ordre de l'énum)
répétée autant de fois qu'elle est présente, jetons séparés par un espace. Une
case vide ne produit aucun jeton (d'où les `,,` consécutifs).

**Format `Inventory`** : `food` est un **nombre de rations** (et non des unités de
vie), de départ 10.

### 7.2 Événements vers les GUI (`protocol/GuiProtocol`)

Diffusés à tous les clients GUI via `toAllGui` (sauf l'état initial, propre à un
GUI). Liste complète implémentée :

| Méthode | Message | Sens |
|---------|---------|------|
| `msz` | `msz X Y` | taille de la carte |
| `bct` | `bct X Y r0..r6` | contenu d'une case |
| `bctAll` | suite de `bct` | toute la carte |
| `tna` | `tna NAME` | nom d'équipe |
| `sgt` / `sst` | `sgt T` / `sst T` | fréquence (requête / changement) |
| `suc` / `sbp` | `suc` / `sbp` | commande GUI inconnue / mauvais paramètre |
| `pnw` | `pnw #id X Y O L NAME` | nouveau joueur |
| `ppo` | `ppo #id X Y O` | position/orientation |
| `plv` | `plv #id L` | niveau |
| `pin` | `pin #id X Y inv0..inv6` | inventaire |
| `pex` | `pex #id` | éjection |
| `pbc` | `pbc #id msg` | broadcast |
| `pic` / `pie` | `pic X Y L #id…` / `pie X Y R` | incantation début / fin (R = 1/0) |
| `pfk` | `pfk #id` | ponte (Fork) |
| `pgt` / `pdr` | `pgt #id i` / `pdr #id i` | ressource ramassée / déposée |
| `pdi` | `pdi #id` | mort d'un joueur |
| `enw` | `enw #egg #player X Y` | nouvel œuf |
| `ebo` / `edi` | `ebo #egg` / `edi #egg` | éclosion / destruction d'œuf |
| `seg` | `seg NAME` | fin de partie (équipe gagnante) |
| `smg` | `smg msg` | message serveur |

**État initial** (`sendInitialState`) envoyé à un GUI qui se connecte : `msz`,
`sgt`, un `tna` par équipe, toute la carte (`bctAll`), puis pour chaque joueur
`pnw`+`pin`+`plv` et pour chaque œuf `enw`.

### 7.3 Commandes GUI entrantes (`Server::handleGuiLine`)

Sous-ensemble géré : `msz`, `mct` (toute la carte), `tna`, `sgt`, `bct X Y`,
`sst T`. Règles : commande inconnue → `suc` ; paramètre invalide ou hors carte →
`sbp`. `sst T` change réellement la fréquence du jeu (`_config.freq`) et la
rediffuse à tous les GUI.

---

## 8. Exécution des commandes IA (`commands/CommandExecutor`)

### File et minuterie

- `dispatch(playerId, line)` empile la ligne dans la file du joueur. La file est
  **plafonnée à 10** commandes ; au-delà, les commandes supplémentaires sont
  **ignorées**.
- `pumpPlayerQueues()` est appelée à chaque tour : pour chaque joueur `Idle` ayant
  une commande en attente, elle démarre la suivante (**une seule commande à la fois
  par joueur**).
- `startNext` :
  - découpe la ligne ; verbe inconnu → `ko` immédiat (le joueur reste `Idle`) ;
  - commande **instantanée** (durée 0) → exécutée immédiatement ;
  - commande à durée → le joueur passe `Busy`, le `onStart` éventuel est exécuté
    tout de suite, et le handler est planifié à l'échéance `durée/freq`. À
    l'échéance, le handler s'exécute puis le joueur repasse `Idle`.

La `struct Command { int durationUnits; Handler handler; Handler onStart; }`
porte un hook optionnel `onStart` (effet visible dès le lancement, utilisé par
`Fork` pour émettre `pfk`).

### Table des commandes

| Commande | Coût (unités) | État | Effet |
|----------|---------------|------|-------|
| `Forward` | 7 | **fait** | avance d'une case (tore) + `ppo` |
| `Right` | 7 | **fait** | rotation horaire + `ppo` |
| `Left` | 7 | **fait** | rotation antihoraire + `ppo` |
| `Look` | 7 | **fait** | renvoie le cône de vision formaté |
| `Inventory` | 1 | **fait** | renvoie l'inventaire formaté |
| `Connect_nbr` | 0 | **fait** | renvoie les slots libres de l'équipe |
| `Take <obj>` | 7 | **fait** | ramasse 1 ressource sur la case → `pgt` + `bct` + `pin` |
| `Set <obj>` | 7 | **fait** | dépose 1 ressource sur la case → `pdr` + `bct` + `pin` |
| `Fork` | 42 | **fait** | `pfk` au lancement ; à l'échéance pond un œuf + `freeSlots++` + `enw` |
| `Incantation` | 300 | **fait** | élévation collective (table `ELEV[8]`) ; `pic`/`plv`/`pie` |
| `Broadcast` | 7 | **stub** (`ok`) | à implémenter (J6) |
| `Eject` | 7 | **stub** (`ok`) | à implémenter (J6) |

Détails des commandes implémentées :

- **`Take` / `Set`** valident le nom de la ressource (`resourceIndex`). `Take`
  échoue (`ko`) si la case ne contient pas l'objet ; `Set` échoue si l'IA ne le
  détient pas. Sur `food`, `lifeUnits` est ajusté de `±126` pour rester cohérent
  avec le compteur de rations.
- **`Fork`** crée un œuf sur la case du joueur avec `layerId = id` du pondeur, puis
  rouvre un slot d'équipe (`freeSlots++`) — ce qui permet à une nouvelle IA de se
  connecter et d'éclore un œuf via le handshake.

### Incantation (élévation de niveau)

L'incantation est traitée à part dans `startNext` (elle ne suit pas le flux
générique `Busy`). Table des prérequis `ELEV[8]`, indexée par le **niveau courant**
(1 à 7), chaque ligne donnant le nombre de joueurs requis sur la case et le nombre
de pierres de chaque type :

| Niveau | Joueurs | linemate | deraumere | sibur | mendiane | phiras | thystame |
|--------|---------|----------|-----------|-------|----------|--------|----------|
| 1 → 2 | 1 | 1 | 0 | 0 | 0 | 0 | 0 |
| 2 → 3 | 2 | 1 | 1 | 1 | 0 | 0 | 0 |
| 3 → 4 | 2 | 2 | 0 | 1 | 0 | 2 | 0 |
| 4 → 5 | 4 | 1 | 1 | 2 | 0 | 1 | 0 |
| 5 → 6 | 4 | 1 | 2 | 1 | 3 | 0 | 0 |
| 6 → 7 | 6 | 1 | 2 | 3 | 0 | 1 | 0 |
| 7 → 8 | 6 | 2 | 2 | 2 | 2 | 2 | 1 |

Déroulé :

1. **Check initial** (`startIncantation`) : les participants sont les joueurs
   présents sur la case **au même niveau** que l'auteur. Si le nombre de joueurs ou
   les pierres présentes sur la case ne suffisent pas, l'auteur reçoit `ko`
   **immédiatement** (il reste `Idle`, sans coût en temps).
2. **Gel** : chaque participant passe à l'état `Incanting` (toute commande en cours
   est annulée), reçoit `Elevation underway\n`, et les GUI reçoivent
   `pic X Y level #id…`. L'événement de fin est planifié à **+300 unités**.
   L'événement n'est rattaché à aucun joueur : il se déclenche même si l'auteur se
   déconnecte, garantissant le dégel des autres participants.
3. **Re-check à l'échéance** (`finishIncantation`) : on ne garde que les
   participants encore figés, vivants, au bon niveau et toujours sur la case. Si les
   prérequis restent satisfaits : les **pierres exactes sont consommées** sur la
   case, chaque participant passe au niveau supérieur et reçoit `Current level: k\n`,
   les GUI reçoivent un `plv` par joueur, un `bct` pour la case et `pie X Y 1`.
   Sinon les participants reçoivent `ko` et les GUI `pie X Y 0`. Dans tous les cas,
   les participants sont dégelés (`Idle`) et `pumpPlayerQueues` est relancé.

Les pierres sont consommées **sur la case** (jamais dans l'inventaire) ; la
nourriture continue d'être décomptée pendant l'incantation (un participant peut
mourir de faim et être simplement écarté du re-check).

---

## 9. Mécaniques périodiques

### Faim (`scheduleFoodTick` / `consumeFood`)

Toutes les **126 unités de temps**, le joueur consomme une ration : `inventory.food
-= 1` et `lifeUnits -= 126`. Si la nourriture tombe à 0, `killPlayer` est appelé ;
sinon le GUI reçoit `pin` et le prochain tick est replanifié. Un joueur démarre
avec 10 rations (≈ `1260/freq` secondes de survie sans manger).

`killPlayer` : annule les événements du joueur (commande en cours + faim), diffuse
`pdi`, envoie `dead\n` à l'IA en best-effort, retire le joueur et ferme la
connexion.

### Réapparition des ressources (`scheduleRespawn`)

Toutes les **20 unités de temps** : `World::refill()` recomplète la carte au seuil
de densité, chaque case modifiée est diffusée aux GUI (`bct`), puis l'événement se
replanifie.

### Victoire (`checkVictory`)

Vérifiée à chaque tour : la **première équipe comptant au moins 6 joueurs de niveau
≥ 8** gagne. La partie est marquée terminée, l'équipe gagnante est enregistrée et
les GUI reçoivent `seg`.

### Déconnexion (`removeClient`)

Une déconnexion d'IA vaut **mort immédiate** : les événements du joueur sont
annulés, `pdi` est diffusé, le joueur est retiré, puis le fd est fermé et nettoyé
de `_pollFds` et `_clients`.

---

## 10. Décisions de calibration (contre le binaire de référence)

| Réf | Question | Décision |
|-----|----------|----------|
| R7 | `CLIENT-NUM` avant/après décrément | **après** décrément (slots restants) |
| R2 | format `Look` | `player` puis ressources, cases vides → jetons absents |
| R8 | `food` en temps ou en objets | **en objets** (rations), départ 10 |
| —  | numérotation joueurs/œufs | démarre à **0** |

---

## 11. État d'avancement

**Implémenté et testé** : analyse des arguments, boucle `poll`/scheduler,
handshakes IA et GUI, état initial GUI et diffusion temps réel, déplacements
(`Forward`/`Right`/`Left`), `Look`, `Inventory`, `Connect_nbr`, `Take`, `Set`,
`Fork`, `Incantation` (élévation, table `ELEV[8]`), faim/mort, réapparition des
ressources, victoire, déconnexion.

**Restant (stubs renvoyant `ok`)** : `Broadcast` (calcul de la direction `K`),
`Eject`. Durcissement final non encore fait : passes `valgrind`/`strace`,
validation croisée avec le GUI et le serveur de référence.
