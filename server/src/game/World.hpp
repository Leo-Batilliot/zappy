#pragma once

#include <vector>
#include <random>
#include "game/Types.hpp"
#include "game/Tile.hpp"

class World {
public:
    World(int width, int height);

    int width() const { return _width; }
    int height() const { return _height; }

    Pos wrap(int x, int y) const;
    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    Pos step(Pos pos, Orientation orient, int n = 1) const;

    void placePlayer(int id, Pos pos);
    void removePlayer(int id, Pos pos);
    void movePlayer(int id, Pos from, Pos to);

    void placeEgg(int id, Pos pos);
    void removeEgg(int id, Pos pos);

    void initResources();
    std::vector<Pos> refill();

    Pos shortestDelta(Pos from, Pos to) const;
    std::vector<Pos> visibleTiles(Pos pos, Orientation orient, int level) const;

private:
    int _width;
    int _height;
    std::vector<Tile> _grid;
    std::mt19937 _rng;

    int idx(int x, int y) const { return y * _width + x; }
    int resourceTarget(int r) const;
    void distributeResource(int r, int count);
};
