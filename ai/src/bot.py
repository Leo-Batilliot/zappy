import os
import sys
import time

from network import NetworkClient
import pathfinding
from strategy import Strategy, FOOD_HUNGRY, FOOD_PANIC
from state import Inventory, ActionState, ELEVATION_REQUIREMENTS, STONES

DEBUG = bool(os.environ.get("ZAPPY_DEBUG"))


def dbg(*args):
    if DEBUG:
        print(*args)


class ZappyBot:
    """Bot Zappy : boucle réseau, cadencement des commandes et exécution de la stratégie."""

    def __init__(self, host: str, port: int, team_name: str):
        self.team_name = team_name
        self.network = NetworkClient(host, port)
        self.client_num = 0
        self.map_size = (0, 0)

        self.pending_commands = 0
        self.command_queue = []
        self.sent_commands = []

        self.inventory = Inventory()
        self.strategy = Strategy(team_name)
        self.last_players_on_tile = 1
        self.bcast_tick = 0
        self.awaiting_incant_result = False

    # --------------------------------------------------------------- cycle de vie
    def run(self):
        """Connexion, handshake, puis boucle de jeu jusqu'à la fin."""
        print(f"[IA] Démarrage du bot pour l'équipe {self.team_name}...")
        self.network.connect()
        self._handshake()
        self._game_loop()
        self.network.close()

    def _handshake(self):
        welcome = self.network.receive_line_blocking()
        if welcome != "WELCOME":
            print(f"Erreur Handshake : 'WELCOME' attendu, reçu '{welcome}'")
            sys.exit(84)

        self.network.send_line(self.team_name)
        try:
            self.client_num = int(self.network.receive_line_blocking())
            x_str, y_str = self.network.receive_line_blocking().split()
            self.map_size = (int(x_str), int(y_str))
            print(f"[IA] Connecté ! Places: {self.client_num}, Map: {self.map_size}")
        except ValueError:
            print("Erreur de parsing lors du handshake.")
            sys.exit(84)

    def _game_loop(self):
        print("[IA] Début de la boucle de jeu...")
        self._queue_command("Inventory")
        now = time.time()
        last_activity = now
        last_beat = now
        watchdog = float(os.environ.get("ZAPPY_WATCHDOG_SEC", "20"))
        while True:
            lines = self.network.receive_lines()
            for line in lines:
                self._handle_server_message(line)
            self._send_pending_commands()
            now = time.time()
            if lines:
                last_activity = now
            elif (not self.awaiting_incant_result
                  and "incantation" not in self.sent_commands
                  and now - last_activity > watchdog):
                print("[IA] [watchdog] Blocage détecté, resynchronisation.")
                self.command_queue.clear()
                self.sent_commands.clear()
                self.pending_commands = 0
                self._queue_command("Inventory")
                last_activity = now
            if os.environ.get("ZAPPY_HEARTBEAT") and now - last_beat > 2:
                last_beat = now
                print(f"[HB idx={self.strategy.idx} lvl={self.strategy.level} "
                      f"st={self.strategy.state.name} pend={self.pending_commands} "
                      f"q={len(self.command_queue)} sent={self.sent_commands} "
                      f"food={self.inventory.food}]", flush=True)

    # ------------------------------------------------------------------- réseau
    def _queue_command(self, command: str):
        self.command_queue.append(command)

    def _send_pending_commands(self):
        """Envoie les commandes en file, une seule en vol à la fois."""
        while self.command_queue and self.pending_commands < 1:
            cmd = self.command_queue.pop(0)
            self.network.send_line(cmd)
            self.pending_commands += 1
            self.sent_commands.append(cmd.split()[0].lower())
            dbg(f"[OUT] -> {cmd}")

    # --------------------------------------------------------------- décisions
    def _decide_next_move(self, tiles):
        """Choisit les commandes à envoyer à partir du dernier Look et de l'état."""
        beacon = self.strategy.beacon()
        if beacon:
            self.bcast_tick += 1
            send = (beacon[0] in ("Z", "G") and self.bcast_tick % 3 == 0) \
                   or self.bcast_tick % 7 == 0
            if send:
                self._queue_command(f"Broadcast {beacon}")

        state = self.strategy.state
        players_here = tiles[0].count('player')
        dbg(f"[STRAT] idx={self.strategy.idx} état={state.name} "
            f"niv={self.strategy.level} leader={self.strategy.leader_idx} "
            f"dir={self.strategy.gather_direction} ici={players_here} "
            f"food={self.inventory.food}")

        if state == ActionState.DONE:
            if 'food' in tiles[0]:
                self._queue_command("Take food")
            self._queue_command("Inventory")
            return

        if state == ActionState.INCANT:
            self._trigger_incantation()
            return

        if state == ActionState.LEAD:
            self.last_players_on_tile = players_here
            tile_ready = self._set_satisfied(tiles[0], include_inv=True)
            if not self.strategy.deposited:
                self._drop_full_set()
                self.strategy.deposited = True
            self.strategy.on_ready(players_here, tile_ready)
            if self.strategy.state == ActionState.INCANT:
                print(f"[IA] [LEADER] {players_here} joueurs, set prêt : INCANTATION.")
                self._trigger_incantation()
                return
            if 'food' in tiles[0]:
                self._queue_command("Take food")
            self._queue_command("Inventory")
            return

        if state == ActionState.FOLLOW:
            self._grab_underfoot(tiles[0])
            if self.inventory.food <= FOOD_PANIC:
                fidx = pathfinding.find_nearest(tiles, 'food')
                if fidx:
                    for move in pathfinding.moves_to_tile(fidx):
                        self._queue_command(move)
                    self._queue_command("Take food")
                    self._queue_command("Inventory")
                    return
            direction = self.strategy.gather_direction
            if direction:
                for move in pathfinding.moves_toward_broadcast(direction):
                    self._queue_command(move)
            self._queue_command("Inventory")
            return

        if state == ActionState.WAIT_TILE:
            self._drop_full_set()
            if self.strategy.phase != "G":
                if self._fetch_missing_for_tile(tiles):
                    return
            if 'food' in tiles[0]:
                self._queue_command("Take food")
            elif self.inventory.food <= FOOD_PANIC:
                fidx = pathfinding.find_nearest(tiles, 'food')
                if fidx:
                    for move in pathfinding.moves_to_tile(fidx):
                        self._queue_command(move)
                    self._queue_command("Take food")
            self._queue_command("Inventory")
            return

        self._grab_underfoot(tiles[0])
        target = self.strategy.target_resource(self.inventory)
        if target:
            idx = pathfinding.find_nearest(tiles, target)
            if idx is not None:
                for move in pathfinding.moves_to_tile(idx):
                    self._queue_command(move)
                self._queue_command(f"Take {target}")
            else:
                self._queue_command("Forward")
        else:
            if tiles[0].count('player') >= 2:
                pass
            else:
                crowd = self._find_crowd(tiles)
                if crowd is not None:
                    for move in pathfinding.moves_to_tile(crowd):
                        self._queue_command(move)
                else:
                    self._queue_command("Forward")
        self._queue_command("Inventory")

    def _find_crowd(self, tiles):
        """Index de la tuile visible (hors la sienne) rassemblant le PLUS de joueurs
        (>= 2 = attroupement, potentiel rituel). None si aucun attroupement."""
        best_idx, best_n = None, 1
        for i, tile in enumerate(tiles):
            if i == 0:
                continue
            n = tile.count('player')
            if n > best_n:
                best_n, best_idx = n, i
        return best_idx

    def _drop_full_set(self):
        """Pose toutes les pierres de l'inventaire sur la case (mise en commun)."""
        for stone in STONES:
            count = getattr(self.inventory, stone)
            for _ in range(count):
                self._queue_command(f"Set {stone}")
            setattr(self.inventory, stone, 0)

    def _required_pairs(self):
        """[(pierre, quantité requise), ...] pour le palier courant (0 ignorés)."""
        req = ELEVATION_REQUIREMENTS.get(self.strategy.level)
        if not req:
            return []
        _, *needed = req
        return [(s, n) for s, n in zip(STONES, needed) if n > 0]

    def _set_satisfied(self, tile0, include_inv: bool) -> bool:
        """Le set complet du palier est-il présent sur la case (+ inventaire) ?"""
        for stone, need in self._required_pairs():
            have = tile0.count(stone)
            if include_inv:
                have += getattr(self.inventory, stone)
            if have < need:
                return False
        return True

    def _grab_underfoot(self, tile0):
        """Ramasse la food et les pierres utiles présentes sous les pieds."""
        if 'food' in tile0:
            self._queue_command("Take food")
        missing = self.inventory.missing_stones_for(self.strategy.level)
        for stone in missing:
            if stone in tile0:
                self._queue_command(f"Take {stone}")

    def _fetch_missing_for_tile(self, tiles) -> bool:
        """Va chercher une pierre requise encore absente de la case (rattrapage des
        trous du split par rang). True si une action a été engagée (le bot quitte
        brièvement le rendez-vous, uniquement tant que le set n'est pas complet)."""
        tile0 = tiles[0]
        for stone, need in self._required_pairs():
            if tile0.count(stone) >= need:
                continue
            idx = pathfinding.find_nearest(tiles, stone)
            if idx:
                for move in pathfinding.moves_to_tile(idx):
                    self._queue_command(move)
                self._queue_command(f"Take {stone}")
                self._queue_command("Inventory")
                return True
        return False

    def _trigger_incantation(self):
        """Dépose les pierres requises puis lance l'incantation."""
        req = ELEVATION_REQUIREMENTS.get(self.strategy.level)
        if not req:
            return
        self.command_queue.clear()
        _, *needed = req
        for name, count in zip(STONES, needed):
            have = getattr(self.inventory, name)
            for _ in range(min(count, have)):
                self._queue_command(f"Set {name}")
        self._queue_command("Incantation")

    # ----------------------------------------------------- routage des messages
    def _handle_server_message(self, message: str):
        """Route un message serveur : événement asynchrone ou réponse à une commande."""
        dbg(f"[IN]  <- {message}")

        if self.awaiting_incant_result and (
                message.startswith("Current level") or message == "ko"):
            self.awaiting_incant_result = False
            self.command_queue.clear()
            self.sent_commands.clear()
            self.pending_commands = 0
            if message.startswith("Current level"):
                print(f"[IA] [FOLLOWER] Niveau atteint : {message}")
                self.strategy.on_level_up()
            else:
                print("[IA] [FOLLOWER] Incantation ratée, resynchronisation.")
                self.strategy.on_incant_failed()
            self._queue_command("Inventory")
            return

        if self._is_async_event(message):
            self._handle_async_event(message)
            return

        if self.pending_commands > 0:
            self.pending_commands -= 1
        if not self.sent_commands:
            print(f"[Alerte] Message inattendu (aucune commande en attente) : {message}")
            return

        expected = self.sent_commands.pop(0)
        self._dispatch_command_response(expected, message)

    def _is_async_event(self, message: str) -> bool:
        if message == "dead" or message.startswith("message") or message.startswith("eject:"):
            return True
        if message == "Elevation underway" or message.startswith("Current level"):
            return "incantation" not in self.sent_commands
        return False

    def _handle_async_event(self, message: str):
        if message == "dead":
            print("[IA] Le drone est mort de faim. Fin.")
            sys.exit(0)

        elif message.startswith("eject:"):
            print("[IA] Éjecté ! Je me resynchronise.")
            self.awaiting_incant_result = False
            self.command_queue.clear()
            self.strategy.state = ActionState.COLLECT
            self.strategy._reset_election()
            self.strategy.deposited = False
            self._queue_command("Inventory")

        elif message == "Elevation underway":
            print("[IA] [FOLLOWER] Rituel amorcé sur ma case, j'attends.")
            self.awaiting_incant_result = True

        elif message.startswith("Current level"):
            self.awaiting_incant_result = False
            self.command_queue.clear()
            self.sent_commands.clear()
            self.pending_commands = 0
            print(f"[IA] [FOLLOWER] Niveau atteint : {message}")
            self.strategy.on_level_up()
            self._queue_command("Inventory")

        elif message.startswith("message"):
            self._parse_broadcast(message)

    def _parse_broadcast(self, message: str):
        content = message[len("message"):]
        parts = content.split(",", 1)
        if len(parts) == 2:
            try:
                direction = int(parts[0].strip())
                text = parts[1].strip()
                self.strategy.on_broadcast_received(direction, text)
            except ValueError:
                pass

    # ------------------------------------------------- réponses synchrones
    def _dispatch_command_response(self, command: str, message: str):
        if command == "inventory":
            self._handle_inventory_response(message)
        elif command == "look":
            self._decide_next_move(pathfinding.parse_look(message))
        elif command == "take":
            pass
        elif command == "incantation":
            self._handle_incantation_response(message)
        elif command == "connect_nbr":
            self._handle_connect_nbr_response(message)
        elif command == "fork":
            self._handle_fork_response(message)
        elif command == "set":
            pass

    def _handle_inventory_response(self, message: str):
        self._update_inventory_from_string(message)
        self.strategy.update(self.inventory)
        forkable = (self.strategy.state in (ActionState.COLLECT, ActionState.SURVIVE)
                    or self.strategy.is_progress_stalled())
        if forkable and self.strategy.cycle % 12 == 1:
            self._queue_command("Connect_nbr")
        else:
            self._queue_command("Look")

    def _update_inventory_from_string(self, message: str):
        clean = message.strip(" []")
        if not clean:
            return
        for item in clean.split(","):
            parts = item.strip().split()
            if len(parts) == 2 and hasattr(self.inventory, parts[0]):
                setattr(self.inventory, parts[0], int(parts[1]))

    def _handle_connect_nbr_response(self, message: str):
        try:
            slots = int(message)
            self.client_num = slots
        except ValueError:
            slots = 0
        if self.strategy.want_fork(slots):
            print(f"[IA] [LEADER] Ponte (équipe={self.strategy.team_size()}/6).")
            self._queue_command("Fork")
        else:
            self._queue_command("Look")

    def _handle_incantation_response(self, message: str):
        if message == "ko":
            print("[IA] Incantation ko : on garde les pierres et on réessaie.")
            self.strategy.on_incant_failed()
            self._queue_command("Inventory")
        elif message == "Elevation underway":
            print("[IA] Rituel amorcé... attente de la résolution serveur.")
            self.sent_commands.insert(0, "incantation")
            self.pending_commands += 1
        elif message.startswith("Current level"):
            print(f"[IA] RITUEL RÉUSSI ! {message}")
            self.strategy.on_level_up()
            self._queue_command("Inventory")

    def _handle_fork_response(self, message: str):
        if message == "ok":
            print("[IA] Ponte validée. Signal de spawn au launcher.")
            self._launch_clone()
        else:
            print("[IA] Échec de la ponte (ko).")
        self._queue_command("Inventory")

    def _launch_clone(self):
        print("##FORK##", flush=True)
