#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define MAX_BUFFER_SIZE 8192
#define BACKLOG 10

ssize_t send_all(int sockfd, const void *buf, size_t len, int flags) {
    size_t total_sent = 0;
    const char *ptr = (const char *)buf; 

    while (total_sent < len) {
        ssize_t bytes_sent = send(sockfd, ptr, len - total_sent, flags);
        if (bytes_sent <= 0) { 
            if (bytes_sent == 0) {
                return total_sent;
            } else {
                perror("send"); 
                return -1;
            }
        }
        total_sent += bytes_sent;
        ptr += bytes_sent;
    }
    return total_sent;
}

void handle_client(int client_fd) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_received = recv(client_fd, buffer, MAX_BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        return;
    }
    buffer[bytes_received] = '\0';

    printf("Received request:\n%s\n", buffer);

    const char *response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "\r\n"
                           "<html><body><h1>Hello, World!</h1></body></html>\r\n";

    ssize_t bytes_sent = send_all(client_fd, response, strlen(response), 0);
    if (bytes_sent == -1) {
        perror("send_all");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    const char *port = argv[1];

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, port, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        close(sockfd);
        exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    freeaddrinfo(res);

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        close(sockfd);
        exit(1);
    }

    printf("HTTP server listening on port %s...\n", port);

    while (1) {
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("accept");
            continue; 
        }

        handle_client(client_fd);
        close(client_fd);
    }

    close(sockfd); 
    return 0;
}
