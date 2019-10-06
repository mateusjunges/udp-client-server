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

    /*Recvfrom()
     * The recvfrom() function shall receive a message from a connection-mode or connectionless-mode socket. It is normally used with
     * connectionless-mode sockets because it permits the application to retrieve the source address of received data.
     * The recvfrom() function takes the following arguments:
     * socket: Specifies the socket file descriptor.
     * buffer: Points to the buffer where the message should be stored.
     * length: Specifies the length in bytes of the buffer pointed to by the buffer argument.
     * Specifies the type of message reception. Values of this argument are formed by logically OR'ing zero or more of the following values:
        MSG_PEEK
        Peeks at an incoming message. The data is treated as unread and the next recvfrom() or similar function shall still return this data.
        MSG_OOB
        Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific.
        MSG_WAITALL
        On SOCK_STREAM sockets this requests that the function block until the full amount of data can be returned. The function may return the smaller amount of data if the socket is a message-based socket, if a signal is caught, if the connection is terminated, if MSG_PEEK was specified, or if an error is pending for the socket.
     * address: A null pointer, or points to a sockaddr structure in which the sending address is to be stored. The length and format of the address depend on the address family of the socket.
     * address_len: Specifies the length of the sockaddr structure pointed to by the address argument.
     *
     * These calls return the number of bytes received, or -1 if an error occurred. The return value will be 0 when the peer has performed an orderly shutdown.
     * */
    ssize_t r = recvfrom(sockfd, buf, bufsize, 0, (struct sockaddr *) &client_addr, &client_addrlen); // used to receive messages from a socket
    if (r == -1) { // if recvfrom() returns -1, something went wrong
        perror("recvfrom");
        free(buf); // deallocates the memory previously allocated by a call to calloc, malloc, or realloc
        return -2;
    }

    char client_address[INET6_ADDRSTRLEN];
    /*
     * getnameinfo(): it converts a socket address to a
     * corresponding host and service, in a
     * protocol-independent manner.
     * */
    int err = getnameinfo((struct sockaddr *) &client_addr, client_addrlen, client_address, sizeof(client_address), 0, 0, NI_NUMERICHOST);
    if (err != 0) {
        printf("Failure converting the address (code=%d)\n", err);
    } else
        printf("Client address: %s\n", client_address);

    size_t recv_bytes = r;
    int nint = recv_bytes / sizeof(uint32_t); //number of integers
    uint32_t sum = 0;
    uint32_t prefix_sum[nint];

    // Initialize all array fields.
    for (int i = 0; i < nint; i++) {
        prefix_sum[i] = 0;
    }

    for (int i = 0; i < nint; i++) {
        sum += ntohl(buf[i]);
        if (i > 0) {
            prefix_sum[i] = prefix_sum[i-1] + ntohl(buf[i]);
        } else {
            prefix_sum[i] = ntohl(buf[i]);
        }
    }
    free(buf); // deallocates the memory previously allocated by a call to calloc, malloc, or realloc
    sum = htonl(sum); // converts the unsigned integer hostlong from host byte order to network byte order.

    /*
     * sendto()
     * The sendto() function sends data on the socket with descriptor socket.
     * The sendto() call applies to either connected or unconnected sockets
     * Params:
     * socket: The socket descriptor.
       buffer: The pointer to the buffer containing the message to transmit.
        length: The length of the message in the buffer pointed to by the msg parameter.
        flags: Setting these flags is not supported in the AF_UNIX domain. The following flags are available:
            MSG_OOB: Sends out-of-band data on the socket. Only SOCK_STREAM sockets support out-of-band data. The out-of-band data is a single byte.
                Before out-of-band data can be sent between two programs, there must be some coordination of effort. If the data is intended to not be read inline,
                the recipient of the out-of-band data must specify the recipient of the SIGURG signal that is generated when the out-of-band data is sent.
                If no recipient is set, no signal is sent. The recipient is set up by using F_SETOWN operand of the fcntl() command,
                specifying either a pid or gid. For more information on this operand, refer to the fcntl() command.
                The recipient of the data determines whether to receive out-of-band data inline or not inline by the setting of the SO_OOBINLINE option of setsockopt().
                For more information on receiving out-of-band data, refer to the setsockopt(), recv(), recvfrom() and recvmsg() commands.
            MSG_DONTROUTE: The SO_DONTROUTE option is turned on for the duration of the operation.
                This is usually used only by diagnostic or routing programs.
        address: The address of the target.
        addr_len: The size of the address pointed to by address.

        On success, these calls return the number of characters sent. On error, -1 is returned, and errno is set appropriately
     * */
    ssize_t s = sendto(sockfd, (void *) &sum, sizeof(uint32_t), 0, (struct sockaddr *) &client_addr, client_addrlen);
    ssize_t prefix = sendto(sockfd, (void *) &prefix_sum, sizeof(uint32_t)*nint, 0, (struct sockaddr *) &client_addr, client_addrlen);

    if (s != sizeof(uint32_t)) {
        perror("sendto");
        return -2;
    }
    if (prefix != sizeof(uint32_t)*nint) {
        perror("sendto (prefix sum)");
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
    /*
     * bind()
     * Defines a relationship between the socket you created and the addresses that are available on your host.
     * For example you can bind a socket on all addresses or on a specific IP which has been configured on a
     * network adapter by the host's operating system.
     * assigns the address specified by addr to the socket referred to by the file
       descriptor sockfd.  addrlen specifies the size, in bytes, of the
       address structure pointed to by addr.  Traditionally, this operation
       is called “assigning a name to a socket”
       On success, zero is returned.  On error, -1 is returned, and errno is
       set appropriately.
     * */
    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    while (1) {
        server_udp(sockfd);

    }
}
