/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Renderer.hpp
*/

#pragma once

#include <array>
#include <string>
#include <string_view>
#include <SFML/Graphics.hpp>
#include "world/World.hpp"

struct UiAction {
    enum class Type {
        NONE,
        SPEED_UP,
        SPEED_DOWN,
        SELECT_PLAYER
    };

    Type type = Type::NONE;
    int playerId = -1;
};

class Renderer {
public:
    Renderer(unsigned int width = 1280, unsigned int height = 720);

    bool isOpen() const;
    UiAction handleEvents(const World &world);
    void render(const World &world, bool connectionLost);

private:
    struct Layout {
        float offsetX;
        float offsetY;
        float tile;
    };

    Layout computeLayout(const World &world) const;
    float mapAreaWidth() const;
    void updateMapView();
    UiAction handleClick(const World &world, int px, int py);

    void drawGrid(const World &world, const Layout &l);
    void drawIncantations(const World &world, const Layout &l);
    void drawResources(const World &world, const Layout &l);
    void drawEggs(const World &world, const Layout &l);
    void drawPlayers(const World &world, const Layout &l);
    void drawEffects(const World &world, const Layout &l);
    void drawSelection(const World &world, const Layout &l);
    void drawPanel(const World &world);
    void drawConnectionLost();
    void drawGameOver(const World &world);

    void drawText(std::string_view text, float x, float y,
                  unsigned int size, sf::Color color);
    sf::Color teamColor(const World &world, const std::string &team) const;

    sf::RenderWindow _window;
    sf::Font _font;
    bool _fontLoaded = false;
    std::array<sf::Texture, RESOURCE_COUNT> _resTextures;
    std::array<bool, RESOURCE_COUNT> _resTexturesLoaded{};
    sf::View _mapView;
    float _zoom = 1.f;
    sf::Vector2f _center;
    sf::Vector2u _windowSize;
    int _selectedPlayer = -1;
    int _selectedTileX = -1;
    int _selectedTileY = -1;
};
