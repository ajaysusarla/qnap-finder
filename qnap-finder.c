/*
 * qnap-finder
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "qnap-finder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


/* socket descriptors  */
int sendsock;
int recvsock;

struct sockaddr_in broadcast_addr;

/* Request Messages */
char msg0[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x6c,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x04 };
char msg1[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x6c,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x04 };
char msg2[] = {
	0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c,
	0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char msg3[] = {
	0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c,
	0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char msg4[] = {
	0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c,
	0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char msg5[] = {
	0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c,
	0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };

/* Full list of messages
char peer0_0[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x6c, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_1[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x6c, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_2[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_3[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_4[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_5[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_6[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x6c, 
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_7[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_8[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_9[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_10[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_11[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_12[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_13[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
char peer0_14[] = {
0x00, 0x08, 0x9b, 0xc3, 0xec, 0x6a, 0x43, 0x6c, 
0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x25, 0x04 };
 */
#define BROADCAST_IP   "255.255.255.255"
#define BROADCAST_PORT 8097
#define MAX_PACKET_SIZE 5000

void broadcast_udp_msg(void *msg, int msg_len)
{
	int sockfd;
	struct sockaddr_in broadcast_addr;
	char *broadcast_ip;
	unsigned short broadcast_port;
	int broadcast = 1;
	int loop = 1;
	int bytes_sent;

	broadcast_ip = BROADCAST_IP;
	broadcast_port = BROADCAST_PORT;

	/* Create a socket datagram */
	sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0) {
		fprintf(stderr, "could not create socket!\n");
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Initialize the broadcast socket addr */
	memset(&broadcast_addr, 0, sizeof(broadcast_addr));
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip);
	broadcast_addr.sin_port = htons(broadcast_port);

	/* Set broadcast on socket */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast, sizeof(broadcast)) < 0) {
		fprintf(stderr, "could not set socket option SO_BROADCAST!\n");
		perror("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	/* Disable loopback so that we don't receieve our own datagrams */
	if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&loop, sizeof(loop)) < 0) {
		fprintf(stderr, "could not set socket option IP_MULTICAST_LOOP!\n");
		perror("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	/* Send the message to the multicast group */
	bytes_sent = sendto(sockfd, msg, msg_len, 0,
			    (struct sockaddr *)&broadcast_addr,
			    sizeof(broadcast_addr));

	if (bytes_sent != msg_len) {
		fprintf(stderr, "sendto failed!\n");
		perror("sendto");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "message sent:%d\n", bytes_sent);
}

/*
int main(int argc, char **argv)
{
	broadcast_udp_msg(msg0, 16); // Initial list
	broadcast_udp_msg(msg2, 16); // Complete info
	exit(EXIT_SUCCESS);
}
*/

int send_query(void)
{
	int num;
	int msg_len = 16;

	printf("-->send_query\n");
	/* Send the message to the multicast group */
	num = sendto(sendsock, msg0, msg_len, 0,
		     (struct sockaddr *)&broadcast_addr,
		     sizeof(broadcast_addr));

	if (num != msg_len) {
		fprintf(stderr, "sendto failed!\n");
		perror("sendto");
		return -1;
	}

	printf("-->exit send_query\n");
	return 0;
}

void process_recv_data(void)
{
	char data[MAX_PACKET_SIZE];
	char hostip[INET_ADDRSTRLEN];
	struct sockaddr_in fromsockaddr_struct;
	struct sockaddr * fromsockaddr = (struct sockaddr *) &fromsockaddr_struct;
	socklen_t fromsockaddrsize;
	int ret;

	printf("-->process_recv_data\n");

	fromsockaddrsize = sizeof (struct sockaddr_in);

	ret = recvfrom(recvsock, data, MAX_PACKET_SIZE, 0,
		       fromsockaddr, &fromsockaddrsize);

	inet_ntop(AF_INET, &(fromsockaddr_struct.sin_addr), hostip, INET_ADDRSTRLEN);
	printf("Received %d bytesfrom %s.\n", ret, hostip);
	printf("========================\n");
	//fwrite(data, MAX_PACKET_SIZE, 1, stdout);
	printf("\n========================\n");

	printf("-->exit process_recv_data\n");

}

void loop(void)
{
	fd_set readfd;
	fd_set writefd;
	int netfd;
	int maxfd = 0;
	int readwrite = 0;
	struct timeval selecttime;
	int rv;
	int i = 0;

	printf("-->loop\n");

	while (i++ < 100) {
		FD_ZERO(&readfd);
		FD_ZERO(&writefd);

		maxfd = 0;

		netfd = recvsock;
		FD_SET(netfd, &readfd);
		if(netfd >= maxfd)
			maxfd = netfd + 1;

		do {
			/* selec timeout 0.1s */
			selecttime.tv_sec = 0;
			selecttime.tv_usec = 1000000;

			if(readwrite) {
				rv = select(maxfd, (void *)&readfd,
					    &writefd, NULL, &selecttime);
			} else {
				rv = select(maxfd, (void *)&readfd,
					    NULL, NULL, &selecttime);
			}
		} while((rv < 0) && (errno == EINTR));

		if(rv < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		readwrite = 0;

		if (FD_ISSET(netfd, &readfd)) {
			process_recv_data();
			readwrite = 1;
		}

		send_query();
	}
	printf("-->exit loop\n");
}

int net_init(char *broadcast_ip, unsigned short broadcast_port)
{
	int rv;
	int bcast = 1;
	int loop = 1;
	struct sockaddr_in si_me;

	printf("-->net_init\n");

	/** Sending socket **/
	sendsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sendsock < 0) {
		perror("socket");
		return -1;
	}

	/* Initialize the broadcast socket addr */
	memset(&broadcast_addr, 0, sizeof(broadcast_addr));
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip);
	broadcast_addr.sin_port = htons(broadcast_port);

	/* Set broadcast on socket */
	rv = setsockopt(sendsock, SOL_SOCKET, SO_BROADCAST, (void *)&bcast, sizeof(bcast));
	if (rv < 0){
		perror("setsockopt");
		return -1;
	}

	/* Disable loopback so that we don't receieve our own datagrams */
	rv = setsockopt(sendsock, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&loop, sizeof(loop));
	if (rv < 0){
		perror("setsockopt");
		return -1;
	}

	/** Receving socket **/
	recvsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (recvsock < 0) {
		perror("socket");
		return -1;
	}

	/* Set broadcast on socket */
	rv = setsockopt(recvsock, SOL_SOCKET, SO_BROADCAST, (void *)&bcast, sizeof(bcast));
	if (rv < 0){
		perror("setsockopt");
		return -1;
	}

	/* Disable loopback so that we don't receieve our own datagrams */
	rv = setsockopt(recvsock, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&loop, sizeof(loop));
	if (rv < 0){
		perror("setsockopt");
		return -1;
	}

	memset(&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(broadcast_port);
	si_me.sin_addr.s_addr = INADDR_ANY;
	bind(recvsock, (struct sockaddr *)&si_me, sizeof(struct sockaddr));

	printf("-->exit net_init\n");
	return 0;
}

void net_fin(void)
{
	printf("-->net_fin\n");
	if (sendsock >= 0)
		close(sendsock);
	if (recvsock >= 0)
		close(recvsock);

	printf("-->exit net_fin\n");
}

int main(int argc, char **argv)
{
	printf("-->main\n");
	net_init(BROADCAST_IP, BROADCAST_PORT);
	loop();
	net_fin();

	printf("-->exit main\n");
	exit(EXIT_SUCCESS);
}
