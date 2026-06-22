#pragma once

#include <functional>
#include <cstdint>
#include "time/Clock.hpp"

struct Event {
    uint64_t id;
    TimePoint due;
    std::function<void()> run;
    bool cancelled = false;

    bool operator>(const Event &o) const { return due > o.due; }
};
