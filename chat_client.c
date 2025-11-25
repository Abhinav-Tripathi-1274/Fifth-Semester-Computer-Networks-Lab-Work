// chat_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
int sock;

void* receive_messages(void* arg) {
    char buffer[1024];
    while (1) {
        int val = read(sock, buffer, sizeof(buffer)-1);
        if (val <= 0) {
            printf("\nServer disconnected.\n");
            exit(0);
        }
        buffer[val] = '\0';
        printf("\nServer: %s\nYou: ", buffer);
        fflush(stdout);
    }
}

int main() {
    struct sockaddr_in serv_addr;
    char message[1024];
    pthread_t recv_thread;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("Connected to server!\n");

    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    while (1) {
        printf("You: ");
        fgets(message, sizeof(message), stdin);
        send(sock, message, strlen(message), 0);
    }

    close(sock);
    return 0;
}

