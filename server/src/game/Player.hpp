#pragma once

#include <string>
#include <deque>
#include <cstdint>
#include "game/Types.hpp"

class Player {
public:
    enum class State { Idle, Busy, Incanting, Dead };

    int id() const { return _id; }
    void setId(int id) { _id = id; }

    const std::string &teamName() const { return _teamName; }
    void setTeamName(const std::string &teamName) { _teamName = teamName; }

    int connFd() const { return _connFd; }
    void setConnFd(int connFd) { _connFd = connFd; }

    Pos pos() const { return _pos; }
    void setPos(Pos pos) { _pos = pos; }

    Orientation orient() const { return _orient; }
    void setOrient(Orientation orient) { _orient = orient; }

    int level() const { return _level; }
    void setLevel(int level) { _level = level; }

    Inventory &inventory() { return _inventory; }
    const Inventory &inventory() const { return _inventory; }

    int lifeUnits() const { return _lifeUnits; }
    void setLifeUnits(int lifeUnits) { _lifeUnits = lifeUnits; }

    State state() const { return _state; }
    void setState(State state) { _state = state; }

    std::deque<std::string> &cmdQueue() { return _cmdQueue; }
    const std::deque<std::string> &cmdQueue() const { return _cmdQueue; }

    uint64_t currentEventId() const { return _currentEventId; }
    void setCurrentEventId(uint64_t currentEventId) { _currentEventId = currentEventId; }

    uint64_t foodEventId() const { return _foodEventId; }
    void setFoodEventId(uint64_t foodEventId) { _foodEventId = foodEventId; }

private:
    int _id = 0;
    std::string _teamName;
    int _connFd = -1;
    Pos _pos{};
    Orientation _orient = Orientation::North;
    int _level = 1;
    Inventory _inventory{};
    int _lifeUnits = INITIAL_FOOD_UNITS;
    State _state = State::Idle;
    std::deque<std::string> _cmdQueue;
    uint64_t _currentEventId = 0;
    uint64_t _foodEventId = 0;
};
