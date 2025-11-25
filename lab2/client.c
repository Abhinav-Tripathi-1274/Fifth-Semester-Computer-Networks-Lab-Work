/* client.c
 * Simple interactive client to talk to the compute server.
 *
 * Compile:
 *   gcc -Wall -O2 client.c -o client
 * Run:
 *   ./client 127.0.0.1 12345
 * Then type commands like:
 *   ADD 2 3
 *   FACT 10
 *   FIB 20
 *   PRIME 97
 *   QUIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFSIZE 256

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return 1;
    }
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0) port = 12345;

    int sockfd;
    struct sockaddr_in serv_addr;
    char sendbuf[BUFSIZE], recvbuf[BUFSIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }
    memset(&(serv_addr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    FILE *sockfp = fdopen(sockfd, "r+");
    if (!sockfp) {
        perror("fdopen");
        close(sockfd);
        return 1;
    }

    /* read welcome */
    if (fgets(recvbuf, sizeof(recvbuf), sockfp))
        fputs(recvbuf, stdout);

    printf("Enter commands (e.g., ADD 1 2, FACT 5, FIB 10, PRIME 17, QUIT):\n");
    while (fgets(sendbuf, sizeof(sendbuf), stdin)) {
        /* send to server */
        if (fputs(sendbuf, sockfp) == EOF) {
            perror("send");
            break;
        }
        fflush(sockfp);

        /* read response (single line expected) */
        if (!fgets(recvbuf, sizeof(recvbuf), sockfp)) {
            printf("Server closed connection.\n");
            break;
        }
        fputs(recvbuf, stdout);

        /* if user typed QUIT, we'll close locally too */
        char tmp[32];
        if (sscanf(sendbuf, "%31s", tmp) == 1 && strcasecmp(tmp, "QUIT") == 0) {
            break;
        }
    }

    fclose(sockfp); /* closes socket */
    return 0;
}

