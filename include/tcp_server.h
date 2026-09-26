#include <string>
class TCPServer {
public:
    TCPServer(int port);
    void start();

private:
    int port_;
    int server_fd_;

    void setup_socket();
    void handle_client(int client_fd);
    int find_char(const std::string& s, char c);
};