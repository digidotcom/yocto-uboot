/*
 *  /common/digi/cmd_testhw/cpu/ns9360.c
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
 *  !Descr:      Implements NS9360 specific commands
 *                  bigendian_watchdog_reset_switch
 *                  watchdog_reset
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS9360))

#include <cmd_testhw/testhw.h>

#include "ns9750_ser.h"		/* for serial configuration */
#include "ns9750_sys.h"

/* ********** global functions ********** */

/* from bigendian_watchdog_reset_switch.S */
extern void testhw_bigendian_watchdog_reset_switch( void ); 

/* ********** local functions ********** */

/*! \return never */
static void TriggerWatchdogResetAndWait( void )
{
        /* trigger watchdog for immediately reset */
        *get_sys_reg_addr( NS9750_SYS_SW_WDOG_TIMER ) = 0x1;
        *get_sys_reg_addr( NS9750_SYS_SW_WDOG_CFG   ) = 
                NS9750_SYS_SW_WDOG_CFG_SWWE    |       
                NS9750_SYS_SW_WDOG_CFG_SWWIC   |
                NS9750_SYS_SW_WDOG_CFG_SWTCS_2;

        while( 1 ) {
                /* do nothing, wait for reset */
        }

        /* never reached */
}

/*! \brief resets the system */
/*! Performs a switch to big-endianess and then a watchdog reset
 *  On some modules (CC9P9360 Rev. 5) it will hang, simulating
 *  a behaviour like NET+OS
 *  It will never return */
static int do_testhw_bigendian_watchdog_reset( int argc, char* argv[] )
{
        printf( "Resetting system\n" );

        serial_tx_flush();

        /* jump to assembler code performing the switch over and reset */
        testhw_bigendian_watchdog_reset_switch();

        TriggerWatchdogResetAndWait();

        /* never reached  */
        return 0;
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

TESTHW_CMD( bigendian_watchdog_reset,
            "Performs a bigendian switch and then a watchdog reset" );
TESTHW_CMD( watchdog_reset,
            "Performs a watchdog reset" );

#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS9360))*/


