from enum import Enum, auto
from dataclasses import dataclass
from typing import Optional

RESOURCES = ['food', 'linemate', 'deraumere', 'sibur', 'mendiane', 'phiras', 'thystame']

ELEVATION_REQUIREMENTS: dict[int, tuple[int, ...]] = {
    1: (1, 1, 0, 0, 0, 0, 0),
    2: (2, 1, 1, 1, 0, 0, 0),
    3: (2, 2, 0, 1, 0, 2, 0),
    4: (4, 1, 1, 2, 0, 1, 0),
    5: (4, 1, 2, 1, 3, 0, 0),
    6: (6, 1, 2, 3, 0, 1, 0),
    7: (6, 2, 2, 2, 2, 2, 1),
}

FOOD_CRITICAL = 10
FOOD_SAFE = 20


class BotState(Enum):
    """États de la machine à états (FSM) du bot."""
    SURVIVE = auto()
    COLLECT = auto()
    INCANTATE = auto()
    FORK = auto()


@dataclass
class Inventory:
    """Inventaire courant du bot, tel que renvoyé par la commande Inventory."""
    food: int = 0
    linemate: int = 0
    deraumere: int = 0
    sibur: int = 0
    mendiane: int = 0
    phiras: int = 0
    thystame: int = 0

    def has_stones_for(self, level: int) -> bool:
        """Retourne True si l'inventaire couvre les besoins en pierres pour l'élévation depuis level."""
        req = ELEVATION_REQUIREMENTS.get(level)
        if req is None:
            return False
        _, *needed = req
        actual = [self.linemate, self.deraumere, self.sibur,
                  self.mendiane, self.phiras, self.thystame]
        return all(have >= need for have, need in zip(actual, needed))

    def missing_stones_for(self, level: int) -> dict[str, int]:
        """Retourne un dict {pierre: quantité manquante} pour l'élévation depuis level."""
        req = ELEVATION_REQUIREMENTS.get(level)
        if req is None:
            return {}
        _, *needed = req
        stone_names = RESOURCES[1:]
        actual = [self.linemate, self.deraumere, self.sibur,
                  self.mendiane, self.phiras, self.thystame]
        return {
            name: need - have
            for name, need, have in zip(stone_names, needed, actual)
            if need > have
        }


class Strategy:
    """Machine à états qui décide de l'objectif courant du bot.

    Utilisation :
        s = Strategy()
        s.update(inventory)       # met à jour l'état FSM
        resource = s.target_resource(inventory)  # ressource à chercher
    """

    def __init__(self):
        self.state = BotState.SURVIVE
        self.level = 1

    def update(self, inventory: Inventory) -> None:
        """Met à jour l'état FSM en fonction de l'inventaire courant."""
        if inventory.food <= FOOD_CRITICAL:
            self.state = BotState.SURVIVE
            return

        if self.state == BotState.SURVIVE and inventory.food >= FOOD_SAFE:
            self.state = BotState.COLLECT

        if self.state == BotState.COLLECT and inventory.has_stones_for(self.level):
            self.state = BotState.INCANTATE

        if self.state == BotState.INCANTATE:
            self.state = BotState.COLLECT

    def on_level_up(self) -> None:
        """À appeler quand une incantation réussit pour incrémenter le niveau."""
        self.level += 1
        self.state = BotState.COLLECT

    def target_resource(self, inventory: Inventory) -> Optional[str]:
        """Retourne la ressource à chercher en priorité selon l'état actuel.

        Retourne None si aucune collecte n'est nécessaire (ex: prêt à incanter).
        """
        if self.state == BotState.SURVIVE or inventory.food <= FOOD_SAFE:
            return 'food'
        if self.state == BotState.COLLECT:
            missing = inventory.missing_stones_for(self.level)
            if missing:
                return next(iter(missing))
        return None
