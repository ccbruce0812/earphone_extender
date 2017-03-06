#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <esp8266.h>
#include <esp/uart.h>

#include <etstimer.h>
#include <espressif/esp_common.h>

#include <lwip/inet.h>

#include <i2c/i2c.h>
#include <rda5807m/rda5807m.h>
#include <kt0803l/kt0803l.h>
#include <httpd/httpd.h>

#include <string.h>
#include <assert.h>

#include "../common/toolhelp.h"

#include "statab.h"
#include "wsmsg.h"
#include "ws.h"

#ifdef EARPHONE_END
void onWSMsg(struct tcp_pcb *pcb, unsigned char *data, unsigned short len, unsigned char mode) {
	char *bufRecv=malloc(len+1);
	unsigned short msgRecv=MSG_MIN;
	const char *arg[16];
	int res=-1;
	const char *bufSend=NULL;
	
	assert(bufRecv);
	memset(bufRecv, 0, len+1);
	memcpy(bufRecv, data, len);
	memset(arg, 0, sizeof(arg));
	if((res=parseRawMsg(bufRecv, &msgRecv, &arg))>=0) {
		switch(msgRecv) {
			case MSG_GET_STA_LIST: {
				char val[256];
				
				staTab2Str(val);
				bufSend=makeRawMsg(MSG_GET_STA_LIST_REPLY, "%s", buf);
				
				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_SET_STA: {
				unsigned long val0, val1;
				
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				if(!getStaFreq(arg[0], &val0)) {
					RDA5807M_setFreq(val0);
					RDA5807M_getFreq(&val1);

					if(val0==val1)
						bufSend=makeRawMsg(MSG_SET_STA_REPLY, "0;%ld", val1);
					else
						bufSend=makeRawMsg(MSG_SET_STA_REPLY, "-1");	
				} else
					bufSend=makeRawMsg(MSG_SET_STA_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_STA:
				break;

			case MSG_SET_VOLUME: {
				unsigned char val0, val1;
				
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				val0=atoi(arg[0]);

				RDA5807M_setVolume(val0);
				RDA5807M_getVolume(&val1);
				
				if(val0==val1)
					bufSend=makeRawMsg(MSG_SET_VOLUME_REPLY, "0;%d", val1);
				else
					bufSend=makeRawMsg(MSG_SET_VOLUME_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_VOLUME:
				break;

			case MSG_SET_UNMUTE: {
				RDA5807M_BOOL val0, val1;
				
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				val0=atoi(arg[0])?RDA5807M_TRUE:RDA5807M_FALSE;
				
				RDA5807M_unmute(val0);
				RDA5807M_isUnmute(&val1);
				
				if(val0==val1)
					bufSend=makeRawMsg(MSG_SET_UNMUTE_REPLY, "0;%d", val1);
				else
					bufSend=makeRawMsg(MSG_SET_UNMUTE_REPLY, "-1");
	
				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_UNMUTE:
				break;

			default:
				;
		}
	}

	free(bufRecv);
}
#else
void onWSMsg(struct tcp_pcb *pcb, unsigned char *data, unsigned short len, unsigned char mode) {
	char *bufRecv=malloc(len+1);
	unsigned short msgRecv=MSG_MIN;
	const char *arg[16];
	int res=-1;
	const char *bufSend=NULL;
	
	assert(bufRecv);
	memset(bufRecv, 0, len+1);
	memcpy(bufRecv, data, len);
	memset(arg, 0, sizeof(arg));
	if((res=parseRawMsg(bufRecv, &msgRecv, &arg))>=0) {
		switch(msgRecv) {
			case MSG_SET_CHANNEL: {
				unsigned short val0, val1;
				
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				KT0803L_setFreq((val0=(unsigned short)atoi(arg[0])));
				KT0803L_getFreq(&val1);
				
				if(val0==val1)
					bufSend=makeRawMsg(MSG_SET_CHANNEL_REPLY, "0;%d", val1);
				else
					bufSend=makeRawMsg(MSG_SET_CHANNEL_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_CHANNEL: {
				unsigned short val;
				
				DBG("msgRecv=%d\n", msgRecv);
				KT0803L_getFreq(&val);
				
				if(val>=860 && val<=1070)
					bufSend=makeRawMsg(MSG_GET_CHANNEL_REPLY, "0;%d", val);
				else
					bufSend=makeRawMsg(MSG_GET_CHANNEL_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);				
				break;
			}

			default:
				;
		}
	}

	free(bufRecv);
}
#endif

void onWSOpen(struct tcp_pcb *pcb, const char *uri) {
    if(strcmp(uri, "/CmdSvr.ws")) {
		websocket_write(pcb, NULL, 0, 0x08);
		DBG("Illegal URL detected. Disconnect immediately.\n");
	}
}

void initWS(void) {
	websocket_register_callbacks((tWsOpenHandler)onWSOpen, (tWsHandler)onWSMsg);
}
