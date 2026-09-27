#include "tcp_server.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <atomic>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

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

void test_server_handles_concurrent_clients() {
    constexpr int client_count = 8;
    constexpr int commands_per_client = 50;

    TCPServer server(0);
    std::vector<std::thread> server_threads;
    std::vector<std::thread> client_threads;
    std::vector<std::string> responses(client_count);
    std::vector<std::exception_ptr> client_errors(client_count);
    std::vector<int> client_sockets(client_count, -1);
    std::atomic<int> clients_ready{0};
    std::atomic<bool> start_clients{false};

    for (int client = 0; client < client_count; ++client) {
        int sockets[2];
        require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0,
                "the test should create every connected socket pair");

        client_sockets[client] = sockets[1];
        server_threads.emplace_back(
            [&server, server_socket = sockets[0]] {
                server.serve_client(server_socket);
            });

        client_threads.emplace_back([&, client] {
            try {
                ++clients_ready;
                while (!start_clients.load()) {
                    std::this_thread::yield();
                }

                std::string commands;
                for (int command = 0; command < commands_per_client; ++command) {
                    const auto key = "client-" + std::to_string(client) +
                                     "-key-" + std::to_string(command);
                    commands += "SET " + key + " value\nGET " + key + "\n";
                }

                send_all(client_sockets[client], commands);
                shutdown(client_sockets[client], SHUT_WR);
                responses[client] =
                    receive_until_closed(client_sockets[client]);
            } catch (...) {
                client_errors[client] = std::current_exception();
                shutdown(client_sockets[client], SHUT_RDWR);
            }
            close(client_sockets[client]);
            client_sockets[client] = -1;
        });
    }

    while (clients_ready.load() != client_count) {
        std::this_thread::yield();
    }
    start_clients = true;

    for (auto& client_thread : client_threads) {
        client_thread.join();
    }
    for (auto& server_thread : server_threads) {
        server_thread.join();
    }

    for (const auto& error : client_errors) {
        if (error) {
            std::rethrow_exception(error);
        }
    }

    std::string expected;
    for (int command = 0; command < commands_per_client; ++command) {
        expected += "OK\nvalue\n";
    }
    for (const auto& response : responses) {
        require(response == expected,
                "every concurrent client should receive all expected responses");
    }
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"fragmented and batched TCP commands",
         test_server_processes_fragmented_and_batched_commands},
        {"concurrent TCP clients", test_server_handles_concurrent_clients},
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    std::cout << '\n' << tests.size() - failures << '/' << tests.size()
              << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
