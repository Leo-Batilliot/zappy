#!/usr/bin/env python3
"""
Script de lancement automatique des bots.

Usage : ./zappy_team -p PORT -n TEAM [-h HOST]

Lance 2 bots initiaux, puis surveille leur stdout.
Quand un bot envoie ##FORK## (après une commande Fork réussie),
le launcher spawn automatiquement un nouveau bot pour occuper l'oeuf.

Pour tester manuellement sans spawn auto : lancer ./zappy_ai directement.
"""

import subprocess
import sys
import threading
import argparse
import time
import os

FORK_SIGNAL = "##FORK##"
INITIAL_BOTS = 6


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-p", type=int, required=True)
    parser.add_argument("-n", type=str, required=True)
    parser.add_argument("-h", type=str, default="localhost")
    parser.add_argument("-help", action="store_true")
    args, _ = parser.parse_known_args()

    if args.help:
        print(f"USAGE: {sys.argv[0]} -p port -n name [-h machine]")
        return 0

    ai_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "zappy_ai")
    ai_path = os.path.normpath(ai_path)
    bot_cmd = [ai_path, "-p", str(args.p), "-n", args.n, "-h", args.h]

    if not os.path.isfile(ai_path):
        print(f"[LAUNCHER] Erreur : {ai_path} introuvable. Lance 'make' d'abord.")
        return 84

    spawn_lock = threading.Lock()

    def spawn_bot():
        with spawn_lock:
            try:
                proc = subprocess.Popen(
                    bot_cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    bufsize=1,
                )
                t = threading.Thread(target=monitor, args=(proc,), daemon=True)
                t.start()
                print(f"[LAUNCHER] Bot lancé (PID {proc.pid})", flush=True)
            except Exception as e:
                print(f"[LAUNCHER] Erreur spawn : {e}", flush=True)

    def monitor(proc):
        for line in proc.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
            if line.strip() == FORK_SIGNAL:
                spawn_bot()

    for _ in range(INITIAL_BOTS):
        spawn_bot()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[LAUNCHER] Arrêt.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
