# AI_PLAN — Plan d'implémentation de `zappy_ai`

> Projet **Zappy**. Cible : Linux.
> Langage : **Python 3**. Client IA autonome gérant la survie, la collecte et l'élévation d'un drone sur la planète Trantor.

---

## 1. Architecture du Client IA

Pour permettre le travail en parallèle, le projet est découpé en trois couches indépendantes :

1.  **Couche Réseau (`network.py`)** : Gère l'I/O (sockets, transmission et réception ligne par ligne). Ne connaît **aucune** règle du jeu.
2.  **Couche État (`bot.py` & `state.py`)** : Gère l'inventaire, le niveau actuel, et la file des requêtes en attente.
3.  **Couche Stratégie (`strategy.py` & `pathfinding.py`)** : La machine à états (FSM). Prend des décisions et planifie les trajectoires.

---

## 2. Répartition des tâches (Travail en parallèle)

### **Dev A : Réseau, Environnement et Parsing**
*Fichiers principaux : `network.py`, `bot.py`, `state.py`*

- [x] **Handshake TCP** : Implémenter la connexion et la réception de `CLIENT-NUM` et `X Y`.
- [ ] **Boucle de jeu & Buffering** : Gérer l'envoi asynchrone des commandes (file d'attente de maximum 10 commandes) et la réception continue sans désynchroniser le socket.
- [ ] **Parsing des réponses** : Transformer les chaînes de caractères brutes du serveur en structures Python claires :
  - `[food 345, sibur 3...]` -> Dictionnaire/Objet `Inventory`.
  - `[player, food,, thystame,,,]` -> Tableau de listes pour la vision.
- [ ] **Gestion des déconnexions et événements imprévus** : Intercepter proprement les messages prioritaires du serveur (ex: `eject: K` ou la mort du drone) sans bloquer la file d'attente.

### **Dev B : Stratégie, Déplacement et Broadcast**
*Fichiers principaux : `strategy.py`, `pathfinding.py`, `broadcast.py`*

- [ ] **Survie (Priorité absolue)** : Surveiller l'inventaire de manière constante. Si la nourriture descend sous un seuil critique, interrompre l'objectif en cours pour basculer sur la recherche de nourriture.
- [ ] **Logique de Déplacement & Pathfinding** : Analyser le résultat textuel du `Look` fourni par le Dev A, localiser la ressource ciblée (nourriture ou pierre) et calculer la séquence exacte de mouvements (`Forward`, `Left`, `Right`) pour l'atteindre.
- [ ] **Décryptage du Broadcast** : 
  - Analyser la direction du son `K` (1 à 8) pour orienter le drone vers l'émetteur.
  - Concevoir le protocole de communication de l'équipe (format des messages pour se rassembler ou demander de l'aide).
- [ ] **Le Rituel d'Élévation (Incantation)** : 
  - Stocker la table des prérequis de chaque niveau.
  - Coordonner le dépôt des pierres nécessaires (`Set <pierre>`) et déclencher l'action `Incantation`.
- [ ] **Gestion des Œufs (Reproduction)** : Surveiller le nombre de slots disponibles et planifier un `Fork` pour agrandir l'équipe lorsque les ressources le permettent.

---

## 3. Phases de développement (Milestones)

* **Phase 1 : Le Marcheur Aveugle**
    * L'IA se connecte, maintient sa connexion réseau sans planter et met à jour son inventaire en temps réel.
* **Phase 2 : Le Survivant Autonome**
    * L'IA utilise sa vision (`Look`) pour repérer la nourriture, s'y déplacer intelligemment et survivre indéfiniment seule.
* **Phase 3 : Le Mineur**
    * L'IA récolte les pierres nécessaires à son premier changement de niveau et monte au niveau 2 en solo.
* **Phase 4 : La Ruche (Fin de partie)**
    * Plusieurs processus d'IA collaborent par `Broadcast` pour se regrouper, partager leurs ressources et atteindre le niveau 8.