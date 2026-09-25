#pragma once

#include <string>
#include "kv_store.h"
#include "command.h"


class CommandExecutor {
public:
    CommandExecutor(KVStore& store);

    std::string execute(const Command& command);

private:
    KVStore& store_;
};
