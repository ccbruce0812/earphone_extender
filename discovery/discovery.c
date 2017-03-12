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

#define OPCODE_MIN		(0)
#define OPCODE_RENEW	(OPCODE_MIN+1)
#define OPCODE_LEAVE	(OPCODE_MIN+2)

typedef struct __attribute__((packed)) {
	unsigned short opCode;
	DISCOVERY_Dev dev;
} Packet;

static bool g_isSvrInited=false;
static void *g_svrContext=NULL;
static DISCOVERY_onRenew g_onRenew=NULL;
static DISCOVERY_onLeave g_onLeave=NULL;
static struct udp_pcb *g_udpSvrCtx=NULL;

static bool g_isInited=false;
static void *g_context=NULL;
static struct udp_pcb *g_udpCtx=NULL;

static void onData(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, unsigned short port) {
	Packet data;
	unsigned int idx=0;
	struct pbuf *now=p;
	bool abort=false;

	if(!now) {
		DBG("No payload.\n");
		return;
	}

	memset(&data, 0, sizeof(data));
	while(now) {
		if(idx+now->len>sizeof(data)) {
			abort=true;
			DBG("Out of buffer.\n");
			break;
		}
		
		memcpy(&((unsigned char *)&data)[idx], now->payload, now->len);
		idx+=now->len;
		now=now->next;
	}

	pbuf_free(p);
	
	if(abort) {
		DBG("Aborted. Unmatched buffer size. (Expected=%d, Actual=%d)\n", sizeof(data), p->tot_len);
		return;
	}
	
	data.opCode=ntohs(data.opCode);
	data.dev.freq=ntohl(data.dev.freq);
	switch(data.opCode) {
		case OPCODE_RENEW:
			g_onRenew(g_svrContext, &data.dev);
			break;
			
		case OPCODE_LEAVE:
			g_onLeave(g_svrContext, &data.dev);
			break;
			
		default:
			DBG("Bad format.\n");
	}
}

int DISCOVERY_initSvr(void *context, DISCOVERY_onRenew onRenew, DISCOVERY_onLeave onLeave) {
	if(!onRenew || !onLeave) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	if(g_isSvrInited) {
		DBG("This module has already been initialized.\n");
		goto failed;
	}
	
	g_svrContext=context;
	g_onRenew=onRenew;
	g_onLeave=onLeave;

	g_udpSvrCtx=udp_new();
	if(!g_udpSvrCtx) {
		DBG("Failed to invoke udp_new().\n");
		goto failed;
	}

	udp_bind(g_udpSvrCtx, IP_ADDR_ANY, DISCOVERY_PORT);
	udp_recv(g_udpSvrCtx, onData, NULL);
	
	g_isSvrInited=true;
	return 0;
	
failed:
	return -1;
}

int DISCOVERY_init(void *context) {
#if 0
	ip_addr_t addrToBind={
		.addr=IPADDR_ANY
	};
	
	if(g_isInited) {
		DBG("This module has already been initialized.\n");
		goto failed;
	}
	
	g_context=context;

	g_udpCtx=udp_new();
	if(!g_udpCtx) {
		DBG("Failed to invoke udp_new().\n");
		goto failed;
	}

//	ip_set_option(g_udpCtx, SOF_BROADCAST);
//	IP4_ADDR(&addrToBind, 192, 168, 2, 113);
	udp_bind(g_udpCtx, &addrToBind, DISCOVERY_PORT+1);

	g_isInited=true;
	return 0;

failed:
	return -1;
#else
	return 0;
#endif
}

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

int DISCOVERY_renew(const DISCOVERY_Dev *dev) {
#if 0
	struct pbuf *p=NULL;
	Packet *packet=NULL;
	ip_addr_t addrDest={
		.addr=IPADDR_BROADCAST
	};
	
	if(!dev) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	if(!g_isInited) {
		DBG("Invoke DISCOVERY_init() first.\n");
		goto failed;
	}
	
	if(!(p=pbuf_alloc(PBUF_TRANSPORT, sizeof(Packet), PBUF_RAM))) {
		DBG("Failed to invoke pbuf_alloc().\n");
		goto failed;
	}

	packet=(Packet *)p->payload;
	packet->opCode=htons(OPCODE_RENEW);
	memcpy(&packet->dev, dev, sizeof(DISCOVERY_Dev));
	packet->dev.freq=htonl(packet->dev.freq);
//	IP4_ADDR(&addrDest, 192, 168, 2, 110);
	udp_sendto(g_udpCtx, p, &addrDest, DISCOVERY_PORT);

	pbuf_free(p);

	return 0;

failed:
	return -1;
#else
	int res=-1,
		fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
	res=sendto(fd, &packet, sizeof(packet), 0, &addrPeer, sizeof(addrPeer));
	if(res<0) {
		printf("Failed to invoke sendto().\n");
		goto failed0;
	}
	
	close(fd);
	return 0;

failed1:
	close(fd);

failed0:
	return -1;
#endif
}
