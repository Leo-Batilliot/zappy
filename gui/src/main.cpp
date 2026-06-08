/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** main.cpp
*/

#include <iostream>
#include <string>
#include "App.hpp"

static void print_usage(const char *bin)
{
    std::cerr << "USAGE: " << bin << " -p port -h machine\n";
}

int main(int argc, char **argv)
{
    std::string host;
    std::string port;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if ((arg == "-h" || arg == "--help") && i + 1 >= argc) {
            print_usage(argv[0]);
            return 0;
        }
        if (arg == "-p" && i + 1 < argc)
            port = argv[++i];
        else if (arg == "-h" && i + 1 < argc)
            host = argv[++i];
        else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (host.empty() || port.empty()) {
        print_usage(argv[0]);
        return 84;
    }

    App app(host, port);
    return app.run();
}
