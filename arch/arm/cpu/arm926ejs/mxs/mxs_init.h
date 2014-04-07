/*
 * Freescale i.MX28 SPL functions
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef	__M28_INIT_H__
#define	__M28_INIT_H__

void early_delay(int delay);

void mxs_power_init(void);

#ifdef	CONFIG_SPL_MXS_PSWITCH_WAIT
void mxs_power_wait_pswitch(void);
#else
static inline void mxs_power_wait_pswitch(void) { }
#endif

void mxs_mem_init(void);
uint32_t mxs_mem_get_size(void);

void mxs_lradc_init(void);
void mxs_lradc_enable_batt_measurement(void);

/*
 *	Overview of MX28 clock generation
 *	(See RefMan Rev 1 Figure 10.2.2 "Logical Diagram of Clock Domains")
 *
 *	PLL0 generates 480Mhz base reference clock (ref_pll)
 *	=>	which is divided by a "fractional divider" to
 *		produce the subsystem reference clock (ref_<sub>)
 *		=>	which is then divided by the subsystem divider
 *			to produce the subsystem clock (CLK_<SUB>):
 *
 *	subsys		ref										subsys		subsys
 *	clock		pll			'fractional divider'		ref clk		divider
 *	========	===		  =========================		========	======================
 *	CLK_P	 := 480 * (18/HW_CLKCTRL_FRAC0.CPUFRAC ) [= ref_cpu ] / HW_CLKCTRL_CPU.DIV_CPU
 *	CLK_EMI	 := 480 * (18/HW_CLKCTRL_FRAC0.EMIFRAC ) [= ref_emi ] / HW_CLKCTRL_EMI.DIV_EMI
 *	CLK_SSPx := 480 * (18/HW_CLKCTRL_FRAC0.IOxFRAC ) [= ref_ioX ] / HW_CLKCTRL_SSPx.DIV
 *	CLK_GPMI := 480 * (18/HW_CLKCTRL_FRAC0.GPMIFRAC) [= ref_gpmi] / HW_CLKCTRL_GPMI.DIV
 *
 *	CLK_H	 := CLK_P / HW_CLKCTRL_HBUS.DIV
 *
 *	Additionally, each subsystem has a 'bypass' mode where the PLL is bypassed
 *	and the 24MHz ref_xtal is used as the subsystem clock source and then
 *	further divided by the subsystem DIV_XTAL field.
 *
 *	These defines select from the small subset of possible combinations
 *	which have been validated by Freescale (see IMX28CEC data sheet,
 *	table 15 "Recommended Operating States"). There are many other
 *	possible voltage/clocking combinations which we don't want to use
 *	since Freescale may not have tested them.
 *
 *						BO
 *	State	VDDD		VDDD 	CLK_P (div,frac)	CLK_H (div)		CLK_EMI (div,frac)
 *	-----	-----		-----	----------------	-----------		------------------
 *	 "64"	 tbd	 	tbd	 64    (5,27)		 64    (1)		130.91 (2,33)
 *	 "261"	1.350 (22)	1.250	261.81 (1,33)		130.91 (2)		130.91 (2,33)
 *	 "360"	1.350 (22)	1.250	360    (1,24)		120    (3)		130.91 (2,33)
 *	 "400"	1.450 (26)	1.350
 *	 "454"	1.550 (30)	1.450	454.73 (1,19)		151.57 (3)		205.71 (2,21)
 */

#define STATE_64_VDDD		tbd
#define STATE_64_CPU_FRAC	27
#define STATE_64_CPU_DIV	5
#define STATE_64_HBUS_DIV	1
#define STATE_64_EMI_FRAC	33
#define STATE_64_EMI_DIV	2

#define STATE_261_VDDD		1350
#define STATE_261_CPU_FRAC	33
#define STATE_261_CPU_DIV	1
#define STATE_261_HBUS_DIV	2
#define STATE_261_EMI_FRAC	33
#define STATE_261_EMI_DIV	2

#define STATE_360_VDDD		1350
#define STATE_360_CPU_FRAC	24
#define STATE_360_CPU_DIV	1
#define STATE_360_HBUS_DIV	3
#define STATE_360_EMI_FRAC	33
#define STATE_360_EMI_DIV	2

#define STATE_400_VDDD		1450
#define STATE_400_CPU_FRAC	tbd
#define STATE_400_CPU_DIV	tbd
#define STATE_400_HBUS_DIV	tbd
#define STATE_400_EMI_FRAC	tbd
#define STATE_400_EMI_DIV	tbd

#define STATE_454_VDDD		1550
#define STATE_454_CPU_FRAC	19
#define STATE_454_CPU_DIV	1
#define STATE_454_HBUS_DIV	3
#define STATE_454_EMI_FRAC	21
#define STATE_454_EMI_DIV	2

#define CONCAT(a,b,c)		a ## _ ## b ## _ ## c
#define VDDD(x)			CONCAT( STATE, x, VDDD )
#define CPU_FRAC(x)		CONCAT( STATE, x, CPU_FRAC )
#define CPU_DIV(x)		CONCAT( STATE, x, CPU_DIV )
#define HBUS_DIV(x)		CONCAT( STATE, x, HBUS_DIV )
#define EMI_FRAC(x)		CONCAT( STATE, x, EMI_FRAC )
#define EMI_DIV(x)		CONCAT( STATE, x, EMI_DIV )

#endif	/* __M28_INIT_H__ */
