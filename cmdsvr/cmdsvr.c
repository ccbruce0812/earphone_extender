#include <httpd/httpd.h>

#include "../common/toolhelp.h"

#include "cmdsvr.h"
#include "statab.h"
#include "ws.h"
#include "cgi.h"
#include "ssi.h"

static bool g_isInited=false;

int CMDSVR_init(void) {
	if(g_isInited) {
		DBG("This module has already been initialized.\n");
		goto failed;
	}
	
	initStaTab();
	initCGI();
	initSSI();
	initWS();
    httpd_init();
	
	g_isInited=true;
	return 0;
	
failed:
	return -1;
}

int CMDSVR_renewStaTab(const char *name, unsigned long freq) {
	if(!g_isInited) {
		DBG("Invoke CMDSVR_init() first.\n");
		goto failed;
	}

	renewStaTab(name, freq);
	
	return 0;
	
failed:
	return -1;
}
