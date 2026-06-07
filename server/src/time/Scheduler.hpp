#pragma once

#include <queue>
#include <vector>
#include <unordered_set>
#include <functional>
#include <optional>
#include <cstdint>
#include "time/Event.hpp"

class Scheduler {
public:
    uint64_t schedule(TimePoint due, std::function<void()> fn);
    void     cancel(uint64_t id);

    std::optional<TimePoint> nextDeadline();
    std::optional<Event>     popReady(TimePoint now);

private:
    void pruneTop();

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> _heap;
    std::unordered_set<uint64_t> _cancelled;
    uint64_t _nextId = 1;
};
