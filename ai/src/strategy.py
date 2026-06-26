import random
from typing import Optional

from state import (Inventory, ActionState, ELEVATION_REQUIREMENTS,
                   MAX_LEVEL)

# --- Seuils de nourriture (1 food = 126 unités de temps = 1,26 s à f=100) -----
FOOD_PANIC = 20        # en dessous : survie absolue, tout lâcher pour manger
FOOD_HUNGRY = 50       # en dessous : manger avant de collecter des pierres
FOOD_TARGET = 70       # cap d'un suiveur
LEADER_RESERVE = 60    # food mini pour qu'un leader se fige en balise
FOOD_MIN_INCANT = 35   # solo : ne pas incanter si on frôle la famine

LEADER_SILENCE_TIMEOUT = 30   # cycles sans Z/G du leader -> le croire mort
INCANT_RETRY_COOLDOWN = 8     # cycles d'attente après une incantation ratée
MAX_FORKS = 8

TEAM_MAX = 6                  # effectif visé (max requis aux paliers 6->7 et 7->8)
ELECTION_GRACE = 12           # cycles avant qu'un candidat puisse se figer en leader
PEER_WINDOW = 80              # fenêtre (cycles) pour compter un coéquipier vivant
STALL_FALLBACK = 80           # cycles d'attente max avant d'incanter avec ce qui est là

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

    def __init__(self):
        self.idx = random.randint(1, 2_000_000_000)
        self.level = 1
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
        self.cycle = 0
        self.level_age = 0
        self.stall = 0

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

    def beacon(self) -> Optional[str]:
        """Message à diffuser ce cycle. Le leader figé balise sa phase (Z forage /
        G assemblée) ; tout le monde ping P (présence + élection)."""
        if self.level >= MAX_LEVEL or self.needed_players() == 1:
            return None
        if self.is_leader() and self.state in (ActionState.LEAD, ActionState.INCANT):
            tag = TAG_GATHER if self.tile_ready else TAG_FORAGE
            return f"{tag}:{self.level}:{self.idx}"
        return f"{TAG_PING}:{self.level}:{self.idx}"

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
        if self.state == ActionState.INCANT:
            return
        if inventory.food <= FOOD_PANIC:
            self.state = ActionState.SURVIVE
            return
        if self.committed:
            self.state = ActionState.LEAD
            return
        if self.level_age >= ELECTION_GRACE and inventory.food >= LEADER_RESERVE:
            self.committed = True
            self.state = ActionState.LEAD
        else:
            self.state = ActionState.COLLECT

    def _update_follower(self, inventory: Inventory) -> None:
        if self.gather_direction is None:
            self.state = ActionState.COLLECT
        elif self.gather_direction == 0:
            self.state = ActionState.WAIT_TILE
        elif self.phase == TAG_GATHER:
            self.state = ActionState.FOLLOW
        elif inventory.has_stones_for(self.level):
            self.state = ActionState.FOLLOW
        else:
            self.state = ActionState.COLLECT

    # --------------------------------------------------------- réception radio
    def on_broadcast_received(self, direction: int, text: str) -> None:
        parts = text.split(":")
        if len(parts) != 3:
            return
        tag, lvl_s, rank_s = parts
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

        Phase FORAGE tant que le set n'est pas là (balise Z). Une fois le set
        déposé (balise G), on attend TOUTE l'équipe vivante (lockstep) puis on
        incante. Fallback anti-blocage : au bout de STALL_FALLBACK cycles on
        incante dès que le minimum réglementaire du palier est présent."""
        if self.state != ActionState.LEAD or self.incant_cooldown > 0:
            return
        self.tile_ready = tile_ready
        if not tile_ready:
            self.stall = 0
            return
        needed = self.needed_players()
        target = max(needed, self.team_size())
        if players_on_tile >= target:
            self.state = ActionState.INCANT
            self.stall = 0
        else:
            self.stall += 1
            if players_on_tile >= needed and self.stall >= STALL_FALLBACK:
                self.state = ActionState.INCANT
                self.stall = 0

    # ------------------------------------------------------------ recrutement
    def want_fork(self, slots: int) -> bool:
        """Pondre pour maintenir l'effectif vers le nombre requis du palier."""
        if self.level >= MAX_LEVEL or self.total_forks >= MAX_FORKS:
            return False
        if self.team_size() >= self.needed_players():
            return False
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
        self.stall = 0
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
        """Ressource à viser dans le Look. En mode gather tout le monde forage le
        set du palier (collecte distribuée) puis le déposera ; la survie (food)
        reste prioritaire dès qu'on a faim."""
        if inventory.food > FOOD_HUNGRY:
            missing = inventory.missing_stones_for(self.level)
            if missing:
                return next(iter(missing))
        big = self.is_leader() or self.needed_players() == 1
        cap = LEADER_RESERVE if big else FOOD_TARGET
        if inventory.food < cap:
            return 'food'
        return None
