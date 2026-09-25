#include <string.h>
#include<vector>

struct Command {
    enum class Type {
        GET,
        SET,
        DELETE
    };

    Type type;
    std::vector<std::string> args;
}