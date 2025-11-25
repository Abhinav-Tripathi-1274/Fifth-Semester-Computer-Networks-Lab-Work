// chat_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080

int new_socket;

void* receive_messages(void* arg) {
    char buffer[1024];
    while (1) {
        int val = read(new_socket, buffer, sizeof(buffer)-1);
        if (val <= 0) {
            printf("\nClient disconnected.\n");
            exit(0);
        }
        buffer[val] = '\0';
        printf("\nClient: %s\nYou: ", buffer);
        fflush(stdout);
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char message[1024];
    pthread_t recv_thread;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);

    printf("Server waiting for client...\n");
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    printf("Client connected!\n");

    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    while (1) {
        printf("You: ");
        fgets(message, sizeof(message), stdin);
        send(new_socket, message, strlen(message), 0);
    }

    close(new_socket);
    close(server_fd);
    return 0;
}

