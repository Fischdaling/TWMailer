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
#include <fstream> 
#include <vector>

#define MAX 80 
std::string spoolDir;

void print_usage(char *programm_name){
    fprintf(stderr,"Usage: %s <port>  <mail-spool-directoryname>\n\n", programm_name);
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

bool receive_line(int clientSocket, std::string& buffer, std::string& line){
    while (true) {
        size_t pos = buffer.find('\n');

        if (pos != std::string::npos) { //if there isn't a \n in the string, pos gets set to npos
            line = buffer.substr(0,pos); // copy the line to line
            buffer.erase(0,pos +1); // remove the copied line and the \n
            return true;
        }

        char received[1024];
        ssize_t n = recv(clientSocket, received, sizeof(received), 0);
        if (n < 0){
            std::cerr << "recv failed" <<std::endl; /// if 0 SUCCESS if -1 ERRROORR
            return false;
        }
        if (n==0) {
            return false;
        }
        buffer.append(received, n);
    }
}

bool receiveMessage(int clientSocket, std::string& buffer, std::string& body){
   std::string line;
   while (receive_line(clientSocket, buffer, line)) {        
        if (line == ".") {
            return true;
        }
        body += line + "\n";
   }
    return false;
    /*
        char buffer[1024] = {0};
        ssize_t n = recv(clientSocket, buffer, sizeof(buffer)-1, 0); // buffer size -1 sowe don't get the \0
        if (n < 0){
            std::cerr << "recv failed" <<std::endl; /// if 0 SUCCESS if -1 ERRROORR
            return EXIT_FAILURE;
        }
        buffer[n] = '\0'; // add to let cout know when to stopp

        std::cout << "Message from client: " << buffer << std::endl;
        return EXIT_SUCCESS;
        */
       
}

void createFile(std::string& subject,std::string& sender,std::string& receiver, std::string& body){
    std::filesystem::path userDir = std::filesystem::path(spoolDir) / receiver;

    if(!std::filesystem::exists(userDir)){
        std::filesystem::create_directories(userDir);
    }
    int next = 1;
    for (const auto& entry : std::filesystem::directory_iterator(userDir)) {
        try {
            next = std::max(next, std::stoi(entry.path().stem().string()) + 1);
        } catch (const std::exception&) {}        // ignore files that aren't numbers
    }
    std::ofstream mail(userDir / (std::to_string(next) + ".txt"));

    mail <<"sender: "<< sender<<std::endl;
    mail <<"receiver: "<< receiver<<std::endl;
    mail <<"subject: "<< subject<<std::endl;
    mail <<"body: "<< body<<std::endl;

    mail.close();

}

std::vector<std::filesystem::path> list_messages(const std::string& username){
    std::vector<std::filesystem::path> files;
    std::filesystem::path userDir = std::filesystem::path(spoolDir) / username;
    if(!std::filesystem::exists(userDir)){
        return {};
    }
    for (const auto& entry : std::filesystem::directory_iterator()) {
        files.push_back(entry);
    }
    return files;
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


    std::string buffer, requestType;
    while(receive_line(clientSocket, buffer, requestType)){
        if (requestType == "SEND") {
            std::string sender, receiver, subject, body;
            bool complete = receive_line(clientSocket, buffer, sender)
                        && receive_line(clientSocket, buffer, receiver)
                        && receive_line(clientSocket, buffer, subject)
                        && receiveMessage(clientSocket, buffer, body);
            if (!complete) break;   // client vanished mid-command

            if (subject.size() > MAX) {
                send_all(clientSocket, "ERR\n");
            } else {
                createFile(subject, sender, receiver, body);
                // save it, then OK or ERR depending on whether saving worked
                send_all(clientSocket, "OK\n");
            }
        }
        else if(requestType == "LIST"){
            std::string username;
            if (!receive_line(clientSocket, buffer, username)) break;

            std::vector<std::filesystem::path> files = list_messages(username);
            std::string response = std::to_string(files.size()) + "\n";

            for (const auto& file : files) {
                std::ifstream mail;
                std::string sender, receiver,subject;
                std::getline(mail, sender);
                std::getline(mail, receiver);
                std::getline(mail, subject);
                response += subject + "\n";
            }
            send_all(clientSocket, response);

        }else if (requestType == "QUIT") {
            break;
        }
    }

    close(clientSocket);
    close(serverSocket);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    char *programm_name; 
    int port;
    size_t pos; // position of parsed numbers (shows where parser stopped)
    spoolDir = argv[2];
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
    
    if(!std::filesystem::exists(spoolDir)){
        std::filesystem::create_directories(spoolDir);
    }

    if(connection(port) != 0){
        std::cerr << "connection failed qwq" << std::endl;
        return EXIT_FAILURE;
    }else {
        std::cerr << "connection worki yaay" << std::endl;
        return EXIT_SUCCESS;
    }
}