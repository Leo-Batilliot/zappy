/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** World.hpp
*/

#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "Types.hpp"

class World {
public:
    World(int width = 10, int height = 10);

    void setMapSize(int width, int height);
    int width() const { return _width; }
    int height() const { return _height; }
    const Tile &tile(int x, int y) const;
    Tile &tile(int x, int y);

    void addTeam(const std::string &name);
    void addPlayer(int id, int x, int y, int orientation, int level, const std::string &team);
    void movePlayer(int id, int x, int y, int orientation);
    void setPlayerPosition(int id, int x, int y);
    void setPlayerLevel(int id, int level);
    void setPlayerInventory(int id, const std::array<int, RESOURCE_COUNT> &inventory);
    void removePlayer(int id);
    std::optional<std::reference_wrapper<const Player>> findPlayer(int id) const;
    const std::map<int, Player> &players() const { return _players; }
    const std::vector<std::string> &teams() const { return _teams; }

    void addEgg(int id, int x, int y, int layerId);
    void removeEgg(int id);
    const std::map<int, Egg> &eggs() const { return _eggs; }

    void startIncantation(int x, int y, int level, const std::vector<int> &players);
    void endIncantation(int x, int y, bool success);
    const std::vector<Incantation> &incantations() const { return _incantations; }

    void dropResource(int playerId, int resource);
    void takeResource(int playerId, int resource);

    void addEffect(EffectType type, int x, int y, bool success = false);
    void removeExpiredEffects();
    const std::vector<Effect> &effects() const { return _effects; }

    void setFrequency(int frequency);
    int frequency() const { return _frequency; }

    void setWinner(const std::string &team);
    bool isGameOver() const { return _gameOver; }
    const std::string &winner() const { return _winner; }

    void addLog(const std::string &message);
    const std::deque<std::string> &logs() const { return _logs; }

private:
    bool inBounds(int x, int y) const;

    static constexpr std::size_t MAX_LOGS = 8;

    int _width;
    int _height;
    int _frequency = 100;
    bool _gameOver = false;
    std::string _winner;
    std::vector<Tile> _tiles;
    std::map<int, Player> _players;
    std::vector<std::string> _teams;
    std::map<int, Egg> _eggs;
    std::vector<Incantation> _incantations;
    std::vector<Effect> _effects;
    std::deque<std::string> _logs;
};
