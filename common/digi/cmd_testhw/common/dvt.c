/*
 *  common/digi/cmd_testhw/common/dvt.c
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
 *                dvt on|off
*/

#include <common.h>

#if defined(CONFIG_CMD_BSP) &&         \
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && \
    defined(CONFIG_DVT_PROVIDED)

#include <stdarg.h>             /* va_start */
#include <dvt.h>
#include <cmd_testhw/testhw.h>

#define USAGE "on|off|reset"

char g_bDVTIsEnabled     = 0;
char g_bDVTStatusChanged = 0;
char g_bDVTHadError      = 0;
char g_bDVTHadWarning    = 0;

static void DVTEnable( char bEnabled ) 
{
        /* reset status, print any pending errors */
        DVTReset();

        g_bDVTIsEnabled = bEnabled;

        printf( "DVT specific handling is %s\n",
                ( g_bDVTIsEnabled ? "On" : "Off" ) );
}

/**
 * do_testhw_udelay - trigger and wait in seconds
 */
static int do_testhw_dvt( int argc, char* argv[] )
{
        int iRes;
        
        if( 1 != argc ) {
                eprintf( "Usage: dvt %s\n", USAGE );
                goto error;
        }

        iRes = !DVTError();

        if( !strcmp( argv[ 0 ], "on" ) )
                DVTEnable( 1 );
        else if( !strcmp( argv[ 0 ], "off" ) )
                DVTEnable( 0 );
        else if( !strcmp( argv[ 0 ], "reset" ) )
                /* only error register */
                DVTReset();
        else {
                printf( "Unknown options %s\n", argv[ 0 ] );
                goto error;
        }
        
        return iRes;

error:
        return 0;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( dvt, USAGE );
#endif  /* (CONFIG_CMD_BSP) ... */
