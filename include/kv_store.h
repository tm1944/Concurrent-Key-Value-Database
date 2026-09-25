#pragma once

#include <unordered_map>
#include <string>
#include <optional>


class KVStore {
public:
    //automatically generated for now
    //KVStore();
    //~KVStore();

    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);  // std::optional means the return value may contain a string or no value
    bool remove(const std::string& key);

private:
    std::unordered_map<std::string, std::string> store_;
};
