#ifndef NETWORK_H
#define NETWORK_H

// Network functions
int create_server(int port);
int accept_connection(int server_socket);
int connect_to_server(const char *address, int port);
int send_data(int socket, const char* data);
char* receive_data(int socket);
void close_socket(int socket);

#endif // NETWORK_H
