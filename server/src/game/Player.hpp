#pragma once

#include <string>
#include <deque>
#include <cstdint>
#include "game/Types.hpp"

struct Player {
    int id;
    std::string teamName;
    int connFd = -1;
    Pos pos{};
    Orientation orient = Orientation::North;
    int level = 1;
    Inventory inventory{};
    int lifeUnits = INITIAL_FOOD_UNITS;

    enum class State { Idle, Busy, Incanting, Dead };
    State state = State::Idle;

    std::deque<std::string> cmdQueue;
    uint64_t currentEventId = 0;
    uint64_t foodEventId = 0;
};
