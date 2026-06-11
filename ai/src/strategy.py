from typing import Optional
from state import Inventory, ActionState

FOOD_CRITICAL = 10
FOOD_SAFE = 20

class Strategy:
    """Machine à états qui décide de l'objectif courant du bot."""

    def __init__(self):
        self.state = ActionState.SURVIVE
        self.level = 1

    def update(self, inventory: Inventory) -> None:
        if inventory.food <= FOOD_CRITICAL:
            self.state = ActionState.SURVIVE
            return

        if self.state == ActionState.SURVIVE and inventory.food >= FOOD_SAFE:
            self.state = ActionState.COLLECT

        if self.state == ActionState.COLLECT and inventory.has_stones_for(self.level):
            self.state = ActionState.INCANTATE

        if self.state == ActionState.INCANTATE:
            self.state = ActionState.COLLECT

    def on_level_up(self) -> None:
        self.level += 1
        self.state = ActionState.COLLECT

    def target_resource(self, inventory: Inventory) -> Optional[str]:
        if self.state == ActionState.SURVIVE or inventory.food <= FOOD_SAFE:
            return 'food'
        if self.state == ActionState.COLLECT:
            missing = inventory.missing_stones_for(self.level)
            if missing:
                return next(iter(missing))
        return None