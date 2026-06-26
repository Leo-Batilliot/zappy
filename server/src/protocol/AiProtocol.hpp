#pragma once

#include <string>
#include <vector>
#include "game/Player.hpp"
#include "game/Types.hpp"

struct AiProtocol {
    static std::string ok() { return "ok\n"; }
    static std::string ko() { return "ko\n"; }
    static std::string dead() { return "dead\n"; }

    static std::string look(const std::vector<std::string> &tiles)
    {
        std::string out = "[";
        for (std::size_t i = 0; i < tiles.size(); ++i) {
            if (i)
                out += ",";
            if (!tiles[i].empty())
                out += " " + tiles[i];
        }
        out += " ]\n";
        return out;
    }

    static std::string inventory(const Player &p)
    {
        std::string out = "[";
        for (int i = 0; i < RESOURCE_COUNT; ++i) {
            if (i)
                out += ",";
            out += " ";
            out += resourceName(i);
            out += " ";
            out += std::to_string(p.inventory()[i]);
        }
        out += " ]\n";
        return out;
    }

    static std::string connectNbr(int n) { return std::to_string(n) + "\n"; }
    static std::string elevationUnderway() { return "Elevation underway\n"; }
    static std::string currentLevel(int k) { return "Current level: " + std::to_string(k) + "\n"; }

    static std::string message(int k, const std::string &text)
    {
        return "message " + std::to_string(k) + ", " + text + "\n";
    }

    static std::string eject(int k) { return "eject: " + std::to_string(k) + "\n"; }
};
