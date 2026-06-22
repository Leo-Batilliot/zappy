from typing import Optional
from state import Inventory, ActionState, ELEVATION_REQUIREMENTS

FOOD_CRITICAL = 10
FOOD_SAFE = 20
FOOD_FORK_MIN = 20
MIN_TEAM_SLOTS = 2

class Strategy:
    """Machine à états qui décide de l'objectif courant du bot."""

    def __init__(self):
        self.state = ActionState.SURVIVE
        self.level = 1
        self.gather_direction: Optional[int] = None
        self.ready_count: int = 0
        self._pending_broadcast: Optional[str] = None
        self.spawn_cooldown: int = 0
        self.allies_requested: int = 0

    def _needed_players(self) -> int:
        req = ELEVATION_REQUIREMENTS.get(self.level)
        return req[0] if req else 1

    def update(self, inventory: Inventory) -> None:
        if self.spawn_cooldown > 0:
            self.spawn_cooldown -= 1
        if inventory.food <= FOOD_CRITICAL:
            self.state = ActionState.SURVIVE
            self.ready_count = 0
            self.allies_requested = 0
            self.gather_direction = None
            return
        if inventory.food <= FOOD_CRITICAL and self.state not in (ActionState.WAIT_FOR_TEAM, ActionState.ARRIVED):
            self.state = ActionState.SURVIVE
            return
        if self.state == ActionState.ARRIVED:
            return
        if self.state == ActionState.SURVIVE and inventory.food >= FOOD_SAFE:
            self.state = ActionState.COLLECT

        if self.state == ActionState.COLLECT and inventory.has_stones_for(self.level):
            if self._needed_players() == 1:
                self.state = ActionState.INCANTATE
            else:
                self.state = ActionState.WAIT_FOR_TEAM
                self.ready_count = 0
                self._pending_broadcast = f"GATHER_{self.level}"

    def on_broadcast_received(self, direction: int, text: str) -> None:
        if text == f"GATHER_{self.level}":
            if self.state in (ActionState.COLLECT, ActionState.SURVIVE, ActionState.MOVE_TO_GATHER):
                self.gather_direction = direction
                if direction == 0:
                    self.state = ActionState.ARRIVED
                    self._pending_broadcast = "READY"
                else:
                    self.state = ActionState.MOVE_TO_GATHER
        elif text == "READY" and self.state == ActionState.WAIT_FOR_TEAM:
            self.ready_count += 1
            if self.ready_count >= self._needed_players() - 1:
                self.state = ActionState.INCANTATE

    def on_connect_nbr(self, slots: int, inventory: Inventory) -> None:
        """Décide de recruter uniquement si on est le Leader et qu'il manque du monde."""
        if self.state != ActionState.WAIT_FOR_TEAM:
            return
        if self.spawn_cooldown > 0:
            return
        players_needed = self._needed_players()
        players_present = self.ready_count + 1
        if players_present + self.allies_requested < players_needed:
            if slots > 0:
                print(f"[STRATÉGIE] Leader : J'invoque UN seul allié manquant.")
                self.state = ActionState.SPAWN_ALLY
                self.allies_requested += 1
                self.spawn_cooldown = 10
            elif inventory.food >= FOOD_FORK_MIN:
                print(f"[STRATÉGIE] Leader : Je ponds UN seul œuf manquant.")
                self.state = ActionState.FORK
                self.allies_requested += 1
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
        self.ready_count = 0
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
