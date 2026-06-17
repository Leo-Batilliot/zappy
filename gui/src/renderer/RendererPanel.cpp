/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** RendererPanel.cpp
*/

#include "renderer/Renderer.hpp"
#include "renderer/RendererPrivate.hpp"

#include <algorithm>
#include <string>

void Renderer::drawPanel(const World &world)
{
    sf::Vector2u ws = _window.getSize();
    float x0 = static_cast<float>(ws.x) - PANEL_WIDTH;
    float x = x0 + 14.f;
    float y = 12.f;

    sf::RectangleShape background(sf::Vector2f(PANEL_WIDTH,
        static_cast<float>(ws.y)));
    background.setPosition(x0, 0.f);
    background.setFillColor(sf::Color(24, 24, 34));
    _window.draw(background);

    drawText("ZAPPY", x, y, 20, sf::Color::White);
    y += 34.f;
    drawText("Carte : " + std::to_string(world.width()) + " x "
        + std::to_string(world.height()), x, y, 14, sf::Color(200, 200, 210));
    y += 20.f;
    drawText("Frequence : " + std::to_string(world.frequency())
        + "  (+/- pour changer)", x, y, 14, sf::Color(200, 200, 210));
    y += 20.f;
    drawText("Joueurs : " + std::to_string(world.players().size()),
        x, y, 14, sf::Color(200, 200, 210));
    y += 30.f;

    drawText("Equipes", x, y, 16, sf::Color::White);
    y += 24.f;
    sf::RectangleShape swatch(sf::Vector2f(12.f, 12.f));
    for (const auto &team : world.teams()) {
        int count = 0;
        int maxLevel = 0;
        for (const auto &[id, p] : world.players()) {
            if (p.team != team)
                continue;
            ++count;
            maxLevel = std::max(maxLevel, p.level);
        }
        swatch.setPosition(x, y + 3.f);
        swatch.setFillColor(teamColor(world, team));
        _window.draw(swatch);
        drawText(team + "  (" + std::to_string(count) + "j, niv "
            + std::to_string(maxLevel) + ")", x + 20.f, y, 14,
            sf::Color(200, 200, 210));
        y += 20.f;
    }
    y += 10.f;

    drawText("Ressources", x, y, 16, sf::Color::White);
    y += 24.f;
    sf::CircleShape dot(6.f);
    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        dot.setPosition(x, y + 3.f);
        dot.setFillColor(resourceColor(static_cast<Resource>(r)));
        _window.draw(dot);
        drawText(RESOURCE_NAMES[r], x + 20.f, y, 14, sf::Color(200, 200, 210));
        y += 20.f;
    }
    y += 10.f;

    auto optSelected = world.findPlayer(_selectedPlayer);
    if (optSelected) {
        const Player &selected = optSelected->get();
        drawText("Joueur #" + std::to_string(_selectedPlayer), x, y, 16,
            teamColor(world, selected.team));
        y += 24.f;
        drawText("Equipe : " + selected.team, x, y, 14, sf::Color(200, 200, 210));
        y += 20.f;
        drawText("Niveau : " + std::to_string(selected.level)
            + "   Direction : " + std::string(ORIENTATION_NAMES[
                (selected.orientation >= 1 && selected.orientation <= 4)
                    ? selected.orientation : 0]),
            x, y, 14, sf::Color(200, 200, 210));
        y += 20.f;
        drawText("Position : (" + std::to_string(selected.x) + ", "
            + std::to_string(selected.y) + ")", x, y, 14,
            sf::Color(200, 200, 210));
        y += 20.f;
        drawText("Inventaire :", x, y, 14, sf::Color::White);
        y += 20.f;
        for (int r = 0; r < RESOURCE_COUNT; ++r) {
            drawText(std::string(RESOURCE_NAMES[r]) + " : "
                + std::to_string(selected.inventory[r]),
                x + 16.f, y, 13, sf::Color(190, 190, 200));
            y += 18.f;
        }
    } else if (_selectedTileX >= 0 && _selectedTileX < world.width() &&
        _selectedTileY >= 0 && _selectedTileY < world.height()) {
        const Tile &tile = world.tile(_selectedTileX, _selectedTileY);
        drawText("Tuile (" + std::to_string(_selectedTileX) + ", "
            + std::to_string(_selectedTileY) + ")", x, y, 16, sf::Color::White);
        y += 24.f;
        for (int r = 0; r < RESOURCE_COUNT; ++r) {
            if (tile.resources[r] <= 0)
                continue;
            drawText(std::string(RESOURCE_NAMES[r]) + " : "
                + std::to_string(tile.resources[r]),
                x + 16.f, y, 13, sf::Color(190, 190, 200));
            y += 18.f;
        }
        int playersHere = 0;
        for (const auto &[id, p] : world.players())
            playersHere += (p.x == _selectedTileX && p.y == _selectedTileY);
        int eggsHere = 0;
        for (const auto &[id, e] : world.eggs())
            eggsHere += (e.x == _selectedTileX && e.y == _selectedTileY);
        drawText("Joueurs : " + std::to_string(playersHere)
            + "   Oeufs : " + std::to_string(eggsHere),
            x, y, 14, sf::Color(200, 200, 210));
    }

    const auto &logs = world.logs();
    if (!logs.empty()) {
        float ly = static_cast<float>(ws.y) - 24.f - 16.f * logs.size();
        drawText("Journal", x, ly, 16, sf::Color::White);
        ly += 22.f;
        for (const auto &entry : logs) {
            std::string line = entry.size() > 38
                ? entry.substr(0, 37) + "..." : entry;
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
