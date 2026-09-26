#include <iostream>
#include "../include/tcp_server.h"

int main(){
    TCPServer server(6379);
    server.start();
    return 0;
}
