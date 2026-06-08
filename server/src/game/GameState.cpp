#include "game/GameState.hpp"
#include <algorithm>
#include <stdexcept>

// ─── Construction ─────────────────────────────────────────────────────────────

GameState::GameState(const Config &cfg)
    : _world(cfg.width, cfg.height)
{
    _world.initResources();

    for (const std::string &name : cfg.teams) {
        Team t;
        t.name      = name;
        t.freeSlots = cfg.clients;
        _teams[name] = std::move(t);
    }

    std::uniform_int_distribution<int> randX(0, cfg.width  - 1);
    std::uniform_int_distribution<int> randY(0, cfg.height - 1);
    std::mt19937 rng{std::random_device{}()};

    for (const std::string &name : cfg.teams) {
        for (int i = 0; i < cfg.clients; ++i) {
            Pos pos{randX(rng), randY(rng)};
            createEgg(name, pos);
        }
    }
}

// ─── Joueurs ──────────────────────────────────────────────────────────────────

int GameState::createPlayer(const std::string &teamName, int connFd,
                             Pos pos, Orientation orient)
{
    int id = _nextPlayerId++;

    Player p;
    p.id       = id;
    p.teamName = teamName;
    p.connFd   = connFd;
    p.pos      = pos;
    p.orient   = orient;
    p.inventory[static_cast<int>(Resource::Food)] = INITIAL_FOOD_UNITS;

    _players[id] = std::move(p);
    _world.placePlayer(id, pos);
    _teams.at(teamName).playerIds.push_back(id);
    if (connFd != -1)
        _fdToPlayerId[connFd] = id;

    return id;
}

void GameState::removePlayer(int id)
{
    auto it = _players.find(id);
    if (it == _players.end())
        return;

    Player &p = it->second;
    _world.removePlayer(id, p.pos);

    if (p.connFd != -1)
        _fdToPlayerId.erase(p.connFd);

    auto &pids = _teams.at(p.teamName).playerIds;
    pids.erase(std::remove(pids.begin(), pids.end(), id), pids.end());

    _players.erase(it);
}

int GameState::playerIdByFd(int fd) const
{
    auto it = _fdToPlayerId.find(fd);
    return it != _fdToPlayerId.end() ? it->second : -1;
}

// ─── Œufs ─────────────────────────────────────────────────────────────────────

int GameState::createEgg(const std::string &teamName, Pos pos, int layerId)
{
    int id = _nextEggId++;

    Egg e;
    e.id      = id;
    e.team    = teamName;
    e.pos     = pos;
    e.layerId = layerId;

    _eggs[id] = std::move(e);
    _world.placeEgg(id, pos);
    _teams.at(teamName).eggIds.push_back(id);

    return id;
}

void GameState::removeEgg(int id)
{
    auto it = _eggs.find(id);
    if (it == _eggs.end())
        return;

    Egg &e = it->second;
    _world.removeEgg(id, e.pos);

    auto &eids = _teams.at(e.team).eggIds;
    eids.erase(std::remove(eids.begin(), eids.end(), id), eids.end());

    _eggs.erase(it);
}

int GameState::hatchEgg(const std::string &teamName)
{
    Team &t = _teams.at(teamName);
    if (t.eggIds.empty())
        return -1;

    std::uniform_int_distribution<int> dist(0, static_cast<int>(t.eggIds.size()) - 1);
    std::mt19937 rng{std::random_device{}()};
    int idx = dist(rng);

    return t.eggIds[idx];
}

// ─── Équipes ──────────────────────────────────────────────────────────────────

bool GameState::hasFreeSlot(const std::string &name) const
{
    auto it = _teams.find(name);
    return it != _teams.end() && it->second.freeSlots > 0;
}
