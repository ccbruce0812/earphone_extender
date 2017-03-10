#ifndef CMDSVR_H
#define CMDSVR_H

#ifdef __cplusplus
extern "C" {
#endif

int CMDSVR_init(void);
int CMDSVR_renewStaTab(const char *name, unsigned long freq);

#ifdef __cplusplus
}
#endif

#endif
