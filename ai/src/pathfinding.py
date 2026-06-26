import math
from typing import Optional

def parse_look(response: str) -> list[list[str]]:
    """Transforme la réponse brute du Look en liste de contenus par tile.

    '[player, food, , linemate]' -> [['player'], ['food'], [], ['linemate']]

    Le séparateur de tile est une virgule (suivie ou non d'un espace).
    Plusieurs objets sur la même tile sont séparés par un espace.
    Tile 0 = tile actuelle du joueur.
    """
    content = response.strip().lstrip('[').rstrip(']')
    tiles = []
    for tile_str in content.split(','):
        tile_str = tile_str.strip()
        tiles.append(tile_str.split() if tile_str else [])
    return tiles


def tile_index_to_coords(index: int) -> tuple[int, int]:
    """Convertit un index de tile du Look en coordonnées relatives (x, y).

    y = nombre de tiles en avant (0 = tile actuelle).
    x = décalage latéral : négatif = gauche, positif = droite.

    Exemple pour un joueur niveau 2 :
        ligne 2 :  (-2,2) (-1,2) (0,2) (1,2) (2,2)   tiles 4..8
        ligne 1 :        (-1,1) (0,1) (1,1)            tiles 1..3
        ligne 0 :              (0,0)                    tile  0
    """
    if index == 0:
        return (0, 0)
    row = int(math.isqrt(index))
    x = (index - row * row) - row
    return (x, row)


def moves_to_tile(tile_index: int) -> list[str]:
    """Retourne la séquence de commandes pour atteindre une tile visible dans Look.

    Le joueur est supposé être sur la tile 0, face à sa direction actuelle.
    Stratégie : avancer y fois pour atteindre la bonne ligne,
    puis tourner et avancer |x| fois latéralement.
    """
    if tile_index == 0:
        return []
    x, y = tile_index_to_coords(tile_index)
    moves: list[str] = ['Forward'] * y
    if x > 0:
        moves.append('Right')
        moves += ['Forward'] * x
    elif x < 0:
        moves.append('Left')
        moves += ['Forward'] * abs(x)
    return moves


def find_nearest(tiles: list[list[str]], resource: str) -> Optional[int]:
    """Retourne l'index de la tile la plus proche contenant resource, ou None.

    La tile 0 (position actuelle) est vérifiée en premier, pour ramasser
    une ressource sur laquelle le bot se trouve déjà avant de se déplacer.
    """
    for i, tile in enumerate(tiles):
        if resource in tile:
            return i
    return None


# Direction du son K (sujet p.6), vue du récepteur orienté « devant » :
#        2  1  8      1 devant, 3 gauche, 7 droite, 5 derrière, 0 = même case
#        3 [moi] 7
#        4  5  6
# Homing : on PIVOTE *et* on AVANCE dans le même cycle pour que chaque beacon donne
# une vraie progression (avant, un simple pivot « gâchait » un beacon sans avancer,
# d'où une convergence deux fois trop lente). On garde un seul Forward pour ne pas
# survoler la case du leader (la direction est réévaluée au beacon suivant).
_BROADCAST_MOVES: dict[int, list[str]] = {
    0: [],                              # même case : on est arrivé
    1: ['Forward'],                     # devant
    2: ['Forward'],                     # devant-gauche
    8: ['Forward'],                     # devant-droite
    3: ['Left', 'Forward'],             # gauche
    4: ['Left', 'Forward'],             # arrière-gauche
    5: ['Left', 'Left', 'Forward'],     # derrière : demi-tour puis avance
    6: ['Right', 'Forward'],            # arrière-droite
    7: ['Right', 'Forward'],            # droite
}


def moves_toward_broadcast(k: int) -> list[str]:
    """Retourne le déplacement (pivot + avance) vers la source du broadcast K.

    À appeler en boucle (en relançant Look/beacon entre chaque appel) pour se
    rapprocher progressivement de l'émetteur, sans survoler sa case.
    """
    return list(_BROADCAST_MOVES.get(k, []))
