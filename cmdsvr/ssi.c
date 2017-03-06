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

#include "ssi.h"

const char *g_ssiTab[]={
	"version",
	"apSSID",
	"lcSSID",
	"ip",
	"netmask",
	"gateway",
	"page"
};

int onSSI(int idx, char *ins, int len) {
    switch(idx) {
        case 0:
            snprintf(ins, len, "%s", sysStr());
            break;
			
		case 1: {
			struct sdk_station_config cfg;
			
			memset(&cfg, 0, sizeof(cfg));
			sdk_wifi_station_get_config(&cfg);
			snprintf(ins, len, "%s", cfg.ssid);
			break;
		}
		
		case 2: {
			struct sdk_softap_config cfg;
			
			memset(&cfg, 0, sizeof(cfg));
			sdk_wifi_softap_get_config(&cfg);
			snprintf(ins, len, "%s", cfg.ssid);
			break;
		}

		case 3: {
			struct ip_info info;
			
			memset(&info, 0, sizeof(info));
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "%s", inet_ntoa(info.ip));
			break;
		}

		case 4: {
			struct ip_info info;
			
			memset(&info, 0, sizeof(info));
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "%s", inet_ntoa(info.netmask));
			break;
		}
		
		case 5: {
			struct ip_info info;
			
			memset(&info, 0, sizeof(info));
			sdk_wifi_get_ip_info(STATION_IF, &info);
			snprintf(ins, len, "%s", inet_ntoa(info.gw));
			break;
		}
		
		case 6: {
#ifdef EARPHONE_END
			snprintf(ins, len, "earphoneEnd.ssi");
#else
			snprintf(ins, len, "stationEnd.ssi");
#endif
			break;
		}

        default:
            snprintf(ins, len, "N/A");
    }

    return (strlen(ins));
}

void initSSI(void) {
	http_set_ssi_handler((tSSIHandler)onSSI, g_ssiTab, sizeof(g_ssiTab)/sizeof(g_ssiTab[0]));
}
