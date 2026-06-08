#pragma once

#include <string>
#include "game/Types.hpp"

struct Egg {
    int id;
    std::string team;
    Pos pos{};
    int layerId = -1;
};
