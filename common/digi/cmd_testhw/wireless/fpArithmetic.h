#if 0
#include <common.h>

#if CFG_HAS_WIRELESS

#ifndef _FPARITHMETIC_H
#define _FPARITHMETIC_H

/* Approach: 100 codes per 5 dBm change maximum */
/* Integer required: 7 bits + sign = 8 bits */
/* Fraction required: 8 bits --> 0.0039 Granularity */

typedef union FIXED816_8tag
{
	int full;
	struct part16_8tag
	{
	        signed int integer;
        	unsigned char fraction;
    	} part;
} FIXED16_8;


#endif

#endif /* CFG_HAS_WIRELESS */

#endif /*if 0*/
