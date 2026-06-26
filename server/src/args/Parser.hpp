#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include "Error.hpp"

constexpr int DEFAULT_PORT = 4242;
constexpr int DEFAULT_WIDTH = 10;
constexpr int DEFAULT_HEIGHT = 10;
constexpr int DEFAULT_CLIENTS = 1;
constexpr int DEFAULT_FREQ = 100;
constexpr const char *DEFAULT_TEAM = "team1";

constexpr int PORT_MIN = 1;
constexpr int PORT_MAX = 65535;
constexpr int DIMENSION_MIN = 1;
constexpr int DIMENSION_MAX = 1000;
constexpr int CLIENTS_MIN = 1;
constexpr int CLIENTS_MAX = 1000;
constexpr int FREQ_MIN = 1;
constexpr int FREQ_MAX = 10000;
constexpr std::size_t TEAMS_MIN = 1;
constexpr std::size_t TEAMS_MAX = 100;

struct Config {
    int port = DEFAULT_PORT;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    std::vector<std::string> teams;
    int clients = DEFAULT_CLIENTS;
    int freq = DEFAULT_FREQ;
};

class Parser {
public:
    static Config parse(int argc, char **argv);
    static void printUsage();

private:
    static int parseInt(const std::string &value, const std::string &flag);
    static void validate(const Config &cfg);
};
