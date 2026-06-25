# Zappy

Zappy est un jeu en réseau dans lequel plusieurs équipes d'IA évoluent sur une
planète torique, **Trantor**. Chaque équipe pilote des robots qui collectent des
ressources, communiquent et réalisent des rituels d'**élévation** pour monter en
niveau. **La première équipe dont 6 joueurs atteignent le niveau 8 gagne.**

Le projet est composé de trois exécutables indépendants qui communiquent via TCP :

| Exécutable | Langage | Rôle |
|------------|---------|------|
| `zappy_server` | C++20 | Héberge la partie, la carte, les ressources et arbitre les règles. |
| `zappy_gui` | C++17 / SFML | Visualise la partie en temps réel (clients « GRAPHIC »). |
| `zappy_ai` | Python 3 | Robot autonome qui joue pour une équipe. |

```
        ┌────────────┐         TCP          ┌──────────────┐
        │  zappy_ai  │◀───────────────────▶│              │
        └────────────┘                      │              │
        ┌────────────┐                      │ zappy_server │
        │  zappy_ai  │◀───────────────────▶│  (la partie) │
        └────────────┘                      │              │
        ┌────────────┐    (équipe GRAPHIC)  │              │
        │  zappy_gui │◀───────────────────▶│              │
        └────────────┘                      └──────────────┘
```

---

## Sommaire

- [Prérequis](#prérequis)
- [Récupérer le projet](#récupérer-le-projet)
- [Compilation](#compilation)
- [Démarrage rapide](#démarrage-rapide)
- [Utilisation détaillée](#utilisation-détaillée)
- [Règles du jeu](#règles-du-jeu)
- [Tests](#tests)
- [Structure du dépôt](#structure-du-dépôt)
- [Documentation complémentaire](#documentation-complémentaire)
- [Dépannage](#dépannage)

---

## Prérequis

Le projet cible **Linux** (testé sous Linux / WSL2).

| Composant | Dépendances |
|-----------|-------------|
| **Build commun** | `make`, `g++` ≥ 10 (support C++20) |
| **Serveur** | Aucune dépendance externe (STL + POSIX) |
| **GUI** | [SFML](https://www.sfml-project.org/) 2.x (`libsfml-dev`) |
| **IA** | `python3` ≥ 3.8 (bibliothèque standard uniquement) |
| **Tests (optionnel)** | `python3-venv`, `pytest` |

### Installation des dépendances (Debian / Ubuntu)

```sh
sudo apt update
sudo apt install build-essential libsfml-dev python3 python3-venv
```

> ℹ️ L'IA n'utilise **que la bibliothèque standard** de Python : aucun `pip
> install` n'est nécessaire pour la jouer. `pytest` n'est requis que pour la
> suite de tests du serveur.

---

## Récupérer le projet

```sh
# via SSH
git clone git@github.com:EpitechPGE2-2025/G-YEP-400-LIL-4-1-zappy-7.git

# ou via HTTPS
git clone https://github.com/EpitechPGE2-2025/G-YEP-400-LIL-4-1-zappy-7.git

cd G-YEP-400-LIL-4-1-zappy-7
```

---

## Compilation

Un `Makefile` à la racine construit les trois composants et place les binaires à
la racine du dépôt :

```sh
make            # compile zappy_server, zappy_gui et zappy_ai
make server     # compile uniquement le serveur  → ./zappy_server
make gui        # compile uniquement la GUI       → ./zappy_gui
make ai         # génère le lanceur de l'IA       → ./zappy_ai
make clean      # supprime les fichiers objets
make fclean     # supprime objets + binaires
make re         # fclean puis all
```

Après `make`, vous disposez à la racine de :

```
./zappy_server   ./zappy_gui   ./zappy_ai
```

> `zappy_ai` est un petit script shell qui lance `python3 ai/src/main.py` : il
> n'y a pas de réelle « compilation » de l'IA.

---

## Démarrage rapide

Ouvrez **trois terminaux** (ou utilisez `&` pour lancer en arrière-plan).

```sh
# 1) Le serveur : carte 10×10, équipes "red" et "blue", 5 slots/équipe
./zappy_server -p 4242 -x 10 -y 10 -n red blue -c 5 -f 100

# 2) La visualisation (se connecte comme client graphique)
./zappy_gui -p 4242 -h 127.0.0.1

# 3) Un (ou plusieurs) robot(s) pour l'équipe "red"
./zappy_ai -p 4242 -n red -h 127.0.0.1
```

Lancez autant de `zappy_ai` que de slots disponibles par équipe (`-c`). Pour une
vraie partie, lancez aussi des IA pour les autres équipes (`-n blue`).

---

## Utilisation détaillée

### Serveur — `zappy_server`

```
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
```

| Flag | Description | Obligatoire | Défaut |
|------|-------------|:-----------:|:------:|
| `-p` | Port d'écoute (1..65535) | ✅ | — |
| `-x` | Largeur de la carte (> 0) | ✅ | — |
| `-y` | Hauteur de la carte (> 0) | ✅ | — |
| `-n` | Noms des équipes (≥ 1, uniques, `GRAPHIC` interdit) | ✅ | — |
| `-c` | Nombre de slots (clients) par équipe (> 0) | ✅ | — |
| `-f` | Fréquence : inverse de l'unité de temps (> 0) | ❌ | `100` |
| `--help` | Affiche l'aide | — | — |

- **`-f` (fréquence)** : plus elle est grande, plus le jeu est rapide. Une action
  de durée `d` prend `d / f` secondes. À `f = 100`, avancer d'une case (`Forward`,
  7 unités) prend `0,07 s`.
- En cas d'argument invalide, le serveur affiche l'erreur et quitte avec le
  **code 84**.

### GUI — `zappy_gui`

```
./zappy_gui -p port -h machine
```

| Flag | Description | Défaut |
|------|-------------|:------:|
| `-p` | Port du serveur | — |
| `-h` | Adresse du serveur | `127.0.0.1` |
| `--help` | Affiche l'aide | — |

La GUI se connecte au serveur en s'annonçant comme l'équipe spéciale `GRAPHIC` et
affiche la carte, les ressources, les joueurs, les incantations et les broadcasts
en temps réel.

### IA — `zappy_ai`

```
./zappy_ai -p port -n name -h machine
```

| Flag | Description | Défaut |
|------|-------------|:------:|
| `-p` | Port du serveur | — |
| `-n` | Nom de l'équipe à rejoindre | — |
| `-h` | Adresse du serveur | `127.0.0.1` |
| `-help` | Affiche l'aide | — |

Chaque instance occupe **un slot** de l'équipe. Quand un slot est libre (œuf
disponible), une nouvelle IA peut se connecter ; sinon la connexion est refusée.

---

## Règles du jeu

### Le monde

- La carte est un tore de `width × height` cases : sortir d'un bord ramène de
  l'autre côté (haut↔bas, gauche↔droite).
- Chaque case peut contenir des ressources, des joueurs et des œufs.
- Une partie commence avec, pour chaque équipe, autant d'**œufs** et de **slots**
  que la valeur de `-c`.

### Les ressources

Il existe **7 ressources** : la **nourriture** (`food`) et **6 pierres**
précieuses utilisées pour les rituels d'élévation.

| Ressource | Densité sur la carte |
|-----------|:--------------------:|
| food | 0,5 |
| linemate | 0,3 |
| deraumere | 0,15 |
| sibur | 0,1 |
| mendiane | 0,1 |
| phiras | 0,08 |
| thystame | 0,05 |

La quantité cible d'une ressource ≈ `width × height × densité` (au moins 1
exemplaire). **Toutes les 20 unités de temps**, le serveur recomplète chaque
ressource jusqu'à sa quantité cible.

### La survie (nourriture)

- Un joueur démarre avec **10 unités de nourriture**.
- Chaque unité de nourriture le maintient en vie **126 unités de temps**
  (soit `126 / f` secondes).
- Quand sa nourriture tombe à zéro, le joueur **meurt** (`dead`) et est déconnecté.
- Pour survivre, un robot doit donc ramasser (`Take food`) régulièrement de la
  nourriture.

### Déplacement et vision

- Un joueur a une **orientation** (Nord / Est / Sud / Ouest) et se déplace case
  par case (`Forward`), avec rotation (`Left` / `Right`).
- La commande `Look` renvoie un **cône de vision** dont la portée dépend du
  niveau : au niveau `L`, le joueur voit `(L+1)²` cases (sa case + `L` rangées
  devant lui, de plus en plus larges).

### Communication

- `Broadcast <texte>` envoie un message **à tous les joueurs** de la partie.
- Chaque destinataire reçoit le message accompagné d'une **direction** (1 à 8)
  indiquant d'où vient le son par rapport à son orientation (`0` = même case).
  Cela permet aux IA de se coordonner et de se rassembler.

### Reproduction (`Fork`) et éjection (`Eject`)

- `Fork` pond un **œuf** : cela crée un nouveau slot libre dans l'équipe, qu'un
  nouveau `zappy_ai` pourra venir occuper.
- `Eject` pousse hors de la case **tous les autres joueurs** qui s'y trouvent (et
  détruit les œufs présents). Utile pour déloger des adversaires.

### L'élévation (montée de niveau)

C'est le cœur du jeu. Pour passer du niveau `L` à `L+1`, un joueur lance la
commande `Incantation` sur une case réunissant **le bon nombre de joueurs de même
niveau** et **les bonnes pierres** :

| Élévation | Joueurs (même niveau) | linemate | deraumere | sibur | mendiane | phiras | thystame |
|:---------:|:---------------------:|:--------:|:---------:|:-----:|:--------:|:------:|:--------:|
| 1 → 2 | 1 | 1 | 0 | 0 | 0 | 0 | 0 |
| 2 → 3 | 2 | 1 | 1 | 1 | 0 | 0 | 0 |
| 3 → 4 | 2 | 2 | 0 | 1 | 0 | 2 | 0 |
| 4 → 5 | 4 | 1 | 1 | 2 | 0 | 1 | 0 |
| 5 → 6 | 4 | 1 | 2 | 1 | 3 | 0 | 0 |
| 6 → 7 | 6 | 1 | 2 | 3 | 0 | 1 | 0 |
| 7 → 8 | 6 | 2 | 2 | 2 | 2 | 2 | 1 |

- L'incantation dure **300 unités de temps**. Les pierres requises doivent être
  **posées sur la case** (via `Set`) et sont consommées en cas de succès.
- Si les conditions ne sont plus réunies à la fin du rituel (joueur parti, pierres
  manquantes…), l'incantation **échoue** et personne ne monte de niveau.
- Tous les joueurs de même niveau présents sur la case montent ensemble.

### Durée des commandes

Chaque action coûte un temps fixe (en unités, à diviser par `f` pour des
secondes). Un joueur ne peut avoir que **10 commandes en attente** au maximum ;
au-delà, les commandes supplémentaires sont ignorées.

| Commande | Durée | Effet |
|----------|:-----:|-------|
| `Forward` | 7 | Avance d'une case |
| `Right` / `Left` | 7 | Tourne de 90° |
| `Look` | 7 | Observe les alentours |
| `Inventory` | 1 | Liste l'inventaire et la nourriture |
| `Connect_nbr` | 0 | Nombre de slots libres dans l'équipe |
| `Take <objet>` | 7 | Ramasse une ressource |
| `Set <objet>` | 7 | Dépose une ressource |
| `Broadcast <texte>` | 7 | Émet un message |
| `Fork` | 42 | Pond un œuf |
| `Eject` | 7 | Pousse les autres joueurs de la case |
| `Incantation` | 300 | Tente une élévation |

### Condition de victoire

> 🏆 La première équipe ayant **au moins 6 joueurs de niveau 8** remporte la partie.

---

## Tests

Une suite de tests `pytest` valide le comportement du serveur.

```sh
# Crée un environnement virtuel et installe pytest
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-dev.txt

# Lance les tests
pytest zappy_server_tests
```

> ℹ️ Pour éviter les erreurs de connexion liées à l'épuisement des slots,
> lancez le serveur de test avec un `-c` confortable (par ex. `-c 64`).

---

## Structure du dépôt

```
.
├── Makefile              # build global (server + gui + ai)
├── README.md             # ce fichier
├── server/               # serveur de jeu (C++20)
│   ├── src/              # code source (args, time, game, server, protocol, commands)
│   └── docs/SERVER.md    # documentation technique détaillée du serveur
├── gui/                  # interface graphique (C++17 / SFML)
│   ├── src/              # app, network, parser, renderer, world
│   └── assets/           # police, ressources visuelles
├── ai/                   # robot autonome (Python 3)
│   └── src/              # main, bot, network, state, strategy, pathfinding
├── docs/                 # documents de conception (AI_PLAN.md, SERVER_DOC.md)
└── zappy_server_tests/   # suite de tests pytest du serveur
```

---

## Documentation complémentaire

- [`server/docs/SERVER.md`](server/docs/SERVER.md) — documentation technique
  exhaustive du serveur (architecture, boucle d'événements, protocoles AI/GUI,
  commandes, règles internes).
- [`docs/`](docs/) — notes de conception (plan de l'IA, doc serveur).

---

## Dépannage

| Problème | Piste |
|----------|-------|
| `bind failed on port ...` | Le port est déjà utilisé : changez `-p` ou attendez quelques secondes. |
| La GUI ne compile pas (`-lsfml-*`) | Installez SFML : `sudo apt install libsfml-dev`. |
| L'IA refuse de se connecter | L'équipe est inconnue, ou tous les slots sont pris (`-c`). Vérifiez `-n` et le nombre d'IA lancées. |
| `ConnectionError` dans les tests | Lancez le serveur de test avec un `-c` plus élevé (ex. `-c 64`). |
| Le jeu va trop vite / trop lentement | Ajustez `-f` (plus grand = plus rapide). |
</content>
</invoke>
