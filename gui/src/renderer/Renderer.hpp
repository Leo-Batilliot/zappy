/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Renderer.hpp
*/

#pragma once

#include <SFML/Graphics.hpp>
#include "World.hpp"

class Renderer {
public:
    Renderer(unsigned int width = 1280, unsigned int height = 720);

    bool isOpen() const;
    void handleEvents();
    void render(const World &world);

private:
    void drawGrid(const World &world, float offsetX, float offsetY, float tile);
    void drawResources(const World &world, float offsetX, float offsetY, float tile);
    void drawPlayers(const World &world, float offsetX, float offsetY, float tile);

    sf::Color teamColor(const World &world, const std::string &team) const;

    sf::RenderWindow _window;
};
