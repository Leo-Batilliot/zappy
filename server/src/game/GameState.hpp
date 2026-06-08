#pragma once

#include <string>
#include <unordered_map>
#include "args/Parser.hpp"
#include "game/Types.hpp"
#include "game/World.hpp"
#include "game/Player.hpp"
#include "game/Egg.hpp"
#include "game/Team.hpp"

class GameState {
public:
    explicit GameState(const Config &cfg);

    int createPlayer(const std::string &team, int connFd, Pos pos, Orientation orient);
    void removePlayer(int id);
    int createEgg(const std::string &team, Pos pos, int layerId = -1);
    void removeEgg(int id);
    int hatchEgg(const std::string &team);

    Player &player(int id);
    const Player &player(int id) const;
    Egg &egg(int id);
    Team &team(const std::string &name);
    World &world();
    const World &world() const;
    bool isTeam(const std::string &name) const;
    bool hasFreeSlot(const std::string &name) const;

    const std::unordered_map<int, Player> &players() const { return _players; }
    const std::unordered_map<int, Egg> &eggs() const { return _eggs; }
    const std::unordered_map<std::string, Team> &teams() const { return _teams; }

    bool finished = false;
    std::string winner;

    int nextPlayerId();
    int nextEggId();

private:
    World _world;
    std::unordered_map<int, Player> _players;
    std::unordered_map<int, Egg> _eggs;
    std::unordered_map<std::string, Team> _teams;
    int _nextPlayerId = 1;
    int _nextEggId = 1;
};
