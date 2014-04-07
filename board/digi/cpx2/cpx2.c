/*
 * Digi ConnectPort X2
 *
 * Copyright (C) 2013 Digi International Inc.
 *
 * Based on ccardimx28.c:
 * Copyright (C) 2013 Digi International
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
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/mii.h>
#include <miiphy.h>
#include <netdev.h>
#include <errno.h>
#include "../common/cmd_nvram/lib/include/nvram.h"

DECLARE_GLOBAL_DATA_PTR;

extern unsigned int get_total_ram(void);

/*
 * Functions
 */
int board_early_init_f(void)
{
	/* IO0 clock at 480MHz */
	mx28_set_ioclk(MXC_IOCLK0, 480000);
	/* IO1 clock at 480MHz */
	mx28_set_ioclk(MXC_IOCLK1, 480000);

	/* SSP0 clock at 96MHz */
	mx28_set_sspclk(MXC_SSPCLK0, 96000, 0);

#ifdef	CONFIG_CMD_USB
	mxs_iomux_setup_pad(MX28_PAD_SSP2_SS1__USB1_OVERCURRENT);
	mxs_iomux_setup_pad(MX28_PAD_AUART2_RX__GPIO_3_8 |
			MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL);
	gpio_direction_output(MX28_PAD_AUART2_RX__GPIO_3_8, 1);
#endif

	return 0;
}

int dram_init(void)
{
	return mxs_dram_init();
}

int board_init(void)
{
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

#ifdef	CONFIG_CMD_MMC

int board_mmc_init(bd_t *bis)
{
	return mxsmmc_initialize(bis, 0, NULL);
}
#endif

#ifdef	CONFIG_CMD_NET

#define KSZ80x1_OPMODE_STRAPOV		0x16

#define MII_PHY_CTRL2			0x1f
#define HP_AUTO_MDI			(1 << 15)
#define ENABLE_JABBER_COUNTER		(1 << 8)
#define RMII_50MHZ_CLOCK		(1 << 7)

int fecmxc_mii_postcall(int phy)
{
	unsigned short val;

	if (phy == 0)
		miiphy_write("FEC0", phy, MII_PHY_CTRL2,
			     HP_AUTO_MDI | ENABLE_JABBER_COUNTER |
			     RMII_50MHZ_CLOCK);
	if (phy == 3) {
		miiphy_write("FEC1", phy, MII_PHY_CTRL2,
			     HP_AUTO_MDI | ENABLE_JABBER_COUNTER |
			     RMII_50MHZ_CLOCK);
		/*
		 * Set bit 9 of register 0x16 (undocumented) to work
		 * around Micrel PHY bug that causes the second PHY, with
		 * address=3, to also respond to reads/writes addressed
		 * to the first PHY, which has address=0.
		 * The setting of this bit for platforms having only
		 * one PHY at address 0 is harmless.
		 */
		if (!miiphy_read("FEC1", phy, KSZ80x1_OPMODE_STRAPOV, &val))
			miiphy_write("FEC1", phy, KSZ80x1_OPMODE_STRAPOV,
				     (1 << 9));
	}
	return 0;
}

int board_eth_init(bd_t *bis)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;
	struct eth_device *dev;
	int ret;

	ret = cpu_eth_init(bis);

	/* cpx2 uses ENET_CLK PAD to drive FEC clock */
	writel(CLKCTRL_ENET_TIME_SEL_RMII_CLK | CLKCTRL_ENET_CLK_OUT_EN,
					&clkctrl_regs->hw_clkctrl_enet);

	/* Reset FEC PHYs */
	gpio_direction_output(MX28_PAD_PWM4__GPIO_3_29, 0);
	udelay(10300);  // from datasheet: tvr + tsr
	gpio_set_value(MX28_PAD_PWM4__GPIO_3_29, 1);
	/* Ensure recommended delay of 100us in PHY datasheet is met before
	 * accessing the MDIO interface */
	udelay(100);

	ret = fecmxc_initialize_multi(bis, 0, 0, MXS_ENET0_BASE);
	if (ret) {
		puts("FEC MXS: Unable to init FEC0\n");
		return ret;
	}

	ret = fecmxc_initialize_multi(bis, 1, 3, MXS_ENET1_BASE);
	if (ret) {
		puts("FEC MXS: Unable to init FEC1\n");
		return ret;
	}

	dev = eth_get_dev_by_name("FEC0");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC0 device entry\n");
		return -EINVAL;
	}
	ret = fecmxc_register_mii_postcall(dev, fecmxc_mii_postcall);
	if (ret) {
		printf("FEC MXS: Unable to register FEC0 mii postcall\n");
		return ret;
	}

#ifndef CONFIG_FEC1_INIT_ONLY_MAC
	dev = eth_get_dev_by_name("FEC1");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC1 device entry\n");
		return -EINVAL;
	}

	ret = fecmxc_register_mii_postcall(dev, fecmxc_mii_postcall);
	if (ret) {
		printf("FEC MXS: Unable to register FEC1 mii postcall\n");
		return ret;
	}
#endif

	return ret;
}

void mx28_adjust_mac(int dev_id, unsigned char *mac)
{
	nv_critical_t *pNvram;

	/* Get MAC from Digi NVRAM */
	if (!NvCriticalGet(&pNvram))
		return;

	if (0 == dev_id) {
		/* 1st wired MAC (ENET0) */
		memcpy(mac, &pNvram->s.p.xID.axMAC[0], 6);
	}
	else if (1 == dev_id) {
		/* 2nd wired MAC (ENET1) corresponds to ethaddr3
		 * because ethaddr2 is for the wireless.
		 * ethaddr3 is a field out of xID which originally
		 * only had space for two interfaces (wired and wireless)
		 */
		memcpy(mac, &pNvram->s.p.eth1addr, 6);
	}

	return;
}
#endif /* CONFIG_CMD_NET */

/*
 * This function returns the size of available RAM for doing a TFTP transfer.
 * This size depends on:
 *   - The total RAM available
 *   - The loadaddr
 * U-Boot is assumed to be loaded at a very low address (below loadaddr) so
 * it is not a variable in this calculation
 */
unsigned int get_tftp_available_ram(unsigned int loadaddr)
{
	return loadaddr - gd->bd->bi_dram[0].size;
}
