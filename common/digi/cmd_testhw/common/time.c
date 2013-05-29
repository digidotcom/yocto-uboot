/*
 *  common/digi/cmd_testhw/common/time.c
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
 *  !Descr:      Implements commands:
 *                   udelay : Trigger LED for measuring exact times and wait
 *                            using udelay
 *                   sleep  : Triger LED for measuring and wait in s using
 *                            system timer
 *               On ns9xxx, 2 different timers are used, one for udelay, one
 *                            for system
*/

#include <common.h>

#if defined(CONFIG_CMD_BSP) &&			\
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&	\
    defined(CONFIG_STATUS_LED)

#include <status_led.h>
#include <cmd_testhw/testhw.h>

/* ********** local functions ********** */

/* we need also the GPIO because we use __led_set directly for improved
 * precision in udelay */
#if defined( STATUS_LED_BIT3 )
# define GPIO STATUS_LED_BIT3
#elif defined( STATUS_LED_BIT2 )
# define GPIO STATUS_LED_BIT2
#elif defined( STATUS_LED_BIT1 )
# define GPIO STATUS_LED_BIT1
#elif defined( STATUS_LED_BIT )
# define GPIO STATUS_LED_BIT
#else
# error Need STATUS_LED_BIT set
#endif

/**
 * do_testhw_udelay - trigger and wait us
 */
static int do_testhw_udelay( int argc, char* argv[] )
{
        int iDelay = 1;
        
        if( ( argc > 1 ) ) {
                eprintf( "Usage: udelay <us>\n" );
                goto error;
        }

        iDelay  = simple_strtoul( argv[ 0 ], NULL, 10 );
        printf( "Delaying for %i us. Signal on status LED (GPIO %i)\n", iDelay, GPIO );

        /* disable it */
        __led_set( GPIO, STATUS_LED_OFF );

        /* enable it for trigger */
        __led_set( GPIO, STATUS_LED_ON );
        udelay( iDelay );
        /* disable it */
        __led_set( GPIO, STATUS_LED_OFF );

        return 1;

error:
        return 0;
}

/**
 * do_testhw_udelay - trigger and wait in seconds
 */
static int do_testhw_sleep( int argc, char* argv[] )
{
        int iDelay = 1;
        ulong ulStart;
        ulong ulWait;
        
        if( ( argc > 1 ) ) {
                eprintf( "Usage: sleep <s>\n" );
                goto error;
        }

        iDelay  = simple_strtoul( argv[ 0 ], NULL, 10 );
        printf( "Sleeping for %i s. Signal on status LED (GPIO %i)\n", iDelay, GPIO );

        ulWait = CONFIG_SYS_HZ * iDelay;

        /* disable it */
        __led_set( GPIO, STATUS_LED_OFF );

        /* enable it for trigger */
        __led_set( GPIO, STATUS_LED_ON );

        ulStart = get_timer( 0 );
        while( get_timer( ulStart ) < ulWait ) {
                /* wait for timer to expire, do nothing */
        }

        /* disable it */
        __led_set( GPIO, STATUS_LED_OFF );

        return 1;

error:
        return 0;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( udelay, "<us>" );
TESTHW_CMD( sleep,  "<s>"  );

#endif /* (CONFIG_CMD_BSP) ... */


