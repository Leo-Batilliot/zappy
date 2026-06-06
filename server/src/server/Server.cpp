#include "Server.hpp"
#include "Error.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <optional>
#include <cstring>

Server::Server(const Config &config) : _config(config)
{
    setupSocket();
    _pollFds.push_back({_listenFd, POLLIN, 0});
    for (const std::string &team : _config.teams)
        _freeSlots[team] = _config.clients;
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
    while (true) {
        if (poll(_pollFds.data(), _pollFds.size(), -1) < 0)
            throw NetworkError("poll failed");

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
    }
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
    _clients.at(fd).appendToReadBuffer(buffer, static_cast<std::size_t>(bytes));

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
    case Client::State::Ai:
        break;
    case Client::State::Gui:
        break;
    }
}

void Server::doHandshake(Client &client, const std::string &teamName)
{
    if (teamName == "GRAPHIC") {
        client.setState(Client::State::Gui);
        std::cout << "fd " << client.fd() << ": GUI connected" << std::endl;
        return;
    }

    auto slot = _freeSlots.find(teamName);
    if (slot == _freeSlots.end() || slot->second <= 0) {
        removeClient(client.fd());
        return;
    }

    slot->second--;
    client.setState(Client::State::Ai);
    client.queueWrite(std::to_string(slot->second) + "\n");
    client.queueWrite(std::to_string(_config.width) + " " + std::to_string(_config.height) + "\n");
    std::cout << "fd " << client.fd() << ": joined team " << teamName << std::endl;
}

void Server::flushWrites(Client &client)
{
    const std::string &data = client.writeBuffer();
    ssize_t sent = send(client.fd(), data.data(), data.size(), 0);

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
