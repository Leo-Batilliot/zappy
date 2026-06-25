#include "renderer/Renderer.hpp"
#include "renderer/RendererPrivate.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <string_view>

static constexpr std::array<std::string_view, RESOURCE_COUNT> ASSET_NAMES = {
    "apple", "ore1", "ore2", "ore3", "ore4", "ore5", "ore6",
};

static constexpr std::size_t PALETTE_SIZE = 8;
static const std::array<sf::Color, PALETTE_SIZE> TEAM_PALETTE = {
    sf::Color(220, 80, 80),
    sf::Color(80, 120, 220),
    sf::Color(80, 200, 80),
    sf::Color(220, 180, 50),
    sf::Color(200, 80, 200),
    sf::Color(80, 200, 200),
    sf::Color(220, 130, 50),
    sf::Color(150, 150, 150),
};

Renderer::Renderer(unsigned int width, unsigned int height)
    : _window(sf::VideoMode(width, height), "Zappy GUI")
{
    _window.setFramerateLimit(60);

    for (std::string_view path : std::array<std::string_view, 2>{"gui/assets/DejaVuSans.ttf", "assets/DejaVuSans.ttf"}) {
        if (std::filesystem::exists(path) && _font.loadFromFile(std::string(path))) {
            _fontLoaded = true;
            break;
        }
    }
    if (!_fontLoaded)
        std::cerr << "Police introuvable (gui/assets/DejaVuSans.ttf) : "
                  << "le GUI fonctionnera sans texte." << std::endl;

    for (int r = 0; r < RESOURCE_COUNT; ++r) {
        for (std::string_view base : std::array<std::string_view, 2>{"gui/assets/", "assets/"}) {
            std::string path = std::string(base) + std::string(ASSET_NAMES[r]) + ".png";
            if (std::filesystem::exists(path) && _resTextures[r].loadFromFile(path)) {
                _resTextures[r].setSmooth(true);
                _resTexturesLoaded[r] = true;
                break;
            }
        }
    }

    _windowSize = {width, height};
    _center = {mapAreaWidth() / 2.f, static_cast<float>(height) / 2.f};
    updateMapView();
}

bool Renderer::isOpen() const
{
    return _window.isOpen();
}

float Renderer::mapAreaWidth() const
{
    float width = static_cast<float>(_window.getSize().x) - PANEL_WIDTH;
    return std::max(width, 1.f);
}

Renderer::Layout Renderer::computeLayout(const World &world) const
{
    float availW = mapAreaWidth();
    float availH = static_cast<float>(_window.getSize().y);
    float tile = std::min(availW / world.width(), availH / world.height());
    return {
        (availW - tile * world.width()) / 2.f,
        (availH - tile * world.height()) / 2.f,
        tile,
    };
}

void Renderer::updateMapView()
{
    sf::Vector2u ws = _window.getSize();
    float mapW = mapAreaWidth();
    float mapH = static_cast<float>(ws.y);

    _mapView.setSize(mapW * _zoom, mapH * _zoom);
    _mapView.setCenter(_center);
    _mapView.setViewport(sf::FloatRect(0.f, 0.f, mapW / ws.x, 1.f));
}

void Renderer::render(const World &world, bool connectionLost)
{
    _window.clear(sf::Color(30, 30, 40));
    Layout l = computeLayout(world);

    _window.setView(_mapView);
    drawGrid(world, l);
    drawIncantations(world, l);
    drawSelection(world, l);
    drawResources(world, l);
    drawEggs(world, l);
    drawPlayers(world, l);
    drawEffects(world, l);

    sf::Vector2u ws = _window.getSize();
    _window.setView(sf::View(sf::FloatRect(0.f, 0.f,
        static_cast<float>(ws.x), static_cast<float>(ws.y))));
    drawPanel(world);
    if (world.isGameOver())
        drawGameOver(world);
    if (connectionLost)
        drawConnectionLost();

    _window.display();
}

sf::Color Renderer::teamColor(const World &world, const std::string &team) const
{
    const auto &teams = world.teams();
    for (std::size_t i = 0; i < teams.size(); ++i) {
        if (teams[i] == team)
            return TEAM_PALETTE[i % PALETTE_SIZE];
    }
    return sf::Color::White;
}

void Renderer::drawText(std::string_view text, float x, float y,
    unsigned int size, sf::Color color)
{
    if (!_fontLoaded)
        return;
    sf::Text label(std::string(text), _font, size);
    label.setPosition(x, y);
    label.setFillColor(color);
    _window.draw(label);
}
