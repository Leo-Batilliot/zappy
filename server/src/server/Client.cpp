#include "server/Client.hpp"

Client::Client(int fd) : _fd(fd) {}

int Client::fd() const
{
    return _fd;
}

Client::State Client::state() const
{
    return _state;
}

void Client::setState(State state)
{
    _state = state;
}

void Client::appendToReadBuffer(const char *data, std::size_t size)
{
    _readBuffer.append(data, size);
}

std::size_t Client::readBufferSize() const
{
    return _readBuffer.size();
}

std::optional<std::string> Client::extractLine()
{
    std::size_t pos = _readBuffer.find('\n');
    if (pos == std::string::npos)
        return std::nullopt;

    std::string line = _readBuffer.substr(0, pos);
    _readBuffer.erase(0, pos + 1);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    return line;
}

void Client::queueWrite(const std::string &data)
{
    _writeBuffer += data;
}

bool Client::hasPendingWrites() const
{
    return !_writeBuffer.empty();
}

const std::string &Client::writeBuffer() const
{
    return _writeBuffer;
}

void Client::consumeWritten(std::size_t count)
{
    _writeBuffer.erase(0, count);
}
