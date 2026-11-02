#ifndef CLIENT_H
#define CLIENT_H

// establish connection with server
int connect_to_server(const char* ip, int port);

// client-functions
void send_message(int socket_fd);
void list_messages(int socket_fd);
void read_message(int socket_fd);
void delete_message(int socket_fd);
void quit_connection(int socket_fd);

//usage
void print_usage(char *programm_name);


#endif



/* 
SEND
SEND\n
<Sender>\n
<Receiver>\n
<Subject (max. 80 chars)>\n
<message (multi-line; no length restrictions)\n>
.\n
• The final dot ends the command.
• The server always responds with either “OK\n” or “ERR\n”.
*/

/*
LIST
LIST\n
<Username>\n
• The server responds with
<count of messages of the user (0 if no message or user unknown)>\n
<subject 1>\n
<subject 2>\n
…
<subject N>\n
*/

/*
READ
READ\n
<Username>\n
<Message-Number>\n
• The server responds with
OK\n
<complete message content (as defined in SEND)>\n
• or
ERR\n
*/

/*
DEL
DEL\n
<Username>\n
<Message-Number>\n
• The server responds with
OK\n
• or
ERR\n
*/

/*
QUIT
QUIT\n
• The server does not respond.
*/