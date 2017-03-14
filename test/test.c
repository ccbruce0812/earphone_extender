#define _GNU_SOURCE
#define _BSD_SOURCE
#define _SVID_SOURCE

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

#define DISCOVERY_PORT		(10000)

#define ANY_IP				"0.0.0.0"
#define BCAST_IP			"255.255.255.255"
#define MCAST_IP			"224.0.0.1"

#define OPCODE_MIN			(0)
#define OPCODE_RENEW		(OPCODE_MIN+1)
#define OPCODE_LEAVE		(OPCODE_MIN+2)

typedef struct __attribute__((packed)) {
	char name[32];
	unsigned long freq;
} DISCOVERY_Dev;

typedef struct __attribute__((packed)) {
	unsigned short opCode;
	DISCOVERY_Dev dev;
} Packet;

int initSocket(void) {
	int ret=-1;
	
	if((ret=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0) {
		printf("Failed to invoke socket(). errno=%d\n", errno);
		return -1;
	}
	
	return ret;
}

int bindTo(int fd, const char *str, unsigned short port) {
	int res=-1;
	struct sockaddr_in addr;
	
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(str);
	addr.sin_port=htons(port);
	if((res=bind(fd, &addr, sizeof(addr)))<0) {
		printf("Failed to invoke bind(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

int setBroadcast(int fd) {
	int opt=~0, res=-1;
	
	if((res=setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)))) {
		printf("Failed to invoke setsockopt(). res=%d, errno=%d\n", res, errno);
		return -1;
	}

	return 0;
}

int joinGroup(int fd, const char *str) {
	int res=-1;
	struct ip_mreq mreq;
	
	mreq.imr_multiaddr.s_addr=inet_addr(str);
	mreq.imr_interface.s_addr=INADDR_ANY;
	if((res=setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))<0) {
		printf("Failed to invoke setsockopt(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

int sender(void) {
	int res=-1, fd=-1, i;
	Packet packet={
		.opCode=htons(OPCODE_RENEW),
		.dev={
			.name="PSEUDO_SENDER",
			.freq=htonl(96000)
		}
	};
	struct sockaddr_in addr;
	
	if((fd=initSocket())<0)
		goto failed0;

#ifdef BCAST
	if(setBroadcast(fd)<0)
		goto failed1;
#endif
	
	if(bindTo(fd, ANY_IP, DISCOVERY_PORT+1)<0)
		goto failed1;

#ifdef MCAST
	if(joinGroup(fd, MCAST_IP)<0)
		goto failed1;
#endif

	for(i=0;;i++) {
		addr.sin_family=AF_INET;
		addr.sin_port=htons(DISCOVERY_PORT);
#ifdef MCAST
		addr.sin_addr.s_addr=inet_addr(MCAST_IP);
#else
		addr.sin_addr.s_addr=inet_addr(BCAST_IP);
#endif
		if((res=sendto(fd, &packet, sizeof(packet), 0, &addr, sizeof(addr)))<0) {
			printf("Failed to invoke sendto(). res=%d, errno=%d\n", res, errno);
			break;
		}

		printf("Round %d\n", i);
		usleep(1000000);
	}
	
	close(fd);
	return 0;

failed1:
	close(fd);

failed0:
	return -1;
}

int receiver(void) {
	int res=-1, fd=-1;
	struct sockaddr_in addr;
	socklen_t addrLen=sizeof(addr);
	Packet packet;
	
	if((fd=initSocket())<0)
		goto failed0;
	
	if(bindTo(fd, ANY_IP, DISCOVERY_PORT)<0)
		goto failed1;

#ifdef MCAST
	if(joinGroup(fd, MCAST_IP)<0)
		goto failed1;
#endif

	while(1) {
		if((res=recvfrom(fd, &packet, sizeof(packet), 0, &addr, &addrLen))<0) {
			if(errno!=EINTR)
				printf("Failed to invoke recvfrom(). res=%d, errno=%d\n", res, errno);
			break;
		}
		
		packet.opCode=ntohs(packet.opCode);
		packet.dev.freq=ntohl(packet.dev.freq);
		printf("opcode=%d, name=%s, freq=%ld.\n", packet.opCode, packet.dev.name, packet.dev.freq);
	}
	
	close(fd);
	return 0;

failed1:
	close(fd);
	
failed0:
	return -1;
}

int main(int argc, char *argv[]) {
	if(argc<2)
		return -1;
	
	if(!strcmp(argv[1], "r"))
		return receiver();
	
	return sender();
}
