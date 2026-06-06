#pragma once


#include <string>
#include <optional>
#include <cstddef>

class Client {
public:
    enum class State { AwaitingTeam, Ai, Gui };

    explicit Client(int fd);

    int fd() const { return _fd; }
    State state() const { return _state; }
    void setState(State state) { _state = state; }

    void appendToReadBuffer(const char *data, size_t len);
    std::optional<std::string> extractLine();

    void queueWrite(const std::string &data);
    std::string &writeBuffer() { return _writeBuffer; }
    bool hasPendingWrite() const { return !_writeBuffer.empty(); }

private:
    int _fd;
    State _state = State::AwaitingTeam;
    std::string _readBuffer;
    std::string _writeBuffer;
};

