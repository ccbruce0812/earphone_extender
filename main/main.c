#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <esp8266.h>
#include <esp/uart.h>

#include <etstimer.h>
#include <espressif/esp_common.h>

#include <i2c/i2c.h>
#include <rda5807m/rda5807m.h>

#include <string.h>

#include "../common/private_ssid_config.h"
#include "../common/toolhelp.h"
#include "../common/msgstat.h"
#include "../common/pindef.h"
#include "../cmdsvr/cmdsvr.h"

QueueHandle_t g_msgQ=NULL;
ETSTimer g_timer={0};
unsigned char g_curStat=STAT_DISCONNECTED;

static void onBlinkLED(void *param) {
	static bool stat=false;
	
	gpio_write(LED_PIN, stat);
	stat=!stat;
}

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
					case STAT_DISCONNECTED:
						DBG("WiFi is not OK.\n");
						break;
					
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

void user_init(void) {
    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };
	
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
#else
	FMTX_SETTING setting={
		.useExtInductor=FMTX_FALSE,
		.clkSetting={
			.isUpToSW=FMTX_TRUE,
			.isXTAL=FMTX_TRUE,
			.freq=FMTX_CLK_FREQ_32_768KHZ
		},
		.isPLTAmpHigh=FMTX_FALSE,
		.isPHTCNST50us=FMTX_FALSE,
		.isFDEV112_5KHZ=FMTX_FALSE,
		.isCHSELPAOff=FMTX_FALSE
	};
#endif
	
    uart_set_baud(0, 115200);
    DBG("SDK version: %s\n", sdk_system_get_sdk_version());
	
	gpio_enable(LED_PIN, GPIO_OUTPUT);
	gpio_write(LED_PIN, false);
	gpio_enable(KEY_PIN, GPIO_INPUT);
	gpio_set_pullup(KEY_PIN, true, false);
	GPIO.STATUS_CLEAR=0x0000ffff;
	gpio_set_interrupt(KEY_PIN, GPIO_INTTYPE_EDGE_NEG, onGPIO);

	i2c_init(SCL_PIN, SDA_PIN);
	
#ifdef EARPHONE_END
	RDA5807M_init(&setting);
	RDA5807M_setFreq(96000);
	RDA5807M_enableOutput(RDA5807M_TRUE);
	RDA5807M_setVolume(1);
	RDA5807M_unmute(RDA5807M_TRUE);
#else
	FMTX_init(&setting);
	FMTX_setFreq(960);
	FMTX_setPGAGain(FMTX_PGA_GAIN_M5DB, FMTX_FALSE);
	FMTX_PADown(FMTX_FALSE);
#endif
	
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);
    sdk_wifi_station_connect();

#ifdef EARPHONE_END
	sdk_ets_timer_setfn(&g_timer, onFakeTXRenew, NULL);
	sdk_ets_timer_arm(&g_timer, 5000, true);
#endif

	g_msgQ=xQueueCreate(8, sizeof(Msg));
	xTaskCreate(msgTask, "msgTask", 512, NULL, 4, NULL);
}
