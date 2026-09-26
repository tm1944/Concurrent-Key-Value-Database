#pragma once

#include <string>

class TCPServer {
public:
    TCPServer(int port);
    void start();
    void serve_client(int client_fd);

private:
    int port_;
    int server_fd_;

    void setup_socket();
    int find_char(const std::string& s, char c);
};
