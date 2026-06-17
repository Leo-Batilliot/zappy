/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** RendererDraw.cpp
*/

#include "renderer/Renderer.hpp"
#include "renderer/RendererPrivate.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <map>

static sf::ConvexShape makeTriangle(float cx, float cy, float s, int orientation)
{
    sf::ConvexShape tri(3);
    switch (orientation) {
        case 1:
            tri.setPoint(0, {cx, cy - s});
            tri.setPoint(1, {cx - s * 0.6f, cy + s * 0.5f});
            tri.setPoint(2, {cx + s * 0.6f, cy + s * 0.5f});
            break;
        case 2:
            tri.setPoint(0, {cx + s, cy});
            tri.setPoint(1, {cx - s * 0.5f, cy - s * 0.6f});
            tri.setPoint(2, {cx - s * 0.5f, cy + s * 0.6f});
            break;
        case 3:
            tri.setPoint(0, {cx, cy + s});
            tri.setPoint(1, {cx - s * 0.6f, cy - s * 0.5f});
            tri.setPoint(2, {cx + s * 0.6f, cy - s * 0.5f});
            break;
        default:
            tri.setPoint(0, {cx - s, cy});
            tri.setPoint(1, {cx + s * 0.5f, cy - s * 0.6f});
            tri.setPoint(2, {cx + s * 0.5f, cy + s * 0.6f});
            break;
    }
    return tri;
}

void Renderer::drawGrid(const World &world, const Layout &l)
{
    sf::RectangleShape cell(sf::Vector2f(l.tile - 1.f, l.tile - 1.f));
    cell.setOutlineThickness(1.f);
    cell.setOutlineColor(sf::Color(60, 70, 60));

    for (int y = 0; y < world.height(); ++y) {
        for (int x = 0; x < world.width(); ++x) {
            cell.setPosition(l.offsetX + x * l.tile, l.offsetY + y * l.tile);
            bool dark = (x + y) % 2 == 0;
            cell.setFillColor(dark ? sf::Color(45, 80, 55) : sf::Color(55, 95, 65));
            _window.draw(cell);
        }
    }
}

void Renderer::drawIncantations(const World &world, const Layout &l)
{
    sf::RectangleShape overlay(sf::Vector2f(l.tile - 1.f, l.tile - 1.f));
    overlay.setFillColor(sf::Color(170, 110, 255, 90));
    overlay.setOutlineThickness(2.f);
    overlay.setOutlineColor(sf::Color(200, 160, 255, 180));

    for (const auto &inc : world.incantations()) {
        overlay.setPosition(l.offsetX + inc.x * l.tile, l.offsetY + inc.y * l.tile);
        _window.draw(overlay);
    }
}

void Renderer::drawSelection(const World &world, const Layout &l)
{
    if (_selectedTileX < 0 || _selectedTileX >= world.width() ||
        _selectedTileY < 0 || _selectedTileY >= world.height())
        return;

    sf::RectangleShape outline(sf::Vector2f(l.tile - 1.f, l.tile - 1.f));
    outline.setPosition(l.offsetX + _selectedTileX * l.tile,
        l.offsetY + _selectedTileY * l.tile);
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineThickness(2.5f);
    outline.setOutlineColor(sf::Color::White);
    _window.draw(outline);
}

void Renderer::drawResources(const World &world, const Layout &l)
{
    float radius = l.tile * 0.07f;
    if (radius < 1.5f)
        return;

    bool showCounts = _fontLoaded && (l.tile / _zoom) >= 48.f;
    sf::CircleShape dot(radius);
    dot.setOutlineThickness(0.f);

    for (int y = 0; y < world.height(); ++y) {
        for (int x = 0; x < world.width(); ++x) {
            const Tile &t = world.tile(x, y);
            float cellX = l.offsetX + x * l.tile;
            float cellY = l.offsetY + y * l.tile;
            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                if (t.resources[r] <= 0)
                    continue;
                int col = r % 4;
                int row = r / 4;
                float cx = cellX + l.tile * 0.2f + col * l.tile * 0.2f;
                float cy = cellY + l.tile * 0.3f + row * l.tile * 0.3f;
                dot.setPosition(cx - radius, cy - radius);
                dot.setFillColor(resourceColor(static_cast<Resource>(r)));
                _window.draw(dot);
                if (showCounts)
                    drawText(std::to_string(t.resources[r]),
                        cx + radius, cy - l.tile * 0.09f,
                        static_cast<unsigned int>(l.tile * 0.13f),
                        sf::Color::White);
            }
        }
    }
}

void Renderer::drawEggs(const World &world, const Layout &l)
{
    float rx = l.tile * 0.18f;
    float ry = l.tile * 0.24f;
    if (rx < 1.5f)
        return;

    sf::CircleShape egg(rx);
    egg.setScale(1.f, ry / rx);
    egg.setFillColor(sf::Color(240, 235, 200));
    egg.setOutlineColor(sf::Color(180, 160, 80));
    egg.setOutlineThickness(1.f);

    for (const auto &[id, e] : world.eggs()) {
        float cx = l.offsetX + e.x * l.tile + l.tile / 2.f - rx;
        float cy = l.offsetY + e.y * l.tile + l.tile * 0.62f - ry;
        egg.setPosition(cx, cy);
        _window.draw(egg);
    }
}

void Renderer::drawPlayers(const World &world, const Layout &l)
{
    if (l.tile * 0.35f < 2.f)
        return;

    static const std::array<sf::Vector2f, 4> OFFSETS = {
        sf::Vector2f(-0.2f, -0.2f),
        sf::Vector2f(0.2f, -0.2f),
        sf::Vector2f(-0.2f, 0.2f),
        sf::Vector2f(0.2f, 0.2f),
    };

    std::map<std::pair<int, int>, int> occupants;
    for (const auto &[id, p] : world.players())
        occupants[{p.x, p.y}]++;

    std::map<std::pair<int, int>, int> indexOnTile;
    for (const auto &[id, p] : world.players()) {
        float cx = l.offsetX + p.x * l.tile + l.tile / 2.f;
        float cy = l.offsetY + p.y * l.tile + l.tile / 2.f;
        float s = l.tile * 0.35f;

        if (occupants[{p.x, p.y}] > 1) {
            int idx = indexOnTile[{p.x, p.y}]++ % static_cast<int>(OFFSETS.size());
            cx += OFFSETS[idx].x * l.tile;
            cy += OFFSETS[idx].y * l.tile;
            s = l.tile * 0.22f;
        }

        sf::ConvexShape tri = makeTriangle(cx, cy, s, p.orientation);
        tri.setFillColor(teamColor(world, p.team));
        tri.setOutlineColor(p.inIncantation ? sf::Color::White : sf::Color::Black);
        tri.setOutlineThickness(1.5f);
        _window.draw(tri);

        if (id == _selectedPlayer) {
            sf::CircleShape ring(s * 1.4f);
            ring.setOrigin(s * 1.4f, s * 1.4f);
            ring.setPosition(cx, cy);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f);
            ring.setOutlineColor(sf::Color::White);
            _window.draw(ring);
        }
    }
}

void Renderer::drawEffects(const World &world, const Layout &l)
{
    auto now = std::chrono::steady_clock::now();
    float duration = std::chrono::duration<float>(EFFECT_DURATION).count();

    for (const auto &e : world.effects()) {
        float t = std::chrono::duration<float>(now - e.start).count() / duration;
        t = std::clamp(t, 0.f, 1.f);
        sf::Uint8 alpha = static_cast<sf::Uint8>(200.f * (1.f - t));
        float cx = l.offsetX + e.x * l.tile + l.tile / 2.f;
        float cy = l.offsetY + e.y * l.tile + l.tile / 2.f;

        if (e.type == EffectType::BROADCAST) {
            float radius = l.tile * (0.2f + t);
            sf::CircleShape ring(radius);
            ring.setOrigin(radius, radius);
            ring.setPosition(cx, cy);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(3.f);
            ring.setOutlineColor(sf::Color(255, 230, 90, alpha));
            _window.draw(ring);
        } else if (e.type == EffectType::EJECTION) {
            float radius = l.tile * 0.4f;
            sf::CircleShape ring(radius);
            ring.setOrigin(radius, radius);
            ring.setPosition(cx, cy);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(4.f);
            ring.setOutlineColor(sf::Color(255, 140, 60, alpha));
            _window.draw(ring);
        } else {
            sf::RectangleShape flash(sf::Vector2f(l.tile - 1.f, l.tile - 1.f));
            flash.setPosition(l.offsetX + e.x * l.tile, l.offsetY + e.y * l.tile);
            flash.setFillColor(e.success
                ? sf::Color(90, 220, 120, alpha)
                : sf::Color(230, 80, 80, alpha));
            _window.draw(flash);
        }
    }
}
