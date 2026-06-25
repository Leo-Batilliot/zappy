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
#include <vector>

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

// Deterministic per-tile RNG: stable across frames, pans, and zooms.
static unsigned tileRng(int x, int y)
{
    unsigned h = static_cast<unsigned>(x) * 2246822519u
               ^ static_cast<unsigned>(y) * 2654435761u;
    h ^= h >> 17;
    h *= 0x45d9f3bu;
    h ^= h >> 15;
    return h;
}

void Renderer::drawForest(const World &world, const Layout &l)
{
    // Compute visible tile range from the current map view.
    sf::Vector2f vc = _mapView.getCenter();
    sf::Vector2f vs = _mapView.getSize();
    int x0 = static_cast<int>(std::floor((vc.x - vs.x * 0.5f - l.offsetX) / l.tile)) - 1;
    int x1 = static_cast<int>(std::ceil( (vc.x + vs.x * 0.5f - l.offsetX) / l.tile)) + 1;
    int y0 = static_cast<int>(std::floor((vc.y - vs.y * 0.5f - l.offsetY) / l.tile)) - 1;
    int y1 = static_cast<int>(std::ceil( (vc.y + vs.y * 0.5f - l.offsetY) / l.tile)) + 1;

    // 1. Draw the continuous checker-board floor outside the game board.
    sf::RectangleShape cell(sf::Vector2f(l.tile - 1.f, l.tile - 1.f));
    cell.setOutlineThickness(1.f);
    cell.setOutlineColor(sf::Color(60, 70, 60));
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (x >= 0 && x < world.width() && y >= 0 && y < world.height())
                continue;
            cell.setPosition(l.offsetX + x * l.tile, l.offsetY + y * l.tile);
            bool dark = (x + y) % 2 == 0;
            cell.setFillColor(dark ? sf::Color(45, 80, 55) : sf::Color(55, 95, 65));
            _window.draw(cell);
        }
    }

    // 2. Collect out-of-board tiles that contain a tree (~90% density).
    if (!_showTrees)
        return;

    bool anyTree = false;
    for (int i = 0; i < TREE_TEX_COUNT; ++i)
        anyTree |= _treeTexturesLoaded[i];
    if (!anyTree)
        return;

    struct TreeEntry { int x, y; float distSq; unsigned rng; };
    std::vector<TreeEntry> trees;
    trees.reserve(static_cast<std::size_t>((x1 - x0 + 1) * (y1 - y0 + 1)));

    float bcx = world.width()  * 0.5f;
    float bcy = world.height() * 0.5f;

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (x >= 0 && x < world.width() && y >= 0 && y < world.height())
                continue;
            unsigned rng = tileRng(x, y);
            if ((rng & 0xFF) < 26) // ~90 % have a tree
                continue;
            float dx = (x + 0.5f) - bcx;
            float dy = (y + 0.5f) - bcy;
            trees.push_back({x, y, dx * dx + dy * dy, rng});
        }
    }

    // 3. Sort farthest-from-center first so near trees overdraw far ones.
    std::sort(trees.begin(), trees.end(), [](const TreeEntry &a, const TreeEntry &b) {
        return a.distSq > b.distSq;
    });

    // 4. Draw trees.
    for (const auto &e : trees) {
        // Pick a loaded texture deterministically.
        int idx = static_cast<int>((e.rng >> 8) % TREE_TEX_COUNT);
        for (int k = 0; k < TREE_TEX_COUNT; ++k) {
            if (_treeTexturesLoaded[(idx + k) % TREE_TEX_COUNT]) {
                idx = (idx + k) % TREE_TEX_COUNT;
                break;
            }
        }
        if (!_treeTexturesLoaded[idx])
            continue;

        // Progressive rotation: angle from board center so diagonal tiles get
        // intermediate angles. atan2(dy,dx)+90° maps "above board" → 0° (upright).
        float dx = (e.x + 0.5f) - bcx;
        float dy = (e.y + 0.5f) - bcy;
        static constexpr float RAD2DEG = 57.29577951f;
        float rotation = std::atan2(dy, dx) * RAD2DEG + 90.f;
        // Small per-tile jitter so the grid doesn't feel perfectly mechanical.
        rotation += static_cast<float>((e.rng >> 24) % 21) - 10.f;

        // Uniform scale: width fits exactly in one tile; height overflows naturally.
        // Small variation (0.85–1.0×) for a natural look.
        float scaleVar = 0.85f + static_cast<float>((e.rng >> 16) % 16) * 0.01f;
        sf::Vector2u ts = _treeTextures[idx].getSize();
        float uniformScale = (l.tile * scaleVar) / static_cast<float>(ts.x);

        sf::Sprite spr(_treeTextures[idx]);
        // Origin at trunk base: horizontally centered, 1/5 from top.
        // Origin at bottom-center: sprite grows upward/outward from the tile anchor.
        spr.setOrigin(ts.x * 0.5f, static_cast<float>(ts.y));
        spr.setScale(uniformScale, uniformScale);
        spr.setPosition(l.offsetX + (e.x + 0.5f) * l.tile,
                        l.offsetY + (e.y + 0.5f) * l.tile);
        spr.setRotation(rotation);
        _window.draw(spr);
    }
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
    if (!_showResources)
        return;
    float radius = l.tile * 0.07f;
    if (radius < 1.5f)
        return;

    static const std::array<sf::Vector2f, RESOURCE_COUNT> RES_POS = {{
        {0.15f, 0.15f}, {0.50f, 0.15f}, {0.78f, 0.15f},
        {0.15f, 0.50f}, {0.78f, 0.50f},
        {0.15f, 0.85f}, {0.50f, 0.85f},
    }};

    float spriteHalf = l.tile * 0.12f;
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
                float cx = cellX + RES_POS[r].x * l.tile;
                float cy = cellY + RES_POS[r].y * l.tile;

                if (_resTexturesLoaded[r]) {
                    sf::Vector2u ts = _resTextures[r].getSize();
                    sf::Sprite spr(_resTextures[r]);
                    spr.setOrigin(ts.x * 0.5f, ts.y * 0.5f);
                    spr.setScale(spriteHalf * 2.f / ts.x, spriteHalf * 2.f / ts.y);
                    spr.setPosition(cx, cy);
                    _window.draw(spr);
                } else {
                    dot.setPosition(cx - radius, cy - radius);
                    dot.setFillColor(resourceColor(static_cast<Resource>(r)));
                    _window.draw(dot);
                }
            }
        }
    }
}

void Renderer::drawResourceCounts(const World &world, const Layout &l)
{
    if (!_fontLoaded || !_showResources)
        return;
    float screenTile = l.tile / _zoom;
    if (screenTile < 48.f)
        return;

    static const std::array<sf::Vector2f, RESOURCE_COUNT> RES_POS = {{
        {0.15f, 0.15f}, {0.50f, 0.15f}, {0.78f, 0.15f},
        {0.15f, 0.50f}, {0.78f, 0.50f},
        {0.15f, 0.85f}, {0.50f, 0.85f},
    }};
    float spriteHalf = l.tile * 0.12f;
    float radius = l.tile * 0.07f;
    float mapW = mapAreaWidth();
    float winH = static_cast<float>(_window.getSize().y);

    for (int y = 0; y < world.height(); ++y) {
        for (int x = 0; x < world.width(); ++x) {
            const Tile &t = world.tile(x, y);
            float cellX = l.offsetX + x * l.tile;
            float cellY = l.offsetY + y * l.tile;
            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                if (t.resources[r] <= 0)
                    continue;
                float cx = cellX + RES_POS[r].x * l.tile;
                float cy = cellY + RES_POS[r].y * l.tile;
                float textX = cx + (_resTexturesLoaded[r] ? spriteHalf : radius);
                float textY = cy + l.tile * 0.02f;

                sf::Vector2i sp = _window.mapCoordsToPixel({textX, textY}, _mapView);
                if (sp.x < 0 || sp.x >= static_cast<int>(mapW) ||
                    sp.y < 0 || sp.y >= static_cast<int>(winH))
                    continue;

                const std::string numStr = std::to_string(t.resources[r]);
                int digits = static_cast<int>(numStr.size());
                float df = (digits <= 1) ? 0.13f : (digits == 2) ? 0.11f : 0.09f;
                unsigned int fontSize = std::max(8u, static_cast<unsigned int>(screenTile * df));
                drawText(numStr, static_cast<float>(sp.x), static_cast<float>(sp.y),
                    fontSize, sf::Color::White);
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

        // Select ghost texture: 0=basic(N/S), 1=right(E/W), 2=special(incantation)
        int texIdx = 0;
        bool mirrorX = false;
        if (p.inIncantation && _ghostTexturesLoaded[2]) {
            texIdx = 2;
        } else if ((p.orientation == 2 || p.orientation == 4) && _ghostTexturesLoaded[1]) {
            texIdx = 1;
            mirrorX = (p.orientation == 4); // West = horizontal mirror
        }

        if (_ghostTexturesLoaded[texIdx]) {
            sf::Vector2u ts = _ghostTextures[texIdx].getSize();
            sf::Sprite spr(_ghostTextures[texIdx]);
            spr.setOrigin(ts.x * 0.5f, ts.y * 0.5f);
            float size = s * 2.f;
            float scaleX = (mirrorX ? -1.f : 1.f) * size / static_cast<float>(ts.x);
            float scaleY = size / static_cast<float>(ts.y);
            spr.setScale(scaleX, scaleY);
            spr.setPosition(cx, cy);
            spr.setColor(teamColor(world, p.team));
            _window.draw(spr);
        } else {
            sf::ConvexShape tri = makeTriangle(cx, cy, s, p.orientation);
            tri.setFillColor(teamColor(world, p.team));
            tri.setOutlineColor(p.inIncantation ? sf::Color::White : sf::Color::Black);
            tri.setOutlineThickness(1.5f);
            _window.draw(tri);
        }

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
