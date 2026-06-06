#pragma once

#include "args/Parser.hpp"
#include "server/Client.hpp"
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

private:
    void setupSocket();
    void acceptClient();
    void handleClient(int fd);
    void removeClient(int fd);

    void handleLine(Client &client, const std::string &line);
    void doHandshake(Client &client, const std::string &teamName);
    void flushWrites(Client &client);
    void syncPollOut(const Client &client);

    std::unordered_map<int, Client> _clients;
    std::unordered_map<std::string, int> _freeSlots;
    Config _config;
    int _listenFd = -1;
    std::vector<struct pollfd> _pollFds;
};
