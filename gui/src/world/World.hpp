/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** World.hpp
*/

#pragma once

#include <vector>
#include <map>
#include <string>
#include <cstddef>

enum Resource {
    FOOD = 0,
    LINEMATE,
    DERAUMERE,
    SIBUR,
    MENDIANE,
    PHIRAS,
    THYSTAME,
    RESOURCE_COUNT
};

struct Tile {
    int resources[RESOURCE_COUNT] = {0};
};

struct Player {
    int x = 0;
    int y = 0;
    int orientation = 1;
    int level = 1;
    std::string team;
    int inventory[RESOURCE_COUNT] = {0};
};

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
    void setPlayerLevel(int id, int level);
    void setPlayerInventory(int id, const int inventory[RESOURCE_COUNT]);
    void removePlayer(int id);
    const std::map<int, Player> &players() const { return _players; }
    const std::vector<std::string> &teams() const { return _teams; }

private:
    int _width;
    int _height;
    std::vector<Tile> _tiles;
    std::map<int, Player> _players;
    std::vector<std::string> _teams;
};
