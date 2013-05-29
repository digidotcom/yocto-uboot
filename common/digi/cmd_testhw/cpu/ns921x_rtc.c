/*
 *  common/digi/cmd_testhw/cpu/ns921x_rtc.c
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
 *  !Descr:      Provides:
 *                 rtc: Checks rtc clock accuracy against system clock
 *                      System clock has to be checked first
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS921X) && \
     defined(CONFIG_RTC_NS921X))

#include <cmd_testhw/testhw.h>

#include <asm-arm/arch-ns9xxx/ns921x_sys.h>
#include <asm-arm/arch-ns9xxx/ns921x_rtc.h>
#include <asm-arm/arch-ns9xxx/io.h>  /* sys_readl */

#define AHB_CLOCK_TIMER		5

/* from rtc/ns921x_rtc. */
extern void rtc_lowlevel_init( void );

/**
 * rtc_wait_for_second_toggle - waits until the RTC tells one second has elpased
 */
static inline void rtc_wait_for_second_toggle( void )
{
        while( ! ( rtc_readl( RTC_EVENT ) & RTC_EVENT_SEC ) ) {
                /* wait for second to elapse */
        }
}

/**
 * rtc_calc_and_print_deviation - determine RTC deviation based on AHB clock
 */
static void rtc_calc_and_print_deviation( void )
{
        u32 uiAHBTicks;
        int iDelta;
        int iPPM;

        /* the event field might be already set. clear it to wait for next
         * second. */
        rtc_readl( RTC_EVENT );
        
        rtc_wait_for_second_toggle();
        /* go, go, go */
        sys_rmw32( SYS_TIMER_MASTER_CTRL,
                   | SYS_TIMER_MASTER_CTRL_EN( AHB_CLOCK_TIMER ) );
        /* now measure how long a second is */
        rtc_wait_for_second_toggle();

        /* get elapses uiAHBTicks. */
        uiAHBTicks = sys_readl( SYS_TIMER_READ( AHB_CLOCK_TIMER ) );

        /* calculate deviation in ppm */
        iDelta = uiAHBTicks - ahb_clock_freq();

        /* use floats to avoid integer overflow at 2^31 */
        iPPM = ( ( (float ) iDelta ) * 1000000 ) / ahb_clock_freq();
        if( iPPM < 0 )
                iPPM = -iPPM;
        
        printf( "Deviation is %i ticks = %i ppm relative to system clock accuracy\n",
                iDelta, iPPM );
}

/**
 * do_testhw_rtc - checks clock accuracy
 */
static int do_testhw_rtc( int argc, char* argv[] )
{
        /* prepare timer. AHB_CLOCK_TIMER counts all AHB clocks until
         * rtc reports 1s elapsed. */

        /* don't know the state. Ensure it doesn't start counting immediately*/
        sys_rmw32( SYS_TIMER_MASTER_CTRL,
                   & ~SYS_TIMER_MASTER_CTRL_EN( AHB_CLOCK_TIMER ) );

        sys_writel( 0, SYS_TIMER_RELOAD( AHB_CLOCK_TIMER ) );
        sys_writel( SYS_TIMER_CTRL_TE       |
                       SYS_TIMER_CTRL_TCS_1    |
                       SYS_TIMER_CTRL_TM_INT   |
                       SYS_TIMER_CTRL_UP       |
                       SYS_TIMER_CTRL_32,
                       SYS_TIMER_CTRL( AHB_CLOCK_TIMER ) );

        /* initialize RTC */
        rtc_lowlevel_init();

        rtc_calc_and_print_deviation();

        /* disable timer */
        sys_rmw32( SYS_TIMER_MASTER_CTRL,
                   & ~SYS_TIMER_MASTER_CTRL_EN( AHB_CLOCK_TIMER ) );
        sys_writel( 0, SYS_TIMER_CTRL( AHB_CLOCK_TIMER ) );
        
        return 1;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( rtc, "checks rtc clock accuracy" );

#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS921X))*/

