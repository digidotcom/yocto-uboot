/*
 *  linux/drivers/char/watchdog/mxc_wdt.h
 *
 *  BRIEF MODULE DESCRIPTION
 *      MXC Watchdog timer register definitions
 *
 * Author: MontaVista Software, Inc.
 *       <AKuster@mvista.com> or <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc.
 * Copyright 2007-2010 Freescale Semiconductor, Inc.
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef __MXC_WDT_H__
#define __MXC_WDT_H__

#define MXC_WDT_WCR             0x00
#define MXC_WDT_WSR             0x02
#define MXC_WDT_WRSR            0x04
#define WCR_WDW_BIT             (1 << 7)
#define WCR_WDA_BIT             (1 << 5)
#define WCR_SRS_BIT             (1 << 4)
#define WCR_WDT_BIT             (1 << 3)
#define WCR_WDE_BIT             (1 << 2)
#define WCR_WDBG_BIT            (1 << 1)
#define WCR_WDZST_BIT           (1 << 0)
#define WDT_MAGIC_1             0x5555
#define WDT_MAGIC_2             0xAAAA

#define TIMER_MARGIN_MAX    	127
#define TIMER_MARGIN_DEFAULT	60	/* 60 secs */
#define TIMER_MARGIN_MIN	1

void mxc_wdt_config(void *base);
void mxc_wdt_enable(void *base);
void mxc_wdt_disable(void *base);
void mxc_wdt_adjust_timeout(unsigned new_timeout);
u16 mxc_wdt_get_timeout(void *base);
u16 mxc_wdt_get_bootreason(void *base);
void mxc_wdt_set_timeout(void *base);

#endif				/* __MXC_WDT_H__ */
