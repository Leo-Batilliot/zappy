from enum import Enum, auto
from dataclasses import dataclass

RESOURCES = ['food', 'linemate', 'deraumere', 'sibur', 'mendiane', 'phiras', 'thystame']
STONES = RESOURCES[1:]

# (nb_players, linemate, deraumere, sibur, mendiane, phiras, thystame)
ELEVATION_REQUIREMENTS: dict[int, tuple[int, ...]] = {
    1: (1, 1, 0, 0, 0, 0, 0),
    2: (2, 1, 1, 1, 0, 0, 0),
    3: (2, 2, 0, 1, 0, 2, 0),
    4: (4, 1, 1, 2, 0, 1, 0),
    5: (4, 1, 2, 1, 3, 0, 0),
    6: (6, 1, 2, 3, 0, 1, 0),
    7: (6, 2, 2, 2, 2, 2, 1),
}

MAX_LEVEL = 8

# Quantité max d'une pierre requise sur un palier (tous paliers confondus).
STONE_QUOTA: dict[str, int] = {
    stone: max(req[1 + i] for req in ELEVATION_REQUIREMENTS.values())
    for i, stone in enumerate(STONES)
}


class ActionState(Enum):
    """États de la machine à états (FSM) du bot."""
    SURVIVE = auto()      # food critique : ne fait que chercher à manger
    COLLECT = auto()      # rassemble le set de pierres du niveau courant
    LEAD = auto()         # je suis le point de rendez-vous : immobile, je compte
    FOLLOW = auto()       # je converge vers le leader (homing sur beacon)
    WAIT_TILE = auto()    # arrivé sur la case du leader : pierres déposées, j'attends
    INCANT = auto()       # le leader a lancé l'incantation, attente du résultat
    DONE = auto()         # niveau max atteint


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

    def stone_counts(self) -> list[int]:
        return [self.linemate, self.deraumere, self.sibur,
                self.mendiane, self.phiras, self.thystame]

    def has_stones_for(self, level: int) -> bool:
        """True si l'inventaire couvre un set complet de pierres pour ce niveau."""
        req = ELEVATION_REQUIREMENTS.get(level)
        if req is None:
            return False
        _, *needed = req
        return all(have >= need for have, need in zip(self.stone_counts(), needed))

    def missing_stones_for(self, level: int) -> dict[str, int]:
        """{pierre: quantité manquante} pour atteindre un set complet du niveau."""
        req = ELEVATION_REQUIREMENTS.get(level)
        if req is None:
            return {}
        _, *needed = req
        return {
            name: need - have
            for name, need, have in zip(STONES, needed, self.stone_counts())
            if need > have
        }
