#ifndef TOOLHELP_H
#define TOOLHELP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <espressif/esp_common.h>

#include <stdarg.h>
#include <stdio.h>

#define MSEC2TICKS(n) (n/portTICK_PERIOD_MS)

typedef struct {
	unsigned long fieldMask;
	unsigned char locSSID[32];
	unsigned char locPassword[64];
	AUTH_MODE locAuthMode;
} InitParam;

#ifndef EARPHONE_EXTENDER_NDEBUG
static inline int __dbg__(const char *loc, const char *fmt, ...) {
	va_list args;
	char _fmt[256]="";
	int res;
	
	va_start(args, fmt);
	sprintf(_fmt, "[%s] %s", loc, fmt);
	res=vprintf(_fmt, args);
	va_end(args);
	
	return res;
}

#define DBG(...) __dbg__(__func__, __VA_ARGS__)
#else
#define DBG(...)
#endif

static inline const char *sysStr(void) {
	static char buf[64]="";

	taskENTER_CRITICAL();

#ifdef EARPHONE_END
    sprintf(buf, "SDK version: %s, Earphone End\n", sdk_system_get_sdk_version());
#else
    sprintf(buf, "SDK version: %s, Station End\n", sdk_system_get_sdk_version());
#endif

	taskEXIT_CRITICAL();
	return buf;
}

#ifdef __cplusplus
}
#endif

#endif
