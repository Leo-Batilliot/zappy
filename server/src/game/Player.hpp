#pragma once

#include <string>
#include <deque>
#include <cstdint>
#include "game/Types.hpp"

struct Player {
    enum class State { Idle, Busy, Incanting, Dead };

    int         id;
    std::string teamName;
    int         connFd  = -1;
    Pos         pos{};
    Orientation orient  = Orientation::North;
    int         level   = 1;
    Inventory   inventory{};
    State       state   = State::Idle;

    std::deque<std::string> cmdQueue;
    uint64_t currentEventId = 0;
    uint64_t foodEventId    = 0;
};
