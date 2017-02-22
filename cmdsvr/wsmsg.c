#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../common/toolhelp.h"

#include "wsmsg.h"

typedef struct {
	const char *str;
	unsigned short id;
} MsgMapItem;

static const MsgMapItem g_msgMap[]={
	{"MSG_GET_STA_LIST",		MSG_GET_STA_LIST},
	{"MSG_SET_STA",				MSG_SET_STA},
	{"MSG_GET_STA",				MSG_GET_STA},
	{"MSG_SET_CHANNEL",			MSG_SET_CHANNEL},
	{"MSG_GET_CHANNEL",			MSG_GET_CHANNEL},
	{"MSG_PREV_CHANNEL",		MSG_PREV_CHANNEL},
	{"MSG_NEXT_CHANNEL",		MSG_NEXT_CHANNEL},
	{"MSG_SET_VOLUME",			MSG_SET_VOLUME},
	{"MSG_GET_VOLUME",			MSG_GET_VOLUME},
	{"MSG_SET_UNMUTE",			MSG_SET_UNMUTE},
	{"MSG_GET_UNMUTE",			MSG_GET_UNMUTE},
	{"MSG_GET_STA_LIST_REPLY",	MSG_GET_STA_LIST_REPLY},
	{"MSG_SET_STA_REPLY",		MSG_SET_STA_REPLY},
	{"MSG_GET_STA_REPLY",		MSG_GET_STA_REPLY},
	{"MSG_SET_CHANNEL_REPLY",	MSG_SET_CHANNEL_REPLY},
	{"MSG_GET_CHANNEL_REPLY",	MSG_GET_CHANNEL_REPLY},
	{"MSG_PREV_CHANNEL_REPLY",	MSG_PREV_CHANNEL_REPLY},
	{"MSG_NEXT_CHANNEL_REPLY",	MSG_NEXT_CHANNEL_REPLY},
	{"MSG_SET_VOLUME_REPLY",	MSG_SET_VOLUME_REPLY},
	{"MSG_GET_VOLUME_REPLY",	MSG_GET_VOLUME_REPLY},
	{"MSG_SET_UNMUTE_REPLY",	MSG_SET_UNMUTE_REPLY},
	{"MSG_GET_UNMUTE_REPLY",	MSG_GET_UNMUTE_REPLY},
	{NULL,						MSG_MIN}
};

static unsigned short str2Msg(const char *str) {
	int i;
	
	for(i=0;g_msgMap[i].str;i++) {
		if(!strcmp(g_msgMap[i].str, str))
			return g_msgMap[i].id;
	}
	
	return MSG_MIN;
}

static const char *msg2Str(unsigned short id) {
	int i;
	
	for(i=0;g_msgMap[i].str;i++) {
		if(g_msgMap[i].id==id)
			return g_msgMap[i].str;
	}
	
	return NULL;
}

int parseRawMsg(char *buf, unsigned short *msg, const char * (*arg)[16]) {
	if(!buf || !msg || !arg)
		return -1;
	
	int i=0;
	char *now=NULL;
	
	now=strtok(buf, ",");
	if(!now || (*msg=str2Msg(now))==MSG_MIN)
		return -1;
	
	while(i<16 && (now=strtok(NULL, ";")))
		(*arg)[i++]=now;
	
	return i;
}

const char *makeRawMsg(unsigned short msg, const char *fmt, ...) {
	static char rawMsg[64];
	char body[64];
	const char *str=msg2Str(msg);
	va_list arg;
	
	if(!str)
		return NULL;
	
	if(fmt) {
		sprintf(body, "%s,%s", str, fmt);
		va_start(arg, fmt);
		vsprintf(rawMsg, body, arg);
		va_end(arg);
	} else
		strcpy(rawMsg, str);
	
	return rawMsg;
}
