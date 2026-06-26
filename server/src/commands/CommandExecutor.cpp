#include "commands/CommandExecutor.hpp"
#include "server/Server.hpp"
#include "protocol/AiProtocol.hpp"
#include "time/Clock.hpp"
#include <sstream>

static constexpr std::size_t MAX_QUEUE = 10;

namespace {

std::string formatTile(const GameState &state, Pos pos)
{
    const Tile &tile = state.world().at(pos.x(), pos.y());
    std::vector<std::string> tokens;

    for (std::size_t i = 0; i < tile.players().size(); ++i)
        tokens.emplace_back("player");
    for (int r = 0; r < RESOURCE_COUNT; ++r)
        for (int c = 0; c < tile.resources()[r]; ++c)
            tokens.emplace_back(resourceName(r));

    std::string out;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i)
            out += " ";
        out += tokens[i];
    }
    return out;
}

int resourceIndex(const std::string &name)
{
    for (int i = 0; i < RESOURCE_COUNT; ++i)
        if (name == resourceName(i))
            return i;
    return -1;
}

Orientation turnRight(Orientation o)
{
    return static_cast<Orientation>((static_cast<int>(o) % 4) + 1);
}

Orientation turnLeft(Orientation o)
{
    return static_cast<Orientation>(((static_cast<int>(o) + 2) % 4) + 1);
}

struct ElevationReq {
    std::size_t players;
    int stones[RESOURCE_COUNT];
};

const ElevationReq ELEV[8] = {
    {0, {0, 0, 0, 0, 0, 0, 0}},
    {1, {0, 1, 0, 0, 0, 0, 0}},
    {2, {0, 1, 1, 1, 0, 0, 0}},
    {2, {0, 2, 0, 1, 0, 2, 0}},
    {4, {0, 1, 1, 2, 0, 1, 0}},
    {4, {0, 1, 2, 1, 3, 0, 0}},
    {6, {0, 1, 2, 3, 0, 1, 0}},
    {6, {0, 2, 2, 2, 2, 2, 1}},
};

bool incantationReady(int level, const Tile &tile, std::size_t playerCount)
{
    if (level < 1 || level > 7)
        return false;
    const ElevationReq &req = ELEV[level];
    if (playerCount < req.players)
        return false;
    for (int r = 1; r < RESOURCE_COUNT; ++r)
        if (tile.resources()[r] < req.stones[r])
            return false;
    return true;
}

std::string join(const std::vector<std::string> &parts)
{
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i)
            out += " ";
        out += parts[i];
    }
    return out;
}

int sign(int v)
{
    return (v > 0) - (v < 0);
}

Pos forwardDelta(Orientation o)
{
    switch (o) {
        case Orientation::North: return {0, -1};
        case Orientation::East:  return {1, 0};
        case Orientation::South: return {0, 1};
        case Orientation::West:  return {-1, 0};
    }
    return {0, 0};
}

int soundDirection(Orientation recv, int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return 0;

    int fwdX = 0, fwdY = 0, rgtX = 0, rgtY = 0;
    switch (recv) {
        case Orientation::North: fwdX = 0; fwdY = -1; rgtX = 1; rgtY = 0; break;
        case Orientation::East:  fwdX = 1; fwdY = 0; rgtX = 0; rgtY = 1; break;
        case Orientation::South: fwdX = 0; fwdY = 1; rgtX = -1; rgtY = 0; break;
        case Orientation::West:  fwdX = -1; fwdY = 0; rgtX = 0; rgtY = -1; break;
    }

    int f = sign(dx * fwdX + dy * fwdY);
    int r = -sign(dx * rgtX + dy * rgtY);

    if (f > 0 && r == 0) return 1;
    if (f > 0 && r > 0)  return 2;
    if (f == 0 && r > 0) return 3;
    if (f < 0 && r > 0)  return 4;
    if (f < 0 && r == 0) return 5;
    if (f < 0 && r < 0)  return 6;
    if (f == 0 && r < 0) return 7;
    return 8;
}

}

CommandExecutor::CommandExecutor(Server &server) : _server(server)
{
    registerCommands();
}

void CommandExecutor::registerCommands()
{
    auto ack = [this](int id, const std::vector<std::string> &) {
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto forward = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        Player &p = st.player(id);
        Pos from = p.pos();
        Pos to = st.world().step(from, p.orient(), 1);
        st.world().movePlayer(id, from, to);
        p.setPos(to);
        _server.gui().ppo(p);
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto right = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        Player &p = st.player(id);
        p.setOrient(turnRight(p.orient()));
        _server.gui().ppo(p);
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto left = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        Player &p = st.player(id);
        p.setOrient(turnLeft(p.orient()));
        _server.gui().ppo(p);
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto look = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        const Player &p = st.player(id);
        std::vector<Pos> tiles = st.world().visibleTiles(p.pos(), p.orient(), p.level());
        std::vector<std::string> rendered;
        rendered.reserve(tiles.size());
        for (const Pos &t : tiles)
            rendered.push_back(formatTile(st, t));
        _server.sendToPlayer(id, AiProtocol::look(rendered));
    };

    auto inventory = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        _server.sendToPlayer(id, AiProtocol::inventory(st.player(id)));
    };

    auto connectNbr = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        int slots = st.team(st.player(id).teamName()).freeSlots();
        _server.sendToPlayer(id, AiProtocol::connectNbr(slots));
    };

    auto take = [this](int id, const std::vector<std::string> &args) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        int r = args.empty() ? -1 : resourceIndex(args[0]);
        Player &p = st.player(id);
        Tile &tile = st.world().at(p.pos().x(), p.pos().y());
        if (r < 0 || tile.resources()[r] <= 0) {
            _server.sendToPlayer(id, AiProtocol::ko());
            return;
        }
        tile.resources()[r] -= 1;
        p.inventory()[r] += 1;
        if (r == static_cast<int>(Resource::Food))
            p.setLifeUnits(p.lifeUnits() + FOOD_LIFE_UNITS);
        _server.gui().pgt(id, r);
        _server.gui().bct(p.pos().x(), p.pos().y(), tile);
        _server.gui().pin(p);
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto set = [this](int id, const std::vector<std::string> &args) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        int r = args.empty() ? -1 : resourceIndex(args[0]);
        Player &p = st.player(id);
        if (r < 0 || p.inventory()[r] <= 0) {
            _server.sendToPlayer(id, AiProtocol::ko());
            return;
        }
        p.inventory()[r] -= 1;
        if (r == static_cast<int>(Resource::Food))
            p.setLifeUnits(p.lifeUnits() - FOOD_LIFE_UNITS);
        Tile &tile = st.world().at(p.pos().x(), p.pos().y());
        tile.resources()[r] += 1;
        _server.gui().pdr(id, r);
        _server.gui().bct(p.pos().x(), p.pos().y(), tile);
        _server.gui().pin(p);
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto broadcast = [this](int id, const std::vector<std::string> &args) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        std::string text = join(args);
        Pos src = st.player(id).pos();

        _server.gui().pbc(id, text);
        for (const auto &[pid, p] : st.players()) {
            if (pid == id)
                continue;
            Pos d = st.world().shortestDelta(p.pos(), src);
            int k = soundDirection(p.orient(), d.x(), d.y());
            _server.sendToPlayer(pid, AiProtocol::message(k, text));
        }
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    auto eject = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        Player &ejector = st.player(id);
        Pos from = ejector.pos();
        Pos to = st.world().step(from, ejector.orient(), 1);
        Pos fwd = forwardDelta(ejector.orient());

        Tile &tile = st.world().at(from.x(), from.y());
        std::vector<int> others;
        for (int pid : tile.players())
            if (pid != id && st.hasPlayer(pid))
                others.push_back(pid);
        std::vector<int> eggsHere(tile.eggs().begin(), tile.eggs().end());

        for (int pid : others) {
            Player &victim = st.player(pid);
            st.world().movePlayer(pid, from, to);
            victim.setPos(to);
            int k = soundDirection(victim.orient(), -fwd.x(), -fwd.y());
            _server.sendToPlayer(pid, AiProtocol::eject(k));
            _server.gui().ppo(victim);
        }

        for (int eid : eggsHere) {
            if (!st.hasEgg(eid))
                continue;
            std::string team = st.egg(eid).team();
            st.removeEgg(eid);
            if (st.isTeam(team) && st.team(team).freeSlots() > 0)
                st.team(team).setFreeSlots(st.team(team).freeSlots() - 1);
            _server.gui().edi(eid);
        }

        if (!others.empty()) {
            _server.gui().pex(id);
            _server.sendToPlayer(id, AiProtocol::ok());
        } else {
            _server.sendToPlayer(id, AiProtocol::ko());
        }
    };

    auto forkStart = [this](int id, const std::vector<std::string> &) {
        if (_server.state().hasPlayer(id))
            _server.gui().pfk(id);
    };
    auto fork = [this](int id, const std::vector<std::string> &) {
        GameState &st = _server.state();
        if (!st.hasPlayer(id))
            return;
        Player &p = st.player(id);
        int eggId = st.createEgg(p.teamName(), p.pos(), id);
        st.team(p.teamName()).setFreeSlots(st.team(p.teamName()).freeSlots() + 1);
        const Egg &e = st.egg(eggId);
        _server.gui().enw(eggId, id, e.pos().x(), e.pos().y());
        _server.sendToPlayer(id, AiProtocol::ok());
    };

    _table = {
        {"Forward", {7, forward}},
        {"Right", {7, right}},
        {"Left", {7, left}},
        {"Look", {7, look}},
        {"Inventory", {1, inventory}},
        {"Connect_nbr", {0, connectNbr}},
        {"Take", {7, take}},
        {"Set", {7, set}},
        {"Fork", {42, fork, forkStart}},
        {"Incantation", {300, ack}},
        {"Broadcast", {7, broadcast}},
        {"Eject", {7, eject}},
    };
}

void CommandExecutor::dispatch(int playerId, const std::string &line)
{
    if (!_server.state().hasPlayer(playerId))
        return;

    Player &p = _server.state().player(playerId);
    std::size_t inFlight = (p.state() == Player::State::Busy ||
                            p.state() == Player::State::Incanting) ? 1 : 0;
    if (p.cmdQueue().size() + inFlight >= MAX_QUEUE)
        return;
    p.cmdQueue().push_back(line);
}

void CommandExecutor::pumpPlayerQueues()
{
    std::vector<int> ids;
    ids.reserve(_server.state().players().size());
    for (const auto &[id, p] : _server.state().players()) {
        (void)p;
        ids.push_back(id);
    }

    for (int id : ids) {
        if (!_server.state().hasPlayer(id))
            continue;
        Player &p = _server.state().player(id);
        if (p.state() == Player::State::Idle && !p.cmdQueue().empty())
            startNext(id);
    }
}

void CommandExecutor::startNext(int playerId)
{
    Player &p = _server.state().player(playerId);

    std::string line = p.cmdQueue().front();
    p.cmdQueue().pop_front();

    std::vector<std::string> tokens = tokenize(line);
    if (tokens.empty())
        return;

    auto it = _table.find(tokens[0]);
    if (it == _table.end()) {
        _server.sendToPlayer(playerId, AiProtocol::ko());
        return;
    }

    const Command &cmd = it->second;
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    if (tokens[0] == "Incantation") {
        startIncantation(playerId);
        return;
    }

    if (cmd.durationUnits == 0) {
        cmd.handler(playerId, args);
        return;
    }

    p.setState(Player::State::Busy);
    if (cmd.onStart)
        cmd.onStart(playerId, args);
    TimePoint due = deadlineFor(cmd.durationUnits, _server.freq());
    Handler handler = cmd.handler;

    p.setCurrentEventId(_server.scheduler().schedule(due, [this, playerId, args, handler]() {
        if (!_server.state().hasPlayer(playerId))
            return;
        handler(playerId, args);
        if (_server.state().hasPlayer(playerId)) {
            Player &pl = _server.state().player(playerId);
            pl.setState(Player::State::Idle);
            pl.setCurrentEventId(0);
        }
    }));
}

void CommandExecutor::startIncantation(int playerId)
{
    GameState &st = _server.state();
    Player &issuer = st.player(playerId);
    Pos pos = issuer.pos();
    int level = issuer.level();
    Tile &tile = st.world().at(pos.x(), pos.y());

    std::vector<int> participants;
    for (int pid : tile.players())
        if (st.hasPlayer(pid) && st.player(pid).level() == level)
            participants.push_back(pid);

    if (!incantationReady(level, tile, participants.size())) {
        _server.sendToPlayer(playerId, AiProtocol::ko());
        return;
    }

    for (int pid : participants) {
        Player &part = st.player(pid);
        if (part.currentEventId()) {
            _server.scheduler().cancel(part.currentEventId());
            part.setCurrentEventId(0);
        }
        part.setState(Player::State::Incanting);
        _server.sendToPlayer(pid, AiProtocol::elevationUnderway());
    }

    _server.gui().pic(pos.x(), pos.y(), level, participants);

    TimePoint due = deadlineFor(300, _server.freq());
    _server.scheduler().schedule(due, [this, playerId, pos, level, participants]() {
        finishIncantation(playerId, pos, level, participants);
    });
}

void CommandExecutor::finishIncantation(int issuerId, Pos pos, int level,
                                        std::vector<int> participants)
{
    (void)issuerId;
    GameState &st = _server.state();
    Tile &tile = st.world().at(pos.x(), pos.y());

    std::vector<int> present;
    for (int pid : participants) {
        if (!st.hasPlayer(pid))
            continue;
        const Player &p = st.player(pid);
        if (p.state() != Player::State::Incanting)
            continue;
        if (p.level() != level || p.pos().x() != pos.x() || p.pos().y() != pos.y())
            continue;
        present.push_back(pid);
    }

    bool ok = incantationReady(level, tile, present.size());

    if (ok) {
        const ElevationReq &req = ELEV[level];
        for (int r = 1; r < RESOURCE_COUNT; ++r)
            tile.resources()[r] -= req.stones[r];
        for (int pid : present) {
            Player &p = st.player(pid);
            p.setLevel(level + 1);
            _server.sendToPlayer(pid, AiProtocol::currentLevel(p.level()));
            _server.gui().plv(p);
        }
        _server.gui().bct(pos.x(), pos.y(), tile);
    } else {
        for (int pid : present)
            _server.sendToPlayer(pid, AiProtocol::ko());
    }

    _server.gui().pie(pos.x(), pos.y(), ok);

    for (int pid : present)
        st.player(pid).setState(Player::State::Idle);
    pumpPlayerQueues();
}

std::vector<std::string> CommandExecutor::tokenize(const std::string &line)
{
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok)
        out.push_back(tok);
    return out;
}
