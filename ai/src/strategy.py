from typing import Optional
from state import Inventory, ActionState, ELEVATION_REQUIREMENTS

FOOD_CRITICAL = 10
FOOD_SAFE = 20
FOOD_FORK_MIN = 20
MIN_TEAM_SLOTS = 2

MAX_TOTAL_SPAWN = 8

class Strategy:
    """Machine à états qui décide de l'objectif courant du bot."""

    def __init__(self):
        self.state = ActionState.SURVIVE
        self.level = 1
        self.gather_direction: Optional[int] = None
        self.players_on_tile: int = 1
        self.confirmations: int = 0
        self._pending_broadcast: Optional[str] = None
        self.spawn_cooldown: int = 0
        self.allies_requested: int = 0
        self.total_spawned: int = 0
        self.incant_cooldown: int = 0

    def _needed_players(self) -> int:
        req = ELEVATION_REQUIREMENTS.get(self.level)
        return req[0] if req else 1

    def update(self, inventory: Inventory) -> None:
        if self.spawn_cooldown > 0:
            self.spawn_cooldown -= 1
        if self.incant_cooldown > 0:
            self.incant_cooldown -= 1

        if inventory.food <= FOOD_CRITICAL:
            self.state = ActionState.SURVIVE
            self.players_on_tile = 1
            self.confirmations = 0
            self.allies_requested = 0
            self.gather_direction = None
            return

        if self.state == ActionState.ARRIVED:
            return

        if self.state == ActionState.SURVIVE and inventory.food >= FOOD_SAFE:
            self.state = ActionState.COLLECT

        if self.state == ActionState.COLLECT and inventory.has_stones_for(self.level):
            if self._needed_players() == 1:
                self.state = ActionState.INCANTATE
            elif inventory.food >= FOOD_SAFE:
                self.state = ActionState.WAIT_FOR_TEAM
                self.players_on_tile = 1
                self.confirmations = 0

    def on_broadcast_received(self, direction: int, text: str) -> None:
        if text == f"GATHER_{self.level}":
            if self.state == ActionState.WAIT_FOR_TEAM and direction == 0:
                return
            can_respond = {ActionState.COLLECT, ActionState.SURVIVE, ActionState.MOVE_TO_GATHER}
            # Élection déterministe entre deux leaders : celui qui entend l'autre
            # dans l'hémisphère avant/droite (1,2,3,8) cède et devient follower ;
            # l'autre l'entend dans l'hémisphère arrière/gauche (4,5,6,7) et reste
            # leader. L'asymétrie garantit qu'un seul des deux cède.
            if (self.state == ActionState.WAIT_FOR_TEAM
                    and self.players_on_tile <= 1
                    and direction in (1, 2, 3, 8)):
                can_respond.add(ActionState.WAIT_FOR_TEAM)
            if self.state in can_respond:
                self.gather_direction = direction
                if direction == 0:
                    self.state = ActionState.ARRIVED
                else:
                    self.state = ActionState.MOVE_TO_GATHER
        elif text == "READY" and self.state == ActionState.WAIT_FOR_TEAM:
            # READY compté toutes directions : les followers atterrissent souvent
            # adjacents (pas pile sur la tuile), donc exiger direction 0 empêcherait
            # tout comptage. players_on_tile sert de garde-fou côté on_players_seen.
            self.confirmations += 1

    def on_players_seen(self, players_on_tile: int) -> None:
        """Vérité terrain transmise par le Look : nombre de joueurs sur la tuile.

        On lance dès qu'assez de corps sont physiquement présents (Look) : c'est
        l'information fiable. Le serveur ne valide que si tous sont du bon niveau ;
        un passant de mauvais niveau provoque un ko (rare) sans rien consommer,
        et le leader réessaie. Le comptage READY/confirmations était trop fragile
        (alignement de timing improbable entre READY asynchrones et Look).
        """
        self.players_on_tile = players_on_tile
        if self.state == ActionState.WAIT_FOR_TEAM:
            needed = self._needed_players()
            if players_on_tile >= needed and self.incant_cooldown == 0:
                self.state = ActionState.INCANTATE

    def on_connect_nbr(self, slots: int, inventory: Inventory) -> None:
        """Recrute un allié manquant, avec un plafond DUR anti-explosion."""
        if self.state != ActionState.WAIT_FOR_TEAM:
            return
        if self.spawn_cooldown > 0:
            return
        if self.total_spawned >= MAX_TOTAL_SPAWN:
            return
        players_needed = self._needed_players()
        accounted = self.confirmations + 1 + self.allies_requested
        if accounted >= players_needed:
            return
        if slots > 0:
            print("[STRATÉGIE] Leader : j'occupe une place libre de l'équipe.")
            self.state = ActionState.SPAWN_ALLY
        elif inventory.food >= FOOD_FORK_MIN:
            print("[STRATÉGIE] Leader : je ponds un œuf pour agrandir l'équipe.")
            self.state = ActionState.FORK
        else:
            return
        self.allies_requested += 1
        self.total_spawned += 1
        self.spawn_cooldown = 10

    def pop_broadcast(self) -> Optional[str]:
        """Retourne et efface le message broadcast en attente."""
        msg = self._pending_broadcast
        self._pending_broadcast = None
        return msg

    def on_level_up(self) -> None:
        self.level += 1
        self.state = ActionState.COLLECT
        self.gather_direction = None
        self.players_on_tile = 1
        self._pending_broadcast = None
        self.allies_requested = 0

    def target_resource(self, inventory: Inventory) -> Optional[str]:
        if self.state == ActionState.SURVIVE or inventory.food <= FOOD_SAFE:
            return 'food'
        if self.state == ActionState.COLLECT:
            missing = inventory.missing_stones_for(self.level)
            if missing:
                return next(iter(missing))
        return None
