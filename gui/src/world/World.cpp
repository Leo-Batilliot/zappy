/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** World.cpp
*/

#include "world/World.hpp"

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

bool World::inBounds(int x, int y) const
{
    return x >= 0 && y >= 0 && x < _width && y < _height;
}
