#pragma once

#include "../include/thread_pool.h"
#include "../include/kv_store.h"
#include "../include/write_ahead_log.h"
#include <string>

class TCPServer {
public:
    TCPServer(int port, const std::string& wal_path = "redis.wal");
    void start();
    void serve_client(int client_fd);

private:
    int port_;
    int server_fd_;

    KVStore kvStore_;
    WriteAheadLog write_ahead_log_;
    ThreadPool thread_pool_;

    void setup_socket();
    int find_char(const std::string& s, char c);
};
