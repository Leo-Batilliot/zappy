#!/usr/bin/env python3
"""Lancement automatique d'une cohorte de bots (binaire zappy_team).

Usage : ./zappy_team -p PORT -n TEAM [-h HOST]

Lance INITIAL_BOTS bots et surveille leur stdout. Quand un bot émet ##FORK##
(après un Fork réussi), le launcher spawn un nouveau bot pour occuper l'oeuf.
Pour tester manuellement sans spawn auto : lancer ./zappy_ai directement.
"""

import subprocess
import sys
import threading
import argparse
import time
import os

FORK_SIGNAL = "##FORK##"
INITIAL_BOTS = 1
TEAM_TARGET = 18

_TS = bool(os.environ.get("ZAPPY_TS"))
_T0 = time.monotonic()


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-p", type=int)
    parser.add_argument("-n", type=str)
    parser.add_argument("-h", type=str, default="localhost")
    parser.add_argument("--help", action="store_true")
    args, _ = parser.parse_known_args()

    if args.help or args.p is None or args.n is None:
        print("""USAGE: ./zappy_team -p port -n name [-h machine]
        -p port      port number
        -n name      name of the team
        -h machine   hostname of the server (localhost by default)""")
        return 0 if args.help else 84

    ai_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", "zappy_ai"
    )
    ai_path = os.path.normpath(ai_path)

    if not os.path.isfile(ai_path):
        print(f"[LAUNCHER] Erreur : {ai_path} introuvable. Lance 'make' d'abord.")
        return 84

    spawn_lock = threading.Lock()
    next_index = [0]
    live = []

    def live_count():
        live[:] = [p for p in live if p.poll() is None]
        return len(live)

    def spawn_bot():
        with spawn_lock:
            if live_count() >= TEAM_TARGET:
                return
            idx = next_index[0]
            next_index[0] += 1
            bot_cmd = [ai_path, "-p", str(args.p), "-n", args.n, "-h", args.h]
            child_env = dict(os.environ, PYTHONUNBUFFERED="1")
            try:
                proc = subprocess.Popen(
                    bot_cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    bufsize=1,
                    env=child_env,
                )
                live.append(proc)
                t = threading.Thread(target=monitor, args=(proc,), daemon=True)
                t.start()
                print(f"[LAUNCHER] Bot #{idx} lancé (PID {proc.pid})", flush=True)
            except Exception as e:
                print(f"[LAUNCHER] Erreur spawn : {e}", flush=True)

    def monitor(proc):
        for line in proc.stdout:
            if _TS:
                sys.stdout.write(f"{time.monotonic() - _T0:7.1f}  {line}")
            else:
                sys.stdout.write(line)
            sys.stdout.flush()
            if line.strip() == FORK_SIGNAL:
                spawn_bot()

    for _ in range(INITIAL_BOTS):
        spawn_bot()

    try:
        while True:
            time.sleep(1)
            if next_index[0] > 0 and live_count() == 0:
                print("[LAUNCHER] Tous les bots sont terminés. Arrêt.")
                break
    except KeyboardInterrupt:
        print("\n[LAUNCHER] Arrêt.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
