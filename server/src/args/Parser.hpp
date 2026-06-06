#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <string>
#include <vector>
#include "Error.hpp"

struct Config {
    int port = 0;
    int width = 0;
    int height = 0;
    std::vector<std::string> teams;
    int clients = 0;
    int freq = 100;
};

class Parser {
public:
    static Config parse(int argc, char **argv);
    static void printUsage();

private:
    static int parseInt(const std::string &value, const std::string &flag);
    static void validate(const Config &cfg);
};
