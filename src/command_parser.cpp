#include "../include/command_parser.h"

#include <sstream>

std::optional<Command> CommandParser::parse(const std::string& input){
    Command cmd;
    std::vector<std::string> s; //temp vector parsed at ' '
    std::stringstream ss(input);
    std::string token;

    //pushes all tokens into tmp s vector
    // delimeter ' '
    while( ss >> token ){
        s.push_back(token);
    }

    if(s.empty()){
        return std::nullopt;
    }

    
    if(s.at(0) == "GET"){
        cmd.type = Command::Type::GET;
    }else if(s.at(0) == "SET"){
        cmd.type = Command::Type::SET;
    }else if(s.at(0) == "DELETE"){
        cmd.type = Command::Type::DELETE;
    }else if(s.at(0) == "EXIT"){
        cmd.type = Command::Type::EXIT;
    }else{
        return std::nullopt;
    }
    

    s.erase(s.begin()); //remove first element (the enum TYPE)
    if(validate_command_args(cmd,s)){
        // key + value gets added to cmd args vector;
        for(auto const& arg: s){
            cmd.args.push_back(arg);
        }
        return cmd; // STRUCT FULLY BUILT
    }
    return std::nullopt;
}

bool CommandParser::validate_command_args(const Command& cmd,const std::vector<std::string>& args){
    switch (cmd.type)
    {
    case Command::Type::GET:
        if(args.size() != 1){
            return false;
        }
        break;
    
    case Command::Type::SET:
        if (args.size() != 2){
            return false;
        }
        break;
    
    case Command::Type::DELETE:
        if(args.size() != 1){
            return false;
        }
        break;
    
    case Command::Type::EXIT:
        if(!args.empty()){
            return false;
        }
        break;

    default:
        return false;
    }
    return true;
}



