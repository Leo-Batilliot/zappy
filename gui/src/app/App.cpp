/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** App.cpp
*/

#include "App.hpp"
#include "Renderer.hpp"

App::App(const std::string &host, const std::string &port)
    : _parser(_world), _ready(false)
{
    _ready = _network.connectTo(host, port);
    if (_ready)
        _network.sendLine("GRAPHIC");
}

int App::run()
{
    if (!_ready)
        return 84;
    Renderer renderer;
    while (renderer.isOpen()) {
        pumpNetwork();
        renderer.handleEvents();
        renderer.render(_world);
    }
    return 0;
}

void App::pumpNetwork()
{
    _network.poll();
    std::string message;
    while (_network.nextMessage(message))
        _parser.parse(message);
}
