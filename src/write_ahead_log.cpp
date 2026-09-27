#include "../include/write_ahead_log.h"

#include <stdexcept>

WriteAheadLog::WriteAheadLog(const std::string& path)
    : file_(path,std::ios::app){
        if(!file_){
            throw std::runtime_error("Failed to open WAL file");
        }
    }

void WriteAheadLog::set_log(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_ << "SET " << key << " " << value << "\n";
    file_.flush();
}

void WriteAheadLog::delete_log(const std::string& key){
    std::lock_guard<std::mutex> lock(mutex_);
    file_ << "DELETE " << key << "\n";
    file_.flush();
}
