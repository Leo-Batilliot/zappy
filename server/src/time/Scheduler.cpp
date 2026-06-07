#include "time/Scheduler.hpp"

uint64_t Scheduler::schedule(TimePoint due, std::function<void()> fn)
{
    uint64_t id = _nextId++;
    _heap.push(Event{id, due, std::move(fn), false});
    return id;
}

void Scheduler::cancel(uint64_t id)
{
    _cancelled.insert(id);
}

void Scheduler::pruneTop()
{
    while (!_heap.empty() && _cancelled.count(_heap.top().id)) {
        _cancelled.erase(_heap.top().id);
        _heap.pop();
    }
}

std::optional<TimePoint> Scheduler::nextDeadline()
{
    pruneTop();
    if (_heap.empty())
        return std::nullopt;
    return _heap.top().due;
}

std::optional<Event> Scheduler::popReady(TimePoint now)
{
    pruneTop();
    if (_heap.empty() || _heap.top().due > now)
        return std::nullopt;
    Event ev = _heap.top();
    _heap.pop();
    return ev;
}
