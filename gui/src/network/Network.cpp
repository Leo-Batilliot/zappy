/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Network.cpp
*/

#include "Network.hpp"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
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
    struct addrinfo hints;
    struct addrinfo *result = nullptr;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return false;
    }

    for (struct addrinfo *p = result; p != nullptr; p = p->ai_next) {
        _fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (_fd == -1)
            continue;
        if (connect(_fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(_fd);
        _fd = -1;
    }
    freeaddrinfo(result);

    if (_fd == -1) {
        std::cerr << "Connexion impossible a " << host << ":" << port << std::endl;
        return false;
    }

    char c;
    while (recv(_fd, &c, 1, 0) == 1 && c != '\n')
        ;

    int flags = fcntl(_fd, F_GETFL, 0);
    fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
    return true;
}

bool Network::sendLine(const std::string &line)
{
    if (_fd == -1)
        return false;
    std::string payload = line + "\n";
    ssize_t sent = send(_fd, payload.c_str(), payload.size(), 0);
    return sent == static_cast<ssize_t>(payload.size());
}

void Network::poll()
{
    if (_fd == -1)
        return;

    char chunk[4096];
    ssize_t n;

    while ((n = recv(_fd, chunk, sizeof(chunk), 0)) > 0)
        _buffer.append(chunk, static_cast<std::size_t>(n));

    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        close(_fd);
        _fd = -1;
    }

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
