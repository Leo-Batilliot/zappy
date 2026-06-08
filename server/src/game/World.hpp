#pragma once

#include <vector>
#include "game/Types.hpp"
#include "game/Tile.hpp"

class World {
public:
    World() = default;
    World(int width, int height);

    int width = 0;
    int height = 0;
    std::vector<Tile> grid;

    Tile &at(int x, int y);
    const Tile &at(int x, int y) const;

    Pos wrap(int x, int y) const;
    Pos step(Pos pos, Orientation orient, int n = 1) const;
    Pos shortestDelta(Pos from, Pos to) const;
    std::vector<Pos> visibleTiles(Pos pos, Orientation orient, int level) const;

    void placePlayer(int id, Pos pos);
    void removePlayer(int id, Pos pos);
    void movePlayer(int id, Pos from, Pos to);
    void placeEgg(int id, Pos pos);
    void removeEgg(int id, Pos pos);

    void initResources(int seed = 0);
    std::vector<Pos> refill();
};
