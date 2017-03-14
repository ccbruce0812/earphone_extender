#ifndef DISCOVERY_H
#define DISCOVERY_H

#ifdef __cplusplus
extern "C" {
#endif

#define DISCOVERY_RENEW_MODE_MULTICAST	(0)
#define DISCOVERY_RENEW_MODE_BROADCAST	(1)
#define DISCOVERY_RENEW_MODE_UNICAST	(2)

typedef struct __attribute__((packed)) {
	char name[32];
	unsigned long freq;
} DISCOVERY_Dev;

typedef void (*DISCOVERY_onRenew)(void *context, const DISCOVERY_Dev *dev);
typedef void (*DISCOVERY_onLeave)(void *context, const DISCOVERY_Dev *dev);

int DISCOVERY_init(void *context, DISCOVERY_onRenew onRenew, DISCOVERY_onLeave onLeave, bool multicast);
int DISCOVERY_renew(const DISCOVERY_Dev *dev, unsigned char mode, const char *tarAddr);

#ifdef __cplusplus
}
#endif

#endif
