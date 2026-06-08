#include "protocol/GuiProtocol.hpp"

namespace {

std::string fmtPnw(const Player &p)
{
    return "pnw #" + std::to_string(p.id) + " "
         + std::to_string(p.pos.x) + " " + std::to_string(p.pos.y) + " "
         + std::to_string(static_cast<int>(p.orient)) + " "
         + std::to_string(p.level) + " " + p.teamName + "\n";
}

std::string fmtPin(const Player &p)
{
    std::string out = "pin #" + std::to_string(p.id) + " "
                    + std::to_string(p.pos.x) + " " + std::to_string(p.pos.y);
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        out += " " + std::to_string(p.inventory[i]);
    out += "\n";
    return out;
}

std::string fmtPlv(const Player &p)
{
    return "plv #" + std::to_string(p.id) + " " + std::to_string(p.level) + "\n";
}

std::string fmtPpo(const Player &p)
{
    return "ppo #" + std::to_string(p.id) + " "
         + std::to_string(p.pos.x) + " " + std::to_string(p.pos.y) + " "
         + std::to_string(static_cast<int>(p.orient)) + "\n";
}

std::string fmtEnw(int eggId, int playerId, int x, int y)
{
    return "enw #" + std::to_string(eggId) + " #" + std::to_string(playerId) + " "
         + std::to_string(x) + " " + std::to_string(y) + "\n";
}

std::string fmtBct(int x, int y, const std::string &resources)
{
    return "bct " + std::to_string(x) + " " + std::to_string(y) + " " + resources + "\n";
}

}

GuiProtocol::GuiProtocol(std::unordered_map<int, Client> &clients)
    : _clients(clients)
{
}

std::string GuiProtocol::tileResources(const Tile &t)
{
    std::string out;
    for (int i = 0; i < RESOURCE_COUNT; ++i) {
        if (i)
            out += " ";
        out += std::to_string(t.resources[i]);
    }
    return out;
}

void GuiProtocol::toAllGui(const std::string &msg)
{
    for (auto &[fd, client] : _clients) {
        (void)fd;
        if (client.state() == Client::State::Gui)
            client.queueWrite(msg);
    }
}

void GuiProtocol::sendInitialState(Client &gui, const GameState &state, int freq)
{
    const World &w = state.world();

    gui.queueWrite("msz " + std::to_string(w.width) + " " + std::to_string(w.height) + "\n");
    gui.queueWrite("sgt " + std::to_string(freq) + "\n");
    for (const auto &[name, team] : state.teams()) {
        (void)name;
        gui.queueWrite("tna " + team.name + "\n");
    }
    for (int y = 0; y < w.height; ++y)
        for (int x = 0; x < w.width; ++x)
            gui.queueWrite(fmtBct(x, y, tileResources(w.at(x, y))));
    for (const auto &[id, p] : state.players()) {
        (void)id;
        gui.queueWrite(fmtPnw(p));
        gui.queueWrite(fmtPin(p));
        gui.queueWrite(fmtPlv(p));
    }
    for (const auto &[id, e] : state.eggs()) {
        (void)id;
        gui.queueWrite(fmtEnw(e.id, e.layerId, e.pos.x, e.pos.y));
    }
}

void GuiProtocol::msz(Client &gui, int w, int h)
{
    gui.queueWrite("msz " + std::to_string(w) + " " + std::to_string(h) + "\n");
}

void GuiProtocol::bct(Client &gui, int x, int y, const Tile &t)
{
    gui.queueWrite(fmtBct(x, y, tileResources(t)));
}

void GuiProtocol::bctAll(Client &gui, const World &w)
{
    for (int y = 0; y < w.height; ++y)
        for (int x = 0; x < w.width; ++x)
            gui.queueWrite(fmtBct(x, y, tileResources(w.at(x, y))));
}

void GuiProtocol::tna(Client &gui, const std::string &teamName)
{
    gui.queueWrite("tna " + teamName + "\n");
}

void GuiProtocol::sgt(Client &gui, int freq)
{
    gui.queueWrite("sgt " + std::to_string(freq) + "\n");
}

void GuiProtocol::suc(Client &gui)
{
    gui.queueWrite("suc\n");
}

void GuiProtocol::sbp(Client &gui)
{
    gui.queueWrite("sbp\n");
}

void GuiProtocol::sst(int freq)
{
    toAllGui("sst " + std::to_string(freq) + "\n");
}

void GuiProtocol::pnw(const Player &p)
{
    toAllGui(fmtPnw(p));
}

void GuiProtocol::ppo(const Player &p)
{
    toAllGui(fmtPpo(p));
}

void GuiProtocol::plv(const Player &p)
{
    toAllGui(fmtPlv(p));
}

void GuiProtocol::pin(const Player &p)
{
    toAllGui(fmtPin(p));
}

void GuiProtocol::pex(int playerId)
{
    toAllGui("pex #" + std::to_string(playerId) + "\n");
}

void GuiProtocol::pbc(int playerId, const std::string &msg)
{
    toAllGui("pbc #" + std::to_string(playerId) + " " + msg + "\n");
}

void GuiProtocol::pic(int x, int y, int level, const std::vector<int> &participants)
{
    std::string out = "pic " + std::to_string(x) + " " + std::to_string(y) + " "
                    + std::to_string(level);
    for (int id : participants)
        out += " #" + std::to_string(id);
    out += "\n";
    toAllGui(out);
}

void GuiProtocol::pie(int x, int y, bool success)
{
    toAllGui("pie " + std::to_string(x) + " " + std::to_string(y) + " "
             + (success ? "1" : "0") + "\n");
}

void GuiProtocol::pfk(int playerId)
{
    toAllGui("pfk #" + std::to_string(playerId) + "\n");
}

void GuiProtocol::pdr(int playerId, int resourceIdx)
{
    toAllGui("pdr #" + std::to_string(playerId) + " " + std::to_string(resourceIdx) + "\n");
}

void GuiProtocol::pgt(int playerId, int resourceIdx)
{
    toAllGui("pgt #" + std::to_string(playerId) + " " + std::to_string(resourceIdx) + "\n");
}

void GuiProtocol::pdi(int playerId)
{
    toAllGui("pdi #" + std::to_string(playerId) + "\n");
}

void GuiProtocol::enw(int eggId, int playerId, int x, int y)
{
    toAllGui(fmtEnw(eggId, playerId, x, y));
}

void GuiProtocol::ebo(int eggId)
{
    toAllGui("ebo #" + std::to_string(eggId) + "\n");
}

void GuiProtocol::edi(int eggId)
{
    toAllGui("edi #" + std::to_string(eggId) + "\n");
}

void GuiProtocol::seg(const std::string &teamName)
{
    toAllGui("seg " + teamName + "\n");
}

void GuiProtocol::smg(const std::string &msg)
{
    toAllGui("smg " + msg + "\n");
}
