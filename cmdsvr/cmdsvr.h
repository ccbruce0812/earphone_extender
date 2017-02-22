#ifndef CMDSVR_H
#define CMDSVR_H

#ifdef __cplusplus
extern "C" {
#endif

void CMDSVR_init(void);
void CMDSVR_renewStaTab(const char *name, unsigned long freq);

#ifdef __cplusplus
}
#endif

#endif
