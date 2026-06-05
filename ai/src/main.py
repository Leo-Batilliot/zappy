import sys


def print_usage() -> None:
    print(f"USAGE: {sys.argv[0]} -p port -n name -h machine")


def main() -> int:
    if len(sys.argv) >= 2 and sys.argv[1] == "-help":
        print_usage()
        return 0
    print(f"zappy_ai: bootstrap OK ({len(sys.argv) - 1} argument(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
