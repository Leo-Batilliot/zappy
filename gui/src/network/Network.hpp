/*
** EPITECH PROJECT, 2025
** G-YEP-400-LIL-4-1-zappy-7
** File description:
** Network.hpp
*/

#pragma once

#include <string>
#include <queue>

class Network {
public:
    Network();
    ~Network();

    bool connectTo(const std::string &host, const std::string &port);
    bool sendLine(const std::string &line);
    void poll();
    bool nextMessage(std::string &out);
    bool isConnected() const { return _fd != -1; }

private:
    void flushSend();
    void disconnect();

    int _fd;
    std::string _buffer;
    std::string _sendBuffer;
    std::queue<std::string> _messages;
};
