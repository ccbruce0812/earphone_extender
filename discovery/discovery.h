#ifndef DISCOVERY_H
#define DISCOVERY_H

#ifdef __cplusplus
extern "C" {
#endif

#define DISCOVERY_PORT		(10000)

typedef struct __attribute__((packed)) {
	char name[32];
	unsigned long freq;
} DISCOVERY_Dev;

typedef void (*DISCOVERY_onRenew)(void *context, const DISCOVERY_Dev *dev);
typedef void (*DISCOVERY_onLeave)(void *context, const DISCOVERY_Dev *dev);

int DISCOVERY_initSvr(void *context, DISCOVERY_onRenew onRenew, DISCOVERY_onLeave onLeave);
int DISCOVERY_init(void *context);
int DISCOVERY_renew(const DISCOVERY_Dev *dev);

#ifdef __cplusplus
}
#endif

#endif
