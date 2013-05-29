/*
 *  common/digi/cmd_testhw/cpu/ns921x_powersave.c
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
 *                 powersave
 *  !References: [1] NS9215 Hardware Reference Manual, Preliminary January 2007
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS921X))

#include <cmd_testhw/testhw.h>

#include <asm-arm/proc-armv/system.h>  /* local_irq_enable */

#include <asm-arm/arch-ns9xxx/ns921x_sys.h>
#include <asm-arm/arch-ns9xxx/ns921x_gpio.h>
#include <asm-arm/arch-ns9xxx/io.h>  /* sys_readl */

#include <status_led.h>         /* must be included after our io.h */

#define GPIO_WAKEUP	4
#define EXT_INT_WAKEUP	2

/**
 * wakeup_irq_ack - acknowledges the wakeup irq on all aspects. 
 */
static void wakeup_irq_ack( void )
{
        /* ack edge sensitive irq */
        sys_rmw32( SYS_EXT_INT_CTRL( EXT_INT_WAKEUP ), | SYS_EXT_INT_CTRL_CLR );
        sys_rmw32( SYS_EXT_INT_CTRL( EXT_INT_WAKEUP ), & ~SYS_EXT_INT_CTRL_CLR );

        /* ack wake IRQ */
        sys_rmw32( SYS_POWER, | SYS_POWER_WAKE_INT_CLR );
        sys_rmw32( SYS_POWER, & ~SYS_POWER_WAKE_INT_CLR );

        /* ack irq, will be done in linux automatically */
        sys_writel( 0, SYS_ISADDR );
}

/**
 * wakeup_irq_handler - system is up again
 */
static void wakeup_irq_handler( void* pvData )
{
        wakeup_irq_ack();

        /* light LED here so we know that CPU is up even if something later the
         * initialization might fail */
        status_led_set( STATUS_LED_BOOT, STATUS_LED_ON );
}

/**
 * do_testhw_powersave - sleeps until wakeup is pressed
 */
static int do_testhw_powersave( int argc, char* argv[] )
{
        u32 uiClock;
        u32 uiTimer;
        unsigned long ulFlags;

        /* prepare going down */
        printf( "I'll be back if you press \"Wakeup\".\n"
                "You shouldn't have anything connected on Serial Port A\n" );

        serial_tx_flush();      /* we are going down really fast */

        gpio_cfg_set( GPIO_WAKEUP, GPIO_CFG_FUNC_1 );

        /* wakeup emits an interrupt. Prepare to handle it. */
        irq_install_handler( SYS_ISD_WAKEUP, wakeup_irq_handler, NULL );
        /* configure int 0 directly until we have have a fully implemented
         * irq_install_handler */
        sys_writel( SYS_EXT_INT_CTRL_PLTY_L |
                       SYS_EXT_INT_CTRL_EDGE, SYS_EXT_INT_CTRL( EXT_INT_WAKEUP ) );
        /* don't know the state yet. Acknowledge so it doesn't raise the
         * interrupt to early */
        wakeup_irq_ack();

        /* prepare IRQ */
        sys_writel( 0, SYS_INT_VEC_ADR_BASE );
        sys_writel( ( SYS_INT_CFG_IE  |
                         SYS_INT_CFG_IRQ |
                         SYS_INT_CFG_ISD( SYS_ISD_WAKEUP ) ) << 24, SYS_INT_CFG_BASE );

        local_save_flags( ulFlags );
        local_irq_enable();

        /*
         * now put everything that is possible into sleep. Note that we don't
         * put ETH Phy into reset mode on purpose. */

        /* disable timer */
        uiTimer = sys_readl( SYS_TIMER_MASTER_CTRL );
        sys_writel( 0, SYS_TIMER_MASTER_CTRL );

        uiClock = sys_readl( SYS_CLOCK );

        /* reduce power and clock */
        sys_writel( SYS_POWER_SELF_REFRESH |
                       SYS_POWER_HW_CLOCK     |
                       SYS_POWER_WAKE_INT_EXT( EXT_INT_WAKEUP ),
                       SYS_POWER );

        sys_writel( SYS_CLOCK_CSC_16     |
                       SYS_CLOCK_MAX_CSC_16 |
                       SYS_CLOCK_MCCOUT( 0 ), SYS_CLOCK );

        status_led_set( STATUS_LED_BOOT, STATUS_LED_OFF );

        /* sleep until someone pressed wakeup */
        asm volatile(
                "mcr p15, 0, %0, c7, c0, 4"
                :
                : "r" ( 0 ) );

        /* go back into running mode */
        sys_writel( 0, SYS_POWER );

        /* restore modules clocks */
        sys_writel( uiClock, SYS_CLOCK );
        sys_writel( uiTimer, SYS_TIMER_MASTER_CTRL );

        sys_writel( 0, SYS_INT_CFG_BASE );
        irq_free_handler( SYS_ISD_WAKEUP );
        local_irq_restore( ulFlags );

        /* don't need wakeup any longer */
        gpio_cfg_set( GPIO_WAKEUP, GPIO_CFG_FUNC_GPIO );

        return 1;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( powersave, "sleeps CPU and waits for wakeup event" );

#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS921X))*/

