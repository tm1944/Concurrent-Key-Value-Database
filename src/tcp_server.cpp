#include "../include/tcp_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

TCPServer::TCPServer(int port)
    : port_(port), server_fd_(-1){}


void TCPServer::setup_socket(){

    //creating the socket
    //ipv4, TCP stream 
    server_fd_= socket(AF_INET, SOCK_STREAM, 0);
    
    //specify the address
    sockaddr_in serverAddress; //struct that describes a IPV4
    serverAddress.sin_family = AF_INET;
    //htons - > host to network short
    serverAddress.sin_port = htons(port_); //set the port to the constructor
    serverAddress.sin_addr.s_addr = INADDR_ANY; //accepts connections sent to any network interface

    //bind socket
    bind(server_fd_, (struct sockaddr*)&serverAddress,
        sizeof(serverAddress));
    
    //listen 
    listen(server_fd_,5);

    
    std::cout << "Client Socket Setup Complete!" << std::endl;
}


void TCPServer::handle_client(int client_fd){
    //receiving data
    char buffer[1024] = { 0 };
    recv(client_fd, buffer, sizeof(buffer),0);
    send(client_fd, "PONG\n",5,0); //place holder for now for testing
}

void TCPServer::start(){
    TCPServer::setup_socket();

    //accept connection request
    int clientSocket 
        = accept(server_fd_, nullptr,nullptr);

    if (clientSocket == -1){
        return;
    }

    handle_client(clientSocket);
    close(clientSocket);
}