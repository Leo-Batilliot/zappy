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
#include "world/World.hpp"

class Parser {
public:
    explicit Parser(World &world);
    void parse(const std::string &line);

private:
    using Handler = std::function<void(std::istringstream &)>;

    static int parseId(const std::string &token);
    static std::string restOfLine(std::istringstream &args);

    void handleMapSize(std::istringstream &args);
    void handleTile(std::istringstream &args);
    void handleTeamName(std::istringstream &args);
    void handleNewPlayer(std::istringstream &args);
    void handlePlayerPos(std::istringstream &args);
    void handlePlayerLevel(std::istringstream &args);
    void handlePlayerInventory(std::istringstream &args);
    void handlePlayerDeath(std::istringstream &args);
    void handleNewEgg(std::istringstream &args);
    void handleEggHatch(std::istringstream &args);
    void handleEggDeath(std::istringstream &args);
    void handleIncantationStart(std::istringstream &args);
    void handleIncantationEnd(std::istringstream &args);
    void handleBroadcast(std::istringstream &args);
    void handleEjection(std::istringstream &args);
    void handleEggLaying(std::istringstream &args);
    void handleResourceDrop(std::istringstream &args);
    void handleResourceTake(std::istringstream &args);
    void handleTimeUnit(std::istringstream &args);
    void handleGameOver(std::istringstream &args);
    void handleServerMessage(std::istringstream &args);

    World &_world;
    std::unordered_map<std::string, Handler> _handlers;
};
