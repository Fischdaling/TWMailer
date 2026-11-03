#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define MAX 80 

void print_usage(char *programm_name){
    fprintf(stderr,"Usage: %s <port>  <mail-spool-directoryname>\n\n", programm_name);
}


int connection(int port){
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        std::cerr << "socket failed" <<std::endl; 
         return EXIT_FAILURE;
    }

    sockaddr_in serverAddr{}; // {} initialize it empty
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(serverSocket, (sockaddr*) &serverAddr, sizeof(serverAddr))!= 0){
         std::cerr << "bind failed" <<std::endl; /// if 0 SUCCESS if -1 ERRROORR
         return EXIT_FAILURE;
    }

    if(listen(serverSocket,5) != 0){// n ist the number of requests able to queue concurrenttly
        std::cerr << "listen failed" <<std::endl; 
         return EXIT_FAILURE;
    } 

    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if(clientSocket < 0){
           std::cerr << "accept failed" <<std::endl; 
         return EXIT_FAILURE;
    }

    char buffer[1024] = {0};
    ssize_t n = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
    if (n < 0){
        std::cerr << "recv failed" <<std::endl; /// if 0 SUCCESS if -1 ERRROORR
         return EXIT_FAILURE;
    }
    buffer[n] = '\0'; // add to let cout know when to stopp

    std::cout << "Message from client: " << buffer << std::endl;
    close(clientSocket);
    close(serverSocket);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    char *programm_name; 
    int port;
    size_t pos; // position of parsed numbers (shows where parser stopped)

    programm_name = argv[0];

    if (argc != 3){
        print_usage(programm_name);
        return EXIT_FAILURE;

    }try {
        port = std::stoi(argv[1], &pos);

        if(strlen(argv[1]) != pos){
            print_usage(programm_name);
            return EXIT_FAILURE;
        }

        if(port < 1 || port > 65535){
            print_usage(programm_name);
            return EXIT_FAILURE;
        }


    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
     if(!std::filesystem::exists(argv[2])){
        std::filesystem::create_directories(argv[2]);
      }
    if(connection(port) != 0){
        std::cerr << "connection failed qwq" << std::endl;
        return EXIT_FAILURE;
    }else {
        std::cerr << "connection worki yaay" << std::endl;
        return EXIT_SUCCESS;
    }
}