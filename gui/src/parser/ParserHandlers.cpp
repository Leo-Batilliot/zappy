/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** ParserHandlers.cpp
*/

#include "parser/Parser.hpp"

#include <array>
#include <iostream>
#include <vector>

static constexpr int MAX_MAP_DIM = 10000;

void Parser::handleMapSize(std::istringstream &args)
{
    int w = 0;
    int h = 0;
    if (args >> w >> h && w > 0 && h > 0 && w <= MAX_MAP_DIM && h <= MAX_MAP_DIM)
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
    int id = parseId(idToken);
    if (id < 0)
        return;
    _world.addPlayer(id, x, y, orientation, level, team);
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
    std::array<int, RESOURCE_COUNT> inventory{};
    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        if (!(args >> inventory[r]))
            return;
    }
    _world.setPlayerInventory(parseId(idToken), inventory);
}

void Parser::handlePlayerDeath(std::istringstream &args)
{
    std::string idToken;
    if (!(args >> idToken))
        return;
    int id = parseId(idToken);
    if (id >= 0)
        _world.addLog("Mort du joueur #" + std::to_string(id));
    _world.removePlayer(id);
}

void Parser::handleNewEgg(std::istringstream &args)
{
    std::string eggToken, playerToken;
    int x = 0, y = 0;
    if (!(args >> eggToken >> playerToken >> x >> y))
        return;
    int eggId = parseId(eggToken);
    if (eggId < 0)
        return;
    _world.addEgg(eggId, x, y, parseId(playerToken));
}

void Parser::handleEggHatch(std::istringstream &args)
{
    std::string eggToken;
    if (args >> eggToken)
        _world.removeEgg(parseId(eggToken));
}

void Parser::handleEggDeath(std::istringstream &args)
{
    std::string eggToken;
    if (args >> eggToken)
        _world.removeEgg(parseId(eggToken));
}

void Parser::handleIncantationStart(std::istringstream &args)
{
    int x = 0, y = 0, level = 0;
    if (!(args >> x >> y >> level))
        return;
    std::vector<int> ids;
    std::string token;
    while (args >> token) {
        int id = parseId(token);
        if (id >= 0)
            ids.push_back(id);
    }
    _world.startIncantation(x, y, level, ids);
    _world.addLog("Incantation niv " + std::to_string(level)
        + " en (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    std::cout << "[incantation] debut niveau " << level
              << " en (" << x << ", " << y << ")" << std::endl;
}

void Parser::handleIncantationEnd(std::istringstream &args)
{
    int x = 0, y = 0, result = 0;
    if (!(args >> x >> y >> result))
        return;
    _world.endIncantation(x, y, result != 0);
    _world.addLog(std::string("Incantation ") + (result != 0 ? "reussie" : "echouee")
        + " en (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    std::cout << "[incantation] " << (result != 0 ? "succes" : "echec")
              << " en (" << x << ", " << y << ")" << std::endl;
}

void Parser::handleBroadcast(std::istringstream &args)
{
    std::string idToken;
    if (!(args >> idToken))
        return;
    int id = parseId(idToken);
    if (id < 0)
        return;
    std::string message = restOfLine(args);
    if (auto p = _world.findPlayer(id))
        _world.addEffect(EffectType::BROADCAST, p->get().x, p->get().y);
    _world.addLog("Broadcast #" + std::to_string(id) + " : " + message);
    std::cout << "[broadcast] joueur #" << id << " : " << message << std::endl;
}

void Parser::handleEjection(std::istringstream &args)
{
    std::string idToken;
    if (!(args >> idToken))
        return;
    int id = parseId(idToken);
    if (id < 0)
        return;
    if (auto p = _world.findPlayer(id))
        _world.addEffect(EffectType::EJECTION, p->get().x, p->get().y);
    _world.addLog("Ejection par le joueur #" + std::to_string(id));
    std::cout << "[ejection] joueur #" << id << std::endl;
}

void Parser::handleEggLaying(std::istringstream &args)
{
    std::string idToken;
    if (!(args >> idToken))
        return;
    int id = parseId(idToken);
    if (id < 0)
        return;
    _world.addLog("Ponte du joueur #" + std::to_string(id));
    std::cout << "[ponte] joueur #" << id << std::endl;
}

void Parser::handleResourceDrop(std::istringstream &args)
{
    std::string idToken;
    int resource = -1;
    if (!(args >> idToken >> resource))
        return;
    _world.dropResource(parseId(idToken), resource);
}

void Parser::handleResourceTake(std::istringstream &args)
{
    std::string idToken;
    int resource = -1;
    if (!(args >> idToken >> resource))
        return;
    _world.takeResource(parseId(idToken), resource);
}

void Parser::handleTimeUnit(std::istringstream &args)
{
    int frequency = 0;
    if (!(args >> frequency))
        return;
    _world.setFrequency(frequency);
    std::cout << "[frequence] " << frequency << std::endl;
}

void Parser::handleGameOver(std::istringstream &args)
{
    std::string team = restOfLine(args);
    _world.setWinner(team);
    _world.addLog("Fin de partie, vainqueur : " + team);
    std::cout << "[fin de partie] vainqueur : " << team << std::endl;
}

void Parser::handleServerMessage(std::istringstream &args)
{
    std::cout << "[serveur] " << restOfLine(args) << std::endl;
}
