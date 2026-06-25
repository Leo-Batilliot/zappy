/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** RendererEvents.cpp
*/

#include "renderer/Renderer.hpp"
#include "renderer/RendererPrivate.hpp"

#include <algorithm>
#include <cmath>

static constexpr float PAN_STEP = 60.f;

UiAction Renderer::handleEvents(const World &world)
{
    UiAction action;
    sf::Event event;

    while (_window.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::Closed:
                _window.close();
                break;
            case sf::Event::Resized: {
                float oldMapW = std::max(static_cast<float>(_windowSize.x) - PANEL_WIDTH, 1.f);
                float oldMapH = static_cast<float>(_windowSize.y);
                float oldTile = std::min(oldMapW / world.width(), oldMapH / world.height());
                float oldOffsetX = (oldMapW - oldTile * world.width()) / 2.f;
                float oldOffsetY = (oldMapH - oldTile * world.height()) / 2.f;
                float gameCx = (oldTile > 0.f) ? (_center.x - oldOffsetX) / oldTile : world.width() * 0.5f;
                float gameCy = (oldTile > 0.f) ? (_center.y - oldOffsetY) / oldTile : world.height() * 0.5f;

                _windowSize = {event.size.width, event.size.height};
                float newMapW = std::max(static_cast<float>(_windowSize.x) - PANEL_WIDTH, 1.f);
                float newMapH = static_cast<float>(_windowSize.y);
                float newTile = std::min(newMapW / world.width(), newMapH / world.height());
                float newOffsetX = (newMapW - newTile * world.width()) / 2.f;
                float newOffsetY = (newMapH - newTile * world.height()) / 2.f;

                _center.x = newOffsetX + gameCx * newTile;
                _center.y = newOffsetY + gameCy * newTile;
                updateMapView();
                break;
            }
            case sf::Event::MouseWheelScrolled:
                _zoom *= event.mouseWheelScroll.delta > 0 ? 0.9f : 1.1f;
                _zoom = std::clamp(_zoom, 0.2f, 4.f);
                updateMapView();
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left &&
                    event.mouseButton.x < mapAreaWidth())
                    action = handleClick(world, event.mouseButton.x,
                        event.mouseButton.y);
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                    case sf::Keyboard::Escape:
                        _window.close();
                        break;
                    case sf::Keyboard::Space:
                        _center = {
                            mapAreaWidth() / 2.f,
                            static_cast<float>(_window.getSize().y) / 2.f
                        };
                        updateMapView();
                        break;
                    case sf::Keyboard::Add:
                    case sf::Keyboard::Equal:
                        action.type = UiAction::Type::SPEED_UP;
                        break;
                    case sf::Keyboard::Subtract:
                    case sf::Keyboard::Hyphen:
                        action.type = UiAction::Type::SPEED_DOWN;
                        break;
                    case sf::Keyboard::Left:
                        _center.x -= PAN_STEP * _zoom;
                        updateMapView();
                        break;
                    case sf::Keyboard::Right:
                        _center.x += PAN_STEP * _zoom;
                        updateMapView();
                        break;
                    case sf::Keyboard::Up:
                        _center.y -= PAN_STEP * _zoom;
                        updateMapView();
                        break;
                    case sf::Keyboard::Down:
                        _center.y += PAN_STEP * _zoom;
                        updateMapView();
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return action;
}

UiAction Renderer::handleClick(const World &world, int px, int py)
{
    UiAction action;
    Layout l = computeLayout(world);
    sf::Vector2f pos = _window.mapPixelToCoords({px, py}, _mapView);
    int gx = static_cast<int>(std::floor((pos.x - l.offsetX) / l.tile));
    int gy = static_cast<int>(std::floor((pos.y - l.offsetY) / l.tile));

    if (gx < 0 || gy < 0 || gx >= world.width() || gy >= world.height()) {
        _selectedPlayer = -1;
        _selectedTileX = -1;
        _selectedTileY = -1;
        return action;
    }

    _selectedTileX = gx;
    _selectedTileY = gy;
    _selectedPlayer = -1;
    for (const auto &[id, p] : world.players()) {
        if (p.x == gx && p.y == gy)
            _selectedPlayer = id;
    }
    if (_selectedPlayer != -1) {
        action.type = UiAction::Type::SELECT_PLAYER;
        action.playerId = _selectedPlayer;
    }
    return action;
}
