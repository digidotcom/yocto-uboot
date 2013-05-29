/*
 * Watchdog driver for Freescale STMP37XX/STMP378X
 *
 * Copyright 2011 Digi International, Inc.
 * Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 */
#include <common.h>
#include <asm/arch/mx28.h>
#include <asm/arch/regs-rtc.h>
#include <asm/io.h>

/* missing bitmask in headers */
#define BV_RTC_PERSISTENT1_GENERAL__RTC_FORCE_UPDATER	0x80000000
/* Maximum configurable watchdog timeout (in miliseconds) */
# define WDT_MAX_TIMEOUT			4294967000UL

unsigned long wdt_enable(unsigned long value)
{
	if (value > WDT_MAX_TIMEOUT)
		value = WDT_MAX_TIMEOUT;
	__raw_writel(value, REGS_RTC_BASE + HW_RTC_WATCHDOG);
	__raw_writel(BV_RTC_PERSISTENT1_GENERAL__RTC_FORCE_UPDATER,
		     REGS_RTC_BASE + HW_RTC_PERSISTENT1_SET);
	__raw_writel(BM_RTC_CTRL_WATCHDOGEN, REGS_RTC_BASE + HW_RTC_CTRL_SET);

	return value;
}

void wdt_disable(void)
{
	__raw_writel(BV_RTC_PERSISTENT1_GENERAL__RTC_FORCE_UPDATER,
		     REGS_RTC_BASE + HW_RTC_PERSISTENT1_CLR);
	__raw_writel(BM_RTC_CTRL_WATCHDOGEN, REGS_RTC_BASE + HW_RTC_CTRL_CLR);
}
