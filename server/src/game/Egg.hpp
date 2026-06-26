#pragma once

#include <string>
#include "game/Types.hpp"

class Egg {
public:
    int id() const { return _id; }
    void setId(int id) { _id = id; }

    const std::string &team() const { return _team; }
    void setTeam(const std::string &team) { _team = team; }

    Pos pos() const { return _pos; }
    void setPos(Pos pos) { _pos = pos; }

    int layerId() const { return _layerId; }
    void setLayerId(int layerId) { _layerId = layerId; }

private:
    int _id = 0;
    std::string _team;
    Pos _pos{};
    int _layerId = -1;
};
