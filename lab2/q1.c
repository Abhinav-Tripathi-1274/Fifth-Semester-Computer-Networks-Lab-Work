#include <stdio.h>
#include <sys/socket.h>
#include <errno.h> // to use 'errno'
#include <string.h> // memset
#include <stdlib.h> //EXIT_FAILURE

enum {
    DEFAULT_PORT = 12345,
    BACKLOG = 10,
    BUFFER_SIZE = 256,
};

int main(int argc, char* argv[]) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); // creating socket
    int conn_fd;
    if (listen_fd == -1) {
      perror("Failed to create socket");
      return EXIT_FAILURE;
server_fd    }
    
    int port = DEFAULT_PORT;
    if (argc >= 2) port = atoi(argv[1]);
     
    struct sockaddr_in server_addr, client_addr;

    //
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_affr.sin_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);
    //

    //
    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))
    //

    //
    listen(listen_fd, BACKLOG);
    //

    //
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGCHILD, &sa, NULL);
    //

    //
    printf("Server started listening on port: %d\n", port);

    while (1) {
        sin_size = sizeof(client_addr);
        conn_fd = accept(listen_fd, (struct sockaddr*) &client_addr, &sin_suize);

        if (conn_fd) {
            if (errno = EINTR) continue;
            perror("There is some error in accepting.");
            continue;
        }

        printf("Accepted connection from %s: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    	pid_t pid = fork();
    	if (pid < 0) {
            perror("There is error is creating child or forking.\n");
            close(conn_fd);
            continue;
    	} else if (pid == 0) {
    
    	} else { //this is parent so it doesn't need this socket
            close(conn_fd);
    	}
    }
    //

    //finally exit the socket
    close(listen_fd);
    //
    
    return 0;
}
