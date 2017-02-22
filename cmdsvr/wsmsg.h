#ifndef WSMSG_H
#define WSMSG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MSG_MIN					(0)
#define MSG_GET_STA_LIST		(MSG_MIN+1)
#define MSG_SET_STA				(MSG_MIN+2)
#define MSG_GET_STA				(MSG_MIN+3)
#define MSG_SET_CHANNEL			(MSG_MIN+6)
#define MSG_GET_CHANNEL			(MSG_MIN+7)
#define MSG_PREV_CHANNEL		(MSG_MIN+8)
#define MSG_NEXT_CHANNEL		(MSG_MIN+9)
#define MSG_SET_VOLUME			(MSG_MIN+10)
#define MSG_GET_VOLUME			(MSG_MIN+11)
#define MSG_SET_UNMUTE			(MSG_MIN+12)
#define MSG_GET_UNMUTE			(MSG_MIN+13)
#define MSG_GET_STA_LIST_REPLY	(MSG_MIN+14)
#define MSG_SET_STA_REPLY		(MSG_MIN+15)
#define MSG_GET_STA_REPLY		(MSG_MIN+16)
#define MSG_SET_CHANNEL_REPLY	(MSG_MIN+19)
#define MSG_GET_CHANNEL_REPLY	(MSG_MIN+20)
#define MSG_PREV_CHANNEL_REPLY	(MSG_MIN+21)
#define MSG_NEXT_CHANNEL_REPLY	(MSG_MIN+22)
#define MSG_SET_VOLUME_REPLY	(MSG_MIN+23)
#define MSG_GET_VOLUME_REPLY	(MSG_MIN+24)
#define MSG_SET_UNMUTE_REPLY	(MSG_MIN+25)
#define MSG_GET_UNMUTE_REPLY	(MSG_MIN+26)

int parseRawMsg(char *buf, unsigned short *msg, const char * (*arg)[16]);
const char *makeRawMsg(unsigned short msg, const char *fmt, ...);
			
#ifdef __cplusplus
}
#endif

#endif
