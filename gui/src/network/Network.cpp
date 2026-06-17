/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Network.cpp
*/

#include "network/Network.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

Network::Network() : _fd(-1)
{
}

Network::~Network()
{
    if (_fd != -1)
        close(_fd);
}

bool Network::connectTo(const std::string &host, const std::string &port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *raw = nullptr;
    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &raw);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return false;
    }
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> result(raw, freeaddrinfo);

    for (auto p = result.get(); p != nullptr; p = p->ai_next) {
        _fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (_fd == -1)
            continue;
        if (connect(_fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(_fd);
        _fd = -1;
    }

    if (_fd == -1) {
        std::cerr << "Connexion impossible a " << host << ":" << port << std::endl;
        return false;
    }

    timeval welcomeTimeout{};
    welcomeTimeout.tv_sec = 5;
    setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &welcomeTimeout,
        sizeof(welcomeTimeout));

    char c = '\0';
    while (c != '\n') {
        if (recv(_fd, &c, 1, 0) != 1) {
            std::cerr << "Le serveur n'a pas envoye WELCOME." << std::endl;
            disconnect();
            return false;
        }
    }

    int flags = fcntl(_fd, F_GETFL, 0);
    fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
    return true;
}

bool Network::sendLine(const std::string &line)
{
    if (_fd == -1)
        return false;
    _sendBuffer += line;
    _sendBuffer += '\n';
    flushSend();
    return true;
}

void Network::flushSend()
{
    while (_fd != -1 && !_sendBuffer.empty()) {
        ssize_t sent = send(_fd, _sendBuffer.data(), _sendBuffer.size(),
            MSG_NOSIGNAL);
        if (sent > 0) {
            _sendBuffer.erase(0, static_cast<std::size_t>(sent));
            continue;
        }
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        disconnect();
    }
}

void Network::disconnect()
{
    if (_fd != -1) {
        close(_fd);
        _fd = -1;
    }
}

void Network::poll()
{
    if (_fd == -1)
        return;
    flushSend();
    std::array<char, 4096> chunk{};
    ssize_t n;
    while ((n = recv(_fd, chunk.data(), chunk.size(), 0)) > 0)
        _buffer.append(chunk.data(), static_cast<std::size_t>(n));
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
        disconnect();
    std::size_t pos;
    while ((pos = _buffer.find('\n')) != std::string::npos) {
        _messages.push(_buffer.substr(0, pos));
        _buffer.erase(0, pos + 1);
    }
}

bool Network::nextMessage(std::string &out)
{
    if (_messages.empty())
        return false;
    out = _messages.front();
    _messages.pop();
    return true;
}
