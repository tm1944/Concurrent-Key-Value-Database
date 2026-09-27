#pragma once

#include <fstream>
#include <mutex>
#include <string>

class WriteAheadLog {
public:
    explicit WriteAheadLog(const std::string& path);

    void set_log(const std::string& key, const std::string& value);
    void delete_log(const std::string& key);

private:
    std::ofstream file_;
    std::mutex mutex_;
};
