/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Parser.cpp
*/

#include "parser/Parser.hpp"

#include <charconv>
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
    _handlers["enw"] = [this](std::istringstream &a) { handleNewEgg(a); };
    _handlers["ebo"] = [this](std::istringstream &a) { handleEggHatch(a); };
    _handlers["edi"] = [this](std::istringstream &a) { handleEggDeath(a); };
    _handlers["pic"] = [this](std::istringstream &a) { handleIncantationStart(a); };
    _handlers["pie"] = [this](std::istringstream &a) { handleIncantationEnd(a); };
    _handlers["pbc"] = [this](std::istringstream &a) { handleBroadcast(a); };
    _handlers["pex"] = [this](std::istringstream &a) { handleEjection(a); };
    _handlers["pfk"] = [this](std::istringstream &a) { handleEggLaying(a); };
    _handlers["pdr"] = [this](std::istringstream &a) { handleResourceDrop(a); };
    _handlers["pgt"] = [this](std::istringstream &a) { handleResourceTake(a); };
    _handlers["sgt"] = [this](std::istringstream &a) { handleTimeUnit(a); };
    _handlers["sst"] = [this](std::istringstream &a) { handleTimeUnit(a); };
    _handlers["seg"] = [this](std::istringstream &a) { handleGameOver(a); };
    _handlers["smg"] = [this](std::istringstream &a) { handleServerMessage(a); };
    _handlers["suc"] = [](std::istringstream &) {
        std::cout << "[serveur] commande inconnue (suc)" << std::endl;
    };
    _handlers["sbp"] = [](std::istringstream &) {
        std::cout << "[serveur] mauvais parametres (sbp)" << std::endl;
    };
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
    std::size_t offset = (!token.empty() && token[0] == '#') ? 1 : 0;
    int value = 0;
    const char *first = token.data() + offset;
    const char *last = token.data() + token.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc() || ptr != last)
        return -1;
    return value;
}

std::string Parser::restOfLine(std::istringstream &args)
{
    std::string rest;
    std::getline(args, rest);
    std::size_t start = rest.find_first_not_of(' ');
    return start == std::string::npos ? "" : rest.substr(start);
}
