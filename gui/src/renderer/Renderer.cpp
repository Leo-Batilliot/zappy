#include "Renderer.hpp"

#include <algorithm>

static const sf::Color TEAM_PALETTE[] = {
    sf::Color(220, 80, 80),
    sf::Color(80, 120, 220),
    sf::Color(80, 200, 80),
    sf::Color(220, 180, 50),
    sf::Color(200, 80, 200),
    sf::Color(80, 200, 200),
    sf::Color(220, 130, 50),
    sf::Color(150, 150, 150),
};
static constexpr int PALETTE_SIZE = 8;

static sf::Color resourceColor(int resource)
{
    switch (resource) {
        case FOOD:      return sf::Color(230, 160, 60);
        case LINEMATE:  return sf::Color(220, 220, 220);
        case DERAUMERE: return sf::Color(90, 140, 230);
        case SIBUR:     return sf::Color(160, 110, 220);
        case MENDIANE:  return sf::Color(230, 215, 90);
        case PHIRAS:    return sf::Color(90, 200, 190);
        case THYSTAME:  return sf::Color(225, 100, 175);
        default:        return sf::Color::White;
    }
}

static sf::ConvexShape makeTriangle(float cx, float cy, float s, int orientation)
{
    sf::ConvexShape tri(3);
    switch (orientation) {
        case 1: // Nord
            tri.setPoint(0, {cx, cy - s});
            tri.setPoint(1, {cx - s * 0.6f, cy + s * 0.5f});
            tri.setPoint(2, {cx + s * 0.6f, cy + s * 0.5f});
            break;
        case 2: // Est
            tri.setPoint(0, {cx + s, cy});
            tri.setPoint(1, {cx - s * 0.5f, cy - s * 0.6f});
            tri.setPoint(2, {cx - s * 0.5f, cy + s * 0.6f});
            break;
        case 3: // Sud
            tri.setPoint(0, {cx, cy + s});
            tri.setPoint(1, {cx - s * 0.6f, cy - s * 0.5f});
            tri.setPoint(2, {cx + s * 0.6f, cy - s * 0.5f});
            break;
        default: // Ouest
            tri.setPoint(0, {cx - s, cy});
            tri.setPoint(1, {cx + s * 0.5f, cy - s * 0.6f});
            tri.setPoint(2, {cx + s * 0.5f, cy + s * 0.6f});
            break;
    }
    return tri;
}

Renderer::Renderer(unsigned int width, unsigned int height)
    : _window(sf::VideoMode(width, height), "Zappy GUI")
{
    _window.setFramerateLimit(60);
}

bool Renderer::isOpen() const
{
    return _window.isOpen();
}

void Renderer::handleEvents()
{
    sf::Event event;
    while (_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            _window.close();
        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::Escape)
            _window.close();
    }
}

sf::Color Renderer::teamColor(const World &world, const std::string &team) const
{
    const auto &teams = world.teams();
    for (int i = 0; i < static_cast<int>(teams.size()); ++i) {
        if (teams[i] == team)
            return TEAM_PALETTE[i % PALETTE_SIZE];
    }
    return sf::Color::White;
}

void Renderer::render(const World &world)
{
    _window.clear(sf::Color(30, 30, 40));

    sf::Vector2u windowSize = _window.getSize();
    float tile = std::min(
        static_cast<float>(windowSize.x) / world.width(),
        static_cast<float>(windowSize.y) / world.height());
    float offsetX = (windowSize.x - tile * world.width()) / 2.f;
    float offsetY = (windowSize.y - tile * world.height()) / 2.f;

    drawGrid(world, offsetX, offsetY, tile);
    drawResources(world, offsetX, offsetY, tile);
    drawPlayers(world, offsetX, offsetY, tile);

    _window.display();
}

void Renderer::drawGrid(const World &world, float offsetX, float offsetY, float tile)
{
    sf::RectangleShape cell(sf::Vector2f(tile - 1.f, tile - 1.f));
    cell.setOutlineThickness(1.f);
    cell.setOutlineColor(sf::Color(60, 70, 60));

    for (int y = 0; y < world.height(); ++y) {
        for (int x = 0; x < world.width(); ++x) {
            cell.setPosition(offsetX + x * tile, offsetY + y * tile);
            bool dark = (x + y) % 2 == 0;
            cell.setFillColor(dark ? sf::Color(45, 80, 55) : sf::Color(55, 95, 65));
            _window.draw(cell);
        }
    }
}

void Renderer::drawResources(const World &world, float offsetX, float offsetY, float tile)
{
    float radius = tile * 0.07f;
    if (radius < 1.5f)
        return;

    sf::CircleShape dot(radius);
    dot.setOutlineThickness(0.f);

    for (int y = 0; y < world.height(); ++y) {
        for (int x = 0; x < world.width(); ++x) {
            const Tile &t = world.tile(x, y);
            float cellX = offsetX + x * tile;
            float cellY = offsetY + y * tile;
            for (int r = 0; r < RESOURCE_COUNT; ++r) {
                if (t.resources[r] <= 0)
                    continue;
                int col = r % 4;
                int row = r / 4;
                float cx = cellX + tile * 0.2f + col * tile * 0.2f;
                float cy = cellY + tile * 0.3f + row * tile * 0.3f;
                dot.setPosition(cx - radius, cy - radius);
                dot.setFillColor(resourceColor(r));
                _window.draw(dot);
            }
        }
    }
}

void Renderer::drawPlayers(const World &world, float offsetX, float offsetY, float tile)
{
    float s = tile * 0.35f;
    if (s < 2.f)
        return;

    for (const auto &[id, p] : world.players()) {
        float cx = offsetX + p.x * tile + tile / 2.f;
        float cy = offsetY + p.y * tile + tile / 2.f;

        sf::Color color = teamColor(world, p.team);
        sf::ConvexShape tri = makeTriangle(cx, cy, s, p.orientation);
        tri.setFillColor(color);
        tri.setOutlineColor(sf::Color::Black);
        tri.setOutlineThickness(1.5f);
        _window.draw(tri);
    }
}
