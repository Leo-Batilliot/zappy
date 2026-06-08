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

private:
    int _fd;
    std::string _buffer;
    std::queue<std::string> _messages;
};
