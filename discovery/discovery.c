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
#include "common.h"

static TaskHandle_t g_task=NULL;
static void *g_context=NULL;
static DISCOVERY_onRenew g_onRenew=NULL;
static DISCOVERY_onLeave g_onLeave=NULL;
static DISCOVERY_onRelay g_onRelay=NULL;
static int g_fd=-1;
static bool g_isMulticast=false;

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
			
			case OPCODE_RELAY:
				g_onRelay(g_context, &addr);
				break;
			
			default:
				DBG("Bad format.\n");
		}
	}
}

int DISCOVERY_init(void *context, DISCOVERY_onRenew onRenew, DISCOVERY_onLeave onLeave, DISCOVERY_onRelay onRelay, bool multicast) {
	if(!onRenew || !onLeave || !onRelay) {
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
	g_onRelay=onRelay;
	g_isMulticast=multicast;

	if((g_fd=initSocket(false))<0)
		goto failed0;
	
	if(!g_isMulticast) {
		if(setBroadcast(g_fd)<0)
			goto failed1;
	}
	
	if(bindTo(g_fd, ANY_IP, DISCOVERY_PORT)<0)
		goto failed1;

	if(g_isMulticast) {
		if(joinGroup(g_fd, MCAST_IP)<0)
			goto failed1;
	}
	
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

int DISCOVERY_renew(const DISCOVERY_Dev *dev, const char *str) {
	Packet packet;
	
	if(!dev) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	if(!str)
		str=g_isMulticast?MCAST_IP:BCAST_IP;
	
	packet.opCode=htons(OPCODE_RENEW);
	memcpy(&packet.dev, dev, sizeof(packet.dev));
	packet.dev.freq=htonl(packet.dev.freq);
	
	return sendTo(g_fd, &packet, str);
}

int DISCOVERY_leave(const DISCOVERY_Dev *dev, const char *str) {
	Packet packet;
	
	if(!dev) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	if(!str)
		str=g_isMulticast?MCAST_IP:BCAST_IP;
	
	packet.opCode=htons(OPCODE_LEAVE);
	memcpy(&packet.dev, dev, sizeof(packet.dev));
	packet.dev.freq=0;
	
	return sendTo(g_fd, &packet, str);
}
