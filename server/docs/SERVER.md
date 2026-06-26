
# Zappy — Documentation du serveur

Documentation technique exhaustive du serveur `zappy_server`.

Le serveur héberge la partie : il maintient la carte torique, génère et
régénère les ressources, gère les joueurs (clients **IA**) et les interfaces
graphiques (clients **GUI**), arbitre les commandes selon le protocole Zappy et
décide de la condition de victoire. Il est **mono-thread**, **non bloquant**
(multiplexage via `poll`) et **piloté par le temps** (échéancier d'événements).

---

## Sommaire

1. [Compilation](#1-compilation)
2. [Lancement & arguments](#2-lancement--arguments)
3. [Vue d'ensemble de l'architecture](#3-vue-densemble-de-larchitecture)
4. [Boucle principale](#4-boucle-principale)
5. [Module temps — `src/time`](#5-module-temps--srctime)
6. [Modèle de jeu — `src/game`](#6-modèle-de-jeu--srcgame)
7. [Réseau — `src/server`](#7-réseau--srcserver)
8. [Protocoles — `src/protocol`](#8-protocoles--srcprotocol)
9. [Commandes IA — `src/commands`](#9-commandes-ia--srccommands)
10. [Règles du jeu](#10-règles-du-jeu)
11. [Gestion des erreurs](#11-gestion-des-erreurs)
12. [Arguments d'analyse — `src/args`](#12-arguments-danalyse--srcargs)
13. [Référence rapide des fichiers](#13-référence-rapide-des-fichiers)

---

## 1. Compilation

```sh
make            # compile zappy_server
make DEBUG=1    # compile avec symboles de debug (-g3 -DDEBUG)
make clean      # supprime les objets (obj/)
make fclean     # supprime objets + binaire
make re         # fclean puis all
```

- **Compilateur** : `g++`, `-std=c++20 -Wall -Wextra -Iinclude -Isrc`.
- **Sortie de dépendances** : `-MMD -MP` génère les `.d` pour la recompilation
  incrémentale (inclus en fin de Makefile).
- **Aucune dépendance externe** : uniquement la STL et l'API POSIX (sockets,
  `poll`, `fcntl`).
- Les objets sont placés sous `obj/` en miroir de l'arborescence `src/`.

---

## 2. Lancement & arguments

```
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
```

**Tous les arguments sont optionnels** : chaque flag a une valeur par défaut
cohérente, et le serveur démarre même sans aucun argument (`./zappy_server`).
Chaque valeur est bornée par un intervalle `[min, max]` ; toute valeur hors
borne est rejetée.

| Flag | Type | Description | Intervalle | Défaut |
|------|------|-------------|:----------:|:------:|
| `-p` | int | Port d'écoute | `1..65535` | `4242` |
| `-x` | int | Largeur de la carte | `1..1000` | `10` |
| `-y` | int | Hauteur de la carte | `1..1000` | `10` |
| `-n` | liste | Noms d'équipes (uniques) | `1..100` équipes | `team1` |
| `-c` | int | Nombre de clients (slots) par équipe | `1..1000` | `1` |
| `-f` | int | Inverse de l'unité de temps | `1..10000` | `100` |
| `--help` | — | Affiche l'aide (avec bornes + défauts) et quitte (code 0) | — | — |

- `freq` est la **fréquence** : plus elle est grande, plus le jeu est rapide.
  Une action de durée `d` unités prend `d / freq` secondes.
- Les bornes et les défauts sont définis comme constantes dans
  `src/args/Parser.hpp` (`DEFAULT_*`, `PORT_MIN/MAX`, `DIMENSION_MIN/MAX`,
  `CLIENTS_MIN/MAX`, `FREQ_MIN/MAX`, `TEAMS_MIN/MAX`).
- Le nom d'équipe `GRAPHIC` est **réservé** (mot-clé de connexion GUI) et
  refusé à la validation.
- Toute erreur d'argument (valeur non entière, hors borne, équipe en double,
  argument inconnu) lève une `ArgError`, affichée sur `stderr`, et le
  programme se termine avec le **code 84**.

Flux de `main` (`src/main.cpp`) :

1. `--help` présent → `Parser::printUsage()` puis `return 0`.
2. `signal(SIGPIPE, SIG_IGN)` : on ignore les `SIGPIPE` pour gérer les
   déconnexions via les codes de retour de `send`/`recv`.
3. `Parser::parse` → `Config`.
4. Construction et exécution du `Server` (`server.run()`).
5. Toute `Error` est rattrapée → message sur `stderr` + code 84.

---

## 3. Vue d'ensemble de l'architecture

```
                         ┌──────────────┐
            argv ───────▶│   Parser     │──▶ Config
                         └──────────────┘
                                │
                                ▼
   ┌───────────────────────────────────────────────────────────────┐
   │                            Server                               │
   │  (boucle poll, sockets, I/O non bloquant, cycle de vie client)  │
   │                                                                 │
   │   ┌──────────┐  ┌────────────┐  ┌───────────┐  ┌────────────┐   │
   │   │GameState │  │ Scheduler  │  │GuiProtocol│  │CommandExec.│   │
   │   │  + World │  │ (heap evt) │  │           │  │            │   │
   │   └──────────┘  └────────────┘  └───────────┘  └────────────┘   │
   └───────────────────────────────────────────────────────────────┘
        ▲                                            │
        │ Clients (fd → Client)                      │ AiProtocol (réponses)
   ┌────┴─────┐                                  ┌───┴────────┐
   │ IA / GUI │◀─────────────────────────────────│  réseau TCP │
   └──────────┘                                  └────────────┘
```

| Module | Rôle | Fichiers |
|--------|------|----------|
| `args` | Analyse et validation de la ligne de commande | `Parser.{hpp,cpp}` |
| `time` | Horloge monotone + échéancier d'événements | `Clock.hpp`, `Event.hpp`, `Scheduler.{hpp,cpp}` |
| `game` | État du monde, joueurs, œufs, équipes, règles | `Types`, `Tile`, `Player`, `Egg`, `Team`, `World`, `GameState` |
| `server` | Sockets, multiplexage, cycle de vie des connexions | `Client.{hpp,cpp}`, `Server.{hpp,cpp}` |
| `protocol` | Sérialisation des messages IA et GUI | `AiProtocol.hpp`, `GuiProtocol.{hpp,cpp}` |
| `commands` | Exécution temporisée des commandes IA | `CommandExecutor.{hpp,cpp}` |

**Principes de conception :**

- **Mono-thread, événementiel.** Aucune concurrence : tout passe par la boucle
  `poll` et l'échéancier. Pas de mutex, pas de course.
- **I/O non bloquant.** Tous les sockets sont en `O_NONBLOCK`. Les écritures
  sont mises en tampon par client et vidées quand le socket est prêt en
  écriture (`POLLOUT`).
- **Séparation état / transport.** `GameState`/`World` ne savent rien des
  sockets ; le `Server` orchestre, les `*Protocol` sérialisent.
- **Temps logique unifié.** Toute action différée (durée d'une commande, faim,
  régénération) passe par le même `Scheduler`.

---

## 4. Boucle principale

`Server::run()` (`src/server/Server.cpp`) tourne tant que `_state.finished`
est faux :

1. **`computePollTimeout()`** : délai jusqu'à la prochaine échéance de
   l'échéancier (ou `-1` = bloquant si aucun événement).
2. **`poll`** sur tous les descripteurs (`_pollFds`). `EINTR` → on recommence.
3. **Parcours des descripteurs en ordre inverse** (pour pouvoir supprimer sans
   invalider l'itération) :
   - `POLLHUP | POLLERR | POLLNVAL` (hors socket d'écoute) → `removeClient`.
   - `POLLOUT` + client connu → `flushWrites` (vidange du tampon d'écriture).
   - `POLLIN` : socket d'écoute → `acceptClient` ; sinon → `handleClient`.
4. **Échéances dues** : `while (popReady(now)) ev->run();` exécute tous les
   événements arrivés à terme.
5. **`pumpPlayerQueues()`** : démarre la prochaine commande des joueurs au repos.
6. **`checkVictory()`** : test de la condition de fin.
7. **`syncPollOut`** sur chaque client : (dés)active `POLLOUT` selon le tampon.

Ordre choisi délibérément : on **lit/accepte d'abord**, on **fait avancer le
temps ensuite**, puis on **planifie les écritures**. Cela garantit que les
réponses produites par les événements de ce tour sont envoyées au tour suivant.

---

## 5. Module temps — `src/time`

### `Clock.hpp`

Aliases et utilitaires de conversion (horloge **monotone**) :

```cpp
using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration  = std::chrono::duration<double>;
```

| Fonction | Rôle |
|----------|------|
| `secondsFor(units, freq)` | `units / freq` secondes (en `double`). |
| `toMilliseconds(Duration)` | Conversion en ms, **plancher à 0** (jamais négatif → `poll` ne bloque pas par erreur). |
| `deadlineFor(units, freq)` | `Clock::now() + secondsFor(units, freq)` : échéance absolue d'une action. |

`steady_clock` est utilisée car elle est monotone (insensible aux changements
d'heure système), ce qui est indispensable pour mesurer des durées.

### `Event.hpp`

```cpp
struct Event {
    uint64_t id;                  // identifiant unique (pour l'annulation)
    TimePoint due;                // échéance absolue
    std::function<void()> run;    // action à exécuter
    bool cancelled = false;
    bool operator>(const Event &o) const { return due > o.due; }
};
```

`operator>` permet d'utiliser l'`Event` dans une *min-heap* (`std::greater`).

### `Scheduler.{hpp,cpp}`

Échéancier d'événements basé sur une file de priorité (min-heap par échéance).
L'annulation est **paresseuse** : on enregistre l'`id` dans un ensemble et on
ignore l'événement au moment de le défiler.

| Méthode | Description |
|---------|-------------|
| `schedule(due, fn) → uint64_t` | Insère un événement, renvoie son `id` (≥ 1). |
| `cancel(id)` | Marque l'`id` comme annulé (suppression différée). |
| `nextDeadline() → optional<TimePoint>` | Échéance du prochain événement non annulé, ou `nullopt`. |
| `popReady(now) → optional<Event>` | Défile **un** événement échu (`due ≤ now`), ou `nullopt`. |
| `pruneTop()` *(privé)* | Retire du sommet les événements annulés avant lecture. |

`pruneTop()` est appelée par `nextDeadline` et `popReady` : elle dépile et
oublie tout sommet présent dans `_cancelled`, garantissant que le sommet
observé est toujours un événement actif.

> ⚠️ Les `id` annulés restent dans `_cancelled` jusqu'à ce qu'ils remontent au
> sommet du tas. C'est sans conséquence en pratique (volume d'événements
> faible), mais à garder en tête pour une partie très longue.

---

## 6. Modèle de jeu — `src/game`

### `Types.hpp` — constantes et types de base

```cpp
enum class Resource { Food=0, Linemate, Deraumere, Sibur,
                      Mendiane, Phiras, Thystame };
constexpr int    RESOURCE_COUNT     = 7;
constexpr double DENSITY[7]         = {0.5,0.3,0.15,0.1,0.1,0.08,0.05};
constexpr int    FOOD_LIFE_UNITS    = 126;   // unités de vie par "food"
constexpr int    INITIAL_FOOD_UNITS = 1260;  // = 10 × 126
constexpr int    INITIAL_FOOD_COUNT = 10;    // "food" dans l'inventaire initial

enum class Orientation { North=1, East=2, South=3, West=4 };
using Inventory = std::array<int, RESOURCE_COUNT>;

class Pos {
public:
    Pos() = default;
    Pos(int x, int y);
    int x() const; int y() const;
    void setX(int x); void setY(int y);
};

const char *resourceName(int idx);  // index → "food", "linemate", ...
```

- Les entités du modèle (`Pos`, `Tile`, `Player`, `Egg`, `Team`) sont des
  **classes** à données privées : l'état n'est accessible que par accesseurs
  (`x()`/`setX()`, `pos()`/`setPos()`, …). Les conteneurs (`Inventory`,
  vecteurs d'ids) sont exposés par référence (`resources()`, `players()`, …).
- `DENSITY[r]` : densité cible d'une ressource = `width*height*DENSITY[r]`
  exemplaires sur la carte (minimum 1).
- `resourceName` renvoie `""` pour un index hors borne (robustesse).

### `Tile.hpp`

```cpp
class Tile {
public:
    Inventory &resources();                 // quantité de chaque ressource
    std::vector<int> &players();            // ids des joueurs présents
    std::vector<int> &eggs();               // ids des œufs présents
    // + surcharges const de chaque accesseur
};
```

### `Player.hpp`

```cpp
class Player {
public:
    enum class State { Idle, Busy, Incanting, Dead };

    int id() const;                   void setId(int);
    const std::string &teamName() const; void setTeamName(const std::string &);
    int connFd() const;               void setConnFd(int);          // socket associé
    Pos pos() const;                  void setPos(Pos);
    Orientation orient() const;       void setOrient(Orientation);
    int level() const;                void setLevel(int);
    Inventory &inventory();           // inventaire (food initialisé à 10)
    int lifeUnits() const;            void setLifeUnits(int);
    State state() const;              void setState(State);
    std::deque<std::string> &cmdQueue();   // commandes en attente (cap. 10)
    uint64_t currentEventId() const;  void setCurrentEventId(uint64_t);
    uint64_t foodEventId() const;     void setFoodEventId(uint64_t);
};
```

Valeurs par défaut : `connFd = -1`, `orient = North`, `level = 1`,
`lifeUnits = INITIAL_FOOD_UNITS`, `state = Idle`, ids d'événements à `0`.

États : `Idle` (au repos), `Busy` (commande temporisée en cours),
`Incanting` (incantation en cours), `Dead` (mort, en cours de retrait).

### `Egg.hpp` & `Team.hpp`

```cpp
class Egg {
public:
    int id() const;          void setId(int);
    const std::string &team() const; void setTeam(const std::string &);
    Pos pos() const;         void setPos(Pos);
    int layerId() const;     void setLayerId(int);   // défaut -1
};

class Team {
public:
    const std::string &name() const; void setName(const std::string &);
    int freeSlots() const;   void setFreeSlots(int); // défaut 0
    std::vector<int> &playerIds();
    std::vector<int> &eggIds();
};
```

- `Egg::layerId()` : id du joueur ayant pondu l'œuf (`-1` pour les œufs initiaux).
- `Team::freeSlots()` : nombre de places encore disponibles dans l'équipe.
  Renvoyé par la commande IA `Connect_nbr` et après chaque connexion.

### `World.{hpp,cpp}` — la carte torique

Grille linéaire `width*height` de `Tile`, indexée par `idx(x,y)=y*width+x`.
La carte est **torique** : tout accès passe par `wrap`.

| Méthode | Description |
|---------|-------------|
| `wrap(x,y) → Pos` | Repli modulaire dans `[0,w[ × [0,h[`, gère les négatifs. |
| `at(x,y)` | Accès à la `Tile` (surcharges mutable/const), avec `wrap`. |
| `step(pos, orient, n=1) → Pos` | Avance de `n` cases dans la direction. |
| `placePlayer / removePlayer / movePlayer` | Maintien des `players` par case. |
| `placeEgg / removeEgg` | Idem pour les œufs. |
| `initResources()` | Distribution initiale (chaque ressource à sa densité cible). |
| `refill() → vector<Pos>` | Recomplète chaque ressource jusqu'à sa cible ; renvoie les cases modifiées. |
| `shortestDelta(from,to) → Pos` | Vecteur torique le plus court de `from` à `to`. |
| `visibleTiles(pos,orient,level) → vector<Pos>` | Cases vues par le `Look`. |

Détails notables :

- **`resourceTarget(r)`** = `max(1, width*height*DENSITY[r])` : au moins un
  exemplaire de chaque ressource, même sur une petite carte.
- **`refill`** compte les exemplaires présents, calcule le manque par ressource,
  et tire des cases aléatoires (`std::mt19937` membre) pour combler. Les cases
  touchées sont renvoyées pour notifier les GUI (voir `Server::scheduleRespawn`).
- **`shortestDelta`** ramène chaque composante dans `]-dim/2, dim/2]` : base du
  calcul de direction des sons (broadcast) et des poussées (eject).
- **`visibleTiles`** construit le cône de vision : pour chaque ligne `line`
  de `0` à `level`, `2*line+1` cases centrées sur l'axe « avant » du joueur
  (`fwd`) et étalées sur l'axe « droite » (`rgt`). Total = `(level+1)²` cases,
  ordonnées comme l'attend le protocole `Look` (case du joueur en premier).

### `GameState.{hpp,cpp}` — état global de la partie

Agrège le `World` et les registres `id → entité`. C'est l'**unique source de
vérité** du jeu (le `World` ne stocke que des ids ; les objets vivent ici).

Registres : `_players`, `_eggs` (`unordered_map<int,…>`), `_teams`
(`unordered_map<string,Team>`), `_fdToPlayerId` (socket → joueur).
Compteurs d'id : `_nextPlayerId`, `_nextEggId`. Flags de fin : `finished`,
`winner`.

**Construction** (`GameState(const Config&)`) :

1. `World(width,height)` puis `initResources()`.
2. Création des équipes avec `freeSlots = clients`.
3. Pour chaque équipe, `clients` **œufs** posés à des positions aléatoires
   (les joueurs écloront depuis ces œufs).

| Méthode | Description |
|---------|-------------|
| `createPlayer(team, fd, pos, orient) → id` | Crée un joueur, `food=10`, l'inscrit dans le monde, l'équipe et `_fdToPlayerId`. |
| `removePlayer(id)` | Retire du monde, de `_fdToPlayerId`, de l'équipe, puis détruit. |
| `playerIdByFd(fd) → int` | Joueur associé à un socket, `-1` si aucun. |
| `player(id)` / `hasPlayer(id)` / `players()` | Accès / test / vue de la map. |
| `createEgg(team, pos, layerId=-1) → id` | Crée un œuf, l'inscrit dans le monde et l'équipe. |
| `removeEgg(id)` | Retire l'œuf du monde et de l'équipe. |
| `hatchEgg(team) → id` | Choisit **aléatoirement** un œuf de l'équipe (ou `-1`). |
| `egg(id)` / `hasEgg(id)` / `eggs()` | Accès / test / vue. |
| `team(name)` / `isTeam(name)` / `hasFreeSlot(name)` / `teams()` | Gestion des équipes. |

> ℹ️ `hatchEgg` ne fait que **sélectionner** un œuf ; c'est `Server::doHandshake`
> qui crée le joueur sur sa position, supprime l'œuf et décrémente `freeSlots`.

---

## 7. Réseau — `src/server`

### `Client.{hpp,cpp}` — un client connecté

Encapsule un socket et ses tampons. Un `Client` ne touche jamais au réseau
lui-même : il **accumule** ce qui est lu/à écrire ; le `Server` fait les
`recv`/`send`.

```cpp
enum class State { AwaitingTeam, Ai, Gui };
```

| Méthode | Description |
|---------|-------------|
| `fd()` | Descripteur du socket. |
| `state()` / `setState()` | État de protocole du client. |
| `appendToReadBuffer(data,size)` | Ajoute des octets reçus au tampon de lecture. |
| `readBufferSize()` | Taille courante du tampon de lecture (garde anti-flood). |
| `extractLine() → optional<string>` | Extrait une ligne complète (`\n`), retire un éventuel `\r` final. |
| `queueWrite(data)` | Empile des octets à envoyer. |
| `hasPendingWrites()` | Tampon d'écriture non vide ? |
| `writeBuffer()` | Accès au tampon d'écriture. |
| `consumeWritten(n)` | Retire les `n` octets effectivement envoyés. |

`extractLine` est **orientée ligne** : tant qu'aucun `\n` n'est présent, elle
renvoie `nullopt` (commande incomplète, on attend la suite). Cela gère
naturellement la fragmentation TCP.

### `Server.{hpp,cpp}` — cœur du serveur

Possède tout l'état exécutable : `_clients` (`fd → Client`), `_config`,
`_state` (`GameState`), `_gui` (`GuiProtocol`), `_scheduler`, `_exec`
(`CommandExecutor`), `_listenFd`, `_pollFds`.

> ⚠️ **Ordre d'initialisation.** Dans `Server.hpp`, `_clients` est déclaré
> **avant** `_config` et `_gui`, car `GuiProtocol` garde une référence vers
> `_clients` (`_gui(_clients)`). L'ordre des membres garantit que `_clients`
> existe avant que `_gui` ne le capture.

#### Mise en place du socket — `setupSocket()`

1. `socket(AF_INET, SOCK_STREAM, 0)`.
2. `SO_REUSEADDR` (réutilisation immédiate du port).
3. `bind` sur `INADDR_ANY:port`.
4. `listen(SOMAXCONN)`.
5. `O_NONBLOCK` via `fcntl`.

Chaque échec lève une `NetworkError`. Le socket d'écoute est ajouté à
`_pollFds` avec `POLLIN` dans le constructeur.

#### Cycle de vie d'un client

- **`acceptClient()`** : `accept` non bloquant ; le nouveau socket passe en
  `O_NONBLOCK`, est ajouté à `_pollFds`, un `Client` est créé, et
  `"WELCOME\n"` est mis en file d'écriture. Le client est alors en
  `AwaitingTeam`.
- **`handleClient(fd)`** : `recv` jusqu'à 4096 octets. `≤ 0` → déconnexion.
  Sinon ajout au tampon de lecture. **Garde anti-flood** : si le tampon dépasse
  `65536` octets, le client est expulsé. Puis on extrait et traite toutes les
  lignes complètes via `handleLine` (en revérifiant à chaque tour que le client
  existe toujours, car une commande peut le supprimer).
- **`handleLine(client,line)`** : aiguillage selon `state()` :
  - `AwaitingTeam` → `doHandshake`.
  - `Ai` → `_exec.dispatch(playerId, line)`.
  - `Gui` → `handleGuiLine`.
- **`removeClient(fd)`** : si un joueur est associé, on annule ses événements
  (`currentEventId`, `foodEventId`), on notifie les GUI (`pdi`) et on le retire
  de l'état ; puis `close(fd)`, retrait de `_pollFds` et de `_clients`.

#### Handshake — `doHandshake(client, teamName)`

- `teamName == "GRAPHIC"` → client GUI : `setState(Gui)` +
  `sendInitialState` (envoi de tout l'état courant).
- Sinon, si l'équipe est inconnue **ou** sans slot libre → déconnexion.
- Sinon : `hatchEgg` choisit un œuf, un joueur est créé sur sa position avec une
  orientation **aléatoire**, l'œuf est supprimé, `freeSlots--`. Le serveur
  renvoie alors, conformément au protocole :
  1. le nombre de slots restants (`freeSlots`),
  2. `"width height\n"`.

  Puis il notifie les GUI : `ebo` (œuf éclos), `pnw` (nouveau joueur), `pin`
  (inventaire) et planifie le **premier tick de faim** (`scheduleFoodTick`).

#### Émission vers un joueur — `sendToPlayer(playerId, msg)`

Résout le joueur → son `fd` → son `Client`, met `msg` en file d'écriture et
appelle `syncPollOut`. C'est le point d'entrée unique utilisé par
`CommandExecutor` pour répondre aux IA.

#### Gestion de la faim

- **`scheduleFoodTick(playerId)`** : planifie un événement à
  `deadlineFor(FOOD_LIFE_UNITS, freq)` (= `126/freq` s) qui appellera
  `consumeFood`. Stocke l'id via `player.setFoodEventId(...)`.
- **`consumeFood(playerId)`** : décrémente `food` d'une unité et `lifeUnits`
  de `FOOD_LIFE_UNITS`. Si `food ≤ 0` → `killPlayer`. Sinon notifie l'inventaire
  (`pin`) et **replanifie** le prochain tick.
- **`killPlayer(playerId)`** : annule ses événements, notifie les GUI (`pdi`),
  envoie `"dead\n"` à l'IA (avec un `send` **immédiat** car la socket va être
  fermée), retire le joueur de l'état et appelle `removeClient`.

#### Régénération des ressources — `scheduleRespawn()`

Planifie un événement périodique toutes les **20 unités de temps**
(`deadlineFor(20, freq)`) qui appelle `world().refill()`, notifie les GUI des
cases modifiées (`bct`) puis **se replanifie**. Démarré au début de `run()`.

#### Condition de victoire — `checkVictory()`

Pour chaque équipe, compte les joueurs vivants de **niveau ≥ 8**. Si une équipe
en a **≥ 6**, la partie est gagnée : `finished = true`, `winner = name`,
notification GUI `seg`. La boucle principale s'arrête au tour suivant.

#### Écritures non bloquantes — `flushWrites` / `syncPollOut`

- **`flushWrites(client)`** : `send(MSG_NOSIGNAL)` du tampon. `EAGAIN`/
  `EWOULDBLOCK` → on réessaiera (socket plein) ; autre erreur → `removeClient`.
  Les octets envoyés sont retirés du tampon (`consumeWritten`).
- **`syncPollOut(client)`** : active `POLLOUT` dans `_pollFds` si le client a
  des écritures en attente, le désactive sinon. Évite de réveiller `poll`
  inutilement quand il n'y a rien à écrire.

---

## 8. Protocoles — `src/protocol`

### `AiProtocol.hpp` — réponses serveur → IA

Fonctions statiques pures qui **formatent** les réponses (terminées par `\n`).

| Fonction | Sortie |
|----------|--------|
| `ok()` | `ok\n` |
| `ko()` | `ko\n` |
| `dead()` | `dead\n` |
| `look(tiles)` | `[ t0, t1, ... ]\n` (cases séparées par `,`) |
| `inventory(p)` | `[ food n, linemate n, ... ]\n` |
| `connectNbr(n)` | `n\n` |
| `elevationUnderway()` | `Elevation underway\n` |
| `currentLevel(k)` | `Current level: k\n` |
| `message(k, text)` | `message k, text\n` (broadcast reçu, `k` = direction) |
| `eject(k)` | `eject: k\n` |

`look` insère un espace avant le contenu de chaque case ; une case vide reste
vide entre les virgules (format attendu par les IA et la référence).

### `GuiProtocol.{hpp,cpp}` — diffusion serveur → GUI

Garde une **référence** vers `_clients` et n'écrit qu'aux clients en état `Gui`
(`toAllGui`). Les helpers anonymes `fmtPnw/fmtPin/fmtPlv/fmtPpo/fmtEnw/fmtBct`
construisent les chaînes.

**État initial** — `sendInitialState(gui, state, freq)` envoie, dans l'ordre :
`msz`, `sgt`, un `tna` par équipe, l'intégralité des cases (`bctAll`), puis
pour chaque joueur `pnw`+`pin`+`plv`, et pour chaque œuf `enw`.

**Messages disponibles** (sous-ensemble du protocole GUI Zappy) :

| Méthode | Message | Sens |
|---------|---------|------|
| `msz` | `msz X Y` | Taille de la carte |
| `bct` | `bct X Y q0..q6` | Contenu d'une case (2 surcharges : un GUI / tous) |
| `bctAll` | série de `bct` | Toute la carte |
| `tna` | `tna N` | Nom d'équipe |
| `sgt` | `sgt T` | Unité de temps courante |
| `sst` | `sst T` | Unité de temps modifiée (diffusé) |
| `suc` | `suc` | Commande GUI inconnue |
| `sbp` | `sbp` | Mauvais paramètre de commande GUI |
| `pnw` | `pnw #n X Y O L N` | Nouveau joueur (connexion) |
| `ppo` | `ppo #n X Y O` | Position/orientation d'un joueur |
| `plv` | `plv #n L` | Niveau d'un joueur |
| `pin` | `pin #n X Y q0..q6` | Inventaire d'un joueur |
| `pex` | `pex #n` | Expulsion (eject) |
| `pbc` | `pbc #n msg` | Broadcast émis |
| `pic` | `pic X Y L #n...` | Début d'incantation |
| `pie` | `pie X Y r` | Fin d'incantation (`r` = 1 succès / 0 échec) |
| `pfk` | `pfk #n` | Ponte d'un œuf (fork) |
| `pdr` | `pdr #n i` | Joueur jette la ressource `i` |
| `pgt` | `pgt #n i` | Joueur ramasse la ressource `i` |
| `pdi` | `pdi #n` | Mort d'un joueur |
| `enw` | `enw #e #n X Y` | Nouvel œuf (`#n` = pondeur) |
| `ebo` | `ebo #e` | Œuf éclos (connexion d'un joueur) |
| `edi` | `edi #e` | Œuf détruit |
| `seg` | `seg N` | Fin de partie, équipe `N` gagnante |
| `smg` | `smg msg` | Message serveur (générique) |

`tileResources` sérialise les 7 quantités d'une case séparées par des espaces.

### Commandes GUI entrantes — `Server::handleGuiLine`

Le GUI peut **interroger** le serveur. La ligne est tokenisée ; selon le
premier mot :

| Requête | Effet | Réponse |
|---------|-------|---------|
| `msz` | Taille de la carte | `msz X Y` |
| `mct` | Toute la carte | `bct` × toutes les cases |
| `tna` | Liste des équipes | `tna` × équipes |
| `sgt` | Unité de temps | `sgt T` |
| `bct X Y` | Une case (coords validées) | `bct ...` ou `sbp` si invalide |
| `sst T` | Change l'unité de temps (`T>0`) | `sst T` (diffusé) ou `sbp` |
| *autre* | — | `suc` |

---

## 9. Commandes IA — `src/commands`

### `CommandExecutor.{hpp,cpp}`

Reçoit les lignes des clients IA, les **met en file**, les exécute en respectant
leur **durée** (en unités de temps) et renvoie les réponses via
`Server::sendToPlayer`. Tient une `_table : nom → Command`.

```cpp
struct Command {
    int durationUnits;       // durée en unités de temps (0 = instantané)
    Handler handler;         // effet, exécuté à l'échéance
    Handler onStart = nullptr;  // effet optionnel au démarrage
};
```

#### File de commandes & cadence

- **`dispatch(playerId, line)`** : met la ligne en file. Le **plafond est de 10
  commandes en attente** (`MAX_QUEUE`), en comptant la commande en cours
  d'exécution : `inFlight` vaut 1 si le joueur est `Busy`/`Incanting`. Au-delà,
  la commande est **silencieusement ignorée** (comportement attendu : une rafale
  de 11 commandes en laisse tomber exactement une).
- **`pumpPlayerQueues()`** : pour chaque joueur `Idle` avec une file non vide,
  démarre la commande suivante (`startNext`). Appelée à chaque tour de boucle.
- **`startNext(playerId)`** : défile une ligne, la tokenise, cherche la commande.
  - inconnue → `ko`.
  - `Incantation` → branchement vers `startIncantation`.
  - `durationUnits == 0` → exécution **immédiate** (ex. `Inventory`,
    `Connect_nbr`).
  - sinon : passe le joueur en `Busy`, exécute `onStart` si présent, planifie
    `handler` à `deadlineFor(durationUnits, freq)`. À l'échéance, le `handler`
    s'exécute puis le joueur repasse `Idle` (`currentEventId = 0`).

#### Table des commandes

| Commande | Durée (unités) | Effet |
|----------|:--------------:|-------|
| `Forward` | 7 | Avance d'une case ; `ppo` ; `ok`. |
| `Right` | 7 | Rotation horaire ; `ppo` ; `ok`. |
| `Left` | 7 | Rotation anti-horaire ; `ppo` ; `ok`. |
| `Look` | 7 | Renvoie le cône de vision (`AiProtocol::look`). |
| `Inventory` | 1 | Renvoie l'inventaire. |
| `Connect_nbr` | 0 | Renvoie les slots libres de l'équipe. |
| `Take` | 7 | Ramasse une ressource (`pgt`,`bct`,`pin`) ou `ko`. |
| `Set` | 7 | Dépose une ressource (`pdr`,`bct`,`pin`) ou `ko`. |
| `Fork` | 42 | Pond un œuf (`onStart`=`pfk`, fin=`enw`+`ok`), `freeSlots++`. |
| `Incantation` | 300 | Élévation de niveau (voir ci-dessous). |
| `Broadcast` | 7 | Diffuse un message à tous (direction par récepteur). |
| `Eject` | 7 | Pousse les autres occupants de la case (`ok`/`ko`). |

Toutes ces fermetures vérifient `hasPlayer(id)` en entrée (le joueur a pu mourir
entre la planification et l'exécution).

#### Détails de commandes complexes

**`Take` / `Set`** — `resourceIndex(name)` convertit le nom en index. `Take`
échoue (`ko`) si la ressource est absente de la case ; `Set` échoue si elle est
absente de l'inventaire. La nourriture ramassée/déposée ajuste aussi
`lifeUnits` de `±FOOD_LIFE_UNITS`. Les GUI sont notifiés (case + inventaire).

**`Look`** — `World::visibleTiles` fournit l'ordre des cases ; `formatTile`
liste d'abord un jeton `player` par joueur présent, puis chaque ressource
répétée selon sa quantité.

**`Broadcast`** — pour chaque autre joueur, on calcule le vecteur torique le
plus court (`shortestDelta`) de l'auditeur vers la source, puis la **direction
relative** `k` (1..8) via `soundDirection`, et on envoie `message k, text`. Le
GUI reçoit `pbc`.

**`soundDirection(recv, dx, dy)`** — convertit un vecteur monde en direction
dans le repère du récepteur : `0` = même case ; sinon `1..8`. `1` = devant,
`3` = à droite, `5` = derrière, `7` = à gauche.

> ⚠️ **Calibration.** Le binaire de référence numérote les 8 directions en
> sens **anti-horaire** (avant-droite = 8, droite = 7, …), à l'inverse du
> diagramme du sujet. La numérotation a été **calibrée octet à octet** contre
> `data/binaries/linux/zappy_server`. Le `r = -sign(...)` inverse l'axe droite
> pour reproduire ce comportement. **Ne pas « corriger » sans revalider contre
> le binaire de référence.**

**`Eject`** — pousse tous les autres joueurs présents sur la case du joueur (et
détruit les œufs présents) d'une case vers l'avant de l'éjecteur. Les victimes
reçoivent `eject: k` où `k` est calculé depuis le sens **opposé** à l'avancée de
l'éjecteur (la poussée vient de derrière). Les œufs détruits décrémentent
`freeSlots` et émettent `edi`. Renvoie `ok` si au moins un joueur a été poussé,
`ko` sinon. (Les listes d'ids sont **copiées** avant itération car déplacer un
joueur/retirer un œuf mute le vecteur de la case.)

**`Fork`** — `onStart` émet `pfk` (animation de ponte) ; à l'échéance, un œuf
est créé sur la position du joueur, `freeSlots++`, `enw` est diffusé et `ok`
renvoyé. Une nouvelle place d'équipe devient ainsi disponible.

#### Incantation (élévation)

L'élévation est gérée hors de la `_table` car elle implique **plusieurs
joueurs** et deux phases.

- **`startIncantation(playerId)`** :
  1. Recense les joueurs **de même niveau** présents sur la case.
  2. Vérifie les prérequis via `incantationReady(level, tile, count)`
     (nombre de joueurs **et** pierres requises). Échec → `ko`.
  3. Tous les participants passent en `Incanting` (leur commande en cours est
     annulée) et reçoivent `Elevation underway`.
  4. Notifie le GUI (`pic`) et planifie la fin à `deadlineFor(300, freq)`.
- **`finishIncantation(issuer, pos, level, participants)`** :
  1. Recalcule les **présents valides** (toujours vivants, encore `Incanting`,
     même niveau, même case).
  2. Revérifie `incantationReady` (les conditions ont pu changer pendant les
     300 unités).
  3. **Succès** : consomme les pierres requises sur la case, monte chaque
     participant d'un niveau (`Current level: L`, GUI `plv`), met à jour la case
     (`bct`). **Échec** : `ko` à tous les présents.
  4. Notifie le GUI (`pie X Y succès`), repasse les présents en `Idle` et
     relance `pumpPlayerQueues`.

**Table d'élévation** (`ELEV[level]`, indices 1..7 = niveau du joueur ;
quantités de pierres : linemate, deraumere, sibur, mendiane, phiras, thystame) :

| Niveau → | Joueurs | line | dera | sibur | mend | phir | thys |
|:--------:|:-------:|:----:|:----:|:-----:|:----:|:----:|:----:|
| 1 → 2 | 1 | 1 | 0 | 0 | 0 | 0 | 0 |
| 2 → 3 | 2 | 1 | 1 | 1 | 0 | 0 | 0 |
| 3 → 4 | 2 | 2 | 0 | 1 | 0 | 2 | 0 |
| 4 → 5 | 4 | 1 | 1 | 2 | 0 | 1 | 0 |
| 5 → 6 | 4 | 1 | 2 | 1 | 3 | 0 | 0 |
| 6 → 7 | 6 | 1 | 2 | 3 | 0 | 1 | 0 |
| 7 → 8 | 6 | 2 | 2 | 2 | 2 | 2 | 1 |

`incantationReady` renvoie faux pour `level < 1 || level > 7` (un joueur niveau
8 ne s'élève plus) et compare quantités présentes/requises.

#### Helpers internes (anonymes)

`formatTile`, `resourceIndex`, `turnRight`/`turnLeft`, `incantationReady`,
`join`, `sign`, `forwardDelta`, `soundDirection`, plus `tokenize` (statique).

---

## 10. Règles du jeu

- **Carte** torique de `X×Y` cases.
- **Ressources** : 7 types distribués selon `DENSITY`, **recomplétés toutes les
  20 unités de temps** jusqu'à leur densité cible.
- **Vie** : un joueur démarre avec `food = 10` (= 1260 unités de vie). Toutes
  les `126/freq` secondes, il consomme 1 `food`. À court de nourriture, il
  **meurt** (`dead`).
- **Œufs & slots** : chaque équipe démarre avec `clients` œufs et `clients`
  slots. Une connexion fait éclore un œuf (slot consommé) ; `Fork` crée un œuf
  (slot rendu) ; détruire un œuf (eject) reprend un slot.
- **Élévation** : commande `Incantation` (300 unités), conditions de la table
  ci-dessus, élève d'un niveau les participants éligibles (1 → 8).
- **Victoire** : première équipe avec **≥ 6 joueurs de niveau 8**.
- **Cadence des commandes** : au plus **10 commandes** en attente par joueur ;
  exécution séquentielle, une commande à la fois, selon sa durée.

---

## 11. Gestion des erreurs

`src/Error.hpp` définit une hiérarchie d'exceptions :

```cpp
class Error : public std::exception {
    // _message (what()) + _where (catégorie : "args", "network")
};
class ArgError     : public Error;  // where = "args"
class NetworkError : public Error;  // where = "network"
```

- `Error::what()` renvoie le message ; `where()` la catégorie.
- Toutes les erreurs fatales remontent jusqu'au `try/catch` de `main`, qui
  affiche le message sur `stderr` et termine avec le **code 84**.
- Les erreurs **non fatales** côté réseau (client qui se déconnecte, socket
  plein, `accept` échoué) ne lèvent pas : elles sont traitées localement
  (déconnexion propre du client concerné, ou nouvelle tentative).

---

## 12. Arguments d'analyse — `src/args`

`Parser` (statique) transforme `argv` en `Config` validée.

| Méthode | Rôle |
|---------|------|
| `parse(argc, argv) → Config` | Analyse complète + validation. |
| `printUsage()` | Affiche l'aide (`USAGE: ...`). |
| `parseInt(value, flag)` *(privé)* | Entier strict via `std::from_chars` ; lève `ArgError` si la chaîne n'est pas **entièrement** un entier. |
| `validate(cfg)` *(privé)* | Contrôles sémantiques (bornes `[min, max]`, équipes). |
| `checkRange(v, min, max, name)` *(libre, privé)* | Vérifie qu'un entier est dans `[min, max]`, sinon `ArgError` détaillée. |

**`Config`** : `port, width, height, clients, freq` (int) et
`teams` (`vector<string>`), **tous préinitialisés à leur défaut**
(`DEFAULT_PORT=4242`, `DEFAULT_WIDTH=10`, `DEFAULT_HEIGHT=10`,
`DEFAULT_CLIENTS=1`, `DEFAULT_FREQ=100`).

Analyse :

- **Aucun flag n'est obligatoire** : un flag absent garde sa valeur par défaut.
- Flags entiers `-p/-x/-y/-c/-f` mappés vers les champs de `Config` ; valeur
  manquante après le flag → `ArgError`.
- `-n` consomme tous les arguments suivants **jusqu'au prochain `-…`**. Si
  aucune équipe n'est fournie, l'équipe par défaut `team1` est ajoutée.
- Argument inconnu → `ArgError`.

Validation (`validate`) — bornes définies dans `Parser.hpp` :

- `port ∈ [1, 65535]`, `width ∈ [1, 1000]`, `height ∈ [1, 1000]`,
  `clients ∈ [1, 1000]`, `freq ∈ [1, 10000]`, nombre d'équipes ∈ `[1, 100]`.
  Tout dépassement produit un message `... must be in range MIN..MAX (got V)`.
- `GRAPHIC` interdit (réservé GUI), pas de doublon de nom d'équipe.

---

## 13. Référence rapide des fichiers

| Fichier | Contenu |
|---------|---------|
| `src/main.cpp` | Point d'entrée, `--help`, `SIGPIPE`, try/catch global. |
| `src/Error.hpp` | Hiérarchie d'exceptions (`Error`, `ArgError`, `NetworkError`). |
| `src/args/Parser.{hpp,cpp}` | Analyse/validation des arguments → `Config`. |
| `src/time/Clock.hpp` | Horloge monotone + conversions de durées. |
| `src/time/Event.hpp` | Structure d'événement planifié. |
| `src/time/Scheduler.{hpp,cpp}` | Échéancier (min-heap + annulation paresseuse). |
| `src/game/Types.hpp` | Constantes (ressources, vie), enums, `Pos`, `Inventory`. |
| `src/game/Tile.hpp` | Case : ressources + ids joueurs/œufs. |
| `src/game/Player.hpp` | Joueur : état, inventaire, file de commandes, événements. |
| `src/game/Egg.hpp` | Œuf : id, équipe, position, pondeur. |
| `src/game/Team.hpp` | Équipe : nom, slots, ids joueurs/œufs. |
| `src/game/World.{hpp,cpp}` | Carte torique : ressources, vision, déplacements. |
| `src/game/GameState.{hpp,cpp}` | État global : joueurs/œufs/équipes + registres. |
| `src/server/Client.{hpp,cpp}` | Tampons et état de protocole d'un client. |
| `src/server/Server.{hpp,cpp}` | Boucle `poll`, sockets, cycle de vie, faim, victoire. |
| `src/protocol/AiProtocol.hpp` | Formatage des réponses IA. |
| `src/protocol/GuiProtocol.{hpp,cpp}` | Diffusion et requêtes GUI. |
| `src/commands/CommandExecutor.{hpp,cpp}` | File, durée et exécution des commandes IA. |
</content>
</invoke>
