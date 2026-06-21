/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** RendererPrivate.hpp
*/

#pragma once

#include <array>
#include <string_view>
#include <SFML/Graphics.hpp>
#include "world/Types.hpp"

static constexpr float PANEL_WIDTH = 300.f;

static constexpr std::array<std::string_view, RESOURCE_COUNT> RESOURCE_NAMES = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame",
};

static constexpr std::array<std::string_view, 5> ORIENTATION_NAMES = {
    "?", "N", "E", "S", "O",
};

inline sf::Color resourceColor(Resource resource)
{
    switch (resource) {
        case Resource::FOOD:      return sf::Color(230, 160, 60);
        case Resource::LINEMATE:  return sf::Color(220, 220, 220);
        case Resource::DERAUMERE: return sf::Color(90, 140, 230);
        case Resource::SIBUR:     return sf::Color(160, 110, 220);
        case Resource::MENDIANE:  return sf::Color(230, 215, 90);
        case Resource::PHIRAS:    return sf::Color(90, 200, 190);
        case Resource::THYSTAME:  return sf::Color(225, 100, 175);
        default:                  return sf::Color::White;
    }
}
