/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Types.hpp
*/

#pragma once

#include <array>
#include <chrono>
#include <string>
#include <vector>

enum class Resource {
    FOOD = 0,
    LINEMATE,
    DERAUMERE,
    SIBUR,
    MENDIANE,
    PHIRAS,
    THYSTAME,
    COUNT
};

inline constexpr int RESOURCE_COUNT = static_cast<int>(Resource::COUNT);

struct Tile {
    std::array<int, RESOURCE_COUNT> resources{};
};

struct Player {
    int x = 0;
    int y = 0;
    int orientation = 1;
    int level = 1;
    std::string team;
    std::array<int, RESOURCE_COUNT> inventory{};
    bool inIncantation = false;
};

struct Egg {
    int id = 0;
    int x = 0;
    int y = 0;
    int layerId = -1;
};

struct Incantation {
    int x = 0;
    int y = 0;
    int level = 1;
    std::vector<int> players;
};

enum class EffectType {
    BROADCAST,
    EJECTION,
    INCANTATION_END
};

struct Effect {
    EffectType type;
    int x = 0;
    int y = 0;
    bool success = false;
    std::chrono::steady_clock::time_point start;
};

inline constexpr std::chrono::milliseconds EFFECT_DURATION{2000};
