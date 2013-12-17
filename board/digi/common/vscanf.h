#ifndef __DIGI_VSCANF_H
#define __DIGI_VSCANF_H

/* !!!!! vsscanf has been tuned down to not support signed operations.
   Therefore, only simple_strtoul are used.
*/
extern int vsscanf(const char * buf, const char * fmt, va_list args);
extern int sscanf(const char * buf, const char * fmt, ...);

#endif  /* NVRAM_H */
