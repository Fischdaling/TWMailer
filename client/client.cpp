#include <cstdio>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <cerrno>
#include <limits>
#include <exception>

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

    int choice = 0;

    while(choice != 5){
        std::cout << "\n--- TW-Mailer ---" << std::endl;
        std::cout << "1. Send message" << std::endl;
        std::cout << "2. List messages" << std::endl;
        std::cout << "3. Read message" << std::endl;
        std::cout << "4. Delete message" << std::endl;
        std::cout << "5. Quit" << std::endl;

        std::cout << "Choose an option: ";
       
        if(!(std::cin >> choice)){
            if(std::cin.eof()){
                break;
            }

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a valid number!" << std::endl;
            continue;
        }

std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch(choice){
            case 1:
                send_message(socket_fd);
                break;
            
            case 2:
                list_messages(socket_fd);
                break;

            case 3:
                read_message(socket_fd);
                break;

            case 4:
                delete_message(socket_fd);
                break;

            case 5:
                quit_connection(socket_fd);
                break;

            default:
                std::cout << "Invalid option!" << std::endl;
                break;

        }
    }

    close(socket_fd);
    return 0;
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

bool send_all(int socket_fd, const std::string& data){ //This method helps my send_message method. It assures that every Byte is properly sent.

    size_t total_sent = 0;

    while(total_sent < data.size()){
        ssize_t bytes_sent = send(
            socket_fd,
            data.c_str() + total_sent,
            data.size() - total_sent,
            MSG_NOSIGNAL
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

bool receive_line(int socket_fd, std::string& response){
    response.clear();
    char character;

    while(true){
        ssize_t bytes_received = recv(socket_fd, &character, 1, 0);

        if(bytes_received > 0){
            if(character == '\n'){
                return true;
            }

            response += character;
        }

        if(bytes_received == 0){
            std::cerr << "Server closed the connection." << std::endl;
            return false;
        }

        if(bytes_received == -1){
            if(errno == EINTR){
                continue;
            }

            perror("Receiving failed");
            return false;
        }
    }
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

    std::string response;

    if(!receive_line(socket_fd, response)){
        std::cerr << "Failed to receive server response." << std::endl;
        return;
    }

    if(response == "OK"){
        std::cout << "Message successfully sent!" << std::endl;
    }
    else if(response == "ERR"){
        std::cerr << "Server couldn't save the message and responded with ERR." << std::endl;
    }
    else{
        std::cerr << "Unexpected or missing server response." << std::endl;
    }
}

void quit_connection(int socket_fd){
    if(!send_all(socket_fd, "QUIT\n")){
        std::cerr << "Failed to send QUIT command." << std::endl;
        return;
    }

    std::cout << "Disconnected from server." << std::endl;
}


void list_messages(int socket_fd){
    std::string username;

    std::cout << "Username: ";
    std::getline(std::cin, username);
    
    std::string request = "LIST\n" + username + "\n";

     if(!send_all(socket_fd, request)){
        std::cerr << "Failed to send LIST request." << std::endl;
        return;
    }

    std::string response;

    if(!receive_line(socket_fd, response)){
        std::cerr << "Failed to receive message count." << std::endl;
        return;
    }

    if(response == "ERR"){
        std::cerr << "Message could not be read." << std::endl;
        return;
    }

    int message_count;

    try{    //assert that we really have a number in our string that willl be converted with stoi
        size_t pos = 0;
        message_count = std::stoi(response, &pos);

        if(pos != response.size() || message_count < 0){
            std::cerr << "Invalid message count." << std::endl;
            return;
        }
    }
    catch(const std::exception& e){
        std::cerr << "Invalid server response: " << e.what() << std::endl;
        return;
    }

    std::cout << "You have " << message_count << " messages:" << std::endl;

    for(int i = 0; i < message_count; i++){
        std::string subject;

        if(!receive_line(socket_fd, subject)){
            std::cerr << "Failed to receive message subject." << std::endl;
            return;
        }

        std::cout << i + 1 << ". " << subject << std::endl;
    }
}

void read_message(int socket_fd)
{
    std::string username;
    std::string message_number;

    std::cout << "Username: ";
    std::getline(std::cin, username);

    std::cout << "Message number: ";
    std::getline(std::cin, message_number);

    std::string request = "READ\n"
                        + username + "\n"
                        + message_number + "\n";

    if(!send_all(socket_fd, request)){
        std::cerr << "Failed to send READ request." << std::endl;
        return;
    }

    std::string response;

    if(!receive_line(socket_fd, response)){
        std::cerr << "Failed to receive READ response." << std::endl;
        return;
    }

    if(response == "ERR"){
        std::cerr << "Message could not be read." << std::endl;
        return;
    }

    if(response != "OK"){
        std::cerr << "Unexpected server response." << std::endl;
        return;
    }

    std::cout << "\n--- Message ---" << std::endl;

    std::string line;

    while(true){
        if(!receive_line(socket_fd, line)){
            std::cerr << "Failed to receive message content." << std::endl;
            return;
        }

        if(line == "."){
            break;
        }

        std::cout << line << std::endl;
    }
    
    std::cout << "Message fully received!" << std::endl;
}


void delete_message(int socket_fd)
{
    std::string username;
    std::string message_number;

    std::cout << "Username: ";
    std::getline(std::cin, username);

    std::cout << "Message number: ";
    std::getline(std::cin, message_number);

    std::string request = "DEL\n"
                        + username + "\n"
                        + message_number + "\n";

    if(!send_all(socket_fd, request)){
        std::cerr << "Failed to send DEL request." << std::endl;
        return;
    }

    std::string response;

    if(!receive_line(socket_fd, response)){
        std::cerr << "Failed to receive DEL response." << std::endl;
        return;
    }
 
    if(response == "OK"){
        std::cout << "Message successfully deleted!" << std::endl;
    }
    else if(response == "ERR"){
        std::cerr << "Server could not delete the message." << std::endl;
    }
    else{
        std::cerr << "Unexpected server response." << std::endl;
    }
}