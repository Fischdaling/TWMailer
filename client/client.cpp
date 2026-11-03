#include <cstdio>
#include <string>
#include <iostream>
#include <stdlib.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "client.h"

int main(int argc, char *argv[]){
 if (argc != 3)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);


    int socket_fd = connect_to_server(ip, port);

    if(socket_fd == -1) return 1;
    std::cout << "Connected to server!" << std::endl;

    close(socket_fd);

}

int connect_to_server(const char* ip, int port){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_fd == -1){
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    if(inet_pton(AF_INET, ip, &server_address.sin_addr) != 1){
        fprintf(stderr, "Invalid IP address\n");
        close(socket_fd);
        return -1;
    }

    if (connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) ==-1){
        perror("Connection failed");
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}



void print_usage(char *programm_name){
        fprintf(stderr,"Usage: %s <ip> <port> \n\n", programm_name);
    }