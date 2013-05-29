/*
 *  common/digi/atoi.c
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision$: 
 *  !Author:     Markus Pietrek
 *  !Descr:      Provides atoi and similar
*/

#include <common.h>             /* simple_strtol */

#include "atoi.h"

int atoi( const char* szStr )
{
        return simple_strtol( szStr, NULL, 10 );
}
