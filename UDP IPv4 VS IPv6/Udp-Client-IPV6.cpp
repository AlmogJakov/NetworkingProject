//
// Udp-Client.cpp Skeleton Ping-Pong.
// Robert Iakobashvili for Ariel uni, BSD/MIT/Apache-license.
//
// 1. Correct the IP-address to be your local-IP address.
// 2. Check that server and client have the same rendezvous  port and the same IP-address
// 3. Compile using either MSVC or g++ compiler.
// 4. First run the server.
// 5. In the console run netstat -a to see the server UDP socket at port 5060.
// 6. Run the client and capture the communication using wireshark.
//
#include "stdio.h"

#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5060

int main() {
	int s = -1;
	char bufferReply[80] = { '\0' };
	char message[] = "Good morning, Vietnam\n";
	int messageLen = strlen(message) + 1;
	// Create socket
	if ((s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Could not create socket : %d\n", errno);
		return -1;
	}
	// Setup the server address structure.
	// Port and IP should be filled in network byte order (learn bin-endian, little-endian)
	struct sockaddr_in6 serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin6_family = AF_INET6;
	serverAddress.sin6_port = htons(SERVER_PORT);
//int rval = inet_pton(AF_INET6, (const char*)SERVER_IP_ADDRESS, &serverAddress.sin6_addr);
    int rval = inet_pton(AF_INET6, "::1", &serverAddress.sin6_addr);//"::1 is for the loopback , otherwise we would have to get
    //our IPv6 address ,which we cant get since we dont have IPv6 in our computers
	if (rval <= 0) {
		printf("inet_pton() failed\n");
		return -1;
	}
	//send the message
	if (sendto(s, message, messageLen, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
		printf("sendto() failed with error code  : %d\n", errno);
		return -1;
	}
	struct sockaddr_in6 fromAddress;
	socklen_t fromAddressSize = sizeof(fromAddress);
	memset((char *)&fromAddress, 0, sizeof(fromAddress));
	// try to receive some data, this is a blocking call
	if (recvfrom(s, bufferReply, sizeof(bufferReply)-1, 0, (struct sockaddr*)&fromAddress, &fromAddressSize) == -1) {
		printf("recvfrom() failed with error code  : %d\n", errno);
		return -1;
	}
	//printf(bufferReply);
	printf("%s", bufferReply);
	close(s);
    return 0;
}
