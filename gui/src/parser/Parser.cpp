/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Parser.cpp
*/

#include "Parser.hpp"

#include <iostream>

Parser::Parser(World &world) : _world(world)
{
    _handlers["msz"] = [this](std::istringstream &a) { handleMapSize(a); };
    _handlers["bct"] = [this](std::istringstream &a) { handleTile(a); };
    _handlers["tna"] = [this](std::istringstream &a) { handleTeamName(a); };
    _handlers["pnw"] = [this](std::istringstream &a) { handleNewPlayer(a); };
    _handlers["ppo"] = [this](std::istringstream &a) { handlePlayerPos(a); };
    _handlers["plv"] = [this](std::istringstream &a) { handlePlayerLevel(a); };
    _handlers["pin"] = [this](std::istringstream &a) { handlePlayerInventory(a); };
    _handlers["pdi"] = [this](std::istringstream &a) { handlePlayerDeath(a); };
}

void Parser::parse(const std::string &line)
{
    std::istringstream stream(line);
    std::string command;
    stream >> command;
    if (command.empty())
        return;

    auto it = _handlers.find(command);
    if (it != _handlers.end())
        it->second(stream);
    else
        std::cout << "<- " << line << std::endl;
}

int Parser::parseId(const std::string &token)
{
    if (!token.empty() && token[0] == '#')
        return std::stoi(token.substr(1));
    return std::stoi(token);
}

void Parser::handleMapSize(std::istringstream &args)
{
    int w = 0;
    int h = 0;
    if (args >> w >> h && w > 0 && h > 0)
        _world.setMapSize(w, h);
}

void Parser::handleTile(std::istringstream &args)
{
    int x = 0;
    int y = 0;
    if (!(args >> x >> y))
        return;
    if (x < 0 || y < 0 || x >= _world.width() || y >= _world.height())
        return;

    Tile &tile = _world.tile(x, y);
    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        int quantity = 0;
        if (!(args >> quantity))
            return;
        tile.resources[r] = quantity;
    }
}

void Parser::handleTeamName(std::istringstream &args)
{
    std::string name;
    if (args >> name)
        _world.addTeam(name);
}

void Parser::handleNewPlayer(std::istringstream &args)
{
    std::string idToken;
    int x = 0, y = 0, orientation = 1, level = 1;
    std::string team;
    if (!(args >> idToken >> x >> y >> orientation >> level >> team))
        return;
    _world.addPlayer(parseId(idToken), x, y, orientation, level, team);
}

void Parser::handlePlayerPos(std::istringstream &args)
{
    std::string idToken;
    int x = 0, y = 0, orientation = 1;
    if (!(args >> idToken >> x >> y >> orientation))
        return;
    _world.movePlayer(parseId(idToken), x, y, orientation);
}

void Parser::handlePlayerLevel(std::istringstream &args)
{
    std::string idToken;
    int level = 0;
    if (!(args >> idToken >> level))
        return;
    _world.setPlayerLevel(parseId(idToken), level);
}

void Parser::handlePlayerInventory(std::istringstream &args)
{
    std::string idToken;
    int x = 0, y = 0;
    if (!(args >> idToken >> x >> y))
        return;
    int inventory[RESOURCE_COUNT] = {0};
    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        if (!(args >> inventory[r]))
            return;
    }
    _world.setPlayerInventory(parseId(idToken), inventory);
}

void Parser::handlePlayerDeath(std::istringstream &args)
{
    std::string idToken;
    if (args >> idToken)
        _world.removePlayer(parseId(idToken));
}
