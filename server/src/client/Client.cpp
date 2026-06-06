#include "Client.hpp"

Client::Client(int fd) : _fd(fd) {}

void Client::appendToReadBuffer(const char *data, size_t len)
{
    _readBuffer.append(data, len);
}

std::optional<std::string> Client::extractLine()
{
    // chercher la position d'un '\n' dans _readBuffer
    //   absent  -> return std::nullopt
    //   present -> ligne = la sous-chaine avant le '\n'
    //              effacer "ligne + '\n'" de _readBuffer
    //              return ligne
    return std::nullopt; // a remplacer
}

void Client::queueWrite(const std::string &data)
{
    _writeBuffer += data;
}