#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <esp8266.h>
#include <esp/uart.h>

#include <etstimer.h>
#include <espressif/esp_common.h>

#include <i2c/i2c.h>
#include <rda5807m/rda5807m.h>
#include <kt0803l/kt0803l.h>
#include <dhcpserver.h>
#include <esp_spiffs.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "../common/private_ssid_config.h"
#include "../common/toolhelp.h"
#include "../common/msgstat.h"
#include "../common/pindef.h"
#include "../cmdsvr/cmdsvr.h"

QueueHandle_t g_msgQ=NULL;
ETSTimer g_timer={0};
unsigned char g_curStat=STAT_DISCONNECTED;

/*
static void onBlinkLED(void *param) {
	static bool stat=false;
	
	gpio_write(LED_PIN, stat);
	stat=!stat;
}
*/

#ifdef EARPHONE_END
static void onFakeTXRenew(void *param) {
	CMDSVR_renewStaTab("Test0", 96000);
	CMDSVR_renewStaTab("Test1", 97000);
}
#endif

static void msgTask(void *param) {
	Msg msgRecv={0};

	CMDSVR_init();

	gpio_write(LED_PIN, true);
	
	while(1) {
		xQueueReceive(g_msgQ, &msgRecv, portMAX_DELAY);
		
		switch(msgRecv.id) {
			case MSG_KEY_PRESSED: {
				switch(g_curStat) {
					case STAT_DISCONNECTED: {
						DBG("WiFi is not OK.\n");
						break;
					}
					
					case STAT_CONNECTED:
						DBG("WiFi is OK.\n");
						break;
						
					default:
						;
				}

				break;
			}
				
			default:
				;
		}
	}
}

static void onGPIO(unsigned char num) {
	static unsigned int prev=0;
	unsigned int now=0;
    
	if(num==KEY_PIN) {
		now=xTaskGetTickCount();
		if(now-prev>=MSEC2TICKS(50)) {
			Msg msg={
				.id=MSG_KEY_PRESSED
			};
			
			prev=now;
			xQueueSendFromISR(g_msgQ, &msg, 0);
		}
	}
}

static void initFS(void) {
    esp_spiffs_init();

    if(esp_spiffs_mount()!=SPIFFS_OK) {
        DBG("Failed to mount SPIFFS\n");
		assert(false);
    }
}

static void initGPIO(void) {
	gpio_enable(LED_PIN, GPIO_OUTPUT);
	gpio_write(LED_PIN, false);

	gpio_enable(KEY_PIN, GPIO_INPUT);
	gpio_set_pullup(KEY_PIN, true, false);
	GPIO.STATUS_CLEAR=0x0000ffff;
	gpio_set_interrupt(KEY_PIN, GPIO_INTTYPE_EDGE_NEG, onGPIO);

	gpio_enable(KT0803L_RST_PIN, GPIO_OUTPUT);
	gpio_write(LED_PIN, false);
}

static void initFM(void) {
#ifdef EARPHONE_END
	RDA5807M_SETTING setting={
		.clkSetting={
			.isClkNoCalb=RDA5807M_FALSE,
			.isClkDirInp=RDA5807M_FALSE,
			.freq=RDA5807M_CLK_FREQ_32_768KHZ,
		},
		.useRDS=RDA5807M_TRUE,
		.useNewDMMethod=RDA5807M_FALSE,
		.isDECNST50us=RDA5807M_FALSE,
		.system={
			.band=RDA5807M_BAND_87_108_MHZ,
			.is6576Sys=RDA5807M_FALSE,
			.space=RDA5807M_SPACE_100_KHZ
		}
	};
	
	RDA5807M_init(&setting);
	RDA5807M_setFreq(96000);
	RDA5807M_enableOutput(RDA5807M_TRUE);
	RDA5807M_setVolume(1);
	RDA5807M_unmute(RDA5807M_TRUE);
#else
	KT0803L_SETTING setting={
		.useExtInductor=KT0803L_FALSE,
		.clkSetting={
			.isUpToSW=KT0803L_TRUE,
			.isXTAL=KT0803L_TRUE,
			.freq=KT0803L_CLK_FREQ_32_768KHZ
		},
		.isPLTAmpHigh=KT0803L_FALSE,
		.isPHTCNST50us=KT0803L_FALSE,
		.isFDEV112_5KHZ=KT0803L_FALSE,
		.isCHSELPAOff=KT0803L_FALSE
	};
	
	gpio_write(KT0803L_RST_PIN, false);
	vTaskDelay(MSEC2TICKS(500));
	gpio_write(KT0803L_RST_PIN, true);

	KT0803L_init(&setting);
	KT0803L_setFreq(960);
	KT0803L_setPGAGain(KT0803L_PGA_GAIN_M5DB, KT0803L_FALSE);
	KT0803L_PADown(KT0803L_FALSE);
#endif
}

static void initWiFi(void) {
	int fin=open("initParam", O_RDONLY);
	struct sdk_softap_config apCfg;
	struct ip_info ipAddr;
	
    sdk_wifi_set_opmode(STATIONAP_MODE);

	memset(&apCfg, 0, sizeof(apCfg));
	sdk_wifi_softap_get_config(&apCfg);
	if(fin>=0) {
		InitParam data;
		
		if(read(fin, &data, sizeof(data))==sizeof(data)) {
			memcpy(apCfg.ssid, data.locSSID, sizeof(data.locSSID));
			DBG("New local SSID is '%s'.\n", apCfg.ssid);
		}
		
		close(fin);
		unlink("initParam");
	} else {
		if(!apCfg.ssid[0]) {
			srand(xTaskGetTickCount());
			sprintf((char *)apCfg.ssid, "%s_%04d", DEFAULT_LOCAL_SSID_PREFIX, rand()%10000);
			strcpy((char *)apCfg.password, DEFAULT_LOCAL_PASS);
		}
	}
	sdk_wifi_softap_set_config(&apCfg);

	memset(&ipAddr, 0, sizeof(ipAddr));
	IP4_ADDR(&ipAddr.ip, 192, 168, 254, 254);
	IP4_ADDR(&ipAddr.gw, 192, 168, 254, 254);
	IP4_ADDR(&ipAddr.netmask, 255, 255, 255, 0);
	sdk_wifi_set_ip_info(SOFTAP_IF, &ipAddr);
	
	memset(&ipAddr, 0, sizeof(ipAddr));
	IP4_ADDR(&ipAddr.ip, 192, 168, 254, 100);
	dhcpserver_start(&ipAddr.ip, 10);

    sdk_wifi_station_connect();
}

void user_init(void) {
	initFS();
    uart_set_baud(0, 115200);
	DBG("%s\n", sysStr());
	initGPIO();
	i2c_init(SCL_PIN, SDA_PIN);
	initFM();
	initWiFi();

#ifdef EARPHONE_END
	sdk_ets_timer_setfn(&g_timer, onFakeTXRenew, NULL);
	sdk_ets_timer_arm(&g_timer, 5000, true);
#endif

	g_msgQ=xQueueCreate(8, sizeof(Msg));
	xTaskCreate(msgTask, "msgTask", 512, NULL, 4, NULL);
}
