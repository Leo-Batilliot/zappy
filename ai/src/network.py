import socket
import sys

class NetworkClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self):
        try:
            self.sock.connect((self.host, self.port))
        except ConnectionRefusedError:
            print(f"Erreur : Impossible de se connecter à {self.host}:{self.port}")
            sys.exit(84)

    def send_line(self, text: str):
        message = f"{text}\n"
        self.sock.sendall(message.encode('utf-8'))

    def receive_line(self) -> str:
        data = b""
        while not data.endswith(b"\n"):
            chunk = self.sock.recv(1)
            if not chunk:
                print("Erreur : Connexion fermée par le serveur.")
                sys.exit(84)
            data += chunk
        return data.decode('utf-8').strip()

    def close(self):
        self.sock.close()