#pragma once

#include "args/Parser.hpp"
#include "server/Client.hpp"
#include "game/GameState.hpp"
#include "protocol/GuiProtocol.hpp"
#include "time/Scheduler.hpp"
#include "commands/CommandExecutor.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <poll.h>

class Server {
public:
    explicit Server(const Config &config);
    ~Server();

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    void run();

    GameState &state() { return _state; }
    Scheduler &scheduler() { return _scheduler; }
    GuiProtocol &gui() { return _gui; }
    int freq() const { return _config.freq; }

    void sendToPlayer(int playerId, const std::string &msg);

private:
    void setupSocket();
    void acceptClient();
    void handleClient(int fd);
    void removeClient(int fd);

    void handleLine(Client &client, const std::string &line);
    void handleGuiLine(Client &client, const std::string &line);
    void doHandshake(Client &client, const std::string &teamName);
    void flushWrites(Client &client);
    void syncPollOut(const Client &client);

    int computePollTimeout();

    void scheduleFoodTick(int playerId);
    void consumeFood(int playerId);
    void killPlayer(int playerId);

    void scheduleRespawn();
    void checkVictory();

    std::unordered_map<int, Client> _clients;
    Config _config;
    GameState _state;
    GuiProtocol _gui;
    Scheduler _scheduler;
    CommandExecutor _exec;
    int _listenFd = -1;
    std::vector<struct pollfd> _pollFds;
};
