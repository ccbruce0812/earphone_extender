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

#include <i2c/i2c.h>
#include <rda5807m/rda5807m.h>
#include <kt0803l/kt0803l.h>
#include <httpd/httpd.h>

#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "../common/private_ssid_config.h"
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

	return 0;
	
failed:
	return -1;
}

int DISCOVERY_init(void *context) {
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

	udp_bind(g_udpCtx, IP_ADDR_ANY, DISCOVERY_PORT+1);

	return 0;
	
failed:
	return -1;
}

int DISCOVERY_renew(const DISCOVERY_Dev *dev) {
	struct pbuf *p=NULL;
	Packet *packet=NULL;
	
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
	udp_sendto(g_udpCtx, p, IP_ADDR_BROADCAST, DISCOVERY_PORT);

	pbuf_free(p);

	return 0;

failed:
	return -1;
}
