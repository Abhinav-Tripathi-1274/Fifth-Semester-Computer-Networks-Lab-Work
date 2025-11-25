/* server.c
 * Multi-client computation server using fork()
 *
 * Protocol (text lines, case-insensitive commands):
 *   ADD a b       -> returns a + b
 *   MUL a b       -> returns a * b
 *   FACT n        -> returns factorial(n) (n <= 20 allowed)
 *   FIB n         -> returns nth Fibonacci (0-based)
 *   PRIME n       -> returns "yes" or "no"
 *   QUIT          -> close connection
 *
 * Compile:
 *   gcc -Wall -O2 server.c -o server
 * Run:
 *   ./server 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BACKLOG 10
#define BUFSIZE 256

/* Reap dead children to avoid zombies */
void sigchld_handler(int s) {
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

long long factorial(int n) {
    if (n < 0) return -1;
    long long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

long long fib(int n) {
    if (n < 0) return -1;
    if (n == 0) return 0;
    if (n == 1) return 1;
    long long a = 0, b = 1, c;
    for (int i = 2; i <= n; ++i) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; (long long)i * i <= n; i += 2)
        if (n % i == 0) return 0;
    return 1;
}

void handle_client(int connfd, struct sockaddr_in *client_addr) {
    char buf[BUFSIZE];
    FILE *fp = fdopen(connfd, "r+"); /* use stdio for easier line handling */
    if (!fp) {
        perror("fdopen");
        close(connfd);
        exit(1);
    }

    fprintf(fp, "Welcome to compute-server. Commands: ADD, MUL, FACT, FIB, PRIME, QUIT\n");
    fflush(fp);

    while (fgets(buf, sizeof(buf), fp)) {
        /* trim newline */
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';

        if (strlen(buf) == 0) {
            fprintf(fp, "Empty command\n");
            fflush(fp);
            continue;
        }

        char cmd[32];
        long long a, b;
        int n;
        /* parse commands */
        if (sscanf(buf, "ADD %lld %lld", &a, &b) == 2) {
            fprintf(fp, "%lld\n", a + b);
        } else if (sscanf(buf, "MUL %lld %lld", &a, &b) == 2) {
            fprintf(fp, "%lld\n", a * b);
        } else if (sscanf(buf, "FACT %d", &n) == 1) {
            if (n < 0 || n > 20) { /* keep within 64-bit safe range */
                fprintf(fp, "ERR factorial: n must be 0..20\n");
            } else {
                fprintf(fp, "%lld\n", factorial(n));
            }
        } else if (sscanf(buf, "FIB %d", &n) == 1) {
            if (n < 0 || n > 92) { /* fib(93) overflows signed 64-bit */
                fprintf(fp, "ERR fib: n must be 0..92\n");
            } else {
                fprintf(fp, "%lld\n", fib(n));
            }
        } else if (sscanf(buf, "PRIME %d", &n) == 1) {
            fprintf(fp, "%s\n", is_prime(n) ? "yes" : "no");
        } else if (strcasecmp(buf, "QUIT") == 0) {
            fprintf(fp, "Goodbye\n");
            fflush(fp);
            break;
        } else {
            fprintf(fp, "ERR unknown command\n");
        }
        fflush(fp);
    }

    fclose(fp); /* closes connfd */
    exit(0);
}

int main(int argc, char *argv[]) {
    int listenfd, connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    int port = 8080;

    if (argc >= 2) port = atoi(argv[1]);
    if (port <= 0) port = 8080;

    /* create socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    /* reuse addr */
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(listenfd);
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listenfd);
        exit(1);
    }

    if (listen(listenfd, BACKLOG) == -1) {
        perror("listen");
        close(listenfd);
        exit(1);
    }

    /* set up SIGCHLD handler */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        close(listenfd);
        exit(1);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        sin_size = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr *)&client_addr, &sin_size);
        if (connfd == -1) {
            if (errno == EINTR) continue; /* interrupted by signal, retry */
            perror("accept");
            continue;
        }

        printf("Accepted connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(connfd);
            continue;
        } else if (pid == 0) {
            /* child */
            close(listenfd); /* child doesn't accept new connections */
            handle_client(connfd, &client_addr);
            /* handle_client exits */
        } else {
            /* parent */
            close(connfd); /* parent doesn't need this socket */
            /* continue to accept */
        }
    }

    close(listenfd);
    return 0;
}

