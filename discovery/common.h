#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define DISCOVERY_PORT		(10000)

#define ANY_IP				"0.0.0.0"
#define BCAST_IP			"255.255.255.255"
#define MCAST_IP			"224.0.0.1"

#define OPCODE_MIN			(0)
#define OPCODE_RENEW		(OPCODE_MIN+1)
#define OPCODE_LEAVE		(OPCODE_MIN+2)
#define OPCODE_RELAY		(OPCODE_MIN+3)

typedef struct __attribute__((packed)) {
	unsigned short opCode;
	DISCOVERY_Dev dev;
} Packet;

#ifdef HELPERSVR
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

static inline unsigned long long now(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000000+tv.tv_usec;
}
#else
#define DBG(...)
#endif
#endif

static inline int initSocket(bool nonblocking) {
	int ret=-1, type=SOCK_DGRAM;
	
	if(nonblocking) {
#ifdef HELPERSVR
		type|=SOCK_NONBLOCK;
#else
		type|=O_NONBLOCK;
#endif
	}
	
	if((ret=socket(AF_INET, type, IPPROTO_UDP))<0) {
		DBG("Failed to invoke socket(). errno=%d\n", errno);
		return -1;
	}
	
	return ret;
}

static inline int bindTo(int fd, const char *str, unsigned short port) {
	int res=-1;
	struct sockaddr_in addr;
	
	if(!str) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(str);
	addr.sin_port=htons(port);
	if((res=bind(fd, (struct sockaddr *)&addr, sizeof(addr)))<0) {
		DBG("Failed to invoke bind(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

static inline int setBroadcast(int fd) {
	int opt=~0, res=-1;

	if((res=setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)))<0) {
		DBG("Failed to invoke setsockopt(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

static inline int joinGroup(int fd, const char *str) {
	int res=-1;
	struct ip_mreq mreq;

	if(!str) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	mreq.imr_multiaddr.s_addr=inet_addr(str);
	mreq.imr_interface.s_addr=INADDR_ANY;
	if((res=setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))<0) {
		DBG("Failed to invoke setsockopt(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

static inline int sendTo(int fd, const Packet *packet, const char *str) {
	int res=-1;
	struct sockaddr_in addr;

	if(!packet || !str) {
		DBG("Bad argument. Check your code.\n");
		assert(false);
	}
	
	addr.sin_family=AF_INET;
	addr.sin_port=htons(DISCOVERY_PORT);		
	addr.sin_addr.s_addr=inet_addr(str);
	
	if((res=sendto(fd, packet, sizeof(Packet), 0, (struct sockaddr *)&addr, sizeof(addr)))<0) {
		DBG("Failed to invoke sendto(). res=%d, errno=%d\n", res, errno);
		return -1;
	}
	
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
