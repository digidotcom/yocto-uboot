/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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

#include <common.h>
#include <asm/errno.h>
#include <asm/arch/regs-clkctrl.h>
#include <asm/cache-cp15.h>
#include <asm/fec.h>
#include <asm/arch/regs.h>
#include <asm/io.h>

static u32 mx28_get_pclk(void)
{
	const u32 xtal = 24, ref = 480;
	u32 clkfrac, clkseq, clkctrl;
	u32 frac, div;
	u32 pclk;

	clkfrac = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC0);
	clkseq = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ);
	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_CPU);

	if (clkctrl & (BM_CLKCTRL_CPU_DIV_XTAL_FRAC_EN |
		BM_CLKCTRL_CPU_DIV_CPU_FRAC_EN)) {
		/* No support of fractional divider calculation */
		pclk = 0;
	} else {
		if (clkseq & BM_CLKCTRL_CLKSEQ_BYPASS_CPU) {
			/* xtal path */
			div = (clkctrl & BM_CLKCTRL_CPU_DIV_XTAL) >>
				BP_CLKCTRL_CPU_DIV_XTAL;
			pclk = xtal / div;
		} else {
			/* ref path */
			frac = (clkfrac & BM_CLKCTRL_FRAC0_CPUFRAC) >>
				BP_CLKCTRL_FRAC0_CPUFRAC;
			div = (clkctrl & BM_CLKCTRL_CPU_DIV_CPU) >>
				BP_CLKCTRL_CPU_DIV_CPU;
			pclk =  (ref * 18 / frac) / div;
		}
	}

	return pclk;
}

static u32 mx28_get_hclk(void)
{
	u32 clkctrl, div, hclk;

	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_HBUS);

	if (clkctrl & BM_CLKCTRL_HBUS_DIV_FRAC_EN) {
		/* No support of fractional divider calculation */
		hclk = 0;
	} else {
		div = (clkctrl & BM_CLKCTRL_HBUS_DIV) >>
			BP_CLKCTRL_HBUS_DIV;
		hclk = mx28_get_pclk() / div;
	}

	return hclk;
}

static u32 mx28_get_emiclk(void)
{
	const u32 xtal = 24, ref = 480;
	u32 clkfrac, clkseq, clkctrl;
	u32 frac, div;
	u32 emiclk;

	clkfrac = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC0);
	clkseq = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ);
	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_EMI);

	if (clkseq & BM_CLKCTRL_CLKSEQ_BYPASS_EMI) {
		/* xtal path */
		div = (clkctrl & BM_CLKCTRL_EMI_DIV_XTAL) >>
			BP_CLKCTRL_EMI_DIV_XTAL;
		emiclk = xtal / div;
	} else {
		/* ref path */
		frac = (clkfrac & BM_CLKCTRL_FRAC0_EMIFRAC) >>
			BP_CLKCTRL_FRAC0_EMIFRAC;
		div = (clkctrl & BM_CLKCTRL_EMI_DIV_EMI) >>
			BP_CLKCTRL_EMI_DIV_EMI;
		emiclk =  (ref * 18 / frac) / div;
	}

	return emiclk;
}
static inline void __enable_gpmi_clk(void)
{
	/* Clear bypass bit*/
	REG_SET(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ,
	       BM_CLKCTRL_CLKSEQ_BYPASS_GPMI);
	/* Set gpmi clock to ref_gpmi/12 */
	REG_WR(REGS_CLKCTRL_BASE, HW_CLKCTRL_GPMI,
	      (REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_GPMI) &
	      ~BM_CLKCTRL_GPMI_DIV & ~BM_CLKCTRL_GPMI_CLKGATE) |
	      1);
}
static u32 mx28_get_gpmiclk(void)
{
	const u32 xtal = 24, ref = 480;
	u32 clkfrac, clkseq, clkctrl;
	u32 frac, div;
	u32 gpmiclk;
	/* Enable gpmi clock */
	__enable_gpmi_clk();

	clkfrac = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_FRAC1);
	clkseq = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ);
	clkctrl = REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_GPMI);

	if (clkseq & BM_CLKCTRL_CLKSEQ_BYPASS_GPMI) {
		/* xtal path */
		div = (clkctrl & BM_CLKCTRL_GPMI_DIV) >>
			BP_CLKCTRL_GPMI_DIV;
		gpmiclk = xtal / div;
	} else {
		/* ref path */
		frac = (clkfrac & BM_CLKCTRL_FRAC1_GPMIFRAC) >>
			BP_CLKCTRL_FRAC1_GPMIFRAC;
		div = (clkctrl & BM_CLKCTRL_GPMI_DIV) >>
			BP_CLKCTRL_GPMI_DIV;
		gpmiclk =  (ref * 18 / frac) / div;
	}

	return gpmiclk;
}
u32 mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return mx28_get_pclk() * 1000000;
	case MXC_GPMI_CLK:
		return mx28_get_gpmiclk() * 1000000;
	case MXC_AHB_CLK:
	case MXC_IPG_CLK:
		return mx28_get_hclk() * 1000000;
	}

	return 0;
}

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	icache_enable();
	dcache_enable();

	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("Freescale i.MX28 family\n");
	printf("CPU:   %d MHz\n", mx28_get_pclk());
	printf("BUS:   %d MHz\n", mx28_get_hclk());
	printf("EMI:   %d MHz\n", mx28_get_emiclk());
	printf("GPMI:   %d MHz\n", mx28_get_gpmiclk());
	return 0;
}
#endif

/*
 * Initializes on-chip ethernet controllers.
 */
int cpu_eth_init(bd_t *bis)
{
	int rc = ENODEV;
#if defined(CONFIG_MXC_FEC)
	/* Turn on ENET clocks */
	REG_WR(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET,
		REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET) &
		~(BM_CLKCTRL_ENET_SLEEP | BM_CLKCTRL_ENET_DISABLE));

	/* Set up ENET PLL for 50 MHz */
	REG_SET(REGS_CLKCTRL_BASE, HW_CLKCTRL_PLL2CTRL0,
		BM_CLKCTRL_PLL2CTRL0_POWER);    /* Power on ENET PLL */
	udelay(10);                             /* Wait 10 us */
	REG_CLR(REGS_CLKCTRL_BASE, HW_CLKCTRL_PLL2CTRL0,
		BM_CLKCTRL_PLL2CTRL0_CLKGATE);  /* Gate on ENET PLL */
	REG_WR(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET,
		REG_RD(REGS_CLKCTRL_BASE, HW_CLKCTRL_ENET) |
		BM_CLKCTRL_ENET_CLK_OUT_EN);    /* Enable pad output */

	/* Board level init */
	enet_board_init();

	rc = mxc_fec_initialize(bis);
#endif
	return rc;
}

#define MX28_BLOCK_SFTRST	(1 << 31)
#define MX28_BLOCK_CLKGATE	(1 << 30)
#define RESET_MAX_TIMEOUT	1000000

int mx28_wait_mask_set(struct mx28_register *reg, uint32_t mask, int timeout)
{
	while (--timeout) {
		if ((readl(&reg->reg) & mask) == mask)
			break;
		udelay(1);
	}

	return !timeout;
}

int mx28_wait_mask_clr(struct mx28_register *reg, uint32_t mask, int timeout)
{
	while (--timeout) {
		if ((readl(&reg->reg) & mask) == 0)
			break;
		udelay(1);
	}

	return !timeout;
}

int mx28_reset_block(struct mx28_register *reg)
{
	/* Clear SFTRST */
	writel(MX28_BLOCK_SFTRST, &reg->reg_clr);

	if (mx28_wait_mask_clr(reg, MX28_BLOCK_SFTRST, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear CLKGATE */
	writel(MX28_BLOCK_CLKGATE, &reg->reg_clr);

	/* Set SFTRST */
	writel(MX28_BLOCK_SFTRST, &reg->reg_set);

	/* Wait for CLKGATE being set */
	if (mx28_wait_mask_set(reg, MX28_BLOCK_CLKGATE, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear SFTRST */
	writel(MX28_BLOCK_SFTRST, &reg->reg_clr);

	if (mx28_wait_mask_clr(reg, MX28_BLOCK_SFTRST, RESET_MAX_TIMEOUT))
		return 1;

	/* Clear CLKGATE */
	writel(MX28_BLOCK_CLKGATE, &reg->reg_clr);

	if (mx28_wait_mask_clr(reg, MX28_BLOCK_CLKGATE, RESET_MAX_TIMEOUT))
		return 1;

	return 0;
}

int mx28_get_bootmode(void)
{
	/* The global boot mode will be detected by ROM code and
	 * a boot mode value will be stored at fixed address depending
	 * on the revision of the CPU:
	 * TO1.0 addr 0x0001a7f0
	 * TO1.2 addr 0x00019BF0
	 */
	return(REG_RD_ADDR(GLOBAL_BOOT_MODE_ADDR) & 0x1f);
}
