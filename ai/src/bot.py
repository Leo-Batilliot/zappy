import sys
from network import NetworkClient
import pathfinding
from strategy import Strategy
from state import Inventory

class ZappyBot:
    def __init__(self, host: str, port: int, team_name: str):
        self.team_name = team_name
        self.network = NetworkClient(host, port)
        self.client_num = 0
        self.map_size = (0, 0)

        self.pending_commands = 0
        self.command_queue = []

        self.sent_commands = []
        self.inventory = Inventory()
        self.strategy = Strategy()

    def run(self):
        """Lance le bot et effectue la connexion initiale."""
        print(f"[IA] Démarrage du bot pour l'équipe {self.team_name}...")
        self.network.connect()
        self._handshake()
        self._game_loop()
        self.network.close()

    def _handshake(self):
        """
        Exécute la séquence d'initialisation.
        
        Déroulement attendu :
        1. Le serveur envoie "WELCOME\n".
        2. Le client répond avec le nom de son équipe ("<team_name>\n").
        3. Le serveur renvoie le nombre de slots restants pour cette équipe ("<client_num>\n").
        4. Le serveur renvoie les dimensions de la carte ("<x> <y>\n").
        
        En cas de non-respect du protocole par le serveur ou de données invalides,
        le programme quitte avec le code d'erreur 84.
        """
        welcome = self.network.receive_line_blocking()
        if welcome != "WELCOME":
            print(f"Erreur Handshake : 'WELCOME' attendu, reçu '{welcome}'")
            sys.exit(84)
            
        self.network.send_line(self.team_name)
        
        try:
            self.client_num = int(self.network.receive_line_blocking())
            map_str = self.network.receive_line_blocking()
            x_str, y_str = map_str.split()
            self.map_size = (int(x_str), int(y_str))
            
            print(f"[IA] Connecté ! Places: {self.client_num}, Map: {self.map_size}")
        except ValueError:
            print("Erreur de parsing lors du handshake.")
            sys.exit(84)

    def _game_loop(self):
        """Boucle principale asynchrone du jeu."""
        print("[IA] Début de la boucle de jeu...")
        
        self._queue_command("Inventory")

        while True:
            lines = self.network.receive_lines()
            for line in lines:
                self._handle_server_message(line)
            self._send_pending_commands()

    def _queue_command(self, command: str):
        """Ajoute une commande à la file d'attente locale."""
        self.command_queue.append(command)

    def _send_pending_commands(self):
        """Envoie les commandes au serveur sans dépasser la limite de 10."""
        while self.command_queue and self.pending_commands < 10:
            cmd = self.command_queue.pop(0)
            self.network.send_line(cmd)
            self.pending_commands += 1
            command_type = cmd.split()[0].lower()
            self.sent_commands.append(command_type)
            print(f"[OUT] -> {cmd}")

    def _handle_server_message(self, message: str):
        """Parse et dispatche les messages reçus du serveur."""
        print(f"[IN]  <- {message}")
        if message == "dead":
            print("[IA] Le drone est mort de faim. Fin de la partie.")
            sys.exit(0)
        elif message.startswith("message"):
            print("[IA] J'ai entendu un broadcast !")
            return
        elif message.startswith("eject:"):
            print("[IA] On m'a poussé !")
            return
            
        if self.pending_commands > 0:
            self.pending_commands -= 1

        if not self.sent_commands:
            return

        expected_command = self.sent_commands.pop(0)

        if expected_command == "inventory":
            self._update_inventory_from_string(message)
            self.strategy.update(self.inventory)
            self._queue_command("Look")

        elif expected_command == "look":
            tiles = pathfinding.parse_look(message)
            self._decide_next_move(tiles)

        elif expected_command == "take":
            if message == "ok":
                self._queue_command("Inventory")
            else:
                self._queue_command("Look")
                
        elif expected_command in ["forward", "right", "left"]:
            pass
    
    def _update_inventory_from_string(self, message: str):
        """Parse le message et met à jour la dataclass Inventory."""
        clean_msg = message.strip(" []")
        if not clean_msg:
            return
            
        for item in clean_msg.split(","):
            parts = item.strip().split()
            if len(parts) == 2:
                resource_name = parts[0]
                amount = int(parts[1])
                if hasattr(self.inventory, resource_name):
                    setattr(self.inventory, resource_name, amount)

    def _decide_next_move(self, tiles):
        """Demande au cerveau (Strategy) et aux yeux (Pathfinding) quoi faire."""
        target = self.strategy.target_resource(self.inventory)
        print(f"[STRATÉGIE] Objectif actuel : {target}")
        
        if target:
            nearest_index = pathfinding.find_nearest(tiles, target)
            
            if nearest_index is not None:
                moves = pathfinding.moves_to_tile(nearest_index)
                for move in moves:
                    self._queue_command(move)
                self._queue_command(f"Take {target}")
            else:
                self._queue_command("Forward")
                self._queue_command("Look")