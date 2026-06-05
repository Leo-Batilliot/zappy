#include <iostream>
#include <string>

static void print_usage(const char *bin)
{
    std::cout << "USAGE: " << bin
              << " -p port -x width -y height -n name1 name2 ..."
              << " -c clientsNb -f freq\n";
}

int main(int argc, char **argv)
{
    if (argc >= 2 && std::string(argv[1]) == "-help") {
        print_usage(argv[0]);
        return 0;
    }
    std::cout << "zappy_server: bootstrap OK (" << argc - 1
              << " argument(s))\n";
    return 0;
}
