#include "../include/command_executor.h"

CommandExecutor::CommandExecutor(KVStore& store)
    :store_(store) {}

std::string CommandExecutor::execute(const Command& cmd){
    std::string res;
    switch (cmd.type)
    {
    case Command::Type::GET: {
        auto value = store_.get(cmd.args[0]);
        if(value.has_value()){
            res = value.value();
        }else{
            res = "(nil)";
        }
        break;
    }
    case Command::Type::SET:
        store_.set(cmd.args[0], cmd.args[1]);
        res = "OK";
        break;
    
    case Command::Type::DELETE:
        if(store_.remove(cmd.args[0])){
            res = "1";
        }else{
            res = "0";
        }
        break;
    
    case Command::Type::EXIT:
        break;

    default:
        return "ERR";
    }
    return res;
}
