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
#include <httpd/httpd.h>

#include <string.h>
#include <assert.h>

#include "../common/toolhelp.h"
#include "cmdsvr.h"
#include "wsmsg.h"
#include "statab.h"

static char *onCGI(int idx, int count, char *param[], char *value[]);

static const tCGI g_cgiTab[]={
	{"/cmdSvr.cgi", (tCGIHandler)onCGI}
};

static const char *g_ssiTab[]={
	"version",
	"ap",
	"ip",
	"netmask",
	"gateway",
	"panel"
};

static char g_curSta[32]="";

static const char *readValue(int count, char *param[], char *value[], const char *key) {
	int i=0;
	
	for(i=0;i<count;i++) {
		if(!strcmp(param[i], key))
			return value[i];
	}
	
	return NULL;
}

static char *onCGI(int idx, int count, char *param[], char *value[]) {
	const char *val=NULL;
	char *ret="/aborted.html";
	
	if((val=readValue(count, param, value, "action"))) {
		switch(atoi(val)) {
			case 0: {
				unsigned char stat=sdk_wifi_station_get_connect_status();
			
				ret=(stat==STATION_GOT_IP)?"/wifiReady.ssi":"/setWiFi.html";
				goto end;
			}

			case 1: {
				struct sdk_station_config config;
				unsigned char stat=STATION_IDLE;
				unsigned int prev=0, now=0;
				
				memset(&config, 0, sizeof(config));
				if(val=readValue(count, param, value, "ap"))
					strncpy((char *)config.ssid, val, 32);
				if(val=readValue(count, param, value, "pass"))
					strncpy((char *)config.password, val, 64);
				
				sdk_wifi_station_set_config(&config);
				sdk_wifi_station_connect();
				
				now=prev=xTaskGetTickCount();
				while(1) {
					if((stat=sdk_wifi_station_get_connect_status())==STATION_GOT_IP) {
						ret="/wifiReady.ssi";
						break;
					}
					
					vTaskDelay(MSEC2TICKS(500));
					if((now=xTaskGetTickCount())-prev>=MSEC2TICKS(5000)) {
						ret="/setWiFi.html";
						break;
					}
				}
			}
			
			default:
				;
		}
	}

end:
    return ret;
}

static int onSSI(int idx, char *ins, int len) {
    switch(idx) {
        case 0:
            snprintf(ins, len, "%s", sdk_system_get_sdk_version());
            break;
			
		case 1: {
			struct sdk_station_config cfg={0};
			
			sdk_wifi_station_get_config(&cfg);
			snprintf(ins, len, "%s", cfg.ssid);
			break;
		}

		case 2: {
			struct ip_info info={0};
			
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "%s", inet_ntoa(info.ip));
			break;
		}

		case 3: {
			struct ip_info info={0};
			
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "%s", inet_ntoa(info.netmask));
			break;
		}
		
		case 4: {
			struct ip_info info={0};
			
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "%s", inet_ntoa(info.gw));
			break;
		}
		
		case 5: {
			struct ip_info info={0};
			
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "<a href=\"http://%s/rx.ssi\">", inet_ntoa(info.ip));
			break;
		}

        default:
            snprintf(ins, len, "N/A");
    }

    return (strlen(ins));
}

static void onWSMsg(struct tcp_pcb *pcb, unsigned char *data, unsigned short len, unsigned char mode) {
	char *bufRecv=malloc(len+1);
	unsigned short msgRecv=MSG_MIN;
	const char *arg[16]={0};
	int res=-1;
	const char *bufSend=NULL;
	
	assert(bufRecv);
	memset(bufRecv, 0, len+1);
	memcpy(bufRecv, data, len);
	if((res=parseRawMsg(bufRecv, &msgRecv, &arg))>=0) {
		switch(msgRecv) {
			case MSG_GET_STA_LIST: {
				char buf[256]=""; staTab2Str(buf);
				
				bufSend=makeRawMsg(MSG_GET_STA_LIST_REPLY, "%s", buf);
				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_SET_STA: {
				unsigned long freq=0;
				
				DBG("arg=%s\n", arg[0]);
				if(!getStaFreq(arg[0], &freq)) {
					RDA5807M_setFreq(freq);
					
					strcpy(g_curSta, arg[0]);
					bufSend=makeRawMsg(MSG_SET_STA_REPLY, "%ld", freq);
				} else
					bufSend=makeRawMsg(MSG_SET_STA_REPLY, ";;-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_STA:
				break;

			case MSG_SET_CHANNEL:	//test
				break;

			case MSG_GET_CHANNEL:	//test
				break;

			case MSG_PREV_CHANNEL:	//test
				break;

			case MSG_NEXT_CHANNEL:	//test
				break;

			case MSG_SET_VOLUME: {
				unsigned char vol=0;
				
				DBG("arg=%s\n", arg[0]);
				vol=atoi(arg[0]);
				if(vol>=1 && vol<=15) {
					RDA5807M_setVolume(vol);
					RDA5807M_getVolume(&vol);
					
					bufSend=makeRawMsg(MSG_SET_VOLUME_REPLY, "%d", vol);
				} else
					bufSend=makeRawMsg(MSG_SET_VOLUME_REPLY, ";;-1");

				DBG("bufSend=%s\n", bufSend);
				websocket_write(pcb, (unsigned char *)bufSend, strlen(bufSend), WS_TEXT_MODE);
				break;
			}

			case MSG_GET_VOLUME:
				break;

			case MSG_SET_UNMUTE: {
				RDA5807M_BOOL flag=RDA5807M_FALSE;
				
				DBG("arg=%s\n", arg[0]);
				flag=(RDA5807M_BOOL)atoi(arg[0]);
				RDA5807M_unmute(flag);
				RDA5807M_isUnmute(&flag);
				
				bufSend=makeRawMsg(MSG_SET_UNMUTE_REPLY, "%d", flag);
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

static void onWSOpen(struct tcp_pcb *pcb, const char *uri) {
    printf("WS URI: %s\n", uri);
    if(strcmp(uri, "/CmdSvr.ws"))
		websocket_write(pcb, NULL, 0, 0x08);
}

void CMDSVR_init(void) {
    http_set_cgi_handlers(g_cgiTab, sizeof(g_cgiTab)/sizeof(g_cgiTab[0]));
    http_set_ssi_handler((tSSIHandler)onSSI, g_ssiTab, sizeof(g_ssiTab)/sizeof(g_ssiTab[0]));
    websocket_register_callbacks((tWsOpenHandler)onWSOpen, (tWsHandler)onWSMsg);
    httpd_init();
}

void CMDSVR_renewStaTab(const char *name, unsigned long freq) {
	renewStaTab(name, freq);
}
