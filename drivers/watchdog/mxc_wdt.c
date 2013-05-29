/*
 * linux/drivers/char/watchdog/mxc_wdt.c
 *
 * Watchdog driver for FSL MXC. It is based on omap1610_wdt.c
 *
 * Copyright (C) 2004-2011 Freescale Semiconductor, Inc.
 * 2005 (c) MontaVista Software, Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 *
 * 20051207: <AKuster@mvista.com>
 *	     	Full rewrite based on
 *		linux-2.6.15-rc5/drivers/char/watchdog/omap_wdt.c
 *	     	Add platform resource support
 *
 */

/*!
 * @defgroup WDOG Watchdog Timer (WDOG) Driver
 */
/*!
 * @file mxc_wdt.c
 *
 * @brief Watchdog timer driver
 *
 * @ingroup WDOG
 */

#include <common.h>
#include <asm/arch/mxc_wdt.h>
#include <asm/types.h>
#include <asm/io.h>

#define WDOG_SEC_TO_COUNT(s)  ((s * 2) << 8)
#define WDOG_COUNT_TO_SEC(c)  ((c >> 8) / 2)

static unsigned timer_margin = TIMER_MARGIN_DEFAULT;

void mxc_wdt_config(void *base)
{
	u16 val;

	val = __raw_readw(base + MXC_WDT_WCR);
	val |= 0xFF00 | WCR_WDA_BIT | WCR_SRS_BIT;
	/* enable suspend WDT */
	val |= WCR_WDZST_BIT | WCR_WDBG_BIT;
	/* generate reset if wdog times out */
	val &= ~WCR_WDT_BIT;

	__raw_writew(val, base + MXC_WDT_WCR);
}

void mxc_wdt_enable(void *base)
{
	u16 val;

	val = __raw_readw(base + MXC_WDT_WCR);
	val |= WCR_WDE_BIT;
	__raw_writew(val, base + MXC_WDT_WCR);
}

void mxc_wdt_disable(void *base)
{
	/* disable not supported by this chip */
}

void mxc_wdt_adjust_timeout(unsigned new_timeout)
{
	if (new_timeout < TIMER_MARGIN_MIN)
		new_timeout = TIMER_MARGIN_DEFAULT;
	if (new_timeout > TIMER_MARGIN_MAX)
		new_timeout = TIMER_MARGIN_MAX;
	timer_margin = new_timeout;
}

u16 mxc_wdt_get_timeout(void *base)
{
	u16 val;

	val = __raw_readw(base + MXC_WDT_WCR);
	return WDOG_COUNT_TO_SEC(val);
}

u16 mxc_wdt_get_bootreason(void *base)
{
	u16 val;

	val = __raw_readw(base + MXC_WDT_WRSR);
	return val;
}

void mxc_wdt_set_timeout(void *base)
{
	u16 val;

	val = __raw_readw(base + MXC_WDT_WCR);
	val = (val & 0x00FF) | WDOG_SEC_TO_COUNT(timer_margin);
	__raw_writew(val, base + MXC_WDT_WCR);
	val = __raw_readw(base + MXC_WDT_WCR);
	timer_margin = WDOG_COUNT_TO_SEC(val);
}
