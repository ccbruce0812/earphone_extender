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
				
				DBG("msgRecv=%d\n", msgRecv);
				
				staTab2Str(val);
				bufSend=makeRawMsg(MSG_GET_STA_LIST_REPLY, "0;%s", val);
				
				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_SET_STA: {
				unsigned long val;
				
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);

				if(!strcmp(arg[0], "none")) {
					RDA5807M_enableOutput(RDA5807M_FALSE);
					bufSend=makeRawMsg(MSG_SET_STA_REPLY, "0;-1");
				} else {
					if(getStaFreq(arg[0], &val)>=0 &&
						RDA5807M_enableOutput(RDA5807M_TRUE)>=0 &&
						RDA5807M_setFreq(val)>=0) {
						bufSend=makeRawMsg(MSG_SET_STA_REPLY, "0;%d", val);
					} else
						bufSend=makeRawMsg(MSG_SET_STA_REPLY, "-1");
				}

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}
			
			case MSG_GET_STA: {
				DBG("msgRecv=%d\n", msgRecv);
				
				bufSend=makeRawMsg(MSG_GET_STA_REPLY, "0;Test0;96000");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_SET_VOLUME: {
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				
				if(RDA5807M_setVolume((unsigned char)atoi(arg[0]))>=0)
					bufSend=makeRawMsg(MSG_SET_VOLUME_REPLY, "0");
				else
					bufSend=makeRawMsg(MSG_SET_VOLUME_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_VOLUME: {
				unsigned char val;
				
				DBG("msgRecv=%d\n", msgRecv);
				
				if(RDA5807M_getVolume(&val)>=0)
					bufSend=makeRawMsg(MSG_GET_VOLUME_REPLY, "0;%d", val);
				else
					bufSend=makeRawMsg(MSG_GET_VOLUME_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_SET_UNMUTE: {
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				
				if(RDA5807M_unmute(atoi(arg[0])?RDA5807M_TRUE:RDA5807M_FALSE)>=0)
					bufSend=makeRawMsg(MSG_SET_UNMUTE_REPLY, "0");
				else
					bufSend=makeRawMsg(MSG_SET_UNMUTE_REPLY, "-1");
	
				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_UNMUTE: {
				RDA5807M_BOOL val;
				
				DBG("msgRecv=%d\n", msgRecv);
				
				if(RDA5807M_isUnmute(&val)>=0)
					bufSend=makeRawMsg(MSG_GET_UNMUTE_REPLY, "0;%d", val);
				else
					bufSend=makeRawMsg(MSG_GET_UNMUTE_REPLY, "-1");
	
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
				DBG("msgRecv=%d, arg=%s\n", msgRecv, arg[0]);
				
				if(KT0803L_setFreq((unsigned short)atoi(arg[0]))>=0)
					bufSend=makeRawMsg(MSG_SET_CHANNEL_REPLY, "0");
				else
					bufSend=makeRawMsg(MSG_SET_CHANNEL_REPLY, "-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_CHANNEL: {
				unsigned short val;
				
				DBG("msgRecv=%d\n", msgRecv);
				
				if(KT0803L_getFreq(&val)>=0)
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
