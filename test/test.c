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

int setAbleToBroadcast(int fd) {
	int opt=0, res=-1;
	socklen_t optLen=sizeof(opt);

	res=getsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, &optLen);
	if(!res) {
		opt=~0;
		res=setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, optLen);
		if(res) {
			printf("Failed to invoke setsockopt().\n");
			return -1;
		}
	} else {
		printf("Failed to invoke getsockopt().\n");
		return -1;
	}
	
	return 0;
}

int sender(void) {
	int res=-1,
		fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
		i;
	Packet packet={
		.opCode=htons(OPCODE_RENEW),
		.dev={
			.name="PSEUDO_SENDER",
			.freq=htonl(96000)
		}
	};
	struct sockaddr_in addrToBind, addrPeer;
	
	if(fd<0) {
		printf("Failed to invoke socket().\n");
		goto failed0;
	}
	
	setAbleToBroadcast(fd);
	
	addrToBind.sin_family=AF_INET;
	addrToBind.sin_port=htons(DISCOVERY_PORT+1);
	addrToBind.sin_addr.s_addr=INADDR_ANY;
	
	res=bind(fd, &addrToBind, sizeof(addrToBind));
	if(res<0) {
		printf("Failed to invoke bind().\n");
		goto failed1;
	}

	addrPeer.sin_family=AF_INET;
	addrPeer.sin_port=htons(DISCOVERY_PORT);
	addrPeer.sin_addr.s_addr=INADDR_BROADCAST;
	
	for(i=0;;i++) {
		res=sendto(fd, &packet, sizeof(packet), 0, &addrPeer, sizeof(addrPeer));
		if(res<0) {
			printf("Failed to invoke sendto().\n");
			goto failed0;
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
	int res=-1,
		fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	Packet packet;
	struct sockaddr_in addrToBind, addrPeer;
	socklen_t addrPeerLen=sizeof(addrPeer);
	
	if(fd<0) {
		printf("Failed to invoke socket().\n");
		goto failed0;
	}

	addrToBind.sin_family=AF_INET;
	addrToBind.sin_port=htons(DISCOVERY_PORT);
	addrToBind.sin_addr.s_addr=INADDR_ANY;

	res=bind(fd, &addrToBind, sizeof(addrToBind));
	if(res<0) {
		printf("Failed to invoke bind().\n");
		goto failed1;
	}

	while(1) {
		res=recvfrom(fd, &packet, sizeof(packet), 0, &addrPeer, &addrPeerLen);
		if(res<0) {
			if(errno!=EINTR)
				printf("Failed to invoke recvfrom.\n");
			
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
	
	if(!strcmp(argv[1], "receiver"))
		return receiver();
	
	return sender();
}
