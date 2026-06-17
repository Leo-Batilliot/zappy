/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** App.hpp
*/

#pragma once

#include "network/Network.hpp"
#include "world/World.hpp"
#include "parser/Parser.hpp"
#include "renderer/Renderer.hpp"

class App {
public:
    App(const std::string &host, const std::string &port);

    int run();

private:
    void pumpNetwork();
    void applyUiAction(const UiAction &action);

    Network _network;
    World _world;
    Parser _parser;
    bool _ready;
    bool _lossReported = false;
};
