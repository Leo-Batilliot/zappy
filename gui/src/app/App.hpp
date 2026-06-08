/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** App.hpp
*/

#pragma once

#include "Network.hpp"
#include "World.hpp"
#include "Parser.hpp"

class App {
public:
    App(const std::string &host, const std::string &port);

    int run();

private:
    void pumpNetwork();

    Network _network;
    World _world;
    Parser _parser;
    bool _ready;
};
