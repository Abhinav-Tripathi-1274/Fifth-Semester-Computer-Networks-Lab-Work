#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    FILE *html_file;
    char *response;
    long file_size;

    // 1. Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind socket to IP/Port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // listen on all interfaces
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    while (1) {
        // 4. Accept client connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        // 5. Read client request
        read(client_fd, buffer, BUFFER_SIZE);
        printf("Client request:\n%s\n", buffer);

        // 6. Open index.html
        html_file = fopen("index.html", "rb");
        if (html_file == NULL) {
            perror("index.html not found");
            char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 File Not Found</h1>";
            write(client_fd, not_found, strlen(not_found));
        } else {
            // Get file size
            fseek(html_file, 0, SEEK_END);
            file_size = ftell(html_file);
            rewind(html_file);

            // Allocate buffer for response
            response = malloc(file_size + 200);
            if (!response) {
                perror("malloc failed");
                fclose(html_file);
                close(client_fd);
                continue;
            }

            // Write HTTP response headers
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n", file_size);

            // Read file into response buffer
            fread(response + strlen(response), 1, file_size, html_file);

            // Send response
            write(client_fd, response, strlen(response));

            free(response);
            fclose(html_file);
        }

        // 7. Close client socket
        close(client_fd);
    }

    // Close server
    close(server_fd);
    return 0;
}

