#include "args/Parser.hpp"
#include "Error.hpp"
#include <iostream>
#include <charconv>
#include <unordered_map>
#include <unordered_set>

void Parser::printUsage()
{
    std::cout
        << "USAGE: ./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq\n"
        << "\t-p port\t\tport number\n"
        << "\t-x width\tworld width\n"
        << "\t-y height\tworld height\n"
        << "\t-n name1 ...\tteam names\n"
        << "\t-c clientsNb\tnumber of clients per team\n"
        << "\t-f freq\t\treciprocal of time unit (default 100)\n";
}

int Parser::parseInt(const std::string &value, const std::string &flag)
{
    int result = 0;
    const char *begin = value.data();
    const char *end = begin + value.size();
    auto [ptr, ec] = std::from_chars(begin, end, result);

    if (ec != std::errc() || ptr != end)
        throw ArgError("invalid integer for " + flag + ": " + value);
    return result;
}

void Parser::validate(const Config &cfg)
{
    if (cfg.port < 1 || cfg.port > 65535)
        throw ArgError("port must be in range 1..65535");
    if (cfg.width <= 0)
        throw ArgError("width must be > 0");
    if (cfg.height <= 0)
        throw ArgError("height must be > 0");
    if (cfg.clients <= 0)
        throw ArgError("clientsNb must be > 0");
    if (cfg.freq <= 0)
        throw ArgError("freq must be > 0");
    if (cfg.teams.empty())
        throw ArgError("at least one team is required");

    std::unordered_set<std::string> unique;
    for (const std::string &team : cfg.teams) {
        if (team == "GRAPHIC")
            throw ArgError("team name \"GRAPHIC\" is reserved");
        if (!unique.insert(team).second)
            throw ArgError("duplicate team name: " + team);
    }
}

Config Parser::parse(int argc, char **argv)
{
    Config cfg;
    const std::unordered_map<std::string, int *> intFlags = {
        {"-p", &cfg.port},
        {"-x", &cfg.width},
        {"-y", &cfg.height},
        {"-c", &cfg.clients},
        {"-f", &cfg.freq},
    };
    std::unordered_set<std::string> seen;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-n") {
            while (i + 1 < argc && argv[i + 1][0] != '-')
                cfg.teams.push_back(argv[++i]);
        } else if (auto flag = intFlags.find(arg); flag != intFlags.end()) {
            if (i + 1 >= argc)
                throw ArgError("missing value for " + arg);
            *flag->second = parseInt(argv[++i], arg);
        } else {
            throw ArgError("unknown argument: " + arg);
        }
        seen.insert(arg);
    }

    const std::unordered_map<std::string, std::string> descriptions = {
        {"-p", "port number"},
        {"-x", "world width"},
        {"-y", "world height"},
        {"-n", "team names"},
        {"-c", "number of clients per team"},
    };

    for (const auto &flag : {"-p", "-x", "-y", "-n", "-c"})
        if (seen.find(flag) == seen.end())
            throw ArgError("missing required argument: " + std::string(flag)
                + " (" + descriptions.at(flag) + ")");

    validate(cfg);
    return cfg;
}
