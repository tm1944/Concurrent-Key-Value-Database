#pragma once

#include <string>
#include <vector>

struct Command {
    enum class Type {
        GET,
        SET,
        DELETE,
        EXIT
    };

    Type type;
    std::vector<std::string> args;
};
