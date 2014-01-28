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
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char **argv)
{
	struct sockaddr_in si_me, si_other;
	int s;
	int port=8097;
	int broadcast=1;

	s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	setsockopt(s, SOL_SOCKET, SO_BROADCAST,
		   &broadcast, sizeof broadcast);

	memset(&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = INADDR_ANY;

	bind(s, (struct sockaddr *)&si_me, sizeof(struct sockaddr));

	while(1)
	{
		char buf[1000];
		int len;
		unsigned slen=sizeof(struct sockaddr);
		char hostip[INET_ADDRSTRLEN];

		len = recvfrom(s, buf, sizeof buf, 0, (struct sockaddr *)&si_other, &slen);

		inet_ntop(AF_INET, &(si_other.sin_addr), hostip, INET_ADDRSTRLEN);

		printf("len received:%d from %s\n", len, hostip);

		fwrite(buf, len, 1, stdout);
		fprintf(stdout, "\n");
		fflush(stdout);
	}

}
