/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** WorldGame.cpp
*/

#include "world/World.hpp"

#include <algorithm>

void World::startIncantation(int x, int y, int level, const std::vector<int> &players)
{
    if (!inBounds(x, y))
        return;
    _incantations.push_back({x, y, level, players});
    for (int id : players) {
        auto it = _players.find(id);
        if (it != _players.end())
            it->second.inIncantation = true;
    }
}

void World::endIncantation(int x, int y, bool success)
{
    for (auto it = _incantations.begin(); it != _incantations.end(); ++it) {
        if (it->x != x || it->y != y)
            continue;
        for (int id : it->players) {
            auto player = _players.find(id);
            if (player != _players.end())
                player->second.inIncantation = false;
        }
        _incantations.erase(it);
        break;
    }
    addEffect(EffectType::INCANTATION_END, x, y, success);
}

void World::dropResource(int playerId, int resource)
{
    auto it = _players.find(playerId);
    if (it == _players.end() || resource < 0 || resource >= RESOURCE_COUNT)
        return;
    const Player &p = it->second;
    if (!inBounds(p.x, p.y))
        return;
    tile(p.x, p.y).resources[resource]++;
}

void World::takeResource(int playerId, int resource)
{
    auto it = _players.find(playerId);
    if (it == _players.end() || resource < 0 || resource >= RESOURCE_COUNT)
        return;
    const Player &p = it->second;
    if (!inBounds(p.x, p.y))
        return;
    int &quantity = tile(p.x, p.y).resources[resource];
    if (quantity > 0)
        --quantity;
}

void World::addEffect(EffectType type, int x, int y, bool success)
{
    if (!inBounds(x, y))
        return;
    _effects.push_back({type, x, y, success, std::chrono::steady_clock::now()});
}

void World::setFrequency(int frequency)
{
    if (frequency > 0)
        _frequency = frequency;
}

void World::setWinner(const std::string &team)
{
    _gameOver = true;
    _winner = team;
}

void World::addLog(const std::string &message)
{
    _logs.push_back(message);
    while (_logs.size() > MAX_LOGS)
        _logs.pop_front();
}

void World::removeExpiredEffects()
{
    auto now = std::chrono::steady_clock::now();
    _effects.erase(
        std::remove_if(_effects.begin(), _effects.end(),
            [now](const Effect &e) { return now - e.start >= EFFECT_DURATION; }),
        _effects.end());
}
