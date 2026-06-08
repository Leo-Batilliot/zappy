#pragma once

#include <unordered_map>
#include <string>
#include "args/Parser.hpp"
#include "game/World.hpp"
#include "game/Player.hpp"
#include "game/Egg.hpp"
#include "game/Team.hpp"

class GameState {
public:
    explicit GameState(const Config &cfg);

    // ── Monde ────────────────────────────────────────────────────────────────
    World&       world()       { return _world; }
    const World& world() const { return _world; }

    // ── Joueurs ──────────────────────────────────────────────────────────────
    int  createPlayer(const std::string &teamName, int connFd, Pos pos, Orientation orient);
    void removePlayer(int id);
    int  playerIdByFd(int fd) const;

    Player&       player(int id)       { return _players.at(id); }
    const Player& player(int id) const { return _players.at(id); }
    bool hasPlayer(int id) const { return _players.count(id) != 0; }

    const std::unordered_map<int, Player>& players() const { return _players; }

    // ── Œufs ─────────────────────────────────────────────────────────────────
    int  createEgg(const std::string &teamName, Pos pos, int layerId = -1);
    void removeEgg(int id);
    int  hatchEgg(const std::string &teamName);

    Egg&       egg(int id)       { return _eggs.at(id); }
    const Egg& egg(int id) const { return _eggs.at(id); }
    bool hasEgg(int id) const { return _eggs.count(id) != 0; }

    const std::unordered_map<int, Egg>& eggs() const { return _eggs; }

    // ── Équipes ──────────────────────────────────────────────────────────────
    Team&       team(const std::string &name)       { return _teams.at(name); }
    const Team& team(const std::string &name) const { return _teams.at(name); }
    bool isTeam(const std::string &name)     const { return _teams.count(name) != 0; }
    bool hasFreeSlot(const std::string &name) const;

    const std::unordered_map<std::string, Team>& teams() const { return _teams; }

    // ── Fin de partie ────────────────────────────────────────────────────────
    bool        finished = false;
    std::string winner;

private:
    World _world;
    std::unordered_map<int, Player> _players;
    std::unordered_map<int, Egg>    _eggs;
    std::unordered_map<std::string, Team> _teams;
    std::unordered_map<int, int> _fdToPlayerId;

    int _nextPlayerId = 1;
    int _nextEggId    = 1;
};
