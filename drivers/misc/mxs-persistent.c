/*
 * Freescale STMP378X Persistent bits manipulation driver
 *
 * Author: Pantelis Antoniou <pantelis@embeddedalley.com>
 *
 * Copyright 2011 Digi International, Inc.
 * Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <common.h>
#include <asm/arch/mx28.h>
#include <asm/arch/regs-rtc.h>
#include <asm/types.h>
#include <asm/io.h>

u32 persistent_reg_read(int index)
{
	u32 msk;

	/* wait for stable value */
	msk = BF_RTC_STAT_STALE_REGS((0x1 << index));
	while (__raw_readl(REGS_RTC_BASE + HW_RTC_STAT) & msk)
		udelay(1);

	return __raw_readl(REGS_RTC_BASE +
			   HW_RTC_PERSISTENT0 + (index * 0x10));
}

void persistent_reg_wait_settle(int index)
{
	u32 msk;

	/* wait until the change is propagated */
	msk = BF_RTC_STAT_NEW_REGS((0x1 << index));
	while (__raw_readl(REGS_RTC_BASE + HW_RTC_STAT) & msk)
		udelay(1);
}

void persistent_reg_write(int index, u32 val)
{
	__raw_writel(val, REGS_RTC_BASE +
		     HW_RTC_PERSISTENT0 + (index * 0x10));
	persistent_reg_wait_settle(index);
}

void persistent_reg_set(int index, u32 val)
{
	__raw_writel(val, REGS_RTC_BASE +
		     HW_RTC_PERSISTENT0 + (index * 0x10) + 0x4);
	persistent_reg_wait_settle(index);
}

void persistent_reg_clr(int index, u32 val)
{
	__raw_writel(val, REGS_RTC_BASE +
		     HW_RTC_PERSISTENT0 + (index * 0x10) + 0x8);
	persistent_reg_wait_settle(index);
}

void persistent_reg_tog(int index, u32 val)
{
	__raw_writel(val, REGS_RTC_BASE +
		     HW_RTC_PERSISTENT0 + (index * 0x10) + 0xc);
	persistent_reg_wait_settle(index);
}
