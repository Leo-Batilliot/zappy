#pragma once

#include <string>
#include <vector>

struct Team {
    std::string name;
    int freeSlots = 0;
    std::vector<int> playerIds;
    std::vector<int> eggIds;
};
