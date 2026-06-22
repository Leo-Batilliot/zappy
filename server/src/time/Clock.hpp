#pragma once

#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::duration<double>;

inline Duration secondsFor(int units, int freq)
{
    return Duration(static_cast<double>(units) / static_cast<double>(freq));
}

inline int toMilliseconds(Duration d)
{
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    return static_cast<int>(ms < 0 ? 0 : ms);
}

inline TimePoint deadlineFor(int units, int freq)
{
    return Clock::now() + std::chrono::duration_cast<Clock::duration>(secondsFor(units, freq));
}
