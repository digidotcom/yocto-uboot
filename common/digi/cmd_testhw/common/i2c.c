/*
 *  common/digi/cmd_testhw/common/i2c.c
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !Descr:      Provides commands:
 *                 i2c speed [<speed>]
*/

#include <common.h>

#if defined(CONFIG_CMD_BSP) && 			\
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&	\
    defined(CFG_I2C_SPEED)

#include <i2c.h>                /* i2c_speed */
#include <cmd_testhw/testhw.h>

/* ********** local functions ********** */

/**
 * do_testhw_i2c - trigger and wait us
 */
static int do_testhw_i2c_speed( int argc, char* argv[] )
{
        int iSpeed;
        
        if( argc > 1 ) {
                eprintf( "Usage: i2c_speed [<kHz>]\n" );
                goto error;
        }

        if( 1 == argc  )
                /* new value */
                i2c_init( simple_strtoul( argv[ 0 ], NULL, 10 ) * 1000, 0 );

        iSpeed = i2c_speed();
        
        printf( "Current I2C Speed is %i kHz\n", iSpeed / 1000 );

        return 1;

error:
        return 0;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( i2c_speed, "[<kHz>]" );

#endif /* (CONFIG_CMD_BSP) ... */


