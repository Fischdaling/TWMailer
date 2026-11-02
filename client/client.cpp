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


}

int connect_to_server(const char* ip, int port){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

}



void print_usage(char *programm_name)
    {
        fprintf(stderr,"Usage: %s <ip> <port> \n\n", programm_name);
    }