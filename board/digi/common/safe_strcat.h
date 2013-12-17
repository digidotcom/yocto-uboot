#ifndef __DIGI_SAFE_STRCAT_H
#define __DIGI_SAFE_STRCAT_H

#define SAFE_STRCAT(dest, src) safe_strcat(dest, src, sizeof(dest))
extern char *safe_strcat(char *dest, const char *src, int dest_size);

#endif  /* __DIGI_SAFE_STRCAT_H */
