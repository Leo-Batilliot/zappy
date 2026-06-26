#include "game/World.hpp"
#include <algorithm>
#include <random>
#include <numeric>

World::World(int width, int height) : _width(width), _height(height), _grid(width * height), _rng(std::random_device{}()) {}

Pos World::wrap(int x, int y) const
{
    return {
        ((x % _width) + _width) % _width,
        ((y % _height) + _height) % _height,
    };
}

Tile& World::at(int x, int y)
{
    Pos p = wrap(x, y);
    return _grid[idx(p.x(), p.y())];
}

const Tile& World::at(int x, int y) const
{
    Pos p = wrap(x, y);
    return _grid[idx(p.x(), p.y())];
}

Pos World::step(Pos pos, Orientation orient, int n) const
{
    int dx = 0, dy = 0;
    switch (orient) {
        case Orientation::North: dy = -1; break;
        case Orientation::East: dx = +1; break;
        case Orientation::South: dy = +1; break;
        case Orientation::West: dx = -1; break;
    }
    return wrap(pos.x() + dx * n, pos.y() + dy * n);
}

void World::placePlayer(int id, Pos pos)
{
    at(pos.x(), pos.y()).players().push_back(id);
}

void World::removePlayer(int id, Pos pos)
{
    auto &v = at(pos.x(), pos.y()).players();
    v.erase(std::remove(v.begin(), v.end(), id), v.end());
}

void World::movePlayer(int id, Pos from, Pos to)
{
    removePlayer(id, from);
    placePlayer(id, to);
}

void World::placeEgg(int id, Pos pos)
{
    at(pos.x(), pos.y()).eggs().push_back(id);
}

void World::removeEgg(int id, Pos pos)
{
    auto &v = at(pos.x(), pos.y()).eggs();
    v.erase(std::remove(v.begin(), v.end(), id), v.end());
}

int World::resourceTarget(int r) const
{
    return std::max(1, static_cast<int>(_width * _height * DENSITY[r]));
}

void World::distributeResource(int r, int count)
{
    if (count <= 0)
        return;
    std::uniform_int_distribution<int> dist(0, _width * _height - 1);
    for (int i = 0; i < count; ++i)
        _grid[dist(_rng)].resources()[r]++;
}

void World::initResources()
{
    for (int r = 0; r < RESOURCE_COUNT; ++r)
        distributeResource(r, resourceTarget(r));
}

std::vector<Pos> World::refill()
{
    std::vector<bool> modified(_width * _height, false);

    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        int present = 0;
        for (const Tile &t : _grid)
            present += t.resources()[r];

        int missing = resourceTarget(r) - present;
        if (missing <= 0)
            continue;

        std::uniform_int_distribution<int> dist(0, _width * _height - 1);
        for (int i = 0; i < missing; ++i) {
            int tileIdx = dist(_rng);
            _grid[tileIdx].resources()[r]++;
            modified[tileIdx] = true;
        }
    }

    std::vector<Pos> changed;
    for (int i = 0; i < _width * _height; ++i)
        if (modified[i])
            changed.push_back({i % _width, i / _width});
    return changed;
}

Pos World::shortestDelta(Pos from, Pos to) const
{
    int dx = to.x() - from.x();
    int dy = to.y() - from.y();

    if (dx > _width / 2)
        dx -= _width;
    if (dx < -_width / 2)
        dx += _width;
    if (dy > _height / 2)
        dy -= _height;
    if (dy < -_height / 2)
        dy += _height;

    return {dx, dy};
}

std::vector<Pos> World::visibleTiles(Pos pos, Orientation orient, int level) const
{
    int fwdX = 0, fwdY = 0, rgtX = 0, rgtY = 0;
    switch (orient) {
        case Orientation::North: fwdX = 0; fwdY = -1; rgtX = 1; rgtY = 0; break;
        case Orientation::East: fwdX = 1; fwdY = 0; rgtX = 0; rgtY = 1; break;
        case Orientation::South: fwdX = 0; fwdY = 1; rgtX = -1; rgtY = 0; break;
        case Orientation::West: fwdX = -1; fwdY = 0; rgtX = 0; rgtY = -1; break;
    }

    std::vector<Pos> tiles;
    tiles.reserve((level + 1) * (level + 1));

    for (int line = 0; line <= level; ++line) {
        for (int off = -line; off <= line; ++off) {
            int wx = pos.x() + fwdX * line + rgtX * off;
            int wy = pos.y() + fwdY * line + rgtY * off;
            tiles.push_back(wrap(wx, wy));
        }
    }
    return tiles;
}
