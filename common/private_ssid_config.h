#ifndef PRIVATE_SSID_CONFIG_H
#define PRIVATE_SSID_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SSID		"5-8F-1x"
#define WIFI_PASS		"26430660x"
#ifdef EARPHONE_END
#define AP_SSID_PREFIX	"EARPHONE_END_"
#else
#define AP_SSID			"STATION_END_"
#endif
#define AP_PASS		"12345678"

#ifdef __cplusplus
}
#endif

#endif
