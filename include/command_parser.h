#pragma once

#include <optional>
#include <string> 

#include "command.h"

class CommandParser {
public:
    std::optional<Command> parse(const std::string& input);
    bool validate_command_args(
        const Command& cmd,const std::vector<std::string>& args);
};