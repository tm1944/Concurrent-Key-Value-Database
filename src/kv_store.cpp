#include "../include/kv_store.h"
#include <mutex>

void KVStore::set(const std::string& key, const std::string& value){
    std::lock_guard<std::mutex> lock(mutex_);
    store_[key] = value;
}

std::optional<std::string> KVStore::get(const std::string& key){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(key);
    if(it != store_.end()){
        return it->second;
    }
    return std::nullopt;
}


bool KVStore::remove(const std::string& key){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(key);
    if(it != store_.end()){
        store_.erase(it);
        return 1;
    }
    return 0;
}
