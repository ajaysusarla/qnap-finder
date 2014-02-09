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
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#define DEBUG 1

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#define BROADCAST_IP    "255.255.255.255"
#define BROADCAST_PORT  8097
#define MAX_PACKET_SIZE 1000

#define HASH_TABLE_LEN  2000
#define HASH_KEY        997


typedef enum {
	MESSAGE_TYPE_QUERY,
	MESSAGE_TYPE_DETAIL
} message_type_t;

message_type_t mtype;

struct QNAPQueryReplyPacket {
	char data[334];
};

/* Globals */
int sendsock;
int recvsock;

struct sockaddr_in broadcast_addr;
int send_done = 0;

/* hastable */
in_addr_t add_tab[HASH_TABLE_LEN] = { 0 };


int get_and_stash_local_ip_addr(void)
{
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		return -1;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET) {
			int pos;
			in_addr_t addr;
			struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;

			addr = sin->sin_addr.s_addr;
			pos = addr % HASH_KEY;

			add_tab[pos] = addr;
		}
	}

	freeifaddrs(ifaddr);

	return 0;
}

int send_msg(message_type_t type, char *data, int len)
{
	int num;
	D(printf("-->send_msg\n");)
	num = sendto(sendsock, data, len, 0,
		     (struct sockaddr *)&broadcast_addr,
		     sizeof(broadcast_addr));

	D(printf("-->exit send_msg\n");)
	if (num != len) {
		perror("sendto");
		return -1;
	} else
		return 0;
}

void *recv_func(void *arg)
{
	char data[MAX_PACKET_SIZE];
	char hostip[INET_ADDRSTRLEN];
	struct sockaddr_in fromsockaddr_struct;
	struct sockaddr * fromsockaddr = (struct sockaddr *) &fromsockaddr_struct;
	socklen_t fromsockaddrsize;
	int ret;

	D(printf("-->recv_func\n");)

	fromsockaddrsize = sizeof (struct sockaddr_in);

	while (!send_done) {
		int pos;
		in_addr_t addr;

		sleep(1);
		memset(&data, 0, MAX_PACKET_SIZE);

		D(printf("\t-->recvfrom\n");)
		ret = recvfrom(recvsock, data, MAX_PACKET_SIZE, 0,
			       fromsockaddr, &fromsockaddrsize);

		addr = fromsockaddr_struct.sin_addr.s_addr;
		pos = addr % HASH_KEY;
		inet_ntop(AF_INET, &(fromsockaddr_struct.sin_addr), hostip, INET_ADDRSTRLEN);
		if (add_tab[pos] == 0) {
			add_tab[pos] = addr;
			printf("Received %d bytesfrom %s.\n", ret, hostip);
			printf("========================\n");
			fwrite(data, MAX_PACKET_SIZE, 1, stdout);
			//printf("\n========================\n");
		}
	}

	D(printf("-->exit recv_func\n");)

	return NULL;
}

int net_init(char *broadcast_ip, unsigned short broadcast_port)
{
	int rv;
	int bcast = 1;
	int loop = 1;
	struct sockaddr_in si_me;

	D(printf("-->net_init\n");)

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

	D(printf("-->exit net_init\n");)
	return 0;
}

void net_fin(void)
{
	D(printf("-->net_fin\n");)
	if (sendsock >= 0)
		close(sendsock);
	if (recvsock >= 0)
		close(recvsock);

	D(printf("-->exit net_fin\n");)
}

int main(int argc, char **argv)
{
	pthread_t recv_thread;
	int ret;


	get_and_stash_local_ip_addr();

	net_init(BROADCAST_IP,BROADCAST_PORT);

	/* create recv_thread first */
	ret = pthread_create(&recv_thread, NULL, recv_func, NULL);
	if (ret) {
		fprintf(stderr, "Error: pthread_create(recv_func): %d\n", ret);
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	/* Send query msg */
	send_msg(MESSAGE_TYPE_QUERY, msg[0], SEND_MESG_LEN);

	if (pthread_join(recv_thread, NULL)) {
		fprintf(stderr, "error joining thread recv_thread\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	/* Process data here??? */


	ret = EXIT_SUCCESS;

cleanup:
	net_fin();

	exit(ret);

}

/*
  header: 00 08 9b c3 ec 6a 53 65  01 00 01 00 00 00 25 04 [query]
          00 08 9b c3 ec 6a 53 65  01 00 05 00 00 00 25 04 [detail]

  footer: ff 04 00 00 00 00 [query]
          ff 04 00 00 00 00 [detail]

 */
