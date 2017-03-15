#ifndef DISCOVERY_H
#define DISCOVERY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
	char name[32];
	unsigned long freq;
} DISCOVERY_Dev;

typedef void (*DISCOVERY_onRenew)(void *context, const DISCOVERY_Dev *dev);
typedef DISCOVERY_onRenew DISCOVERY_onLeave;
typedef void (*DISCOVERY_onRelay)(void *context, const struct sockaddr_in *addr);

int DISCOVERY_init(void *context, DISCOVERY_onRenew onRenew, DISCOVERY_onLeave onLeave, DISCOVERY_onRelay onRelay, bool multicast);
int DISCOVERY_renew(const DISCOVERY_Dev *dev, const char *str);
int DISCOVERY_leave(const DISCOVERY_Dev *dev, const char *str);

#ifdef __cplusplus
}
#endif

#endif
