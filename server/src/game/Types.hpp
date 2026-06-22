#pragma once

#include <array>

enum class Resource { Food = 0, Linemate, Deraumere, Sibur, Mendiane, Phiras, Thystame };
constexpr int RESOURCE_COUNT = 7;
constexpr double DENSITY[RESOURCE_COUNT] = { 0.5, 0.3, 0.15, 0.1, 0.1, 0.08, 0.05 };
constexpr int FOOD_LIFE_UNITS = 126;
constexpr int INITIAL_FOOD_UNITS = 1260;
constexpr int INITIAL_FOOD_COUNT = 10;

enum class Orientation { North = 1, East = 2, South = 3, West = 4 };

using Inventory = std::array<int, RESOURCE_COUNT>;

struct Pos { int x; int y; };

inline const char *resourceName(int idx)
{
    static const char *names[RESOURCE_COUNT] = {
        "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
    };
    if (idx < 0 || idx >= RESOURCE_COUNT)
        return "";
    return names[idx];
}
