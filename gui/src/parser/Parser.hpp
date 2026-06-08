/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Parser.hpp
*/

#pragma once

#include <string>
#include <sstream>
#include <functional>
#include <unordered_map>
#include "World.hpp"

class Parser {
public:
    explicit Parser(World &world);
    void parse(const std::string &line);

private:
    using Handler = std::function<void(std::istringstream &)>;

    static int parseId(const std::string &token);

    void handleMapSize(std::istringstream &args);
    void handleTile(std::istringstream &args);
    void handleTeamName(std::istringstream &args);
    void handleNewPlayer(std::istringstream &args);
    void handlePlayerPos(std::istringstream &args);
    void handlePlayerLevel(std::istringstream &args);
    void handlePlayerInventory(std::istringstream &args);
    void handlePlayerDeath(std::istringstream &args);

    World &_world;
    std::unordered_map<std::string, Handler> _handlers;
};
