#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <esp8266.h>
#include <esp/uart.h>

#include <etstimer.h>
#include <espressif/esp_common.h>

#include <lwip/inet.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
#include <lwip/mem.h>
#include <lwip/api.h>
#include <lwip/sockets.h>

#include <string.h>
#include <assert.h>

#include "../common/toolhelp.h"

#include "discovery.h"

#define DISCOVERY_PORT		(10000)
#define ANY_IP				"0.0.0.0"
#define GROUP_IP			"224.0.0.1"

#define OPCODE_MIN			(0)
#define OPCODE_RENEW		(OPCODE_MIN+1)
#define OPCODE_LEAVE		(OPCODE_MIN+2)

typedef struct __attribute__((packed)) {
	unsigned short opCode;
	DISCOVERY_Dev dev;
} Packet;

static TaskHandle_t g_task=NULL;
static void *g_context=NULL;
static DISCOVERY_onRenew g_onRenew=NULL;
static DISCOVERY_onLeave g_onLeave=NULL;
static int g_fd=-1;

static int initSocket(void) {
	int ret=-1;
	
	if((ret=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0) {
		DBG("Failed to invoke socket(). errno=%d\n", errno);
		return -1;
	}
	
	return ret;
}

static int bindTo(int fd, const char *str, unsigned short port) {
	int res=-1;
	struct sockaddr_in addr;
	
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(str);
	addr.sin_port=htons(port);
	if((res=bind(fd, (struct sockaddr *)&addr, sizeof(addr)))<0) {
		DBG("Failed to invoke bind(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

static int joinGroup(int fd, const char *str) {
	int res=-1;
	ip_mreq mreq;
	
	mreq.imr_multiaddr.s_addr=inet_addr(str);
	mreq.imr_interface.s_addr=INADDR_ANY;
	if((res=setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))<0) {
		DBG("Failed to invoke setsockopt(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

static void discoveryTask(void *param) {
	int res=-1;
	Packet packet;
	struct sockaddr_in addr;
	socklen_t addrLen=sizeof(addr);

	while(1) {
		if((res=recvfrom(g_fd, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, &addrLen))<0) {
			DBG("Failed to invoke recvfrom(). res=%d, errno=%d\n", res, errno);
			continue;
		}

		packet.opCode=ntohs(packet.opCode);
		packet.dev.freq=ntohl(packet.dev.freq);
		switch(packet.opCode) {
			case OPCODE_RENEW:
				g_onRenew(g_context, &packet.dev);
				break;
				
			case OPCODE_LEAVE:
				g_onLeave(g_context, &packet.dev);
				break;
				
			default:
				DBG("Bad format.\n");
		}
	}
}

int DISCOVERY_init(void *context, DISCOVERY_onRenew onRenew, DISCOVERY_onLeave onLeave) {
	if(!onRenew || !onLeave) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	if(g_task) {
		DBG("This module has already been initialized.\n");
		goto failed0;
	}
	
	g_context=context;
	g_onRenew=onRenew;
	g_onLeave=onLeave;

	if((g_fd=initSocket())<0)
		goto failed0;
	
	if(bindTo(g_fd, ANY_IP, DISCOVERY_PORT+1)<0)
		goto failed1;
	
	if(joinGroup(g_fd, GROUP_IP)<0)
		goto failed1;
	
	if(xTaskCreate(discoveryTask, "discoveryTask", 512, NULL, 4, &g_task)!=pdPASS) {
		DBG("Failed to create task.\n");
		goto failed1;
	}
	
	return 0;

failed1:
	close(g_fd);
	
failed0:
	return -1;
}

int DISCOVERY_renew(const DISCOVERY_Dev *dev) {
	int res=-1, fd=-1;
	Packet packet={
		.opCode=htons(OPCODE_RENEW)
	};
	struct sockaddr_in addr;
	
	if(!dev) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	memcpy(&packet.dev, dev, sizeof(packet.dev));
	
	if((fd=initSocket())<0)
		goto failed0;
	
	if(bindTo(fd, ANY_IP, DISCOVERY_PORT+1)<0)
		goto failed1;
	
	if(joinGroup(fd, GROUP_IP)<0)
		goto failed1;

	addr.sin_family=AF_INET;
	addr.sin_port=htons(DISCOVERY_PORT);
	addr.sin_addr.s_addr=inet_addr(GROUP_IP);
	if((res=sendto(fd, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr)))<0)
		DBG("Failed to invoke sendto(). res=%d, errno=%d\n", res, errno);
	
	close(fd);
	return 0;

failed1:
	close(fd);

failed0:
	return -1;
}
