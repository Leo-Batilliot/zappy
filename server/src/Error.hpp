#pragma once

#include <string>
#include <exception>

class Error : public std::exception {
public:
    explicit Error(std::string message, std::string where = "") : _message(std::move(message)), _where(std::move(where)) {}
    const char *what() const noexcept override { return _message.c_str(); }
    const std::string &where() const noexcept { return _where; }
protected:
    std::string _message;
    std::string _where;
};

class ArgError : public Error {
public:
    explicit ArgError(std::string message) : Error(std::move(message), "args") {}
};

class NetworkError : public Error {
public:
    explicit NetworkError(std::string message) : Error(std::move(message), "network") {}
};
