class BotState:
    def __init__(self):
        self.inventory = {
            "food": 0,
            "linemate": 0,
            "deraumere": 0,
            "sibur": 0,
            "mendiane": 0,
            "phiras": 0,
            "thystame": 0
        }
        self.vision = [] # La vision est une liste de listes (chaque sous-liste est une tuile)

    def parse_inventory(self, message: str):
        """
        Transforme l'inventaire en dictionnaire.
        """
        clean_msg = message.strip(" []")
        if not clean_msg:
            return
        items = clean_msg.split(",")
        for item in items:
            parts = item.strip().split()
            if len(parts) == 2:
                resource_name = parts[0]
                amount = int(parts[1])
                self.inventory[resource_name] = amount

    def parse_look(self, message: str):
        """
        Transforme look en liste de tuiles.
        """
        clean_msg = message.strip(" []")
        tiles_raw = clean_msg.split(",")
        self.vision = []
        for tile in tiles_raw:
            entities = tile.strip().split()
            self.vision.append(entities)