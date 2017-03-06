#include <httpd/httpd.h>

#include "../common/toolhelp.h"

#include "cmdsvr.h"
#include "statab.h"
#include "ws.h"
#include "cgi.h"
#include "ssi.h"

void CMDSVR_init(void) {
	initStaTab();
	initCGI();
	initSSI();
	initWS();
    httpd_init();
}

void CMDSVR_renewStaTab(const char *name, unsigned long freq) {
	renewStaTab(name, freq);
}
