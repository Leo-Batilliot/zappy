/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** World.cpp
*/

#include "World.hpp"

#include <algorithm>

World::World(int width, int height)
{
    setMapSize(width, height);
}

void World::setMapSize(int width, int height)
{
    _width = width;
    _height = height;
    _tiles.assign(static_cast<std::size_t>(width) * height, Tile{});
}

const Tile &World::tile(int x, int y) const
{
    return _tiles[static_cast<std::size_t>(y) * _width + x];
}

Tile &World::tile(int x, int y)
{
    return _tiles[static_cast<std::size_t>(y) * _width + x];
}

void World::addTeam(const std::string &name)
{
    if (std::find(_teams.begin(), _teams.end(), name) == _teams.end())
        _teams.push_back(name);
}

void World::addPlayer(int id, int x, int y, int orientation, int level, const std::string &team)
{
    Player p;
    p.x = x;
    p.y = y;
    p.orientation = orientation;
    p.level = level;
    p.team = team;
    _players[id] = p;
}

void World::movePlayer(int id, int x, int y, int orientation)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return;
    it->second.x = x;
    it->second.y = y;
    it->second.orientation = orientation;
}

void World::setPlayerLevel(int id, int level)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return;
    it->second.level = level;
}

void World::setPlayerInventory(int id, const int inventory[RESOURCE_COUNT])
{
    auto it = _players.find(id);
    if (it == _players.end())
        return;
    for (int r = 0; r < RESOURCE_COUNT; ++r)
        it->second.inventory[r] = inventory[r];
}

void World::removePlayer(int id)
{
    _players.erase(id);
}
