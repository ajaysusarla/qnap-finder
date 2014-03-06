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
#include <limits.h>

#include "qnap-finder.h"
#include "list.h"

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

#define FALSE 0
#define TRUE  1


#ifndef _POSIX_HOST_NAME_MAX
#define _POSIX_HOST_NAME_MAX 255
#endif

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

int verbose = FALSE;
int query_detail_info = FALSE;


struct sockaddr_in broadcast_addr;
int send_done = 0;

/* hastable */
in_addr_t add_tab[HASH_TABLE_LEN] = { 0 };

/* Offsets in bytes */

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

	if (verbose)
		printf("-->_data_to_le2\n");

	d[0] = s[1];
	d[1] = s[0];

	if (verbose)
		printf("<--_data_to_le2\n");

	return out;
}

/* int */
static uint32_t _data_to_le4(uint32_t in)
{
	uint32_t out;
	uint8_t *s = (uint8_t *)(void *)&in;
	uint8_t *d = (uint8_t *)(void *)&out;

	if (verbose)
		printf("-->_data_to_le4\n");

	d[0] = s[3];
	d[1] = s[2];
	d[2] = s[1];
	d[3] = s[0];

	if (verbose)
		printf("<--_data_to_le4\n");

	return out;
}

/* long */
static uint64_t _data_to_le8(uint64_t in)
{
	uint64_t out;
	uint8_t *s = (uint8_t *)(void *)&in;
	uint8_t *d = (uint8_t *)(void *)&out;

	if (verbose)
		printf("-->_data_to_le8\n");

	d[0] = s[7];
	d[1] = s[6];
	d[2] = s[5];
	d[3] = s[4];
	d[4] = s[3];
	d[5] = s[2];
	d[6] = s[1];
	d[7] = s[0];

	if (verbose)
		printf("-->_data_to_le8\n");

	return out;
}

#define SWAP_DATA (endian_data.u == (uint32_t)0x01020304)
#define DATA_TO_LE8(x) ((uint64_t)(SWAP_DATA ? _data_to_le8(x) : (uint64_t)(x)))
#define DATA_TO_LE4(x) ((uint32_t)(SWAP_DATA ? _data_to_le4(x) : (uint32_t)(x)))
#define DATA_TO_LE2(x) ((uint16_t)(SWAP_DATA ? _data_to_le2(x) : (uint16_t)(x)))


int get_and_stash_local_ip_addr(void)
{
	struct ifaddrs *ifaddr, *ifa;

	if (verbose)
		printf("-->get_and_stash_local_ip_addr\n");

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

	if (verbose)
		printf("<--get_and_stash_local_ip_addr\n");
	return 0;
}

int send_msg(char *data, int len)
{
	int num;

	if (verbose)
		printf("-->send_msg\n");
	num = sendto(sendsock, data, len, 0,
		     (struct sockaddr *)&broadcast_addr,
		     sizeof(broadcast_addr));


	if (verbose)
		printf("<--send_msg\n");

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

	if (verbose)
		printf("-->recv_func\n");

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
		node->ntype = NODE_TYPE_NONE;

		if (verbose)
			printf("\t-->recvfrom\n");

		ret = recvfrom(recvsock, data, MAX_PACKET_SIZE, 0,
			       fromsockaddr, &fromsockaddrsize);

		addr = fromsockaddr_struct.sin_addr.s_addr;
		pos = addr % HASH_KEY;
		inet_ntop(AF_INET, &(fromsockaddr_struct.sin_addr), hostip, INET_ADDRSTRLEN);

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
					if (verbose)
						printf("\t\t-->It is a Query reponse.\n");
					node->ntype = NODE_TYPE_BRIEF;
				} else if (h.magic[1] == QNAP_MAGIC3) {
					if (verbose)
						printf("\t\t-->It is a Detail reponse.\n");
					node->ntype = NODE_TYPE_BRIEF;
				} else {
					if (verbose)
						printf("\t\t-->Nothing?\n");
				}
			} else {
				if (verbose)
					printf("Not a QNAP response\n");
			}

			if (node->ntype != NODE_TYPE_NONE) {
				node->len = ret;
				node->addr = addr;
				node->hostip = strdup(hostip);
				node->msg = (unsigned char *)malloc(ret);
				node->msg = memcpy(node->msg, data, ret);
				add_node(response_list, node);
			} else {
				free(node);
			}

			if (verbose)
				printf("Magic: 0x%llx:0x%llx\n",
				       (unsigned long long)h.magic[0],
				       (unsigned long long)h.magic[1]);

			add_tab[pos] = addr; /* Add to hash table */
		}
	}

	if (verbose)
		printf("<--recv_func\n");

	return NULL;
}

int net_init(char *broadcast_ip, unsigned short broadcast_port)
{
	int rv;
	int bcast = 1;
	int loop = 1;
	struct sockaddr_in si_me;
	struct timeval tv;

	if (verbose)
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
		perror("setsockopt:SO_BROADCAST");
return -1;
	}

	/* Disable loopback so that we don't receieve our own datagrams */
	rv = setsockopt(sendsock, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&loop, sizeof(loop));
	if (rv < 0){
		perror("setsockopt:IP_MULTICAST_LOOP");
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
		perror("setsockopt:SO_BROADCAST");
		return -1;
	}

	/* Disable loopback so that we don't receieve our own datagrams */
	rv = setsockopt(recvsock, IPPROTO_IP, IP_MULTICAST_LOOP, (void *)&loop, sizeof(loop));
	if (rv < 0){
		perror("setsockopt:IP_MULTICAST_LOOP");
		return -1;
	}

	/* Time out on recv socket */
	tv.tv_sec = 30;  /* 30 Secs Timeout */
	tv.tv_usec = 0;
	rv = setsockopt(recvsock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	if (rv < 0){
		perror("setsockopt:SO_RCVTIMEO");
		return -1;
	}

	memset(&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(broadcast_port);
	si_me.sin_addr.s_addr = INADDR_ANY;
	bind(recvsock, (struct sockaddr *)&si_me, sizeof(struct sockaddr));

	if (verbose)
		printf("<--net_init\n");
	return 0;
}

void net_fin(void)
{
	if (verbose)
		printf("-->net_fin\n");
	if (sendsock >= 0)
		close(sendsock);
	if (recvsock >= 0)
		close(recvsock);

	if (verbose)
		printf("<--net_fin\n");
}


#define QNAP_FIELD_HOSTNAME  01

#define QNAP_FIELD_BRIEF_HOSTNAME_LEN 34
#define QNAP_FIELD_BRIEF_HOSTNAME     35

void parse_brief_response(unsigned char *resp, int len, char *hostip)
{
	int hostname_len;
	char hostname[_POSIX_HOST_NAME_MAX] = { 0 };
	int i;

	if (verbose)
		printf("-->parse_brief_response\n");

	hostname_len = resp[QNAP_FIELD_BRIEF_HOSTNAME_LEN];
	(void)memcpy(hostname, resp+QNAP_FIELD_BRIEF_HOSTNAME, hostname_len);

	fprintf(stdout, "\tHostname    : %s\n", hostname);
	fprintf(stdout, "\tIP Address  : %s\n", hostip);

	i = QNAP_FIELD_BRIEF_HOSTNAME + hostname_len + 2;

	fprintf(stdout, "\tType        : ");
	while (i < len) {
		fprintf(stdout, "%c", resp[i]);
		i++;
		if (resp[i] == 202 && resp[i+1] == 9) {/* check for 0xca 0x09*/
			i+=2;
			break;
		}
	}
	fprintf(stdout, "\n");

#if 0
	while (i < len) {
		fprintf(stdout, ">>> %d: %2x : %c\n", i, resp[i], resp[i]);
		i++;
	}
#endif
	fprintf(stdout, "\tURL         : https://%s/cgi-bin/login.html\n",
		hostip);

	fprintf(stdout, "\n");

	if (verbose)
		printf("<--parse_brief_response\n");
	return;
}

void parse_detail_response(unsigned char *resp, int len, char *hostip)
{
	if (verbose)
		printf("-->parse_detail_response\n");

	if (verbose)
		printf("<--parse_detail_response\n");
	return;
}

void print_qnap_list(LIST *list)
{
	NODE *node;
	int count = 0;

	if (verbose)
		printf("-->print_qnap_list\n");

	if (!list)
		return;

	if (!list->num_entries) {
		fprintf(stdout, "No QNAP boxes found.           \n");
		return;
	}

	node = list->first;

	while (node != NULL) {
		count++;

		fprintf(stdout, "%d)                         \n", count);
		if (node->ntype == NODE_TYPE_BRIEF) {
			parse_brief_response(node->msg, node->len, node->hostip);
		} else if(node->ntype == NODE_TYPE_DETAIL)
			parse_detail_response(node->msg, node->len, node->hostip);

		node = node->next;
	}

	if (verbose)
		printf("<--print_qnap_list\n");

}

static void print_version(void)
{
	printf("qnap-finder v%s\n", VERSION_STRING);
}

static void print_help(void)
{
	print_version();

	printf("\nUsage: qnap-finder [options]");
	printf("\n\noptions include:");
	printf("\n  --help|-h          This help text");
	printf("\n  --detail|-d        Query for detailed information.(default is brief)");
	printf("\n  --verbose|-v       Verbose debug");
	printf("\n  --version|-V       Prints current version\n\n");
}

static void parse_argument(const char *arg)
{
	const char *p = arg+1;

	if (verbose)
		printf("-->parse_argument\n");

	do {
		switch (*p) {
		case 'd':
			query_detail_info = TRUE;
			continue;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case 'v':
			verbose = TRUE;
			continue;
		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
		case '-': /* long options */
			if (strcmp(arg, "--detail") == 0) {
				query_detail_info = TRUE;
				return;
			}
			if (strcmp(arg, "--help") == 0) {
				print_help();
				exit(EXIT_SUCCESS);
			}
			if (strcmp(arg, "--verbose") == 0) {
				verbose = TRUE;
				return;
			}
			if (strcmp(arg, "--version") == 0) {
				print_version();
				exit(EXIT_SUCCESS);
			}
		default:
			fprintf(stderr, "Bad argument '%s'\n", arg);
			exit(1);
		}
	} while(*++p);

	if (verbose)
		printf("<--parse_argument\n");
}

int main(int argc, char **argv)
{
	pthread_t recv_thread;
	int ret;
	int i;


	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
	}

	if (verbose)
		printf("-->main\n");


	fprintf(stdout, "Looking for QNAP boxes.....\r");
	fflush(stdout);

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

	if (query_detail_info) {
		/* request detail packet */
		send_msg(msg[2], SEND_MESG_LEN);

	} else {
		/* request brief packet */
		send_msg(msg[0], SEND_MESG_LEN);
	}

	sleep(3);

	send_done = TRUE;

	if (pthread_join(recv_thread, NULL)) {
		fprintf(stderr, "error joining thread recv_thread\n");
		ret = EXIT_FAILURE;
		goto cleanup;
	}

	print_qnap_list(response_list);

	ret = EXIT_SUCCESS;

cleanup:
	net_fin();
	free_list(response_list);

	if (verbose)
		printf("<--main\n");
	exit(ret);

}
