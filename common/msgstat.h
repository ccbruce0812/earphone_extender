#ifndef MSGSTAT_H
#define MSGSTAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MSG_MIN					(0)
#define MSG_KEY_PRESSED			(MSG_MIN+1)

#define STAT_MIN				(0)
#define STAT_DISCONNECTED		(STAT_MIN+1)
#define STAT_CONNECTED			(STAT_MIN+2)

typedef struct {
	unsigned int id;
} Msg;

#ifdef __cplusplus
}
#endif

#endif
