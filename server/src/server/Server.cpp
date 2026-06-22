#include "Server.hpp"
#include "Error.hpp"
#include "time/Clock.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <optional>
#include <cstring>
#include <cerrno>
#include <random>
#include <sstream>
#include <vector>

namespace {

std::mt19937 &rng()
{
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

Orientation randomOrientation()
{
    std::uniform_int_distribution<int> ro(1, 4);
    return static_cast<Orientation>(ro(rng()));
}

std::optional<int> parseInt(const std::string &s)
{
    try {
        std::size_t consumed = 0;
        int value = std::stoi(s, &consumed);
        if (consumed != s.size())
            return std::nullopt;
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

}

Server::Server(const Config &config) : _config(config), _state(_config), _gui(_clients), _exec(*this)
{
    setupSocket();
    _pollFds.push_back({_listenFd, POLLIN, 0});
}

Server::~Server()
{
    for (const struct pollfd &entry : _pollFds)
        close(entry.fd);
}

void Server::setupSocket()
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        throw NetworkError("socket creation failed");

    int enable = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        throw NetworkError("setsockopt failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(_config.port));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        throw NetworkError("bind failed on port " + std::to_string(_config.port));

    if (listen(_listenFd, SOMAXCONN) < 0)
        throw NetworkError("listen failed");

    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw NetworkError("fcntl failed");
}

void Server::run()
{
    scheduleRespawn();

    while (!_state.finished) {
        int timeout = computePollTimeout();

        if (poll(_pollFds.data(), _pollFds.size(), timeout) < 0) {
            if (errno == EINTR)
                continue;
            throw NetworkError("poll failed");
        }

        for (std::size_t i = _pollFds.size(); i-- > 0;) {
            short revents = _pollFds[i].revents;
            int fd = _pollFds[i].fd;

            if (fd != _listenFd && (revents & (POLLHUP | POLLERR | POLLNVAL))) {
                removeClient(fd);
                continue;
            }
            if ((revents & POLLOUT) && _clients.count(fd) != 0)
                flushWrites(_clients.at(fd));
            if (revents & POLLIN) {
                if (fd == _listenFd)
                    acceptClient();
                else
                    handleClient(fd);
            }
        }

        TimePoint now = Clock::now();
        while (std::optional<Event> ev = _scheduler.popReady(now))
            ev->run();

        _exec.pumpPlayerQueues();
        checkVictory();

        for (auto &[fd, client] : _clients) {
            (void)fd;
            syncPollOut(client);
        }
    }
}

int Server::computePollTimeout()
{
    std::optional<TimePoint> deadline = _scheduler.nextDeadline();
    if (!deadline)
        return -1;
    return toMilliseconds(*deadline - Clock::now());
}

void Server::acceptClient()
{
    int clientFd = accept(_listenFd, nullptr, nullptr);
    if (clientFd < 0)
        return;
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        close(clientFd);
        return;
    }
    _pollFds.push_back({clientFd, POLLIN, 0});
    Client &client = _clients.try_emplace(clientFd, clientFd).first->second;
    client.queueWrite("WELCOME\n");
    syncPollOut(client);
    // OPT
    std::cout << "new client: fd " << clientFd << std::endl;
}

void Server::handleClient(int fd)
{
    char buffer[4096];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        removeClient(fd);
        return;
    }
    Client &c = _clients.at(fd);
    c.appendToReadBuffer(buffer, static_cast<std::size_t>(bytes));

    if (c.readBufferSize() > 65536) {
        removeClient(fd);
        return;
    }

    while (_clients.count(fd) != 0) {
        std::optional<std::string> line = _clients.at(fd).extractLine();
        if (!line)
            break;
        handleLine(_clients.at(fd), *line);
    }
    if (_clients.count(fd) != 0)
        syncPollOut(_clients.at(fd));
}

void Server::handleLine(Client &client, const std::string &line)
{
    switch (client.state()) {
    case Client::State::AwaitingTeam:
        doHandshake(client, line);
        break;
    case Client::State::Ai: {
        int pid = _state.playerIdByFd(client.fd());
        if (pid != -1)
            _exec.dispatch(pid, line);
        break;
    }
    case Client::State::Gui:
        handleGuiLine(client, line);
        break;
    }
}

void Server::handleGuiLine(Client &client, const std::string &line)
{
    std::istringstream iss(line);
    std::vector<std::string> tok;
    std::string s;
    while (iss >> s)
        tok.push_back(s);
    if (tok.empty())
        return;

    const std::string &cmd = tok[0];
    World &w = _state.world();

    if (cmd == "msz") {
        _gui.msz(client, w.width(), w.height());
    } else if (cmd == "mct") {
        _gui.bctAll(client, w);
    } else if (cmd == "tna") {
        for (const auto &[name, team] : _state.teams()) {
            (void)name;
            _gui.tna(client, team.name);
        }
    } else if (cmd == "sgt") {
        _gui.sgt(client, _config.freq);
    } else if (cmd == "bct") {
        std::optional<int> x = (tok.size() == 3) ? parseInt(tok[1]) : std::nullopt;
        std::optional<int> y = (tok.size() == 3) ? parseInt(tok[2]) : std::nullopt;
        if (!x || !y || *x < 0 || *x >= w.width() || *y < 0 || *y >= w.height()) {
            _gui.sbp(client);
            return;
        }
        _gui.bct(client, *x, *y, w.at(*x, *y));
    } else if (cmd == "sst") {
        std::optional<int> t = (tok.size() == 2) ? parseInt(tok[1]) : std::nullopt;
        if (!t || *t <= 0) {
            _gui.sbp(client);
            return;
        }
        _config.freq = *t;
        _gui.sst(_config.freq);
    } else {
        _gui.suc(client);
    }
}

void Server::doHandshake(Client &client, const std::string &teamName)
{
    if (teamName == "GRAPHIC") {
        client.setState(Client::State::Gui);
        _gui.sendInitialState(client, _state, _config.freq);
        // OPT (Debug)
        std::cout << "fd " << client.fd() << ": GUI connected" << std::endl;
        return;
    }

    if (!_state.isTeam(teamName) || !_state.hasFreeSlot(teamName)) {
        removeClient(client.fd());
        return;
    }
    int eggId = _state.hatchEgg(teamName);
    if (eggId < 0) {
        removeClient(client.fd());
        return;
    }

    Team &t = _state.team(teamName);
    Pos pos = _state.egg(eggId).pos;
    int pid = _state.createPlayer(teamName, client.fd(), pos, randomOrientation());
    _state.removeEgg(eggId);
    t.freeSlots--;

    client.setState(Client::State::Ai);
    client.queueWrite(std::to_string(t.freeSlots) + "\n");
    client.queueWrite(std::to_string(_config.width) + " " + std::to_string(_config.height) + "\n");

    _gui.ebo(eggId);
    _gui.pnw(_state.player(pid));
    _gui.pin(_state.player(pid));
    scheduleFoodTick(pid);

    std::cout << "fd " << client.fd() << ": joined team " << teamName << " as player #" << pid << " (egg #" << eggId << ")" << std::endl;
}

void Server::sendToPlayer(int playerId, const std::string &msg)
{
    if (!_state.hasPlayer(playerId))
        return;
    int fd = _state.player(playerId).connFd;
    auto it = _clients.find(fd);
    if (it == _clients.end())
        return;
    it->second.queueWrite(msg);
    syncPollOut(it->second);
}

void Server::scheduleFoodTick(int playerId)
{
    if (!_state.hasPlayer(playerId))
        return;
    TimePoint due = deadlineFor(FOOD_LIFE_UNITS, _config.freq);
    _state.player(playerId).foodEventId =
        _scheduler.schedule(due, [this, playerId]() { consumeFood(playerId); });
}

void Server::consumeFood(int playerId)
{
    if (!_state.hasPlayer(playerId))
        return;

    Player &p = _state.player(playerId);
    p.inventory[static_cast<int>(Resource::Food)] -= 1;
    p.lifeUnits -= FOOD_LIFE_UNITS;

    if (p.inventory[static_cast<int>(Resource::Food)] <= 0) {
        killPlayer(playerId);
        return;
    }
    _gui.pin(p);
    scheduleFoodTick(playerId);
}

void Server::killPlayer(int playerId)
{
    if (!_state.hasPlayer(playerId))
        return;

    Player &p = _state.player(playerId);
    int fd = p.connFd;

    if (p.currentEventId)
        _scheduler.cancel(p.currentEventId);
    if (p.foodEventId)
        _scheduler.cancel(p.foodEventId);

    _gui.pdi(playerId);

    auto it = _clients.find(fd);
    if (it != _clients.end()) {
        it->second.queueWrite("dead\n");
        const std::string &data = it->second.writeBuffer();
        send(fd, data.data(), data.size(), MSG_NOSIGNAL);
    }

    _state.removePlayer(playerId);
    removeClient(fd);
}

void Server::scheduleRespawn()
{
    TimePoint due = deadlineFor(20, _config.freq);
    _scheduler.schedule(due, [this]() {
        std::vector<Pos> changed = _state.world().refill();
        for (const Pos &c : changed)
            _gui.bct(c.x, c.y, _state.world().at(c.x, c.y));
        scheduleRespawn();
    });
}

void Server::checkVictory()
{
    if (_state.finished)
        return;

    for (const auto &[name, team] : _state.teams()) {
        int reached = 0;
        for (int pid : team.playerIds)
            if (_state.hasPlayer(pid) && _state.player(pid).level >= 8)
                ++reached;
        if (reached >= 6) {
            _state.finished = true;
            _state.winner = name;
            _gui.seg(name);
            return;
        }
    }
}

void Server::flushWrites(Client &client)
{
    const std::string &data = client.writeBuffer();
    ssize_t sent = send(client.fd(), data.data(), data.size(), MSG_NOSIGNAL);

    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        removeClient(client.fd());
        return;
    }
    if (sent > 0)
        client.consumeWritten(static_cast<std::size_t>(sent));
    syncPollOut(client);
}

void Server::syncPollOut(const Client &client)
{
    for (struct pollfd &entry : _pollFds) {
        if (entry.fd != client.fd())
            continue;
        if (client.hasPendingWrites())
            entry.events |= POLLOUT;
        else
            entry.events &= ~POLLOUT;
        return;
    }
}

void Server::removeClient(int fd)
{
    int pid = _state.playerIdByFd(fd);
    if (pid != -1) {
        Player &p = _state.player(pid);
        if (p.currentEventId)
            _scheduler.cancel(p.currentEventId);
        if (p.foodEventId)
            _scheduler.cancel(p.foodEventId);
        _gui.pdi(pid);
        _state.removePlayer(pid);
    }

    close(fd);
    for (auto it = _pollFds.begin(); it != _pollFds.end(); ++it) {
        if (it->fd == fd) {
            _pollFds.erase(it);
            break;
        }
    }
    _clients.erase(fd);
    std::cout << "client disconnected: fd " << fd << std::endl;
}
