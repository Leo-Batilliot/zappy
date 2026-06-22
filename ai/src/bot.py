import sys
from network import NetworkClient
import pathfinding
import subprocess
from strategy import Strategy
from state import Inventory, ActionState, ELEVATION_REQUIREMENTS, RESOURCES
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
        # Envoyer le broadcast en attente avant toute décision de mouvement
        msg = self.strategy.pop_broadcast()
        if msg:
            self._queue_command(f"Broadcast {msg}")

        state = self.strategy.state
        print(f"[STRATÉGIE] État : {state.name}, niveau : {self.strategy.level}")

        if state == ActionState.INCANTATE:
            self._trigger_incantation()
            return

        if state == ActionState.WAIT_FOR_TEAM:
            # Leader : re-broadcaster périodiquement et vérifier la nourriture
            self._queue_command(f"Broadcast GATHER_{self.strategy.level}")
            if 'food' in tiles[0]:
                self._queue_command("Take food")
            else:
                self._queue_command("Inventory")
            return

        if state == ActionState.FORK:
            print("[IA] Ponte d'un œuf pour agrandir l'équipe.")
            self._queue_command("Fork")
            self.strategy.state = ActionState.COLLECT
            return

        if state == ActionState.MOVE_TO_GATHER:
            # Follower : avancer d'un pas vers la source du broadcast
            direction = self.strategy.gather_direction
            if direction is not None:
                if direction == 0:
                    stone_names = RESOURCES[1:]
                    for stone in stone_names:
                        count = getattr(self.inventory, stone)
                        for _ in range(count):
                            self._queue_command(f"Set {stone}")
                    self._queue_command("Inventory")
            else:
                moves = pathfinding.moves_toward_broadcast(direction)
                for move in moves:
                    self._queue_command(move)
            self._queue_command("Look")
            return
        
        if state == ActionState.ARRIVED:
            # Le Follower vient d'arriver sur la case. Il doit jeter toutes ses pierres.
            stone_names = RESOURCES[1:]
            dropped_something = False
            
            for stone in stone_names:
                count = getattr(self.inventory, stone)
                for _ in range(count):
                    self._queue_command(f"Set {stone}")
                    dropped_something = True
            if dropped_something:
                for stone in stone_names:
                    setattr(self.inventory, stone, 0)
            self._queue_command("Inventory")
            return
        
        if state == ActionState.SPAWN_ALLY:
            self._launch_clone()
            self.strategy.state = ActionState.WAIT_FOR_TEAM
            self._queue_command("Look")
            return
        
        # États SURVIVE et COLLECT : logique normale de collecte
        target = self.strategy.target_resource(self.inventory)
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

    def _trigger_incantation(self) -> None:
        """Dépose les pierres requises sur la tuile courante puis lance l'incantation."""
        req = ELEVATION_REQUIREMENTS.get(self.strategy.level)
        if not req:
            return
        stone_names = RESOURCES[1:]
        _, *needed = req
        for name, count in zip(stone_names, needed):
            for _ in range(count):
                self._queue_command(f"Set {name}")
        self._queue_command("Incantation")
        self.strategy.state = ActionState.COLLECT

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
            self.command_queue.clear()
            self.strategy.state = ActionState.COLLECT
            self._queue_command("Look")
            
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
                self.strategy.on_broadcast_received(direction, texte)
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
        elif command == "set":
            if message == "ko":
                print("[IA] Échec Set — ressource absente, retour en collecte.")
                self.strategy.state = ActionState.COLLECT
                self._queue_command("Look")
        elif command == "broadcast":
            pass
        elif command in ["forward", "right", "left"]:
            pass

    def _handle_inventory_response(self, message: str) -> None:
        """Met à jour l'état FSM."""
        self._update_inventory_from_string(message)
        self.strategy.update(self.inventory)
        
        if self.strategy.state == ActionState.WAIT_FOR_TEAM:
            self._queue_command("Look")
        else:
            self._queue_command("Connect_nbr")

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
        """Vérifie les slots disponibles, décide du Fork, puis lance le Look."""
        try:
            slots = int(message)
            self.client_num = slots
            print(f"[ÉTAT] Slots libres : {slots}")
            self.strategy.on_connect_nbr(slots, self.inventory)
        except ValueError:
            pass
        self._queue_command("Look")
    
    def _launch_clone(self):
        """Lance le binaire Zappy en tâche de fond."""
        import subprocess
        try:
            subprocess.Popen([
                "./zappy_ai", 
                "-p", str(self.network.port), 
                "-n", self.team_name, 
                "-h", self.network.host
            ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            print("[IA] Nouveau clone lancé en arrière-plan !")
        except Exception as e:
            print(f"[ERREUR] Impossible de lancer le clone : {e}")

    def _handle_fork_response(self, message: str) -> None:
        """Reprend la boucle normale après la ponte et lance le processus."""
        if message == "ok":
            print("[IA] Ponte validée, la place est créée ! Lancement du clone...")
            self._launch_clone()
        else:
            print("[IA] Échec de la ponte (ko).")
        self._queue_command("Inventory")