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
#include <stdbool.h>

#define HELPERSVR
#include "discovery.h"
#include "common.h"

int server(bool multicast) {
	int res=-1, fd=-1;
	struct sockaddr_in addr;
	socklen_t addrLen=sizeof(addr);
	Packet packet;
	unsigned long long prev=0, cur=0;
	
	if((fd=initSocket(true))<0)
		goto failed0;

	if(!multicast) {
		if(setBroadcast(fd)<0)
			goto failed1;
	}
	
	if(bindTo(fd, ANY_IP, DISCOVERY_PORT)<0)
		goto failed1;

	if(multicast) {
		if(joinGroup(fd, MCAST_IP)<0)
			goto failed1;
	}

	prev=cur=now();
	while(1) {
		if((res=recvfrom(fd, &packet, sizeof(packet), MSG_DONTWAIT, &addr, &addrLen))>=0) {
			if(ntohs(packet.opCode)==OPCODE_RENEW)
				sendTo(fd, &packet, multicast?MCAST_IP:BCAST_IP);
		} else {
			if(errno==EINTR) {
				DBG("Bye.\n");
				break;
			} else if(errno==EAGAIN)
				usleep(100000);
			else {
				DBG("Failed to invoke recvfrom(). res=%d, errno=%d\n", res, errno);
				break;
			}
		}
		
		cur=now();
		if(cur-prev>=5000000) {
			packet.opCode=htons(OPCODE_RELAY);
			memset(&packet.dev, 0, sizeof(packet.dev));
			
			sendTo(fd, &packet, multicast?MCAST_IP:BCAST_IP);
			
			prev=cur;
		}
	}
	
	close(fd);
	return 0;

failed1:
	close(fd);
	
failed0:
	return -1;
}

int main(int argc, char *argv[]) {
	bool multicast=false;
	
	if(argc>=2 && argv[1][0]=='m')
		multicast=true;
	
	return server(multicast);
}
