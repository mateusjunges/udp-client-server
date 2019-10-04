#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#include "server.h"

#define MAX_SIZE 16000

int server_udp(int sockfd) {
    /*
     * Assume that sockfd is an open socket
     * which is bound to the server's address,
     * and (kinda) listening to messages,
     * so that it can receive messages from clients.
     * As such, all we have to do is just recvfrom the requests.
     */
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    size_t bufsize = MAX_SIZE * sizeof(uint32_t);
    uint32_t *buf = malloc(bufsize);
    if (buf == NULL) {
        perror("malloc");
        return -1;
    }
    ssize_t r = recvfrom(sockfd, buf, bufsize, 0, (struct sockaddr *) &client_addr, &client_addrlen); // used to receive messages from a socket
    if (r == -1) { // if recvfrom() returns -1, something went wrong
        perror("recvfrom");
        free(buf); // deallocates the memory previously allocated by a call to calloc, malloc, or realloc
        return -2;
    }

    char client_address[INET6_ADDRSTRLEN];
    int err = getnameinfo((struct sockaddr *) &client_addr, client_addrlen, client_address, sizeof(client_address), 0, 0, NI_NUMERICHOST);
    if (err != 0) {
        printf("Failure converting the address (code=%d)\n", err);
    } else
        printf("Client address: %s\n", client_address);

    size_t recv_bytes = r;
    int nint = recv_bytes / sizeof(uint32_t); //number of integers
    uint32_t sum = 0;
    for (int i = 0; i < nint; i++) {
        sum += ntohl(buf[i]);
    }
    free(buf); // deallocates the memory previously allocated by a call to calloc, malloc, or realloc
    sum = htonl(sum); // converts the unsigned integer hostlong from host byte order to network byte order.
    ssize_t s = sendto(sockfd, (void *) &sum, sizeof(uint32_t), 0, (struct sockaddr *) &client_addr, client_addrlen);
    if (s != sizeof(uint32_t)) {
        perror("sendto");
        return -2;
    }
    return 0;
}

int main() {
    struct sockaddr_in servaddr, cliaddr;
    int sockfd;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // If the socket() function returns -1, something went wrong
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0,
           sizeof(servaddr)); // copies the number 0 to the first all characters of the string pointed to, by the argument serveraddr
    memset(&cliaddr, 0,
           sizeof(cliaddr));   // copies the number 0 to the first all characters of the string pointed to, by the argument serveraddr

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(20000);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    while (1)
        server_udp(sockfd);

}
