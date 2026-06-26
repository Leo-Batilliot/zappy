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
        << "\t-p port\t\tport number (" << PORT_MIN << ".." << PORT_MAX
        << ", default " << DEFAULT_PORT << ")\n"
        << "\t-x width\tworld width (" << DIMENSION_MIN << ".." << DIMENSION_MAX
        << ", default " << DEFAULT_WIDTH << ")\n"
        << "\t-y height\tworld height (" << DIMENSION_MIN << ".." << DIMENSION_MAX
        << ", default " << DEFAULT_HEIGHT << ")\n"
        << "\t-n name1 ...\tteam names (1.." << TEAMS_MAX
        << " teams, default " << DEFAULT_TEAM << ")\n"
        << "\t-c clientsNb\tnumber of clients per team (" << CLIENTS_MIN << ".."
        << CLIENTS_MAX << ", default " << DEFAULT_CLIENTS << ")\n"
        << "\t-f freq\t\treciprocal of time unit (" << FREQ_MIN << ".." << FREQ_MAX
        << ", default " << DEFAULT_FREQ << ")\n";
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

static void checkRange(int value, int min, int max, const std::string &name)
{
    if (value < min || value > max)
        throw ArgError(name + " must be in range " + std::to_string(min)
            + ".." + std::to_string(max) + " (got " + std::to_string(value) + ")");
}

void Parser::validate(const Config &cfg)
{
    checkRange(cfg.port, PORT_MIN, PORT_MAX, "port");
    checkRange(cfg.width, DIMENSION_MIN, DIMENSION_MAX, "width");
    checkRange(cfg.height, DIMENSION_MIN, DIMENSION_MAX, "height");
    checkRange(cfg.clients, CLIENTS_MIN, CLIENTS_MAX, "clientsNb");
    checkRange(cfg.freq, FREQ_MIN, FREQ_MAX, "freq");

    if (cfg.teams.size() < TEAMS_MIN || cfg.teams.size() > TEAMS_MAX)
        throw ArgError("number of teams must be in range "
            + std::to_string(TEAMS_MIN) + ".." + std::to_string(TEAMS_MAX)
            + " (got " + std::to_string(cfg.teams.size()) + ")");

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
    }

    if (cfg.teams.empty())
        cfg.teams.push_back(DEFAULT_TEAM);

    validate(cfg);
    return cfg;
}
