#pragma once

#include <string>
#include "../include/write_ahead_log.h"
#include "kv_store.h"
#include "command.h"


class CommandExecutor {
public:
    CommandExecutor(KVStore& store, WriteAheadLog& write_ahead_log);

    std::string execute(const Command& command);

private:
    KVStore& store_;
    WriteAheadLog& write_ahead_log_;
};
