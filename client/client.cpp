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

void print_usage(char *programm_name){
        fprintf(stderr,"Usage: %s <ip> <port> \n\n", programm_name);
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


void send_message(int socket_fd){
    std::string sender;
    std::string receiver;
    std::string subject;
    std::string message;

    std::cout << "Sender: ";
    std::getline(std::cin, sender);

    std::cout << "Receiver: ";
    std::getline(std::cin, receiver);

    std::cout << "Subject: ";
    std::getline(std::cin, subject);

    std::cout << "Message (Finish with a dot<.>):" << std::endl;

    std::string line;

    while (std::getline(std::cin, line)){
        if(line == "."){
            break;
        }

        message += line + "\n";
    }

    std::string request = "SEND\n"
                    + sender + "\n"
                    + receiver + "\n"
                    + subject + "\n"
                    + message
                    + ".\n";
    
     if(!send_all(socket_fd, request)){
        std::cerr << "Failed to send message." << std::endl;
        return;
    }

    std::cout << "Message request sent to server." << std::endl;
}


bool send_all(int socket_fd, const std::string& data) //This method helps my send_message method. It assures that every Byte is properly sent.
{
    size_t total_sent = 0;

    while(total_sent < data.size()){
        ssize_t bytes_sent = send(
            socket_fd,
            data.c_str() + total_sent,
            data.size() - total_sent,
            0
        );

        if(bytes_sent == -1){
            if(errno == EINTR){
                continue;
            }

            perror("Sending failed");
            return false;
        }

        if(bytes_sent == 0){
            std::cerr << "Sending failed: No bytes sent." << std::endl;
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}



