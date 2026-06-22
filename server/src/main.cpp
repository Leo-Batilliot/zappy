#include <iostream>
#include <cstring>
#include <csignal>
#include "args/Parser.hpp"
#include "server/Server.hpp"
#include "Error.hpp"

static bool wantsHelp(int argc, char **argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--help") == 0)
            return true;
    return false;
}

int main(int argc, char **argv) {
    if (wantsHelp(argc, argv)) {
        Parser::printUsage();
        return 0;
    }
    signal(SIGPIPE, SIG_IGN);
    try {
        const Config cfg = Parser::parse(argc, argv);
        Server server(cfg);
        server.run();
    } catch (const Error &e) {
        std::cerr << e.what() << std::endl;
        return 84;
    }
    return 0;
}
