#pragma once

#include <string>
#include <optional>
#include <cstddef>

class Client {
public:
    enum class State {
        AwaitingTeam,
        Ai,
        Gui,
    };

    explicit Client(int fd);

    int fd() const;
    State state() const;
    void setState(State state);

    void appendToReadBuffer(const char *data, std::size_t size);
    std::optional<std::string> extractLine();

    void queueWrite(const std::string &data);
    bool hasPendingWrites() const;
    const std::string &writeBuffer() const;
    void consumeWritten(std::size_t count);

private:
    int _fd;
    State _state = State::AwaitingTeam;
    std::string _readBuffer;
    std::string _writeBuffer;
};
