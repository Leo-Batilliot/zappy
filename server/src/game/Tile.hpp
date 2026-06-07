#pragma once

#include <vector>
#include "game/Types.hpp"

struct Tile {
    Inventory       resources{};
    std::vector<int> players;
    std::vector<int> eggs;
};
