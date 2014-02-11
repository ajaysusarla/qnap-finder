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

#include "qnap-finder.h"
#include "list.h"

#define DEBUG 1

#ifdef DEBUG
#define D(x) printf(x)
#else
#define D(x)
#endif

#define BROADCAST_IP    "255.255.255.255"
#define BROADCAST_PORT  8097
#define MAX_PACKET_SIZE 1000

#define HASH_TABLE_LEN  2000
#define HASH_KEY        997

/* QNAP Response */
struct qnap_response_hdr {
	uint64_t magic[2];
#define QNAP_MAGIC1 0x65536AECC39B0800
#define QNAP_MAGIC2 0x0425000000010001 /* basic */
#define QNAP_MAGIC3 0x0425000000050001 /* detail */
};

#define QNAP_PACKET_UNPACK(a) \
	(void)memcpy((a), &buf[len], sizeof(a)), len += sizeof(a)

/* Globals */
int sendsock;
int recvsock;
LIST *response_list;

struct sockaddr_in broadcast_addr;
int send_done = 0;

/* hastable */
in_addr_t add_tab[HASH_TABLE_LEN] = { 0 };


/* Endianness */
static union {
	char s[4];
	uint32_t u;
} endian_data;

/* short */
static uint16_t _data_to_le2(uint16_t in)
{
	uint16_t out;
	uint8_t *s = (uint8_t *)(void *)&in;
	uint8_t *d = (uint8_t *)(void *)&out;

	d[0] = s[1];
	d[1] = s[0];

	return out;
}

/* int */
static uint32_t _data_to_le4(uint32_t in)
{
	uint32_t out;
	uint8_t *s = (uint8_t *)(void *)&in;
	uint8_t *d = (uint8_t *)(void *)&out;

	d[0] = s[3];
	d[1] = s[2];
	d[2] = s[1];
	d[3] = s[0];

	return out;
}

/* long */
static uint64_t _data_to_le8(uint64_t in)
{
	uint64_t out;
	uint8_t *s = (uint8_t *)(void *)&in;
	uint8_t *d = (uint8_t *)(void *)&out;

	d[0] = s[7];
	d[1] = s[6];
	d[2] = s[5];
	d[3] = s[4];
	d[4] = s[3];
	d[5] = s[2];
	d[6] = s[1];
	d[7] = s[0];

	return out;
}

#define SWAP_DATA (endian_data.u == (uint32_t)0x01020304)
#define DATA_TO_LE8(x) ((uint64_t)(SWAP_DATA ? _data_to_le8(x) : (uint64_t)(x)))
#define DATA_TO_LE4(x) ((uint32_t)(SWAP_DATA ? _data_to_le4(x) : (uint32_t)(x)))
#define DATA_TO_LE2(x) ((uint16_t)(SWAP_DATA ? _data_to_le2(x) : (uint16_t)(x)))


int get_and_stash_local_ip_addr(void)
{
	struct ifaddrs *ifaddr, *ifa;

	D("-->get_and_stash_local_ip_addr\n");
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

	D("-->exit get_and_stash_local_ip_addr\n");
	return 0;
}

int send_msg(char *data, int len)
{
	int num;
	D("-->send_msg\n");
	num = sendto(sendsock, data, len, 0,
		     (struct sockaddr *)&broadcast_addr,
		     sizeof(broadcast_addr));

	D("-->exit send_msg\n");
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

	D("-->recv_func\n");

	(void)memcpy(endian_data.s, "\01\02\03\04", 4);

	fromsockaddrsize = sizeof (struct sockaddr_in);

	while (!send_done) {
		int pos;
		in_addr_t addr;
		NODE *node;

		sleep(1);

		memset(&data, 0, MAX_PACKET_SIZE);
		memset(&hostip, 0, INET_ADDRSTRLEN);

		node = create_node();

		D("\t-->recvfrom\n");
		ret = recvfrom(recvsock, data, MAX_PACKET_SIZE, 0,
			       fromsockaddr, &fromsockaddrsize);

		addr = fromsockaddr_struct.sin_addr.s_addr;
		pos = addr % HASH_KEY;
		inet_ntop(AF_INET, &(fromsockaddr_struct.sin_addr), hostip, INET_ADDRSTRLEN);

		node->len = ret;
		node->addr = addr;
		node->hostip = strdup(hostip);
		node->msg = (char *)malloc(ret);
		memcpy(node->msg, data, ret);

		if (add_tab[pos] == 0) {
			struct qnap_response_hdr h;
			char *buf;
			size_t len = 0;

			buf = data;

			QNAP_PACKET_UNPACK(h.magic);
			h.magic[0] = DATA_TO_LE8(h.magic[0]);
			h.magic[1] = DATA_TO_LE8(h.magic[1]);

			if (h.magic[0] == QNAP_MAGIC1) {
				if (h.magic[1] == QNAP_MAGIC2) {
					D("\t\t-->It is a Query reponse.\n");
				} else if (h.magic[1] == QNAP_MAGIC3) {
					D("\t\t-->It is a Detail reponse.\n");
				} else {
					D("\t\t-->Nothing?\n");
				}
			} else {
				fprintf(stderr, "Not a QNAP response\n");
			}
			/*
			printf("0x%llx:0x%llx\n",
			       (unsigned long long)h.magic[0],
			       (unsigned long long)h.magic[1]);
			*/
			/*
			D(printf("Received %d bytesfrom %s.\n", ret, hostip););
			//fwrite(data, MAX_PACKET_SIZE, 1, stdout);
			//printf("\n========================\n");
			*/
			//add_tab[pos] = addr; /* Add to hash table */
		}
	}

	D("-->exit recv_func\n");

	return NULL;
}

int net_init(char *broadcast_ip, unsigned short broadcast_port)
{
	int rv;
	int bcast = 1;
	int loop = 1;
	struct sockaddr_in si_me;

	D("-->net_init\n");

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

	D("-->exit net_init\n");
	return 0;
}

void net_fin(void)
{
	D("-->net_fin\n");
	if (sendsock >= 0)
		close(sendsock);
	if (recvsock >= 0)
		close(recvsock);

	D("-->exit net_fin\n");
}

int main(int argc, char **argv)
{
	pthread_t recv_thread;
	int ret;


	D("-->main\n");

	response_list = create_list();

	get_and_stash_local_ip_addr();

	net_init(BROADCAST_IP,BROADCAST_PORT);

	/* create recv_thread first */
	ret = pthread_create(&recv_thread, NULL, recv_func, NULL);
	if (ret) {
		fprintf(stderr, "Error: pthread_create(recv_func): %d\n", ret);
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	/* request query packet */
	send_msg(msg[0], SEND_MESG_LEN);

	/* request detail packet */
	send_msg(msg[2], SEND_MESG_LEN);

	if (pthread_join(recv_thread, NULL)) {
		fprintf(stderr, "error joining thread recv_thread\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	/* Process data here??? */


	ret = EXIT_SUCCESS;

cleanup:
	net_fin();
	free_list(response_list);

	D("-->exit main\n");
	exit(ret);

}
