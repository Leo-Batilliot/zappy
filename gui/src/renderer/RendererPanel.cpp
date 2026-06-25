/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** RendererPanel.cpp
*/

#include "renderer/Renderer.hpp"
#include "renderer/RendererPrivate.hpp"

#include <algorithm>
#include <array>
#include <string>

void Renderer::drawResourceIcon(int r, float cx, float cy, float size)
{
    if (_resTexturesLoaded[r]) {
        sf::Vector2u ts = _resTextures[r].getSize();
        sf::Sprite spr(_resTextures[r]);
        spr.setOrigin(ts.x * 0.5f, ts.y * 0.5f);
        spr.setScale(size / ts.x, size / ts.y);
        spr.setPosition(cx, cy);
        _window.draw(spr);
    } else {
        sf::CircleShape dot(size * 0.5f);
        dot.setOrigin(size * 0.5f, size * 0.5f);
        dot.setPosition(cx, cy);
        dot.setFillColor(resourceColor(static_cast<Resource>(r)));
        _window.draw(dot);
    }
}

void Renderer::handlePanelClick(int py)
{
    constexpr float HIT = 22.f;
    for (int i = 0; i < 4; ++i) {
        if (_secHeaderY[i] > 0.f &&
            py >= _secHeaderY[i] && py < _secHeaderY[i] + HIT) {
            _secExpanded[i] = !_secExpanded[i];
            return;
        }
    }
}

void Renderer::drawPanel(const World &world)
{
    sf::Vector2u ws = _window.getSize();
    float x0 = static_cast<float>(ws.x) - PANEL_WIDTH;
    float x  = x0 + 14.f;
    float y  = 12.f;

    sf::RectangleShape background(sf::Vector2f(PANEL_WIDTH, static_cast<float>(ws.y)));
    background.setPosition(x0, 0.f);
    background.setFillColor(sf::Color(24, 24, 34));
    _window.draw(background);

    auto drawSep = [&]() {
        sf::RectangleShape sep(sf::Vector2f(PANEL_WIDTH - 28.f, 1.f));
        sep.setFillColor(sf::Color(60, 60, 80));
        sep.setPosition(x, y);
        _window.draw(sep);
        y += 8.f;
    };

    auto drawTri = [&](float tx, float ty, bool expanded) {
        sf::ConvexShape tri(3);
        if (expanded) {
            tri.setPoint(0, {tx,        ty + 2.f});
            tri.setPoint(1, {tx + 10.f, ty + 2.f});
            tri.setPoint(2, {tx + 5.f,  ty + 10.f});
        } else {
            tri.setPoint(0, {tx,       ty});
            tri.setPoint(1, {tx + 8.f, ty + 5.f});
            tri.setPoint(2, {tx,       ty + 10.f});
        }
        tri.setFillColor(sf::Color(160, 160, 190));
        _window.draw(tri);
    };

    auto drawHeader = [&](const std::string &title, int idx) {
        drawSep();
        _secHeaderY[idx] = y;
        drawTri(x, y + 4.f, _secExpanded[idx]);
        drawText(title, x + 16.f, y, 15, sf::Color(230, 230, 255));
        y += 22.f;
    };

    drawText("ZAPPY", x, y, 20, sf::Color::White);
    y += 30.f;
    drawText("Carte : " + std::to_string(world.width()) + " x "
        + std::to_string(world.height()), x, y, 13, sf::Color(200, 200, 210));
    y += 18.f;
    drawText("Joueurs : " + std::to_string(world.players().size()),
        x, y, 13, sf::Color(200, 200, 210));
    y += 22.f;

    drawHeader("Equipes", 0);
    if (_secExpanded[0]) {
        sf::RectangleShape swatch(sf::Vector2f(12.f, 12.f));
        for (const auto &team : world.teams()) {
            int count = 0, maxLevel = 0;
            for (const auto &[id, p] : world.players()) {
                if (p.team != team) continue;
                ++count;
                maxLevel = std::max(maxLevel, p.level);
            }
            swatch.setPosition(x, y + 3.f);
            swatch.setFillColor(teamColor(world, team));
            _window.draw(swatch);
            drawText(team + "  (" + std::to_string(count) + " joueurs, niv "
                + std::to_string(maxLevel) + ")", x + 20.f, y, 13,
                sf::Color(200, 200, 210));
            y += 18.f;
        }
        y += 6.f;
    }

    std::array<int, RESOURCE_COUNT> totals{};
    for (int ty2 = 0; ty2 < world.height(); ++ty2)
        for (int tx2 = 0; tx2 < world.width(); ++tx2)
            for (int r = 0; r < RESOURCE_COUNT; ++r)
                totals[r] += world.tile(tx2, ty2).resources[r];

    drawHeader("Ressources", 1);
    if (_secExpanded[1]) {
        for (int r = 0; r < RESOURCE_COUNT; ++r) {
            drawResourceIcon(r, x + 8.f, y + 7.f, 13.f);
            drawText(std::string(RESOURCE_NAMES[r]) + " : " + std::to_string(totals[r]),
                x + 22.f, y, 13, sf::Color(200, 200, 210));
            y += 18.f;
        }
        y += 6.f;
    }

    drawHeader("Controles", 2);
    if (_secExpanded[2]) {
        sf::Color kKey(220, 220, 100);
        sf::Color kDesc(180, 180, 195);
        sf::Color kOn(100, 210, 100);
        sf::Color kOff(200, 90, 90);

        auto drawBind = [&](const std::string &key, const std::string &desc, sf::Color dc) {
            drawText(key,  x,        y, 12, kKey);
            drawText(desc, x + 56.f, y, 12, dc);
            y += 15.f;
        };

        drawBind("Espace",  "Recentrer",         kDesc);
        drawBind("Fleches", "Deplacer",           kDesc);
        drawBind("Scroll",  "Zoom",               kDesc);
        drawBind("+  /  -", "Vitesse  ["
            + std::to_string(world.frequency()) + "]", kDesc);
        drawBind("R", std::string("Ressources [")
            + (_showResources ? "ON" : "OFF") + "]",
            _showResources ? kOn : kOff);
        drawBind("T", std::string("Arbres [")
            + (_showTrees ? "ON" : "OFF") + "]",
            _showTrees ? kOn : kOff);
        drawBind("Echap",   "Quitter",            kDesc);
        y += 4.f;
    }

    auto optSelected = world.findPlayer(_selectedPlayer);
    bool hasSelection = optSelected.has_value() ||
        (_selectedTileX >= 0 && _selectedTileX < world.width() &&
         _selectedTileY >= 0 && _selectedTileY < world.height());

    std::string selTitle = "Selection";
    if (optSelected)
        selTitle = "Joueur #" + std::to_string(_selectedPlayer);
    else if (hasSelection)
        selTitle = "Tuile (" + std::to_string(_selectedTileX)
            + ", " + std::to_string(_selectedTileY) + ")";

    drawHeader(selTitle, 3);
    if (_secExpanded[3]) {
        if (optSelected) {
            const Player &p = optSelected->get();
            drawText("Equipe : " + p.team, x, y, 13,
                teamColor(world, p.team));
            y += 17.f;
            drawText("Niv : " + std::to_string(p.level)
                + "  Dir : " + std::string(ORIENTATION_NAMES[
                    (p.orientation >= 1 && p.orientation <= 4)
                        ? p.orientation : 0]),
                x, y, 13, sf::Color(200, 200, 210));
            y += 17.f;
            drawText("Pos : (" + std::to_string(p.x) + ", "
                + std::to_string(p.y) + ")", x, y, 13,
                sf::Color(200, 200, 210));
            y += 17.f;
            drawText("Inventaire :", x, y, 13, sf::Color::White);
            y += 17.f;
            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                drawResourceIcon(r, x + 7.f, y + 6.f, 12.f);
                drawText(std::string(RESOURCE_NAMES[r]) + " : "
                    + std::to_string(p.inventory[r]),
                    x + 20.f, y, 12, sf::Color(190, 190, 200));
                y += 16.f;
            }
        } else if (hasSelection) {
            const Tile &tile = world.tile(_selectedTileX, _selectedTileY);
            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                if (tile.resources[r] <= 0) continue;
                drawResourceIcon(r, x + 7.f, y + 6.f, 12.f);
                drawText(std::string(RESOURCE_NAMES[r]) + " : "
                    + std::to_string(tile.resources[r]),
                    x + 20.f, y, 12, sf::Color(190, 190, 200));
                y += 16.f;
            }
            int playersHere = 0;
            for (const auto &[id, pl] : world.players())
                playersHere += (pl.x == _selectedTileX && pl.y == _selectedTileY);
            int eggsHere = 0;
            for (const auto &[id, e] : world.eggs())
                eggsHere += (e.x == _selectedTileX && e.y == _selectedTileY);
            drawText("Joueurs : " + std::to_string(playersHere)
                + "   Oeufs : " + std::to_string(eggsHere),
                x, y, 13, sf::Color(200, 200, 210));
        } else {
            drawText("Aucune selection", x, y, 12, sf::Color(120, 120, 140));
        }
    }

    const auto &logs = world.logs();
    if (!logs.empty()) {
        float ly = static_cast<float>(ws.y) - 24.f - 16.f * logs.size();
        drawText("Journal", x, ly, 15, sf::Color(230, 230, 255));
        ly += 20.f;
        for (const auto &entry : logs) {
            std::string line = entry.size() > 38 ? entry.substr(0, 37) + "..." : entry;
            drawText(line, x, ly, 12, sf::Color(180, 180, 195));
            ly += 16.f;
        }
    }
}

void Renderer::drawConnectionLost()
{
    float width = static_cast<float>(_window.getSize().x);
    sf::RectangleShape strip(sf::Vector2f(width, 36.f));
    strip.setFillColor(sf::Color(180, 40, 40, 230));
    _window.draw(strip);
    drawText("Connexion au serveur perdue", 14.f, 7.f, 18, sf::Color::White);
}

void Renderer::drawGameOver(const World &world)
{
    sf::Vector2f size(_window.getSize());

    sf::RectangleShape veil(size);
    veil.setFillColor(sf::Color(0, 0, 0, 150));
    _window.draw(veil);

    sf::RectangleShape banner(sf::Vector2f(size.x, size.y * 0.2f));
    banner.setPosition(0.f, size.y * 0.4f);
    banner.setFillColor(teamColor(world, world.winner()));
    banner.setOutlineThickness(4.f);
    banner.setOutlineColor(sf::Color::White);
    _window.draw(banner);

    if (_fontLoaded) {
        sf::Text label("Vainqueur : " + world.winner(), _font, 36);
        sf::FloatRect bounds = label.getLocalBounds();
        label.setPosition((size.x - bounds.width) / 2.f,
            size.y * 0.5f - bounds.height);
        label.setFillColor(sf::Color::Black);
        _window.draw(label);
    }
}
