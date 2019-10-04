#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "limits.h"

#define MAX_SIZE 16000

int get_sum_of_ints_udp_sol(int sockfd, uint32_t *tab, size_t length, long *rep)
{
	/*
	 * We assume that sockfd is an open socket,
	 * already set up to send packets to the server,
	 * and that we only have to send one message to the server,
	 * and receive its answer (one message too).
	 * Because it's UDP.
	 */
	if (length > MAX_SIZE) {
		fprintf(stderr, "Packet is too large to be sent over UDP");
		return -3;
	}
	
	ssize_t sent_bytes = 0, recv_bytes = 0;
	size_t total_bytes = length * sizeof(uint32_t);
	uint32_t *newtab = malloc(total_bytes);
	if (newtab == NULL) {
		perror("malloc");
		return -1;
	}
	for (unsigned int i = 0; i < length; i++) {
		newtab[i] = htonl(tab[i]); // converts the unsigned integer hostlong from host byte order to network byte order.
	}
	/*send()
	 * The send() function sends data on the socket with descriptor socket. The send() call applies to all connected sockets.
	 * Params
	 * socket: The socket descriptor.
	 * msg: The pointer to the buffer containing the message to transmit.
	 * length: The length of the message pointed to by the msg parameter.
	 * flags
        The flags parameter is set by If more than one flag is specified, the logical OR operator (|) must be used to separate them.
        MSG_OOB
            Sends out-of-band data on sockets that support this notion. Only SOCK_STREAM sockets support out-of-band data.
            The out-of-band data is a single byte.
            Before out-of-band data can be sent between two programs, there must be some coordination of effort.
            If the data is intended to not be read inline, the recipient of the out-of-band data must specify the recipient
            of the SIGURG signal that is generated when the out-of-band data is sent. If no recipient is set, no signal is sent.
            The recipient is set up by using F_SETOWN operand of the fcntl command, specifying either a pid or gid.
            For more information on this operand, refer to the fcntl command.
            The recipient of the data determines whether to receive out-of-band data inline or not inline by the setting
            of the SO_OOBINLINE option of setsockopt(). For more information on receiving out-of-band data,
            refer to the setsockopt(), recv(), recvfrom() and recvmsg() commands.

        MSG_DONTROUTE: The SO_DONTROUTE option is turned on for the duration of the operation.
        This is usually used only by diagnostic or routing programs.

        If successful, send() returns 0 or greater indicating the number of bytes sent.
        However, this does not assure that data delivery was complete.
        A connection can be dropped by a peer socket and a SIGPIPE signal generated at a
        later time if data delivery is not complete.
        If unsuccessful, send() returns -1 indicating locally detected errors and sets errno
        to one of the following values. No indication of failure to deliver is implicit in a send() routine.
	 * */
	sent_bytes = send(sockfd, newtab, total_bytes, 0); // Try to send a UDP package to the server.
	if (sent_bytes != (ssize_t) total_bytes) { // Check if all bytes was successfully sent.
		if (sent_bytes == -1) // If the send() function returns -1, something went wrong.
			perror("send");
		free(newtab); // deallocates the memory previously allocated by a call to calloc, malloc, or realloc
		return -2;
	}
	free(newtab); //deallocates the memory previously allocated by a call to calloc, malloc, or realloc
	uint32_t answer;
	recv_bytes = recv(sockfd, &answer, sizeof(uint32_t), 0); //used to receive messages from a socket
	/* Recv Parameters:
	 * sockfd: The socket descriptor
	 * answer: The pointer to the buffer that receives the data.
	 * len: The length in bytes of the buffer pointed to by the buf parameter
	 * flags: Not used, but can receive one or more of the following flags: MSG_CONNTERM|MSG_OOB|MSG_PEEK|MSG_WAITALL
	 * */
	if (recv_bytes != sizeof(uint32_t)) {
		if (recv_bytes == -1) //if recv() function returns -1, something went wrong.
			perror("recv");
		return -2;
	}
	*rep = ntohl(answer); //converts the unsigned integer netlong from network byte order to host byte order
	return 0;
}

int main(int argc, char *argv[])
{
    int l;
    int i;

    uint32_t tab[argc-2]; //Array of integers. Size of argc - 2, since argv[1] is the program name and argv[2] the server ip.

    for (i = 2; i < argc; i++) {
         l = atoi(argv[i]);
         tab[i-2] = l; // Add each number to the tab array.
    }

    int sockfd; // Socket descriptor.
    struct sockaddr_in   servaddr;
    int status;
    struct hostent *hostinfo;
  
    // Creating socket file descriptor
    /*Socket params:
     * domain: The address domain requested, either AF_INET, AF_INET6, AF_UNIX, or AF_RAW
     * type: The type of socket created, either SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW
     * The protocol requested. Some possible values are 0, IPPROTO_UDP, or IPPROTO_TCP
     * */
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {  // If the socket() function returns -1, something went wrong.
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (argc < 3) {
      printf("Use: ./client <server address> <N integers space separated>\n");
      printf("Example: ./client localhost 1 2 3 4 5 6 7 8 9");
      exit(1);
    }
    hostinfo = gethostbyname(argv[1]); //returns a structure of type hostent for the given host name
    memset(&servaddr, 0, sizeof(servaddr)); //copies the number 0 to the first all characters of the string pointed to, by the argument serveraddr

    // Filling server information
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(20000);
    memcpy(&servaddr.sin_addr, hostinfo->h_addr_list[0], hostinfo->h_length);

    /*
     * The connect() system call initiates a connection on a socket.
     * int connect(int s, const struct sockaddr *name, socklen_t namelen);
     * Params:
     * s: Specifies the integer descriptor of the socket to be connected.
     * name: Points to a sockaddr structure containing the peer address. The length and format of the address depend on the address family of the socket.
     * namelen: Specifies the length of the sockaddr structure pointed to by name argument.
     * */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
      perror("Connect");
      close(sockfd);
      return -1;
    }
    long _result;
    int n = sizeof(tab)/sizeof(tab[0]);
    int tablen = n*sizeof(uint32_t);
    

    status = get_sum_of_ints_udp_sol(sockfd, tab, n, &_result); //Sum up all array numbers.
    
    if (!status) {
      printf("Result: %ld\n", _result);
      return 0;
    }  else {
      printf("Error: %d\n", status);
      return 1;
    }
}

