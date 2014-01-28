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

#define BROADCAST_IP   "255.255.255.255"
#define BROADCAST_PORT 8097

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

int main(int argc, char **argv)
{
	broadcast_udp_msg(msg0, 16); /* Initial list */
	broadcast_udp_msg(msg2, 16); /* Complete info */
	exit(EXIT_SUCCESS);
}
