# Zappy — Documentation du serveur

Documentation technique du serveur `zappy_server`. Le serveur héberge la
partie : il gère la carte torique, les ressources, les joueurs (clients IA) et
les interfaces graphiques (clients GUI), et arbitre les commandes selon le
protocole Zappy.

## Sommaire

- [Compilation](#compilation)
- [Lancement](#lancement)
- [Architecture](#architecture)
- [Modèle de jeu (`src/game`)](#modèle-de-jeu-srcgame)
- [Réseau (`src/server`)](#réseau-srcserver)
- [Protocoles (`src/protocol`)](#protocoles-srcprotocol)
- [Commandes IA (`src/commands`)](#commandes-ia-srccommands)
- [Gestion des erreurs](#gestion-des-erreurs)

---

## Compilation

```sh
make            # compile zappy_server
make DEBUG=1    # compile avec symboles de debug (-g3 -DDEBUG)
make clean      # supprime les objets
make fclean     # supprime objets + binaire
make re         # fclean puis all
```

Compilateur : `g++` en `-std=c++20 -Wall -Wextra`.

---

## Lancement

```
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
```

| Flag | Argument    | Description                                   | Requis |
|------|-------------|-----------------------------------------------|--------|
| `-p` | `port`      | Numéro de port (1..65535)                     | oui    |
| `-x` | `width`     | Largeur de la carte (> 0)                     | oui    |
| `-y` | `height`    | Hauteur de la carte (> 0)                     | oui    |
| `-n` | `name1 ...` | Noms des équipes                              | oui    |
| `-c` | `clientsNb` | Nombre de clients par équipe (> 0)            | oui    |
| `-f` | `freq`      | Inverse de l'unité de temps (> 0, défaut 100) | non    |

`--help` affiche l'usage et quitte.

Contraintes validées au démarrage :

- Le nom d'équipe `GRAPHIC` est réservé (il identifie les clients GUI).
- Les noms d'équipes doivent être uniques.

En cas d'argument invalide, le serveur écrit un message sur `stderr` et quitte
avec le code **84**.

---

## Architecture

```
src/
├── main.cpp            point d'entrée : parsing + lancement
├── Error.hpp           hiérarchie d'exceptions
├── args/
│   ├── Parser.hpp      structure Config + interface du parser
│   └── Parser.cpp      parsing et validation de la ligne de commande
├── game/               modèle de la partie (état, carte, entités)
│   ├── Types.hpp       types et constantes partagés
│   ├── Tile.hpp        contenu d'une case
│   ├── World.hpp       carte torique
│   ├── Player.hpp      joueur (client IA)
│   ├── Egg.hpp         oeuf
│   ├── Team.hpp        équipe
│   └── GameState.hpp   état global de la partie
├── protocol/           sérialisation des messages
│   ├── AiProtocol.hpp  réponses au protocole IA (header only)
│   ├── GuiProtocol.hpp interface du protocole GUI
│   └── GuiProtocol.cpp diffusion et sérialisation GUI
├── server/             couche réseau
│   ├── Client.hpp      état d'une connexion + buffers
│   ├── Client.cpp
│   ├── Server.hpp      boucle d'événements (poll)
│   └── Server.cpp
└── commands/           exécution des commandes IA
    ├── CommandExecutor.hpp  table de dispatch + file par joueur
    └── CommandExecutor.cpp  handlers des commandes
```

Flux général :

1. `main` parse les arguments en une `Config`, construit un `Server` et appelle
   `run()`.
2. `Server` ouvre une socket d'écoute non bloquante et boucle sur `poll()`.
3. Chaque connexion devient un `Client` ; après le handshake il est soit un
   client IA (`Ai`), soit un client graphique (`Gui`).
4. Les commandes manipulent le `GameState` ; les réponses sont sérialisées par
   `AiProtocol` (vers les IA) et `GuiProtocol` (vers les GUI).

---

## Modèle de jeu (`src/game`)

### Types et constantes (`Types.hpp`)

- `Resource` : énumération des 7 ressources, dans l'ordre protocolaire
  `Food, Linemate, Deraumere, Sibur, Mendiane, Phiras, Thystame`.
- `RESOURCE_COUNT` = 7.
- `DENSITY[RESOURCE_COUNT]` : densité de génération de chaque ressource sur la
  carte.
- `FOOD_LIFE_UNITS` = 126 : unités de temps de vie apportées par 1 food.
- `INITIAL_FOOD_UNITS` = 1260 : réserve de vie initiale (équivaut à 10 food).
- `Orientation` : `North = 1, East = 2, South = 3, West = 4`.
- `Inventory` : `std::array<int, RESOURCE_COUNT>`, indexé par `Resource`
  (l'index 0 est la food).
- `Pos` : coordonnées `{ x, y }`.
- `resourceName(idx)` : renvoie le nom protocolaire d'une ressource (chaîne vide
  hors bornes). Partagé entre la commande Look et la sérialisation IA.

### `Tile`

Contenu d'une case de la carte :

- `resources` : quantités au sol, indexées par `Resource`.
- `players` : identifiants des joueurs présents.
- `eggs` : identifiants des oeufs présents.

### `World`

Carte torique (les déplacements s'enroulent aux bords). N'expose que des
déclarations ; l'implémentation est dans `World.cpp`.

- `width()`, `height()` : dimensions de la carte (la grille de `Tile` reste
  interne).
- `at(x, y)` : accès à une case avec wrap torique.
- `wrap(x, y)` : normalise des coordonnées dans la carte.
- `step(pos, orient, n)` : avance de `n` pas dans une orientation.
- `shortestDelta(from, to)` : plus court vecteur entre deux cases (torique).
- `visibleTiles(pos, orient, level)` : cases visibles selon le niveau (Look).
- `placePlayer` / `removePlayer` / `movePlayer` : maintien des joueurs sur les
  cases.
- `placeEgg` / `removeEgg` : maintien des oeufs sur les cases.
- `initResources()` : génération initiale des ressources.
- `refill()` : ré-approvisionnement périodique ; renvoie les cases modifiées.

### `Player`

Représente un client IA en jeu :

- `id`, `teamName`.
- `connFd` : descripteur de la connexion (`-1` si déconnecté).
- `pos`, `orient`, `level` (départ à 1).
- `inventory` : index 0 = food.
- `lifeUnits` : réserve de vie restante (départ à `INITIAL_FOOD_UNITS`).
- `state` : `Idle`, `Busy`, `Incanting` ou `Dead`.
- `cmdQueue` : commandes en attente (10 au maximum).
- `currentEventId` : identifiant Scheduler de la commande en cours (`0` = aucune).
- `foodEventId` : identifiant Scheduler du prochain tick de consommation de food.

### `Egg`

- `id`, `team`, `pos`.
- `layerId` : identifiant du joueur qui a pondu l'oeuf (`-1` pour un oeuf
  initial).

### `Team`

- `name`.
- `freeSlots` : nombre de connexions encore disponibles.
- `playerIds` : joueurs de l'équipe.
- `eggIds` : oeufs vivants non encore éclos.

### `GameState`

État global de la partie. Centralise le monde et les collections d'entités, et
fournit l'API de manipulation.

Création / suppression :

- `createPlayer(team, connFd, pos, orient)` → id.
- `removePlayer(id)` : retire le joueur du monde, de l'équipe et des cases.
- `createEgg(team, pos, layerId = -1)` → id.
- `removeEgg(id)`.
- `hatchEgg(team)` : fait éclore un oeuf aléatoire de l'équipe, ou `-1` s'il n'y
  en a aucun.

Accesseurs :

- `player(id)`, `egg(id)`, `team(name)`, `world()`.
- `hasPlayer(id)`, `hasEgg(id)`, `isTeam(name)`, `hasFreeSlot(name)`.
- `playerIdByFd(fd)` : identifiant du joueur associé à une connexion (`-1` si
  aucun).

Collections en lecture seule (utilisées notamment par
`GuiProtocol::sendInitialState`) :

- `players()`, `eggs()`, `teams()`.

État de fin de partie :

- `finished`, `winner`.

---

## Réseau (`src/server`)

### `Client`

Encapsule une connexion et ses buffers. Trois états (`State`) :

- `AwaitingTeam` : connexion ouverte, en attente du handshake.
- `Ai` : client IA authentifié.
- `Gui` : interface graphique (équipe `GRAPHIC`).

Lecture (traitement ligne par ligne) :

- `appendToReadBuffer(data, size)` : empile les octets reçus.
- `extractLine()` : extrait une ligne complète (jusqu'au `\n`, `\r` final
  retiré) ou `std::nullopt` s'il n'y en a pas encore.

Écriture (bufferisée, non bloquante) :

- `queueWrite(data)` : ajoute des octets à envoyer.
- `hasPendingWrites()`, `writeBuffer()`.
- `consumeWritten(count)` : retire les octets effectivement envoyés.

### `Server`

Boucle d'événements basée sur `poll()`. Non copiable.

Cycle de vie :

- `Server(config)` : configure la socket d'écoute et initialise les slots
  libres par équipe.
- `setupSocket()` : `socket` → `SO_REUSEADDR` → `bind` → `listen` →
  `O_NONBLOCK`. Toute erreur lève une `NetworkError`.
- `run()` : boucle infinie sur `poll`. Pour chaque descripteur :
  - `POLLHUP | POLLERR | POLLNVAL` → déconnexion (`removeClient`).
  - `POLLOUT` → `flushWrites` (envoi du buffer en attente).
  - `POLLIN` → `acceptClient` (socket d'écoute) ou `handleClient`.

Gestion des clients :

- `acceptClient()` : `accept` non bloquant, ajout au `poll`, envoi de
  `WELCOME\n`.
- `handleClient(fd)` : `recv`, bufferisation, puis traitement de chaque ligne
  via `handleLine`.
- `handleLine(client, line)` : aiguille selon l'état du client. En
  `AwaitingTeam`, exécute le handshake.
- `doHandshake(client, teamName)` :
  - `GRAPHIC` → le client passe en `Gui`.
  - équipe valide avec slot libre → le client passe en `Ai`, décrémente le slot,
    et reçoit `<slots restants>\n` puis `<width> <height>\n`.
  - sinon → déconnexion.
- `flushWrites(client)` : `send` du buffer, puis `syncPollOut`.
- `syncPollOut(client)` : active/désactive `POLLOUT` selon la présence d'octets
  en attente.
- `removeClient(fd)` : `close`, retrait du `poll` et de la table des clients.

---

## Protocoles (`src/protocol`)

Toutes les chaînes produites se terminent par `\n`, comme l'exige le protocole.

### `AiProtocol` (header only)

Fonctions statiques de sérialisation des réponses au client IA :

| Fonction                | Sortie                                                |
|-------------------------|-------------------------------------------------------|
| `ok()`                  | `ok\n`                                                |
| `ko()`                  | `ko\n`                                                |
| `dead()`                | `dead\n`                                              |
| `look(tiles)`           | `[tile0, tile1, ...]\n` (tuiles déjà formatées)       |
| `inventory(player)`     | `[food N, linemate N, ..., thystame N]\n`             |
| `connectNbr(n)`         | `<n>\n`                                               |
| `elevationUnderway()`   | `Elevation underway\n`                                |
| `currentLevel(k)`       | `Current level: <k>\n`                                |
| `message(k, text)`      | `message <k>, <text>\n`                               |
| `eject(k)`              | `eject: <k>\n`                                        |

### `GuiProtocol`

Sérialise et diffuse les messages du protocole GUI. Conserve une référence sur
la table des `Client` du serveur et n'écrit que vers ceux en `State::Gui`, via
`Client::queueWrite`.

Les formateurs internes (`fmtPnw`, `fmtPin`, `fmtPlv`, `fmtPpo`, `fmtEnw`,
`fmtBct`) sont centralisés dans le `.cpp` pour que la diffusion et
`sendInitialState` produisent des octets strictement identiques.

- `sendInitialState(gui, state, freq)` : envoie l'état complet à un GUI qui se
  connecte, dans l'ordre `msz`, `sgt`, `tna` (par équipe), toutes les cases
  (`bct`), puis pour chaque joueur `pnw` + `pin` + `plv`, et pour chaque oeuf
  `enw`.

Réponses adressées à un GUI précis :

| Méthode               | Message                       |
|-----------------------|-------------------------------|
| `msz(gui, w, h)`      | dimensions de la carte        |
| `bct(gui, x, y, t)`   | contenu d'une case            |
| `bctAll(gui, w)`      | toutes les cases (`mct`)      |
| `tna(gui, teamName)`  | nom d'une équipe              |
| `sgt(gui, freq)`      | unité de temps                |
| `suc(gui)`            | commande inconnue             |
| `sbp(gui)`            | mauvais paramètre             |

Diffusions à tous les GUI :

| Méthode                                 | Événement                  |
|-----------------------------------------|----------------------------|
| `sst(freq)`                             | modification du time unit  |
| `pnw(player)`                           | nouveau joueur             |
| `ppo(player)`                           | position d'un joueur       |
| `plv(player)`                           | niveau d'un joueur         |
| `pin(player)`                           | inventaire d'un joueur     |
| `pex(playerId)`                         | expulsion                  |
| `pbc(playerId, msg)`                    | broadcast d'un joueur      |
| `pic(x, y, level, participants)`        | début d'incantation        |
| `pie(x, y, success)`                    | fin d'incantation          |
| `pfk(playerId)`                         | début de fork              |
| `pdr(playerId, resourceIdx)`            | dépôt de ressource         |
| `pgt(playerId, resourceIdx)`            | ramassage de ressource     |
| `pdi(playerId)`                         | mort d'un joueur           |
| `enw(eggId, playerId, x, y)`            | nouvel oeuf                 |
| `ebo(eggId)`                            | éclosion d'un oeuf         |
| `edi(eggId)`                            | mort d'un oeuf             |
| `seg(teamName)`                         | fin de partie              |
| `smg(msg)`                              | message serveur            |

---

## Commandes IA (`src/commands`)

`CommandExecutor` exécute les commandes envoyées par les clients IA. Chaque
commande a un **coût en unités de temps** ; le joueur passe `Busy` pendant la
durée, puis le handler est exécuté à l'échéance via le `Scheduler`.

- `dispatch(playerId, line)` : empile la commande dans la file du joueur (10 au
  maximum ; au-delà, la commande est ignorée).
- `pumpPlayerQueues()` : démarre la prochaine commande de chaque joueur `Idle`
  (une seule commande à la fois par joueur).

Table des commandes (durée en unités de temps) :

| Commande         | Coût | Effet                                                            |
|------------------|------|------------------------------------------------------------------|
| `Forward`        | 7    | avance d'une case (torique) → `ppo`                              |
| `Right` / `Left` | 7    | rotation → `ppo`                                                 |
| `Look`           | 7    | renvoie le cône de vision formaté                               |
| `Inventory`      | 1    | renvoie l'inventaire                                             |
| `Connect_nbr`    | 0    | renvoie les slots libres de l'équipe                            |
| `Take <obj>`     | 7    | ramasse une ressource → `pgt` + `bct` + `pin`                   |
| `Set <obj>`      | 7    | dépose une ressource → `pdr` + `bct` + `pin`                    |
| `Fork`           | 42   | `pfk` au lancement ; à l'échéance pond un oeuf → `freeSlots++`   |
| `Incantation`    | 300  | élévation collective (table `ELEV[8]`) → `pic` / `plv` / `pie`  |
| `Broadcast <m>`  | 7    | diffusion sonore (voir ci-dessous) → `pbc`                      |
| `Eject`          | 7    | pousse les autres joueurs de la case (voir ci-dessous) → `pex`  |

### `Broadcast <message>`

Le joueur émet un son entendu par **tous les autres joueurs**. Chacun reçoit
`message K, <texte>\n`, où `K` indique la **direction d'origine du son** relative
à sa propre orientation :

- `K = 0` : l'émetteur est sur la **même case** ;
- `K = 1..8` : direction en **sens horaire depuis l'avant** (`1` devant, `3` à
  droite, `5` derrière, `7` à gauche).

La direction est calculée à partir du plus court vecteur torique
(`World::shortestDelta`) projeté dans le repère avant/droite du récepteur. Les
GUI reçoivent `pbc #id <texte>`. L'émetteur reçoit `ok`.

> Le sens de numérotation de `K` (horaire vs trigonométrique) est un point de
> calibration contre le binaire de référence (R1).

### `Eject`

Le joueur pousse **tous les autres joueurs présents sur sa case** d'une case dans
la direction qu'il regarde :

- chaque joueur poussé est déplacé (`World::movePlayer`), reçoit `eject: K\n` (la
  poussée vient du sens opposé à l'avancée du ravisseur, exprimée dans le repère
  du poussé) et déclenche un `ppo` côté GUI ;
- les **oeufs présents sur la case** sont détruits : `removeEgg`, `freeSlots--`
  pour leur équipe, et `edi #egg` côté GUI ;
- si au moins un joueur a été poussé, un `pex #id` est diffusé et le ravisseur
  reçoit `ok` ; sinon il reçoit `ko`.

---

## Gestion des erreurs

Hiérarchie d'exceptions définie dans `Error.hpp` :

- `Error` : base, dérive de `std::exception`. Porte un message (`what()`) et un
  contexte (`where()`).
- `ArgError` : erreur d'argument (contexte `"args"`).
- `NetworkError` : erreur réseau (contexte `"network"`).

`main` capture toute `Error`, écrit `what()` sur `stderr` et renvoie le code de
sortie **84**.
