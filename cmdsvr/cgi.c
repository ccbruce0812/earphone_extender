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
#include <fcntl.h>
#include <unistd.h>

#include "../common/private_ssid_config.h"
#include "../common/toolhelp.h"

#include "cgi.h"

char *onCGI(int idx, int count, char *param[], char *value[]);

const tCGI g_cgiTab[]={
	{"/cmdSvr.cgi", (tCGIHandler)onCGI}
};

static const char *readValue(int count, char *param[], char *value[], const char *key) {
	int i=0;
	
	for(i=0;i<count;i++) {
		if(!strcmp(param[i], key))
			return value[i];
	}
	
	return NULL;
}

char *onCGI(int idx, int count, char *param[], char *value[]) {
	const char *val=NULL;
	char *ret="/aborted.html";
	
	if((val=readValue(count, param, value, "action"))) {
		switch(atoi(val)) {
			case 0: {
				unsigned char stat=sdk_wifi_station_get_connect_status();
			
				ret=(stat==STATION_GOT_IP)?"/wifiReady.ssi":"/setWiFi.ssi";
				goto end;
			}

			case 1: {
				InitParam init;
				struct sdk_softap_config apCfg;
				struct sdk_station_config staCfg;
				int fout=-1;
				
				memset(&init, 0, sizeof(init));
				memset(&apCfg, 0, sizeof(apCfg));
				sdk_wifi_softap_get_config(&apCfg);
				if((val=readValue(count, param, value, "lcSSID")) && strlen(val) && strcmp(val, (const char *)apCfg.ssid)) {
					init.fieldMask|=0x1;
					strncpy((char *)init.locSSID, val, 32);
				}
				if((val=readValue(count, param, value, "lcPass")) && strlen(val)) {
					init.fieldMask|=0x2;
					strncpy((char *)init.locPassword, val, 64);
					init.locAuthMode=AUTH_WPA2_PSK;
				}
				sdk_wifi_softap_set_config(&apCfg);

				if((fout=open("initParam", O_WRONLY|O_CREAT, 0))>=0) {
					if(init.fieldMask&0x1)
						DBG("New local SSID=%s.\n", init.locSSID);
					if(init.fieldMask&0x2)
						DBG("New local password=%s.\n", init.locPassword);
					write(fout, &init, sizeof(init));
					close(fout);
				}

				memset(&staCfg, 0, sizeof(staCfg));
				sdk_wifi_station_get_config(&staCfg);
				if((val=readValue(count, param, value, "apSSID")) && strlen(val))
					strncpy((char *)staCfg.ssid, val, 32);
				if((val=readValue(count, param, value, "apPass")) && strlen(val))
					strncpy((char *)staCfg.password, val, 64);
				else
					memset(staCfg.password, 0, sizeof(staCfg.password));
				sdk_wifi_station_set_config(&staCfg);

				sdk_wifi_station_connect();
				
				ret="/check.html";
				goto end;
			}
			
			default:
				;
		}
	}

end:
    return ret;
}

void initCGI(void) {
	http_set_cgi_handlers(g_cgiTab, sizeof(g_cgiTab)/sizeof(g_cgiTab[0]));
}
