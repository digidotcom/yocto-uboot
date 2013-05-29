/*
 *  common/digi/cmd_testhw/cpu/ns921x_sysclock.c
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
 *                 sysclock: Checks system clock accuracy against an external
 *                           clock
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS921X))

#include <cmd_testhw/testhw.h>

#include <asm-arm/arch-ns9xxx/ns921x_sys.h>
#include <asm-arm/arch-ns9xxx/ns921x_gpio.h>
#include <asm-arm/arch-ns9xxx/io.h>  /* sys_readl */

#define MHz( x )		( ( x ) * 1000000 )
#define EXT_CLOCK_GPIO		78
#define EXT_CLOCK_FUNC		GPIO_CFG_FUNC_2
#define EXT_CLOCK_WHERE		"X20/B12"
#define EXT_CLOCK_FREQ		MHz( 10 )
#define EXT_CLOCK_TIMER		4 /* timer 0-3 are used by U-Boots:timer.c */
#define AHB_CLOCK_TIMER		5

/**
 * do_testhw_sysclock - checks clock accuracy
 */
static int do_testhw_sysclock( int argc, char* argv[] )
{
        u32 uiAHBTicks;
        int iDelta;
        int iPPM;
        
        printf( "Expecting reference clock of %i MHz on GPIO %i @ %s\n",
                EXT_CLOCK_FREQ / MHz( 1 ),
                EXT_CLOCK_GPIO, EXT_CLOCK_WHERE );

        /* configure external clock input */
        gpio_cfg_set( EXT_CLOCK_GPIO, EXT_CLOCK_FUNC );

        /* prepare timers. AHB_CLOCK_TIMER counts all AHB clocks until
         * EXT_CLOCK_FREQ has counted 10 MHz ticks. Then it stops.
         * If no clock is present, the test runs endless. */

        /* don't know the state. Ensure it doesn't start counting immediately*/
        sys_rmw32( SYS_TIMER_MASTER_CTRL,
                   & ~( SYS_TIMER_MASTER_CTRL_EN( EXT_CLOCK_TIMER ) |
                        SYS_TIMER_MASTER_CTRL_EN( AHB_CLOCK_TIMER ) ) );

        sys_writel( 0, SYS_TIMER_RELOAD( AHB_CLOCK_TIMER ) );
        sys_writel( SYS_TIMER_CTRL_TE       |
                       SYS_TIMER_CTRL_TCS_1    |
                       SYS_TIMER_CTRL_TM_INT   |
                       SYS_TIMER_CTRL_UP       |
                       SYS_TIMER_CTRL_32,
                       SYS_TIMER_CTRL( AHB_CLOCK_TIMER ) );

        sys_writel( EXT_CLOCK_FREQ, SYS_TIMER_RELOAD( EXT_CLOCK_TIMER ) );
        sys_writel( SYS_TIMER_CTRL_TE       |
                       SYS_TIMER_CTRL_TCS_EXT  |
                       SYS_TIMER_CTRL_TM_INT   |
                       SYS_TIMER_CTRL_DOWN     |
                       SYS_TIMER_CTRL_32,
                       SYS_TIMER_CTRL( EXT_CLOCK_TIMER ) );

        sys_rmw32( SYS_TIMER_MASTER_CTRL,
                   | ( SYS_TIMER_MASTER_CTRL_EN( EXT_CLOCK_TIMER ) |
                       SYS_TIMER_MASTER_CTRL_EN( AHB_CLOCK_TIMER ) ) );

        while( sys_readl( SYS_TIMER_READ( EXT_CLOCK_TIMER ) ) ) {
                /* wait for 1s to expire */
        }

        uiAHBTicks = sys_readl( SYS_TIMER_READ( AHB_CLOCK_TIMER ) );

        /* disable timers and GPIO */
        sys_rmw32( SYS_TIMER_MASTER_CTRL,
                   & ~( SYS_TIMER_MASTER_CTRL_EN( EXT_CLOCK_TIMER ) |
                        SYS_TIMER_MASTER_CTRL_EN( AHB_CLOCK_TIMER ) ) );
        sys_writel( 0, SYS_TIMER_CTRL( AHB_CLOCK_TIMER ) );
        sys_writel( 0, SYS_TIMER_CTRL( EXT_CLOCK_TIMER ) );

        gpio_cfg_set( EXT_CLOCK_FUNC, GPIO_CFG_FUNC_GPIO );

        /* calculate deviation in ppm */
        iDelta = uiAHBTicks - ahb_clock_freq();

        /* use floats to avoid integer overflow at 2^31 */
        iPPM = ( ( (float ) iDelta ) * 1000000 ) / ahb_clock_freq();
        if( iPPM < 0 )
                iPPM = -iPPM;
        
        printf( "Deviation is %i ticks = %i ppm\n", iDelta, iPPM );
        
        return 1;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( sysclock, "checks system clock accuracy" );

#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS921X))*/

