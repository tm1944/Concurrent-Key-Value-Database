#include "../include/tcp_server.h"
#include "../include/command_parser.h"
#include "../include/command_executor.h"
#include "../include/kv_store.h"
#include "../include/command.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

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


void TCPServer::serve_client(int client_fd){
    //receiving data
    std::string pending;
    char buffer[1024] = { 0 };
    KVStore kvStore;
    CommandParser cmdParser;
    CommandExecutor cmdExec(kvStore);
    std::string res = "";

    while(true){
        int conn = recv(client_fd, buffer, sizeof(buffer),0);
        if(conn == -1  || conn == 0){ // -1 -> error  0 == disconnect 
            std::cout << "Connection Error: " <<  conn << std::endl;
            return;
        }
        

        pending.append(buffer,conn);
        int pos = TCPServer::find_char(pending, '\n');
        while(pos != -1){
            std::string cmd = pending.substr(0,pos);
            pending.erase(0,pos+1); //remove command from tcp message
            auto parsed = cmdParser.parse(cmd);

            if(!parsed.has_value()){
                // invalid command so no struct
                return;
            }
            Command cmdStruct = parsed.value();
            res =  cmdExec.execute(cmdStruct);  //valid struct so execute it
            res += "\n";
            send(client_fd, res.data(), res.size() , 0); //send result from KV DB
            
            pos = TCPServer::find_char(pending,'\n'); //next command
        }
    }
}

void TCPServer::start(){
    TCPServer::setup_socket();

    //accept connection request
    int clientSocket 
        = accept(server_fd_, nullptr,nullptr);

    if (clientSocket == -1){
        return;
    }

    serve_client(clientSocket);
    close(clientSocket);
}

//return index of char in string
int TCPServer::find_char(const std::string& s, char c){
    for(std::size_t it = 0; it < s.size();++it){
        if(s[it] == c){
            return static_cast<int>(it);
        }
    }
    return -1;
}
