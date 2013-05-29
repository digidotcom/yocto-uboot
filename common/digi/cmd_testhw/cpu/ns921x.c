/*
 *  common/digi/cmd_testhw/cpu/ns921x.c
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
 *  !Descr:      Implements NS921x specific commands
 *                  watchdog_reset
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS921X))

#include <cmd_testhw/testhw.h>

#include "asm-arm/arch-ns9xxx/ns921x_sys.h"
#include "asm-arm/arch-ns9xxx/io.h"

/* ********** local functions ********** */

/*! \return never */
static void TriggerWatchdogResetAndWait( void )
{
        sys_writel( 0x1, SYS_SW_WDOG_TIMER );

        /* trigger watchdog for immediately reset */
        sys_writel( SYS_SW_WDOG_CFG_SWWE  |
                       SYS_SW_WDOG_CFG_SWWIC |
                       SYS_SW_WDOG_CFG_SWTCS_2,
                       SYS_SW_WDOG_CFG );

        while( 1 ) {
                /* do nothing, wait for reset */
        }

        /* never reached */
}

/*! \brief resets the system */
/*! It will never return */
static int do_testhw_watchdog_reset( int argc, char* argv[] )
{
        printf( "Resetting system\n" );

        serial_tx_flush();

        TriggerWatchdogResetAndWait();

        /* never reached  */
        return 0;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( watchdog_reset,
            "Performs a watchdog reset" );

#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS921X))*/


