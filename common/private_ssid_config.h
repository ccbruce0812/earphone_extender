#ifndef PRIVATE_SSID_CONFIG_H
#define PRIVATE_SSID_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//#define WIFI_SSID						"5-8F-1"
//#define WIFI_PASS						"26430660"
#ifdef EARPHONE_END
#define DEFAULT_LOCAL_SSID_PREFIX		"EARPHONE_END"
#else
#define DEFAULT_LOCAL_SSID_PREFIX		"STATION_END"
#endif
#define DEFAULT_LOCAL_PASS				"12345678"

#ifdef __cplusplus
}
#endif

#endif
