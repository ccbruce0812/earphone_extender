#ifndef STATAB_H
#define STATAB_H

#ifdef __cplusplus
extern "C" {
#endif

void initStaTab(void);
int renewStaTab(const char *name, unsigned long freq);
int staTab2Str(char *list);
int getStaFreq(const char *name, unsigned long *freq);

#ifdef __cplusplus
}
#endif

#endif