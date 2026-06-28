/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** App.cpp
*/

#include "app/App.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

App::App(const std::string &host, const std::string &port)
    : _parser(_world), _ready(false)
{
    _ready = _network.connectTo(host, port);
    if (_ready) {
        _network.sendLine("GRAPHIC");
        _network.sendLine("sgt");
    }
}

int App::run()
{
    if (!_ready)
        return 84;
    if (!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY")) {
        std::cerr << "Pas d'affichage graphique disponible "
                     "(DISPLAY non defini) : le GUI ne peut pas demarrer."
                  << std::endl;
        return 84;
    }
    Renderer renderer;
    while (renderer.isOpen()) {
        pumpNetwork();
        _world.removeExpiredEffects();
        applyUiAction(renderer.handleEvents(_world));

        if (!_network.isConnected() && !_lossReported) {
            std::cerr << "Connexion au serveur perdue." << std::endl;
            _lossReported = true;
        }
        renderer.render(_world, _lossReported);
    }
    return 0;
}

void App::applyUiAction(const UiAction &action)
{
    switch (action.type) {
        case UiAction::Type::SPEED_UP:
        case UiAction::Type::SPEED_DOWN: {
            int frequency = _world.frequency();
            if (action.type == UiAction::Type::SPEED_UP)
                frequency += 10;
            else
                frequency = std::max(1, frequency - 10);
            _network.sendLine("sst " + std::to_string(frequency));
            std::cout << "[frequence] demande envoyee : " << frequency
                      << std::endl;
            break;
        }
        case UiAction::Type::SELECT_PLAYER: {
            std::string id = std::to_string(action.playerId);
            _network.sendLine("pin #" + id);
            _network.sendLine("plv #" + id);
            break;
        }
        default:
            break;
    }
}

void App::pumpNetwork()
{
    _network.poll();
    std::string message;
    while (_network.nextMessage(message)) {
        try {
            _parser.parse(message);
        } catch (const std::exception &e) {
            std::cerr << "[parse] message ignore (" << e.what() << ") : "
                      << message << std::endl;
        }
    }
}
