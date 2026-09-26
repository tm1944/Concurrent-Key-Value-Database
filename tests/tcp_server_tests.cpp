#include "tcp_server.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void send_all(int socket_fd, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const auto result = send(
            socket_fd, data.data() + sent, data.size() - sent, 0);
        if (result == -1 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            throw std::runtime_error("client could not send test commands");
        }
        sent += static_cast<std::size_t>(result);
    }
}

std::string receive_until_closed(int socket_fd) {
    std::string response;
    char buffer[256];

    while (true) {
        const auto result = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (result == -1 && errno == EINTR) {
            continue;
        }
        if (result < 0) {
            throw std::runtime_error("client could not receive test responses");
        }
        if (result == 0) {
            return response;
        }
        response.append(buffer, static_cast<std::size_t>(result));
    }
}

void test_server_processes_fragmented_and_batched_commands() {
    int sockets[2];
    require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0,
            "the test should create a connected socket pair");

    TCPServer server(0);
    std::thread server_thread([&server, server_socket = sockets[0]] {
        server.serve_client(server_socket);
        close(server_socket);
    });

    try {
        send_all(sockets[1], "SET account ");
        send_all(sockets[1], "active\nGET account\nDELETE account\nGET account\n");
        shutdown(sockets[1], SHUT_WR);

        const auto response = receive_until_closed(sockets[1]);
        require(response == "OK\nactive\n1\n(nil)\n",
                "the server should execute each complete command in order");
    } catch (...) {
        close(sockets[1]);
        server_thread.join();
        throw;
    }

    close(sockets[1]);
    server_thread.join();
}

}  // namespace

int main() {
    try {
        test_server_processes_fragmented_and_batched_commands();
        std::cout << "[PASS] fragmented and batched TCP commands\n\n"
                  << "1/1 tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] fragmented and batched TCP commands: "
                  << error.what() << "\n\n0/1 tests passed\n";
        return 1;
    }
}
