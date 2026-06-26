import os
import random
from typing import Optional

from state import (Inventory, ActionState, ELEVATION_REQUIREMENTS,
                   MAX_LEVEL, STONES)

# --- Seuils de nourriture (1 food = 126 unités de temps = 6,3 s à f=20) --------
# Un bot démarre avec 10 rations. Des seuils trop hauts le forçaient à hoarder la
# nourriture en permanence au lieu de progresser -> montée très lente. On garde une
# marge de survie sûre (8 rations ≈ 50 s à f=20) mais on privilégie la progression.
FOOD_PANIC = 7         # plancher dur : même un bot en attente de rituel part manger ici
FOOD_HUNGRY = 28       # au-dessus : on peut viser des pierres ; en dessous : manger
FOOD_TARGET = 35       # cap de nourriture en mode solo (niveau 1)
# GROS stock à constituer AVANT d'aller attendre un rituel : c'est ce qui permet de
# RESTER sur la case sans repartir manger (la nourriture repop aussi sur place). Le
# churn « quelqu'un repart manger pendant qu'un autre arrive » venait d'un stock trop
# faible -> les bots arrivaient affamés et ne tenaient pas l'attente.
FOOD_RESERVE = 42      # buffer visé en phase de regroupement (>= 2 joueurs)
FOOD_GO = 32           # food mini pour QUITTER la collecte et rejoindre le rendez-vous
LEADER_COMMIT_FOOD = 42  # le leader se fige bien nourri -> tient le point de rendez-vous
FOOD_MIN_INCANT = 14   # solo : ne pas incanter si on frôle la famine
# INVARIANT : FOOD_PANIC < FOOD_HUNGRY <= FOOD_TARGET <= FOOD_RESERVE (pas de zone morte).

LEADER_SILENCE_TIMEOUT = 30   # cycles sans Z/G du leader -> le croire mort
INCANT_RETRY_COOLDOWN = 6     # cycles d'attente après une incantation ratée
# Reproduction : on démarre à 1 bot/équipe et on PEUPLE par fork. Budget par PALIER
# (réinitialisé à chaque montée), throttlé par un cooldown, pour bâtir une cohorte sans
# inonder d'œufs. Forker produit des bébés niveau 1 qui alimentent le pipeline ; à terme
# un niveau accumule assez de joueurs et débloque (l'incantation élève TOUS les présents).
MAX_FORKS = 6                 # pontes max par palier
FORK_COOLDOWN = 18            # cycles mini entre deux pontes d'un même bot
STALL_FORK_DELAY = 22         # cycles bloqué au même niveau avant de forker pour débloquer

TEAM_MAX = 6                  # effectif visé (max requis aux paliers 6->7 et 7->8)
ELECTION_GRACE = 12           # cycles avant qu'un candidat puisse se figer en leader
PEER_WINDOW = 80              # fenêtre (cycles) pour compter un coéquipier vivant

# Tags de balise, encodés SANS espace (le serveur rejette tout Broadcast
# multi-token). P = présence/élection ; Z = phase forage ; G = phase assemblée.
TAG_PING = "P"
TAG_FORAGE = "Z"
TAG_GATHER = "G"


class Strategy:
    """Stratégie lockstep : leader point-fixe + collecte distribuée à 2 phases.

    - Chaque bot tire un **rang aléatoire** unique et l'annonce (`P:<niv>:<rang>`).
      Les broadcasts étant GLOBAUX, tout le monde converge sur le même leader = le
      plus PETIT rang vivant du niveau (élection avec petite grâce anti-split).
    - **Niveau 1 (1 joueur) = SOLO** : chacun monte seul (linemate + food). Pas de
      rendez-vous : démarrage parallèle immédiat.
    - **Niveaux >= 2 = GATHER** : le leader se fige en **point fixe** (rendez-vous
      stable) dès qu'il a de quoi tenir. Tout le monde **forage le set du palier en
      parallèle** et vient le **déposer** sur la case du leader (`Z`). Quand le set
      complet est sur la case, le leader passe en phase **assemblée** (`G`) : tous
      rappliquent et restent ; à `needed` joueurs présents -> incantation, tous
      montent ensemble.
    - **Résilience** : leader muet (mort) -> réélection ; affamé -> manger puis
      reconverger ; incantation ratée -> retentée. L'effectif visé est fenêtré, donc
      la mort d'un bot réduit la cible et débloque l'incantation.
    """

    def __init__(self, team_name: str):
        self.team_name = team_name
        self.idx = random.randint(1, 2_000_000_000)
        # Hook de TEST uniquement : amorcer le niveau interne pour valider les paliers
        # hauts sans la longue montée (à coupler avec /setLevel côté serveur).
        self.level = int(os.environ.get("ZAPPY_START_LEVEL", "1") or "1")
        self.leader_idx = self.idx
        self.state = ActionState.COLLECT
        self.gather_direction: Optional[int] = None
        self.phase = TAG_FORAGE
        self.tile_ready = False
        self.deposited = False
        self.committed = False
        self.silence = 0
        self.peer_seen: dict[int, int] = {}
        self.incant_cooldown = 0
        self.total_forks = 0
        self.last_fork_cycle = -FORK_COOLDOWN
        self.cycle = 0
        self.level_age = 0

    # ------------------------------------------------------------------ infos
    def needed_players(self) -> int:
        req = ELEVATION_REQUIREMENTS.get(self.level)
        return req[0] if req else 1

    def is_leader(self) -> bool:
        return self.leader_idx == self.idx

    def team_size(self) -> int:
        """Effectif vivant estimé : coéquipiers entendus récemment (P) + soi.
        Fenêtré pour qu'un bot mort cesse d'être compté -> le lockstep s'adapte."""
        alive = sum(1 for c in self.peer_seen.values()
                    if self.cycle - c <= PEER_WINDOW)
        return min(TEAM_MAX, alive + 1)

    # --------------------------------------------- collecte distribuée par rang
    def _participants_sorted(self) -> list[int]:
        """Rangs des participants vivants (pairs récents + soi), triés. Déterministe :
        chaque bot calcule la MÊME liste à partir des P entendus, d'où une répartition
        des pierres cohérente sans dialogue point-à-point (impossible dans le protocole)."""
        alive = [r for r, c in self.peer_seen.items()
                 if self.cycle - c <= PEER_WINDOW]
        return sorted(set(alive) | {self.idx})

    def my_share(self) -> dict[str, int]:
        """{pierre: quantité} que CE bot doit forager pour le palier (split par rang).

        Le set requis est étendu par quantité en une liste, distribuée en round-robin
        selon ma position dans les participants triés. Seul (aucun pair connu) -> je
        prends tout le set (pas de split possible). Les trous éventuels (désaccord sur
        les pairs, mort d'un porteur) sont rattrapés par le gap-fill du rendez-vous."""
        req = ELEVATION_REQUIREMENTS.get(self.level)
        if not req:
            return {}
        _, *needed = req
        items: list[str] = []
        for stone, n in zip(STONES, needed):
            items += [stone] * n
        parts = self._participants_sorted()
        n = len(parts)
        pos = parts.index(self.idx)
        share: dict[str, int] = {}
        for i, it in enumerate(items):
            if n <= 1 or i % n == pos:
                share[it] = share.get(it, 0) + 1
        return share

    def missing_share(self, inventory: Inventory) -> dict[str, int]:
        """Ce qui manque dans mon inventaire pour couvrir MA part assignée."""
        have = dict(zip(STONES, inventory.stone_counts()))
        return {s: q - have.get(s, 0)
                for s, q in self.my_share().items() if q > have.get(s, 0)}

    def has_my_share(self, inventory: Inventory) -> bool:
        return not self.missing_share(inventory)

    def beacon(self) -> Optional[str]:
        """Message à diffuser ce cycle. Le leader figé balise sa phase (Z forage /
        G assemblée) ; tout le monde ping P (présence + élection).
        Le nom d'équipe est inclus pour éviter que les bots adverses convergent."""
        if self.level >= MAX_LEVEL or self.needed_players() == 1:
            return None
        if self.is_leader() and self.state in (ActionState.LEAD, ActionState.INCANT):
            tag = TAG_GATHER if self.tile_ready else TAG_FORAGE
            return f"{tag}:{self.team_name}:{self.level}:{self.idx}"
        return f"{TAG_PING}:{self.team_name}:{self.level}:{self.idx}"

    # --------------------------------------------------------- horloge / FSM
    def update(self, inventory: Inventory) -> None:
        """Recalcule l'état. Appelé après chaque réponse Inventory."""
        self.cycle += 1
        self.level_age += 1
        self.silence += 1
        if self.incant_cooldown > 0:
            self.incant_cooldown -= 1

        if self.level >= MAX_LEVEL:
            self.state = ActionState.DONE
            return

        # SOLO : palier à 1 joueur (niveau 1)
        if self.needed_players() == 1:
            self.leader_idx = self.idx
            if inventory.food <= FOOD_PANIC:
                self.state = ActionState.SURVIVE
            elif (inventory.has_stones_for(self.level)
                  and inventory.food >= FOOD_MIN_INCANT):
                self.state = ActionState.INCANT
            else:
                self.state = ActionState.COLLECT
            return

        # GATHER : palier >= 2 joueurs
        if not self.is_leader() and self.silence > LEADER_SILENCE_TIMEOUT:
            self._reset_election()

        if self.is_leader():
            self._update_leader(inventory)
        else:
            self._update_follower(inventory)

    def _update_leader(self, inventory: Inventory) -> None:
        """Le leader se fige TÔT (grâce d'élection + un minimum de food), SANS attendre
        d'avoir tout le set : sinon, aux paliers hauts (7 pierres au niveau 6), il
        roamerait sans jamais baliser et personne ne convergerait. Une fois figé il
        reste en place (rendez-vous stable) et balise Z/G ; ce sont les suiveurs qui
        apportent les pierres en parallèle (collecte distribuée)."""
        if self.state == ActionState.INCANT:
            return
        if inventory.food <= FOOD_PANIC:
            self.state = ActionState.SURVIVE
            return
        if self.committed:
            self.state = ActionState.LEAD
            return
        if self.level_age >= ELECTION_GRACE and inventory.food >= LEADER_COMMIT_FOOD:
            self.committed = True
            self.state = ActionState.LEAD
        else:
            self.state = ActionState.COLLECT

    def _update_follower(self, inventory: Inventory) -> None:
        """2 phases : tant que le set n'est pas sur la case (balise Z), chaque suiveur
        forage SON set puis vient le déposer. Une fois le set au sol (balise G), il
        suffit d'être présent : on converge même sans pierres (assemblage des corps).

        On ne rejoint le rendez-vous qu'avec un STOCK de nourriture suffisant (FOOD_GO)
        pour tenir l'attente sans repartir manger -> fin du churn qui bloquait au niv 4."""
        if inventory.food <= FOOD_PANIC:
            self.state = ActionState.SURVIVE
            return
        if self.gather_direction is None:
            self.state = ActionState.COLLECT           # pas de leader : forager set + stock
            return
        if self.gather_direction == 0:
            self.state = ActionState.WAIT_TILE          # déjà sur la case du leader
            return
        fed = inventory.food >= FOOD_GO
        ready = self.phase == TAG_GATHER or self.has_my_share(inventory)
        self.state = ActionState.FOLLOW if (fed and ready) else ActionState.COLLECT

    # --------------------------------------------------------- réception radio
    def on_broadcast_received(self, direction: int, text: str) -> None:
        parts = text.split(":")
        if len(parts) != 4:
            return
        tag, team, lvl_s, rank_s = parts
        if team != self.team_name:
            return
        try:
            lvl, rank = int(lvl_s), int(rank_s)
        except ValueError:
            return
        if lvl != self.level or rank == self.idx:
            return

        if tag == TAG_PING:
            self.peer_seen[rank] = self.cycle
            if rank < self.leader_idx:
                self.leader_idx = rank
        elif tag in (TAG_FORAGE, TAG_GATHER):
            if rank <= self.leader_idx:
                self.leader_idx = rank
                self.gather_direction = direction
                self.phase = tag
                self.silence = 0

    # --------------------------------------------- déclenchement du rituel
    def on_ready(self, players_on_tile: int, tile_ready: bool) -> None:
        """Appelé par le leader (LEAD) avec le nb de joueurs sur sa case et la
        présence du set complet dessus.

        **Incant-and-retry** : `Look` est aveugle (les jetons `player` ne portent ni
        niveau ni équipe), donc compter les corps ne garantit pas qu'on a `needed`
        joueurs *du bon niveau*. Plutôt que d'attendre un comptage parfait, on TENTE
        l'incantation dès que le set est au sol et qu'au moins `needed` corps sont là.
        Le serveur arbitre : un `ko` est instantané et sans coût (pierres conservées),
        on temporise alors (`on_incant_failed`) et on retente. L'incantation part donc
        à la milliseconde où assez de bons joueurs ont convergé."""
        if self.state != ActionState.LEAD or self.incant_cooldown > 0:
            return
        self.tile_ready = tile_ready
        if tile_ready and players_on_tile >= self.needed_players():
            self.state = ActionState.INCANT

    # ------------------------------------------------------------ recrutement
    def is_progress_stalled(self) -> bool:
        """Progression bloquée : palier de groupe, PAS assez de pairs de mon niveau
        pour avancer, et ça dure (level_age). C'est le signal « il faut peupler » : on
        autorise alors le fork même figé en LEAD/WAIT_TILE (cas 4@6/3@5/1@2 où personne
        ne pouvait évoluer faute d'effectif, et où plus personne ne pondait)."""
        return (self.level < MAX_LEVEL
                and self.needed_players() > 1
                and self.team_size() < self.needed_players()
                and self.level_age >= STALL_FORK_DELAY)

    def want_fork(self, slots: int) -> bool:
        """Pondre pour PEUPLER quand l'effectif de mon niveau est sous le minimum du
        palier. Budget par palier + cooldown pour éviter d'inonder d'œufs. Pas de fork
        en solo niveau 1 (coûteux et inutile : un seul joueur suffit au palier 1->2)."""
        if self.level >= MAX_LEVEL or self.total_forks >= MAX_FORKS:
            return False
        if self.needed_players() == 1:
            return False
        if self.team_size() >= self.needed_players():
            return False
        if self.cycle - self.last_fork_cycle < FORK_COOLDOWN:
            return False
        self.last_fork_cycle = self.cycle
        self.total_forks += 1
        return True

    # --------------------------------------------------------------- élévation
    def on_level_up(self) -> None:
        self.level += 1
        self._reset_election()
        self.committed = False
        self.deposited = False
        self.tile_ready = False
        self.phase = TAG_FORAGE
        self.incant_cooldown = 0
        self.level_age = 0
        self.total_forks = 0          # budget de pontes neuf à chaque palier
        self.peer_seen.clear()
        self.state = ActionState.COLLECT

    def on_incant_failed(self) -> None:
        """Conditions non réunies à la vérif finale : on temporise et on retente."""
        self.incant_cooldown = INCANT_RETRY_COOLDOWN
        self.deposited = False
        if self.is_leader():
            self.committed = True
            self.state = ActionState.LEAD
        else:
            self.state = ActionState.WAIT_TILE

    def _reset_election(self) -> None:
        self.leader_idx = self.idx
        self.gather_direction = None
        self.phase = TAG_FORAGE
        self.silence = 0

    # ------------------------------------------ que ramasser (collecte dirigée)
    def target_resource(self, inventory: Inventory) -> Optional[str]:
        """Ressource à viser dans le Look. Priorité : manger si on a faim, puis récolter
        une pierre manquante du palier, puis CONSTITUER LE STOCK de nourriture (gros en
        mode regroupement pour tenir l'attente du rituel, plus modeste en solo)."""
        if inventory.food <= FOOD_HUNGRY:
            return 'food'
        # En regroupement (>= 2 joueurs) je ne forage que MA part (split par rang) ;
        # en solo je vise le set complet (juste le linemate au niveau 1).
        if self.needed_players() > 1:
            missing = self.missing_share(inventory)
        else:
            missing = inventory.missing_stones_for(self.level)
        if missing:
            return next(iter(missing))
        reserve = FOOD_RESERVE if self.needed_players() > 1 else FOOD_TARGET
        if inventory.food < reserve:
            return 'food'
        return None
