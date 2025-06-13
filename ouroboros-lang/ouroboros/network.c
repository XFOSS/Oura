#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"

// Simplified network implementation for demonstration purposes
int create_server(int port) {
    printf("[NETWORK] Creating server on port %d (simulated)\n", port);
    return 1;  // Return a dummy socket
}

int accept_connection(int server_socket) {
    printf("[NETWORK] Accepting connection on socket %d (simulated)\n", server_socket);
    return 2;  // Return a dummy client socket
}

int connect_to_server(const char* address, int port) {
    printf("[NETWORK] Connecting to %s:%d (simulated)\n", address, port);
    return 3;  // Return a dummy socket
}

int send_data(int socket, const char* data) {
    printf("[NETWORK] Sending data on socket %d: %s\n", socket, data);
    return strlen(data);  // Return the number of bytes sent
}

char* receive_data(int socket) {
    printf("[NETWORK] Receiving data on socket %d (simulated)\n", socket);
    
    // Return a dummy response
    char* response = malloc(28);
    strcpy(response, "Simulated network response");
    return response;
}

void close_socket(int socket) {
    printf("[NETWORK] Closing socket %d\n", socket);
}
