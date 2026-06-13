import sys
import argparse
from bot import ZappyBot

def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-p", type=int, required=True)
    parser.add_argument("-n", type=str, required=True)
    parser.add_argument("-h", type=str, default="127.0.0.1")
    parser.add_argument("-help", action="store_true")

    args, _ = parser.parse_known_args()

    if args.help:
        print(f"USAGE: {sys.argv[0]} -p port -n name -h machine")
        return 0

    bot = ZappyBot(host=args.h, port=args.p, team_name=args.n)
    bot.run()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())