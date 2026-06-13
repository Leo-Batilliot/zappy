import socket
import sys
import select

class NetworkClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.buffer = ""

    def connect(self):
        try:
            self.sock.connect((self.host, self.port))
        except ConnectionRefusedError:
            print(f"Erreur : Impossible de se connecter à {self.host}:{self.port}")
            sys.exit(84)

    def send_line(self, text: str):
        message = f"{text}\n"
        self.sock.sendall(message.encode('utf-8'))

    def receive_line_blocking(self) -> str:
        data = b""
        while not data.endswith(b"\n"):
            chunk = self.sock.recv(1)
            if not chunk:
                print("Erreur : Connexion fermée par le serveur.")
                sys.exit(84)
            data += chunk
        return data.decode('utf-8').strip()
    
    def receive_lines(self, timeout: float = 0.05) -> list:
        """
        Lit les données disponibles sur le socket sans bloquer.
        Retourne une liste contenant toutes les lignes complètes reçues.
        """
        r, _, _ = select.select([self.sock], [], [], timeout)
        
        if self.sock in r:
            try:
                chunk = self.sock.recv(4096).decode('utf-8')
                if not chunk:
                    print("[Réseau] Connexion fermée par le serveur.")
                    sys.exit(0)
                self.buffer += chunk
            except Exception as e:
                print(f"[Réseau] Erreur : {e}")
                sys.exit(84)
        lines = []
        while '\n' in self.buffer:
            line, self.buffer = self.buffer.split('\n', 1)
            if line.strip():
                lines.append(line.strip())
        return lines

    def close(self):
        self.sock.close()