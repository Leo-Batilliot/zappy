# Documentation complète — Interface graphique de Zappy

---

## Table des matières

1. [Qu'est-ce que Zappy ?](#1-quest-ce-que-zappy-)
2. [Qu'est-ce que la GUI ?](#2-quest-ce-que-la-gui-)
3. [Technologies utilisées](#3-technologies-utilisées)
4. [Comment lancer la GUI](#4-comment-lancer-la-gui)
5. [Découverte de l'écran](#5-découverte-de-lécran)
6. [La carte du jeu](#6-la-carte-du-jeu)
7. [Les joueurs](#7-les-joueurs)
8. [Les ressources](#8-les-ressources)
9. [Les œufs](#9-les-œufs)
10. [Les événements animés](#10-les-événements-animés)
11. [Le panneau latéral](#11-le-panneau-latéral)
12. [Les contrôles](#12-les-contrôles)
13. [Fin de partie](#13-fin-de-partie)
14. [Perte de connexion](#14-perte-de-connexion)
15. [Questions fréquentes (FAQ)](#15-questions-fréquentes-faq)
16. [Glossaire](#16-glossaire)

---

## 1. Qu'est-ce que Zappy ?

Zappy est un jeu multijoueur en réseau où plusieurs équipes de robots (appelés **joueurs**) s'affrontent sur une carte en deux dimensions. L'objectif est qu'au moins 6 membres d'une même équipe atteignent le **niveau 8**, le niveau maximum.

Pour monter de niveau, les joueurs doivent :
- Collecter des **ressources** (pierres et nourriture) dispersées sur la carte
- Réaliser des **rituels** appelés **incantations**, qui nécessitent la présence de plusieurs joueurs et un certain nombre de pierres

Les joueurs sont entièrement autonomes : ce sont des programmes informatiques (des IA) qui jouent seuls. La GUI, elle, ne fait que les **observer**.

---

## 2. Qu'est-ce que la GUI ?

**GUI** signifie *Graphical User Interface*, soit « interface graphique utilisateur ».

C'est un programme qui se connecte au serveur de jeu et qui affiche visuellement ce qui se passe en temps réel. Il faut l'imaginer comme une **caméra de surveillance** posée au-dessus du terrain de jeu :

- Elle **voit** tout ce qui se passe
- Elle **n'interagit pas** avec le jeu (elle ne joue pas)
- Elle permet aux humains de **comprendre et suivre** la partie

La GUI s'ouvre dans une fenêtre de **1280 × 720 pixels** et se rafraîchit environ 60 fois par seconde pour un affichage fluide.

---

## 3. Technologies utilisées

Cette section explique les outils et langages qui ont servi à construire la GUI, sans supposer de connaissances techniques.

---

### Langage de programmation — C++ (version 2017)

La GUI est entièrement écrite en **C++**, un langage de programmation très répandu dans le développement de jeux vidéo et d'applications performantes. La version utilisée ici est **C++17**, une version moderne du langage qui apporte des fonctionnalités facilitant l'écriture de code propre et sûr.

> **En termes simples :** C++ est le langage dans lequel sont écrits la plupart des jeux vidéo professionnels. Il permet d'obtenir des programmes très rapides.

---

### Bibliothèque graphique — SFML

**SFML** (*Simple and Fast Multimedia Library*) est la bibliothèque qui permet à la GUI d'ouvrir une fenêtre, de dessiner des formes, d'afficher des images et de lire les actions du clavier et de la souris.

Trois modules de SFML sont utilisés :

| Module SFML | Rôle |
|-------------|------|
| `sfml-graphics` | Dessine tout ce qui apparaît à l'écran : grille, joueurs, ressources, textes, effets |
| `sfml-window` | Gère la fenêtre, le clavier, la souris et la molette |
| `sfml-system` | Utilitaires de base (temps, vecteurs, etc.) |

> **En termes simples :** SFML est à la GUI ce que des pinceaux et une toile sont à un peintre. Sans elle, rien ne s'afficherait à l'écran.

**Version requise :** SFML 2.x (doit être installée sur la machine avant de pouvoir compiler la GUI).

---

### Communication réseau — TCP / Sockets

La GUI communique avec le serveur de jeu via le protocole **TCP** (*Transmission Control Protocol*), le même protocole utilisé par les navigateurs web pour charger des pages. La connexion est établie dès le lancement, et toutes les informations sur la partie arrivent sous forme de lignes de texte envoyées par le serveur.

> **En termes simples :** C'est comme un fil téléphonique entre la GUI et le serveur. Le serveur parle, la GUI écoute et affiche ce qu'elle entend.

---

### Police de caractères — DejaVu Sans

Tous les textes affichés dans la fenêtre (chiffres, noms d'équipes, journal...) utilisent la police **DejaVu Sans**, une police libre et très lisible, incluse dans les ressources du projet (`assets/DejaVuSans.ttf`).

---

### Ressources graphiques (assets)

Les images utilisées dans la GUI sont des fichiers **PNG** stockés dans le dossier `assets/` :

| Fichier | Ce qu'il représente |
|---------|---------------------|
| `ghost.png` | Joueur qui regarde vers le nord ou le sud |
| `ghost_right.png` | Joueur qui regarde vers l'est ou l'ouest |
| `ghost_special.png` | Joueur en train d'incanter |
| `apple.png` | Icône de la ressource **food** (nourriture) |
| `ore1.png` à `ore6.png` | Icônes des 6 ressources minérales (linemate, deraumere, etc.) |
| `tree001.png` à `tree004.png` | 4 variantes d'arbres pour la forêt en bordure de carte |

Si un fichier image est absent, la GUI se rabat automatiquement sur des formes simples (triangles ou cercles colorés) : **le programme ne plante pas**.

---

### Compilateur — g++

Le code source est compilé avec **g++**, le compilateur C++ du projet GNU. Il transforme le code écrit par les développeurs en un programme exécutable (`zappy_gui`).

Les options de compilation activées (`-Wall -Wextra`) demandent au compilateur de signaler le maximum d'avertissements, ce qui aide à écrire un code plus robuste.

---

### Résumé visuel des technologies

```
┌─────────────────────────────────────────────────┐
│                 zappy_gui                        │
│                                                 │
│   Langage    →   C++17                          │
│   Graphisme  →   SFML 2 (graphics/window/system)│
│   Réseau     →   TCP (sockets POSIX)            │
│   Police     →   DejaVu Sans (.ttf)             │
│   Images     →   PNG (ghost, ores, trees...)    │
│   Compilateur→   g++                            │
└─────────────────────────────────────────────────┘
```

---

## 4. Comment lancer la GUI

### Prérequis

- Le serveur Zappy doit déjà être lancé quelque part (sur votre machine ou sur le réseau)
- Vous devez connaître l'**adresse** de la machine qui fait tourner le serveur, ainsi que le **numéro de port**

### Commande à taper dans un terminal

```
./zappy_gui -h <adresse_du_serveur> -p <numéro_de_port>
```

**Exemples concrets :**

| Situation | Commande |
|-----------|----------|
| Serveur sur la même machine | `./zappy_gui -h localhost -p 4242` |
| Serveur sur une machine du réseau | `./zappy_gui -h 192.168.1.10 -p 4242` |

### Explication des paramètres

| Paramètre | Ce que ça veut dire | Exemple |
|-----------|---------------------|---------|
| `-h` | **h**ôte : l'adresse de la machine serveur | `localhost` ou `192.168.1.5` |
| `-p` | **p**ort : le numéro de porte réseau du serveur | `4242` |

> **Si vous oubliez un paramètre**, la GUI affiche simplement comment l'utiliser et se ferme. Ce n'est pas une erreur grave.

### Ce qui se passe au démarrage

Dès que la GUI se connecte, elle envoie automatiquement deux messages au serveur pour s'identifier et récupérer les informations de la partie. La fenêtre de jeu s'ouvre ensuite immédiatement.

---

## 5. Découverte de l'écran

L'écran est divisé en **deux grandes zones** :

```
+------------------------------------------------+------------+
|                                                |            |
|                                                |  PANNEAU   |
|            CARTE DU JEU                        |  LATÉRAL   |
|         (zone principale)                      |            |
|                                                |  300 px    |
|                                                |            |
+------------------------------------------------+------------+
             Zone de jeu                          Info + HUD
```

### Zone de gauche — La carte du jeu

C'est là que tout se passe : les joueurs se déplacent, les ressources s'affichent, les incantations s'animent. C'est la **partie principale** de l'écran.

### Zone de droite — Le panneau d'information

Un panneau vertical de 300 pixels sur fond bleu très sombre. Il contient toutes les **statistiques et informations** sur la partie en cours, organisées en sections pliables.

---

## 6. La carte du jeu

### La grille

La carte est divisée en **cases carrées**, appelées **tuiles**. Le nombre de tuiles est défini par le serveur au démarrage de la partie (par exemple 10 × 10, ou 20 × 30). Chaque tuile peut contenir des ressources, des joueurs, et des œufs.

Les tuiles sont affichées en **damier** avec deux teintes de vert, comme un échiquier végétal :
- Cases foncées : vert sombre
- Cases claires : vert légèrement plus lumineux

### La forêt (autour de la carte)

En dehors des bords de la carte, une **forêt** est affichée pour délimiter le terrain de jeu. Des **arbres** y apparaissent de façon décorée (orientés vers le centre de la carte, légèrement aléatoires). Vous pouvez masquer cette forêt avec la touche **T** (voir la section contrôles).

### Sélection d'une tuile

Un **contour blanc** s'affiche autour de la tuile que vous avez cliqué. Les informations de cette tuile apparaissent alors dans le panneau latéral.

### Zoom et navigation

La carte peut être zoomée et déplacée librement. Voir la section [Contrôles](#11-les-contrôles) pour tous les détails.

---

## 7. Les joueurs

### Apparence

Chaque joueur est représenté par un **fantôme** (une image de petite taille) ou, si l'image n'est pas disponible, par un **triangle**.

- La **direction** dans laquelle pointe le triangle ou le fantôme indique vers où le joueur regarde (Nord, Est, Sud ou Ouest)
- La **couleur** identifie l'équipe du joueur (voir tableau ci-dessous)

### Couleurs des équipes

Chaque équipe a une couleur distincte automatiquement attribuée :

| Équipe | Couleur |
|--------|---------|
| 1ère équipe | Rouge |
| 2ème équipe | Bleu |
| 3ème équipe | Vert clair |
| 4ème équipe | Jaune doré |
| 5ème équipe | Violet rosé |
| 6ème équipe | Cyan |
| 7ème équipe | Orange |
| 8ème équipe | Gris clair |

Au-delà de 8 équipes, les couleurs se répètent depuis le début.

### Plusieurs joueurs sur la même case

Quand plusieurs joueurs occupent la même tuile, ils sont automatiquement **répartis aux quatre coins** de la tuile et leur taille est réduite pour qu'ils restent tous visibles.

### Joueur sélectionné

Quand vous cliquez sur un joueur, un **cercle blanc** apparaît autour de lui pour signaler qu'il est sélectionné. Ses informations détaillées s'affichent dans le panneau latéral.

### Joueur en train d'incanter

Quand un joueur participe à une incantation (un rituel de montée de niveau), son contour devient **blanc** (au lieu du noir habituel), et son image prend une apparence spéciale pour le signaler.

---

## 8. Les ressources

Il existe **7 types de ressources** dans le monde de Zappy. Elles sont dispersées sur les tuiles de la carte et les joueurs peuvent les ramasser ou les déposer.

### Les 7 ressources et leurs couleurs

| Ressource | Couleur | Rôle dans le jeu |
|-----------|---------|------------------|
| **food** (nourriture) | Orange doré | Maintient les joueurs en vie. Sans nourriture, un joueur meurt. |
| **linemate** | Blanc / Gris clair | Pierre nécessaire pour les incantations de bas niveau |
| **deraumere** | Bleu | Pierre requise pour les incantations intermédiaires |
| **sibur** | Violet | Pierre pour les rituels avancés |
| **mendiane** | Jaune pâle | Pierre rare pour les niveaux élevés |
| **phiras** | Cyan / Turquoise | Pierre très rare |
| **thystame** | Rose / Magenta | La pierre la plus rare, nécessaire pour atteindre le niveau 8 |

### Comment les ressources s'affichent sur la carte

Chaque ressource présente sur une tuile est représentée par un **petit point coloré** (ou une petite icône). Leur position dans la tuile est fixe selon leur type, ce qui permet de les distinguer d'un seul coup d'œil.

Quand vous zoomez suffisamment sur la carte, les **quantités exactes** apparaissent en chiffre à côté de chaque ressource.

### Masquer / afficher les ressources

Appuyez sur la touche **R** pour masquer ou réafficher les ressources sur la carte (utile si la carte est trop chargée visuellement).

---

## 9. Les œufs

### Qu'est-ce qu'un œuf ?

Dans Zappy, les nouvelles IA qui rejoignent le jeu ne tombent pas du ciel : elles **éclosent depuis des œufs** pondus par des joueurs déjà présents sur la carte.

### Apparence

Les œufs ressemblent à de petits **ovales** de couleur crème avec un contour doré, discrets et facilement reconnaissables.

### Cycle de vie d'un œuf

1. Un joueur **pond** un œuf sur sa tuile (l'œuf apparaît sur la carte)
2. L'œuf **attend** qu'un nouveau joueur veuille rejoindre la partie
3. L'œuf **éclot** quand un nouveau joueur s'y connecte (l'œuf disparaît, un joueur apparaît)
4. **Ou** l'œuf peut **mourir** sans éclore (il disparaît simplement)

---

## 10. Les événements animés

Certains événements dans le jeu déclenchent des **animations visuelles temporaires** sur la carte. Ces effets durent environ **2 secondes** puis s'estompent progressivement.

### Broadcast — Un joueur envoie un message

> Un joueur peut envoyer un message à tous les joueurs de la carte. C'est une sorte de "cri" sur la carte.

**Ce que vous voyez :** Un **anneau jaune lumineux** qui part du joueur et s'agrandit progressivement vers l'extérieur, comme une onde sonore.

### Éjection — Un joueur pousse ses voisins

> Un joueur peut pousser tous les joueurs présents sur sa tuile dans une direction.

**Ce que vous voyez :** Un **anneau orange** qui apparaît sur la tuile concernée.

### Incantation en cours

> Une incantation (rituel de montée de niveau) nécessite plusieurs joueurs réunis sur la même tuile avec les bonnes pierres. Le rituel prend du temps.

**Ce que vous voyez pendant le rituel :** La tuile est recouverte d'un **voile violet translucide** avec un contour violet lumineux. Les joueurs participants ont leur contour en blanc.

### Fin d'incantation — Résultat

> Quand l'incantation se termine, le résultat est immédiatement visible.

| Résultat | Couleur de l'animation |
|----------|------------------------|
| **Succès** — les joueurs montent de niveau | Flash **vert** sur la tuile |
| **Échec** — les conditions n'étaient pas remplies | Flash **rouge** sur la tuile |

---

## 11. Le panneau latéral

Le panneau à droite de l'écran est organisé en **6 sections pliables**. Vous pouvez cliquer sur le titre de chaque section pour la déplier ou replier.

---

### Section 1 — Informations globales (en haut)

Ces informations sont toujours visibles, même si toutes les sections sont repliées :

| Information | Description |
|-------------|-------------|
| **ZAPPY** | Le titre de l'application |
| **Carte** | La taille de la carte (exemple : `10 x 10`) |
| **Joueurs** | Le nombre total de joueurs actuellement en vie |

---

### Section 2 — Équipes

Liste toutes les équipes présentes dans la partie avec, pour chacune :

- Un **carré de couleur** identifiant l'équipe (la même couleur que celle des joueurs sur la carte)
- Le **nom** de l'équipe
- Le **nombre de joueurs** encore en vie
- Le **niveau maximum** atteint par un joueur de cette équipe

---

### Section 3 — Classement

Un tableau de classement des joueurs, **trié du plus haut niveau au plus bas**.

#### Filtrer par niveau

Des boutons **`<`** et **`>`** permettent de filtrer l'affichage pour ne voir que les joueurs d'un niveau précis :
- `<` : diminue le filtre de niveau (ou désactive le filtre si on est déjà au niveau 1)
- `>` : augmente le filtre de niveau (ou active le filtre sur le niveau 1 si aucun filtre n'est actif)

Quand aucun filtre n'est actif, tous les joueurs sont listés.

#### Faire défiler la liste

Des **flèches** (triangle vers le haut / vers le bas) permettent de faire défiler la liste si elle contient plus de 10 joueurs.

#### Cliquer sur un joueur

Cliquer sur une ligne du classement **sélectionne ce joueur** sur la carte et affiche ses informations détaillées dans la section Sélection.

---

### Section 4 — Ressources (totaux sur la carte)

Affiche la **quantité totale** de chaque ressource présente sur l'ensemble de la carte (pas dans les inventaires des joueurs, uniquement ce qui est au sol).

---

### Section 5 — Sélection

Cette section change dynamiquement selon ce que vous avez cliqué sur la carte.

#### Aucune sélection

Le texte « Aucune selection » s'affiche.

#### Une tuile est sélectionnée (sans joueur dessus)

Le titre de la section devient `Tuile (x, y)` et affiche :
- Les ressources présentes sur cette tuile (uniquement celles dont la quantité est > 0)
- Le nombre de joueurs sur cette tuile
- Le nombre d'œufs sur cette tuile

#### Un joueur est sélectionné

Le titre devient `Joueur #<numéro>` et affiche :
- Son **équipe** (dans sa couleur)
- Son **niveau** et sa **direction** (N, E, S ou O)
- Sa **position** sur la carte (coordonnées x, y)
- Son **inventaire complet** : la quantité de chaque type de ressource qu'il porte

---

### Section 6 — Contrôles

Un rappel visuel de tous les raccourcis clavier disponibles, avec leur état actuel (par exemple, les ressources sont ON ou OFF).

---

### Section 7 — Journal d'événements

Les **8 derniers événements importants** survenus dans la partie, du plus récent au plus ancien. Les messages trop longs sont tronqués avec `...` pour tenir dans le panneau.

Événements enregistrés dans le journal :

| Événement | Message affiché dans le journal |
|-----------|--------------------------------|
| Mort d'un joueur | `Mort du joueur #12` |
| Début d'incantation | `Incantation niv 3 en (5, 7)` |
| Fin d'incantation (succès) | `Incantation reussie en (5, 7)` |
| Fin d'incantation (échec) | `Incantation echouee en (5, 7)` |
| Message diffusé (broadcast) | `Broadcast #5 : [message]` |
| Éjection | `Ejection par le joueur #3` |
| Ponte d'un œuf | `Ponte du joueur #8` |
| Fin de partie | `Fin de partie, vainqueur : TeamA` |

---

## 12. Les contrôles

### Clavier

| Touche | Action | Détail |
|--------|--------|--------|
| `Espace` | **Recentrer** la caméra | Remet la vue au centre de la carte |
| `←` | **Déplacer** la vue vers la gauche | |
| `→` | **Déplacer** la vue vers la droite | |
| `↑` | **Déplacer** la vue vers le haut | |
| `↓` | **Déplacer** la vue vers le bas | |
| `+` ou `=` | **Accélérer** le jeu | Augmente la fréquence du serveur de +10 |
| `-` | **Ralentir** le jeu | Diminue la fréquence du serveur de -10 (minimum : 1) |
| `R` | **Afficher / Masquer** les ressources | Bascule l'affichage des petits points de ressources |
| `T` | **Afficher / Masquer** les arbres | Bascule l'affichage de la forêt en bordure de carte |
| `Échap` | **Fermer** la fenêtre | Quitte le programme |

> **Important sur `+` et `-` :** Ces touches envoient une commande directement au serveur. Cela change la vitesse de la partie pour **tout le monde** (toutes les IA connectées). À utiliser avec précaution lors des démonstrations.

### Souris

| Action | Effet |
|--------|-------|
| **Molette vers le haut** | Zoom avant (la carte grossit, jusqu'à ×5) |
| **Molette vers le bas** | Zoom arrière (la carte rétrécit, jusqu'à ×0.25) |
| **Clic gauche sur la carte** | Sélectionne un joueur ou une tuile |
| **Clic gauche en dehors de la carte** | Désélectionne tout |
| **Clic gauche sur un en-tête de section** | Déploie ou replie la section du panneau |
| **Clic gauche sur une ligne du classement** | Sélectionne ce joueur |

### Priorité de sélection lors d'un clic sur la carte

Quand vous cliquez sur une tuile :
1. S'il y a un **joueur** dessus → le joueur est sélectionné
2. S'il n'y a **pas de joueur** → la tuile est sélectionnée
3. S'il y a **plusieurs joueurs** sur la tuile → le dernier dans la liste interne est sélectionné

---

## 13. Fin de partie

Quand une équipe remporte la partie (6 joueurs ou plus au niveau 8), le serveur envoie le signal de victoire. La GUI affiche alors un **écran de fin** par-dessus la carte :

### Ce qui s'affiche

```
+------------------------------------------------+
|                                                |
|        [voile noir semi-transparent]           |
|                                                |
|   ╔══════════════════════════════════════╗     |
|   ║                                      ║     |
|   ║      Vainqueur : [NomDeLéquipe]      ║     |
|   ║                                      ║     |
|   ╚══════════════════════════════════════╝     |
|       [bannière de la couleur gagnante]        |
|                                                |
+------------------------------------------------+
```

1. Un **voile sombre** recouvre toute la fenêtre
2. Une **bannière colorée** (de la couleur de l'équipe gagnante) apparaît au centre
3. Le texte **"Vainqueur : [nom]"** s'affiche en grand et en noir sur la bannière

### Après la fin de partie

La fenêtre **ne se ferme pas automatiquement**. Vous pouvez continuer à observer la carte dans son état final. Fermez la fenêtre manuellement avec `Échap` ou en cliquant sur la croix.

---

## 14. Perte de connexion

Si la connexion avec le serveur est interrompue (coupure réseau, serveur arrêté, etc.) :

### Ce qui s'affiche

Une **bannière rouge** apparaît en haut de l'écran avec le message :

```
╔══════════════════════════════════════════════╗
║   Connexion au serveur perdue                ║
╚══════════════════════════════════════════════╝
```

### Ce qu'il se passe

- La fenêtre **reste ouverte** : vous pouvez toujours voir la carte dans son dernier état connu
- Le jeu **ne reçoit plus de mises à jour** : tout est figé
- Pour reprendre, vous devez **fermer et relancer** la GUI

---

## 15. Questions fréquentes (FAQ)

**Q : La GUI ne s'ouvre pas, que faire ?**
> Vérifiez que le serveur est bien lancé, et que vous avez utilisé la bonne adresse (`-h`) et le bon port (`-p`). Si le serveur n'est pas accessible, la GUI affiche une erreur dans le terminal et se ferme.

**Q : Je vois des joueurs mais pas leurs ressources, c'est normal ?**
> Oui. Si les tuiles sont trop petites (vue dézoomée), les quantités de ressources sont masquées automatiquement pour ne pas surcharger l'affichage. Zoomez avec la molette pour les voir apparaître.

**Q : Qu'est-ce que le « niveau » d'un joueur ?**
> Le niveau représente la progression du joueur dans le jeu. Il va de 1 (débutant) à 8 (maximum). Pour monter de niveau, les joueurs doivent accomplir des incantations. Le premier niveau facilement atteint est le 2, le niveau 8 est extrêmement difficile.

**Q : Pourquoi certains joueurs ont un contour blanc ?**
> Un contour blanc signifie que le joueur participe actuellement à une **incantation** (un rituel de montée de niveau).

**Q : La carte est toute petite, comment l'agrandir ?**
> Utilisez la molette de la souris vers le haut pour zoomer, ou la touche `Espace` pour recentrer la vue. Si la fenêtre est trop petite, vous pouvez aussi l'agrandir en faisant glisser ses bords.

**Q : Peut-on changer la taille de la fenêtre ?**
> Oui. La carte s'adapte automatiquement quand vous redimensionnez la fenêtre.

**Q : Les touches `+` et `-` changent-elles vraiment la vitesse de toute la partie ?**
> Oui, elles envoient une commande au serveur qui affecte **tous les joueurs**. C'est utile pour observer le jeu au ralenti ou en accéléré lors d'une démonstration.

**Q : Je vois des ovales crème sur la carte, c'est quoi ?**
> Ce sont des **œufs**. Un joueur les a pondus. Un nouveau joueur-IA pourra s'y connecter et en sortir.

**Q : La partie est terminée mais la fenêtre ne se ferme pas, c'est un bug ?**
> Non, c'est voulu. Vous pouvez continuer à observer la carte après la fin de partie. Fermez la fenêtre manuellement avec `Échap`.

**Q : Je vois une bannière rouge en haut, que se passe-t-il ?**
> Le serveur s'est déconnecté ou la connexion a été coupée. La GUI affiche l'état figé de la carte. Relancez la GUI pour vous reconnecter.

---

## 16. Glossaire

| Terme | Définition |
|-------|------------|
| **Broadcast** | Message qu'un joueur envoie à tous les autres joueurs de la carte en même temps |
| **Carte** | Le plateau de jeu, découpé en cases (tuiles) |
| **Client** | Programme qui se connecte à un serveur |
| **Éjection** | Action par laquelle un joueur pousse tous ses voisins hors de sa tuile |
| **Éclosion** | Moment où un œuf s'ouvre et laisse sortir un nouveau joueur |
| **Fréquence** | Vitesse à laquelle le serveur fait avancer le jeu. Plus la fréquence est haute, plus le jeu va vite |
| **GUI** | Graphical User Interface — Interface graphique, ce que vous regardez |
| **HUD** | Heads-Up Display — Affichage d'informations à l'écran (le panneau latéral) |
| **IA** | Intelligence Artificielle — Programme informatique qui joue automatiquement |
| **Incantation** | Rituel réalisé par plusieurs joueurs sur une même tuile pour monter de niveau |
| **Inventaire** | Ce qu'un joueur transporte sur lui (ressources) |
| **Niveau** | Degré de progression d'un joueur (1 à 8). Atteindre le niveau 8 avec 6 joueurs d'une même équipe remporte la partie |
| **Œuf** | Structure pondable par un joueur, depuis laquelle un nouveau joueur peut naître |
| **Overlay** | Superposition visuelle semi-transparente sur la carte (ex. le voile violet des incantations) |
| **Panneau latéral** | Zone à droite de l'écran affichant les statistiques et informations de la partie |
| **Port** | Numéro identifiant un service réseau sur une machine (exemple : 4242) |
| **Ressource** | Objet récupérable sur la carte : nourriture ou pierres précieuses |
| **Serveur** | Programme central qui gère la partie et avec lequel tous les clients communiquent |
| **Tuile** | Une case de la carte du jeu |
| **Zoom** | Agrandissement ou réduction de l'affichage de la carte |

---

*Documentation générée pour le projet Zappy — EPITECH 2025*
