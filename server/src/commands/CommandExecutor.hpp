#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "game/Types.hpp"

class Server;

class CommandExecutor {
public:
    explicit CommandExecutor(Server &server);

    void dispatch(int playerId, const std::string &line);

    void pumpPlayerQueues();

private:
    using Handler = std::function<void(int playerId, const std::vector<std::string> &args)>;

    struct Command {
        int durationUnits;
        Handler handler;
        Handler onStart = nullptr;
    };

    void registerCommands();
    void startNext(int playerId);
    void startIncantation(int playerId);
    void finishIncantation(int issuerId, Pos pos, int level, std::vector<int> participants);

    static std::vector<std::string> tokenize(const std::string &line);

    Server &_server;
    std::unordered_map<std::string, Command> _table;
};
