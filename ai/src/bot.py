import sys
from network import NetworkClient

class ZappyBot:
    def __init__(self, host: str, port: int, team_name: str):
        self.team_name = team_name
        self.network = NetworkClient(host, port)
        self.client_num = 0
        self.map_size = (0, 0)

    def run(self):
        """Lance le bot et effectue la connexion initiale."""
        print(f"[IA] Démarrage du bot pour l'équipe {self.team_name}...")
        self.network.connect()
        self._handshake()
        #TO DO : _gameloop
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
        welcome = self.network.receive_line()
        if welcome != "WELCOME":
            print(f"Erreur Handshake : 'WELCOME' attendu, reçu '{welcome}'")
            sys.exit(84)
            
        self.network.send_line(self.team_name)
        
        try:
            self.client_num = int(self.network.receive_line())
            map_str = self.network.receive_line()
            x_str, y_str = map_str.split()
            self.map_size = (int(x_str), int(y_str))
            
            print(f"[IA] Connecté ! Places: {self.client_num}, Map: {self.map_size}")
        except ValueError:
            print("Erreur de parsing lors du handshake.")
            sys.exit(84)