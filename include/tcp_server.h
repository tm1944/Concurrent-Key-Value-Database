class TCPServer {
public:
    TCPServer(int port);
    void start();
private:
    int port_;
    int server_fd_;

    void setup_socket();
    void handle_client(int cliend_fd);
};