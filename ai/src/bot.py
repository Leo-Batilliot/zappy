import sys
from network import NetworkClient
import pathfinding
from strategy import Strategy
from state import Inventory
from state import ActionState

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

    def _handle_server_message(self, message: str) -> None:
        """
        Point d'entrée de tous les messages provenant du serveur.
        Aiguille le message vers le gestionnaire d'événements asynchrones ou le parseur de commandes.
        """
        print(f"[IN]  <- {message}")

        # 1. Interception prioritaire des événements asynchrones
        if self._is_async_event(message):
            self._handle_async_event(message)
            return
            
        # 2. Gestion de la file d'attente réseau
        if self.pending_commands > 0:
            self.pending_commands -= 1

        if not self.sent_commands:
            print(f"[Alerte] Message inattendu reçu (aucune commande en attente) : {message}")
            return

        # 3. Gestion de la réponse synchrone
        expected_command = self.sent_commands.pop(0)
        self._dispatch_command_response(expected_command, message)

    def _is_async_event(self, message: str) -> bool:
        """Détermine si le message serveur est un événement non sollicité."""
        return message == "dead" or message.startswith("message") or message.startswith("eject:")

    def _handle_async_event(self, message: str) -> None:
        """Traite les événements asynchrones (Mort, Broadcast, Éjection)."""
        if message == "dead":
            print("[IA] Le drone est mort de faim. Fin de la partie.")
            sys.exit(0)
            
        elif message.startswith("eject:"):
            print("[IA] Alerte : Déplacement forcé par une éjection !")
            ####### TO DO : réagir à l'éjection ########
            
        elif message.startswith("message"):
            self._parse_broadcast(message)

    def _parse_broadcast(self, message: str) -> None:
        """Extrait la direction et le texte d'un message broadcasté."""
        content = message[8:]
        parts = content.split(",", 1)
        
        if len(parts) == 2:
            try:
                direction = int(parts[0].strip())
                texte = parts[1].strip()
                print(f"[RADIO] J'entends '{texte}' de la direction {direction}")
                ####### TO DO : self.strategy.on_broadcast_received(direction, texte) ########
            except ValueError:
                pass

    def _dispatch_command_response(self, command: str, message: str) -> None:
        """
        Traite la réponse du serveur en fonction de la commande envoyée précédemment.
        """
        if command == "inventory":
            self._handle_inventory_response(message)
        elif command == "look":
            self._handle_look_response(message)
        elif command == "take":
            self._handle_take_response(message)
        elif command == "incantation":
            self._handle_incantation_response(message)
        elif command == "connect_nbr":
            self._handle_connect_nbr_response(message)
        elif command == "fork":
            self._handle_fork_response(message)
        elif command in ["forward", "right", "left"]:
            pass

    def _handle_inventory_response(self, message: str) -> None:
        """Met à jour l'état FSM et déclenche la prochaine phase d'observation."""
        self._update_inventory_from_string(message)
        self.strategy.update(self.inventory)
        self._queue_command("Look")

    def _handle_look_response(self, message: str) -> None:
        """Transmet le champ de vision à l'algorithme de pathfinding."""
        tiles = pathfinding.parse_look(message)
        self._decide_next_move(tiles)

    def _handle_take_response(self, message: str) -> None:
        """Vérifie le succès de la collecte et ajuste la séquence d'actions."""
        if message == "ok":
            self._queue_command("Inventory")
        else:
            self._queue_command("Look")

    def _handle_incantation_response(self, message: str) -> None:
        """
        Gère le flux complexe du rituel d'élévation, nécessitant l'interception
        de la réponse initiale d'amorçage puis du résultat final asynchrone.
        """
        if message == "ko":
            print("[IA] L'incantation a échoué aux prérequis (ko).")
            from state import ActionState
            self.strategy.state = ActionState.COLLECT 
            self._queue_command("Look")
            
        elif message == "Elevation underway":
            print("[IA] Rituel amorcé... attente de la résolution du serveur.")
            self.sent_commands.insert(0, "incantation")
            self.pending_commands += 1 
            
        elif message.startswith("Current level"):
            print(f"[IA] RITUEL RÉUSSI ! {message}")
            self.strategy.on_level_up()
            self._queue_command("Look")

    def _handle_connect_nbr_response(self, message: str) -> None:
        """Met à jour les informations sur la capacité d'accueil de l'équipe."""
        try:
            self.client_num = int(message)
            print(f"[ÉTAT] Places libres pour l'équipe : {self.client_num}")
        except ValueError:
            pass

    def _handle_fork_response(self, message: str) -> None:
        """Analyse le succès ou l'échec de la ponte d'un œuf."""
        if message == "ok":
            print("[IA] Ponte validée, nouveau slot imminent.")
        else:
            print("[IA] Échec de la ponte (ko).")