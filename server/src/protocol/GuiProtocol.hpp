#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "server/Client.hpp"
#include "game/Tile.hpp"
#include "game/World.hpp"
#include "game/Player.hpp"
#include "game/GameState.hpp"

class GuiProtocol {
public:
    explicit GuiProtocol(std::unordered_map<int, Client> &clients);

    void toAllGui(const std::string &msg);
    void sendInitialState(Client &gui, const GameState &state, int freq);

    void msz(Client &gui, int w, int h);
    void bct(Client &gui, int x, int y, const Tile &t);
    void bctAll(Client &gui, const World &w);
    void tna(Client &gui, const std::string &teamName);
    void sgt(Client &gui, int freq);
    void suc(Client &gui);
    void sbp(Client &gui);

    void sst(int freq);

    void pnw(const Player &p);
    void ppo(const Player &p);
    void plv(const Player &p);
    void pin(const Player &p);
    void pex(int playerId);
    void pbc(int playerId, const std::string &msg);
    void pic(int x, int y, int level, const std::vector<int> &participants);
    void pie(int x, int y, bool success);
    void pfk(int playerId);
    void pdr(int playerId, int resourceIdx);
    void pgt(int playerId, int resourceIdx);
    void pdi(int playerId);

    void enw(int eggId, int playerId, int x, int y);
    void ebo(int eggId);
    void edi(int eggId);

    void seg(const std::string &teamName);
    void smg(const std::string &msg);

private:
    static std::string tileResources(const Tile &t);

    std::unordered_map<int, Client> &_clients;
};
