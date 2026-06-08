#pragma once

#include <array>

enum class Resource {
    Food      = 0,
    Linemate  = 1,
    Deraumere = 2,
    Sibur     = 3,
    Mendiane  = 4,
    Phiras    = 5,
    Thystame  = 6,
};

constexpr int RESOURCE_COUNT = 7;

constexpr double DENSITY[RESOURCE_COUNT] = {
    0.5,   // Food
    0.3,   // Linemate
    0.15,  // Deraumere
    0.1,   // Sibur
    0.1,   // Mendiane
    0.08,  // Phiras
    0.05,  // Thystame
};

constexpr const char *RESOURCE_NAMES[RESOURCE_COUNT] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
};

constexpr int FOOD_LIFE_UNITS    = 126;
constexpr int INITIAL_FOOD_UNITS = 1260;

enum class Orientation {
    North = 1,
    East  = 2,
    South = 3,
    West  = 4,
};

using Inventory = std::array<int, RESOURCE_COUNT>;

struct Pos {
    int x;
    int y;

    bool operator==(const Pos &o) const { return x == o.x && y == o.y; }
    bool operator!=(const Pos &o) const { return !(*this == o); }
};
