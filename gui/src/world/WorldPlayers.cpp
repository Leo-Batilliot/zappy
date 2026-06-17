/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** WorldPlayers.cpp
*/

#include "world/World.hpp"

void World::addPlayer(int id, int x, int y, int orientation, int level, const std::string &team)
{
    if (!inBounds(x, y))
        return;
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
    if (it == _players.end() || !inBounds(x, y))
        return;
    it->second.x = x;
    it->second.y = y;
    it->second.orientation = orientation;
}

void World::setPlayerPosition(int id, int x, int y)
{
    auto it = _players.find(id);
    if (it == _players.end() || !inBounds(x, y))
        return;
    it->second.x = x;
    it->second.y = y;
}

void World::setPlayerLevel(int id, int level)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return;
    it->second.level = level;
}

void World::setPlayerInventory(int id, const std::array<int, RESOURCE_COUNT> &inventory)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return;
    it->second.inventory = inventory;
}

void World::removePlayer(int id)
{
    _players.erase(id);
}

std::optional<std::reference_wrapper<const Player>> World::findPlayer(int id) const
{
    auto it = _players.find(id);
    if (it == _players.end())
        return std::nullopt;
    return std::cref(it->second);
}

void World::addEgg(int id, int x, int y, int layerId)
{
    if (!inBounds(x, y))
        return;
    _eggs[id] = {id, x, y, layerId};
}

void World::removeEgg(int id)
{
    _eggs.erase(id);
}
