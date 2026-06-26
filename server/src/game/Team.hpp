#pragma once

#include <string>
#include <vector>

class Team {
public:
    const std::string &name() const { return _name; }
    void setName(const std::string &name) { _name = name; }

    int freeSlots() const { return _freeSlots; }
    void setFreeSlots(int freeSlots) { _freeSlots = freeSlots; }

    std::vector<int> &playerIds() { return _playerIds; }
    const std::vector<int> &playerIds() const { return _playerIds; }

    std::vector<int> &eggIds() { return _eggIds; }
    const std::vector<int> &eggIds() const { return _eggIds; }

private:
    std::string _name;
    int _freeSlots = 0;
    std::vector<int> _playerIds;
    std::vector<int> _eggIds;
};
