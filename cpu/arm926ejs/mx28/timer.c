/*
 * (C) Copyright 2003
 * Texas Instruments <www.ti.com>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002-2004
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * (C) Copyright 2004
 * Philippe Robin, ARM Ltd. <philippe.robin@arm.com>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/regs-timrot.h>
#include <asm/io.h>

/*
 * TIMROT gets 4 timer instances
 * Define N as 0..3 to specify
 */
#define N		0

/* Ticks per second */
#define CONFIG_SYS_HZ	1000

/* Maximum fixed count */
#define TIMER_LOAD_VAL	0xffffffff

static ulong timestamp;
static ulong lastdec;

int timer_init(void)
{
	/*
	 * Reset Timers and Rotary Encoder module
	 */

	/* Clear SFTRST */
	REG_CLR(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL, 1 << 31);
	while (REG_RD(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL) & (1 << 31))
		;

	/* Clear CLKGATE */
	REG_CLR(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL, 1 << 30);

	/* Set SFTRST and wait until CLKGATE is set */
	REG_SET(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL, 1 << 31);
	while (!(REG_RD(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL) & (1 << 30)))
		;

	/* Clear SFTRST and CLKGATE */
	REG_CLR(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL, 1 << 31);
	REG_CLR(REGS_TIMROT_BASE, HW_TIMROT_ROTCTRL, 1 << 30);

	/*
	* Now initialize timer
	*/

	/* Set fixed_count to 0 */
	REG_WR(REGS_TIMROT_BASE, HW_TIMROT_FIXED_COUNTn(N), 0);

	/* set UPDATE bit and 1Khz frequency */
	REG_WR(REGS_TIMROT_BASE, HW_TIMROT_TIMCTRLn(N),
		BM_TIMROT_TIMCTRLn_RELOAD | BM_TIMROT_TIMCTRLn_UPDATE |
		BV_TIMROT_TIMCTRLn_SELECT__1KHZ_XTAL);

	/* Set fixed_count to maximal value */
	REG_WR(REGS_TIMROT_BASE, HW_TIMROT_FIXED_COUNTn(N), TIMER_LOAD_VAL);

	/* init the timestamp and lastdec value */
	reset_timer_masked();

	return 0;
}

/*
 * timer without interrupts
 */

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

void set_timer(ulong t)
{
	timestamp = t;
}

/* We use the HW_DIGCTL_MICROSECONDS register for sub-millisecond timer. */
#define	MX28_HW_DIGCTL_MICROSECONDS	0x8001c0c0

void udelay(unsigned long usec)
{
	uint32_t old, new, incr;
	uint32_t counter = 0;

	old = readl(MX28_HW_DIGCTL_MICROSECONDS);

	while (counter < usec) {
		new = readl(MX28_HW_DIGCTL_MICROSECONDS);

		/* Check if the timer wrapped. */
		if (new < old) {
			incr = 0xffffffff - old;
			incr += new;
		} else {
			incr = new - old;
		}

		/*
		 * Check if we are close to the maximum time and the counter
		 * would wrap if incremented. If that's the case, break out
		 * from the loop as the requested delay time passed.
		 */
		if (counter + incr < counter)
			break;

		counter += incr;
		old = new;
	}
}

void reset_timer_masked(void)
{
	/* capure current decrementer value time */
	lastdec = REG_RD(REGS_TIMROT_BASE, HW_TIMROT_RUNNING_COUNTn(N));
	/* start "advancing" time stamp from 0 */
	timestamp = 0;
}

ulong get_timer_masked(void)
{
	/* current tick value */
	ulong now = REG_RD(REGS_TIMROT_BASE, HW_TIMROT_RUNNING_COUNTn(N));

	if (lastdec >= now) {		/* normal mode (non roll) */
		/* normal mode */
		timestamp += lastdec - now;
		/* move stamp fordward with absoulte diff ticks */
	} else {
		/* we have overflow of the count down timer */
		/* nts = ts + ld + (TLV - now)
		 * ts=old stamp, ld=time that passed before passing through -1
		 * (TLV-now) amount of time after passing though -1
		 * nts = new "advancing time stamp"...it could also roll
		 * and cause problems.
		 */
		timestamp += lastdec + TIMER_LOAD_VAL - now + 1;
	}
	lastdec = now;

	return timestamp;
}

/* waits specified delay value and resets timestamp */
void udelay_masked(unsigned long usec)
{
	ulong tmo;
	ulong endtime;
	signed long diff;

	if (usec >= 1000) {
		/* if "big" number, spread normalization to seconds */
		tmo = usec / 1000;
		/* start to normalize for usec to ticks per sec */
		tmo *= CONFIG_SYS_HZ;
		/* find number of "ticks" to wait to achieve target */
		tmo /= 1000;
		/* finish normalize. */
	} else {
		/* else small number, don't kill it prior to HZ multiply */
		tmo = usec * CONFIG_SYS_HZ;
		tmo /= (1000*1000);
	}

	endtime = get_timer_masked() + tmo;

	do {
		ulong now = get_timer_masked();
		diff = endtime - now;
	} while (diff >= 0);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	ulong tbclk;

	tbclk = CONFIG_SYS_HZ;
	return tbclk;
}
