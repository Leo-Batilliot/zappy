#pragma once

#include <vector>
#include "game/Types.hpp"

class Tile {
public:
    Inventory &resources() { return _resources; }
    const Inventory &resources() const { return _resources; }

    std::vector<int> &players() { return _players; }
    const std::vector<int> &players() const { return _players; }

    std::vector<int> &eggs() { return _eggs; }
    const std::vector<int> &eggs() const { return _eggs; }

private:
    Inventory _resources{};
    std::vector<int> _players;
    std::vector<int> _eggs;
};
