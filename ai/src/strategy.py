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

    def _needed_players(self) -> int:
        req = ELEVATION_REQUIREMENTS.get(self.level)
        return req[0] if req else 1

    def update(self, inventory: Inventory) -> None:
        if inventory.food <= FOOD_CRITICAL and self.state != ActionState.WAIT_FOR_TEAM:
            self.state = ActionState.SURVIVE
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
                    self._pending_broadcast = "READY"
                else:
                    self.state = ActionState.MOVE_TO_GATHER
        elif text == "READY" and self.state == ActionState.WAIT_FOR_TEAM:
            self.ready_count += 1
            if self.ready_count >= self._needed_players() - 1:
                self.state = ActionState.INCANTATE

    def on_connect_nbr(self, slots: int, inventory: Inventory) -> None:
        """Décide si on doit forker en fonction des slots disponibles."""
        if slots >= MIN_TEAM_SLOTS:
            return
        if self.state not in (ActionState.COLLECT, ActionState.SURVIVE):
            return
        if inventory.food < FOOD_FORK_MIN:
            return
        self.state = ActionState.FORK

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

    def target_resource(self, inventory: Inventory) -> Optional[str]:
        if self.state == ActionState.SURVIVE or inventory.food <= FOOD_SAFE:
            return 'food'
        if self.state == ActionState.COLLECT:
            missing = inventory.missing_stones_for(self.level)
            if missing:
                return next(iter(missing))
        return None
