#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "../common/toolhelp.h"

#include "statab.h"

typedef struct {
	char name[32];
	unsigned long freq;
	unsigned int ts;
} StaTabItem;

static StaTabItem g_staTab[16];

static int purgeStaTab(StaTabItem *(*freeItem)[16]) {
	int i, j=0;

	for(i=0;i<sizeof(g_staTab)/sizeof(g_staTab[0]);i++) {
		unsigned int now=xTaskGetTickCount();
		
		if(g_staTab[i].name[0] &&
			(g_staTab[i].ts>now || now-g_staTab[i].ts>=MSEC2TICKS(30000)))
			memset(&g_staTab[i], 0, sizeof(StaTabItem));
		
		if(freeItem && !g_staTab[i].name[0]) {
			(*freeItem)[j++]=&g_staTab[i];
			continue;
		}
	}
	
	return j;
}

static int getSta(const char *name, StaTabItem **item) {
	int i;

	if(!name || !item) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}

	for(i=0;i<sizeof(g_staTab)/sizeof(g_staTab[0]);i++) {
		if(!strcmp(g_staTab[i].name, name)) {
			*item=&g_staTab[i];
			return 0;
		}
	}

	*item=NULL;
	return -1;
}

static int getAvailSta(StaTabItem *(*availItem)[16]) {
	int i, j=0;

	if(!availItem) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	for(i=0;i<sizeof(g_staTab)/sizeof(g_staTab[0]);i++) {
		if(g_staTab[i].name[0])
			(*availItem)[j++]=&g_staTab[i];
	}
	
	return j;
}

void initStaTab(void) {
	taskENTER_CRITICAL();
	
	memset(g_staTab, 0, sizeof(g_staTab));
	
	taskEXIT_CRITICAL();
}

int renewStaTab(const char *name, unsigned long freq) {
	StaTabItem *freeItem[16]={0}, *matchedItem=NULL;
	int freeItemCnt=0;

	if(!name) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	taskENTER_CRITICAL();
	
	freeItemCnt=purgeStaTab(&freeItem);
	
	if(!getSta(name, &matchedItem)) {
		matchedItem->freq=freq;
		matchedItem->ts=xTaskGetTickCount();
		goto ok;
	} else {
		if(freeItemCnt) {
			strcpy(freeItem[0]->name, name);
			freeItem[0]->freq=freq;
			freeItem[0]->ts=xTaskGetTickCount();
			goto ok;
		}
	}
	
	DBG("No room to store data.\n");
	taskEXIT_CRITICAL();
	return -1;
	
ok:
	taskEXIT_CRITICAL();
	return 0;
}

int staTab2Str(char *str) {
	StaTabItem *availItem[16]={0};
	int availItemCnt=0, i;

	if(!str) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	taskENTER_CRITICAL();

	purgeStaTab(NULL);
	availItemCnt=getAvailSta(&availItem);
	
	taskEXIT_CRITICAL();
	
	str[0]='\0';
	for(i=0;i<availItemCnt;i++) {
		strcat(str, availItem[i]->name);
		strcat(str, ";");
	}
	strcat(str, "none");
	return 0;
}

int getStaFreq(const char *name, unsigned long *freq) {
	StaTabItem *matchedItem=NULL;

	if(!name || !freq) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	taskENTER_CRITICAL();

	purgeStaTab(NULL);
	if(!getSta(name, &matchedItem)) {
		*freq=matchedItem->freq;
		goto ok;
	}
	
	DBG("No item found.\n");
	taskEXIT_CRITICAL();
	return -1;
	
ok:
	taskEXIT_CRITICAL();
	return 0;
}
