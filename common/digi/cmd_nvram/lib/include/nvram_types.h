/*
 *  nvram/lib/include/nvram_types.h
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
 */
/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !Descr:      Defines standard types
 */

#ifndef NVRAM_TYPES_H
#define NVRAM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LINUX)
# include <string.h>            /* memset */
# include <stdio.h>             /* snprintf */
# include <stdint.h>            /* uint8 */
# include <unistd.h>            /* size_t */
# include <assert.h>            /* assert */
# include <sys/types.h>         /* loff_t */
# include <mtd/mtd-abi.h>       /* MTD_NORFLASH */
# include <ctype.h>		/* isalnum */
/* digilib_ */
# include <crc32.h>             /* crc32 */

# define ARRAY_SIZE( x ) (sizeof(x)/sizeof(*(x)))
# define ASSERT( expr ) assert( expr )
# define PRINTF_QUAD "ll"
# define CMD_NAME "nvram"
# ifdef S_SPLINT_S
/* splint checks for posixlibs, so we need to add some linux stuff */
typedef uint64_t loff_t;
extern /*@null@*/ /*@only@*/ char *strdup (char *s) /*@*/ ;
# endif  /* S_SPLINT_S */

# define strnicmp(a,b,c)	strncasecmp(a,b,c)
# define memcpy32(a,b,c)	memcpy(a,b,c)

#elif defined(UBOOT)
# include <common.h>            /* panic */
# include <linux/types.h>       /* loff_t */
# include <linux/mtd/mtd-abi.h> /* MTD_NORFLASH */
# include <malloc.h>            /* malloc */
# include <linux/ctype.h>	/* isalnum */

# include "../common/digi/vscanf.h"            /* vscanf/scanf */
# include "../common/digi/atoi.h"              /* atoi */
# define PRINTF_QUAD "ll"
# define CMD_NAME "intnvram"
# define ASSERT( expr ) \
        do { \
                if( !(expr) )                                           \
                        printf( "**** ASSERT %s@%d:%s", __FILE__, __LINE__, #expr ); \
        } while( 0 )

/* eprintf may fail in early steps before EEPROM is initialized, therefor printf*/

typedef ulong crc32_t;
#elif defined(WINCE)
# include <windows.h> /**/
# include <oal.h>	 /* OAL macros */
#ifdef MX51
# include <mx51_iomux.h> /* GPIO settings */
# include <mx51_ddk.h> /* clock settings */
#endif
# include <args.h>	 /* BSP_ARGS */
# include <crc32.h>  /* crc32 */
/* defines */
# define ARRAY_SIZE( x ) (sizeof(x)/sizeof(*(x)))
# define PRINTF_QUAD "ll"
# define CMD_NAME "nvram"
# define memcpy32(a,b,c)	memcpy(a,b,c)
/* MTD Flash device information */
#define MTD_NORFLASH            3
#define MTD_NANDFLASH           4
/* typedefs */
typedef unsigned long crc32_t;
typedef UINT64 uint64_t;
typedef INT64  int64_t;
typedef UINT32 uint32_t;
typedef INT32  int32_t;
typedef UINT16 uint16_t;
typedef INT16  int16_t;
typedef UINT8  uint8_t;
typedef INT8   int8_t;
typedef uint64_t loff_t;
#else
# error "operating system not defined"
#endif  /* LINUX */

/* we want it identical for all os */
#define CLEAR( x ) 		memset( &x, 0, sizeof( x ) )

typedef unsigned char uchar_t;
typedef char          char_t;

#ifdef __GNUC__
# define IDX(x) [x] =
#else
        /* e.g. Windows */
# define IDX(x)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* NVRAM_TYPES_H */
