/*
 * Digi ConnectCard for i.MX28 board
 *
 * Copyright (C) 2013 Digi International Inc.
 *
 * Based on mx28evk.c:
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
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/mxs_otp.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/mii.h>
#include <miiphy.h>
#include <netdev.h>
#include <errno.h>
#include "../common/cmd_nvram/lib/include/nvram.h"
#include "board-ccardimx28.h"

#if defined(CONFIG_OF_LIBFDT)
#include <fdt.h>
#include <libfdt.h>
#include <fdt_support.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

struct ccardimx28_hwid mod_hwid;
static u8 hwid[CONFIG_HWID_LENGTH];

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
	/* SSP2 clock at 160MHz */
	mx28_set_sspclk(MXC_SSPCLK2, 160000, 0);

#ifdef	CONFIG_CMD_USB
	mxs_iomux_setup_pad(MX28_PAD_SSP2_SS1__USB1_OVERCURRENT);
	mxs_iomux_setup_pad(MX28_PAD_AUART2_RX__GPIO_3_8 |
			MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL);
	gpio_direction_output(MX28_PAD_AUART2_RX__GPIO_3_8, 1);
#endif

	return 0;
}

#ifdef CONFIG_USER_KEY
static void setup_user_keys(void)
{
	/* Configure iomux */
	mxs_iomux_setup_pad(USER_KEY1_GPIO | MXS_PAD_4MA | MXS_PAD_3V3 |
			    MXS_PAD_PULLUP);
	mxs_iomux_setup_pad(USER_KEY2_GPIO | MXS_PAD_4MA | MXS_PAD_3V3 |
			    MXS_PAD_PULLUP);
	/* Configure direction as input */
	gpio_direction_input(USER_KEY1_GPIO);
	gpio_direction_input(USER_KEY2_GPIO);
}

static void run_user_keys(void)
{
	char cmd[60];

	if ((gpio_get_value(USER_KEY1_GPIO) == 0) &&
	    (gpio_get_value(USER_KEY2_GPIO) == 0)) {
		printf("\nUser Key 1 and 2 pressed\n");
		if (getenv("key12") != NULL) {
			sprintf(cmd, "run key12");
			run_command(cmd, 0);
			return;
		}
	}

	if (gpio_get_value(USER_KEY1_GPIO) == 0) {
		printf("\nUser Key 1 pressed\n");
		if (getenv("key1") != NULL) {
			sprintf(cmd, "run key1");
			run_command(cmd, 0);
		}
	}

	if (gpio_get_value(USER_KEY2_GPIO) == 0) {
		printf("\nUser Key 2 pressed\n");
		if (getenv("key2") != NULL) {
			sprintf(cmd, "run key2");
			run_command(cmd, 0);
		}
	}
}
#endif /* CONFIG_USER_KEY */

int dram_init(void)
{
	return mxs_dram_init();
}

int board_init(void)
{
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#ifdef CONFIG_USER_KEY
	setup_user_keys();
#endif

	return 0;
}

#ifdef CONFIG_BOARD_BEFORE_MLOOP_INIT
int board_before_mloop_init (void)
{
#ifdef CONFIG_USER_KEY
	run_user_keys();
#endif
	return 0;
}
#endif /* CONFIG_BOARD_BEFORE_MLOOP_INIT */

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

	/* ccardimx28 uses ENET_CLK PAD to drive FEC clock */
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

#endif

#if defined(CONFIG_PLATFORM_HAS_HWID)
static int is_valid_hw_id(u8 variant)
{
	if (variant < ARRAY_SIZE(ccardimx28_id))
		if (ccardimx28_id[variant].sdram != 0)
			return 1;

	return 0;
}

int array_to_hwid(u8 *hwid)
{
	/*
	 *       | 31..           HWOTP_CUST1            ..0 | 31..          HWOTP_CUST0             ..0 |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * HWID: |    --    | TF (loc) | variant  | HV |Cert |   Year   | Mon |     Serial Number        |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * Byte: 0          1          2          3          4          5          6          7
	 */
	if (!is_valid_hw_id(hwid[2]))
		return -EINVAL;

	mod_hwid.tf = hwid[1];
	mod_hwid.variant = hwid[2];
	mod_hwid.hv = (hwid[3] & 0xf0) >> 4;
	mod_hwid.cert = hwid[3] & 0xf;
	mod_hwid.year = hwid[4];
	mod_hwid.month = (hwid[5] & 0xf0) >> 4;
	mod_hwid.sn = ((hwid[5] & 0xf) << 16) | (hwid[6] << 8) | hwid[7];

	return  0;
}

int get_hwid(void)
{
	nv_critical_t *pNVRAM;

	memset(&mod_hwid, 0, sizeof(struct ccardimx28_hwid));
	memset(hwid, 0, ARRAY_SIZE(hwid));

	/* nvram settings override fuse hwid */
	if (NvCriticalGet(&pNVRAM)) {
		const nv_param_module_id_t *pModule = &pNVRAM->s.p.xID;

		if (!array_to_hwid((u8 *)&pModule->szProductType)) {
			/* Set global hwid with value in NVRAM */
			memcpy(hwid, (u8 *)&pModule->szProductType, CONFIG_HWID_LENGTH);
			return 0;
		}
	}

	/* Use fuses if no valid hwid was set in the nvram */
	if (!read_otp_hwid(hwid))
		array_to_hwid(hwid);

	/* returning something != 0 will halt the boot process */
	return 0;
}

void NvPrintHwID(void)
{
	printf("    TF (location): 0x%02x\n", mod_hwid.tf);
	printf("    Variant:       0x%02x\n", mod_hwid.variant);
	printf("    HW Version:    %d\n", mod_hwid.hv);
	printf("    Cert:          0x%x\n", mod_hwid.cert);
	printf("    Year:          20%02d\n", mod_hwid.year);
	printf("    Month:         %02d\n", mod_hwid.month);
	printf("    S/N:           %d\n", mod_hwid.sn);
}

int variant_has_wireless(void)
{
	return ccardimx28_id[mod_hwid.variant].has_wireless;
}

void fdt_fixup_hwid(void *fdt)
{
	const char *propnames[] = {
		"digi,hwid,tf",
		"digi,hwid,variant",
		"digi,hwid,hv",
		"digi,hwid,cert",
		"digi,hwid,year",
		"digi,hwid,month",
		"digi,hwid,sn",
	};
	char str[20];
	int i;

	/* Register the HWID as main node properties in the FDT */
	for (i = 0; i < ARRAY_SIZE(propnames); i++) {
		/* Convert HWID fields to strings */
		if (!strcmp("digi,hwid,tf", propnames[i]))
			sprintf(str, "0x%02x", mod_hwid.tf);
		else if (!strcmp("digi,hwid,variant", propnames[i]))
			sprintf(str, "0x%02x", mod_hwid.variant);
		else if (!strcmp("digi,hwid,hv", propnames[i]))
			sprintf(str, "0x%x", mod_hwid.hv);
		else if (!strcmp("digi,hwid,cert", propnames[i]))
			sprintf(str, "0x%x", mod_hwid.cert);
		else if (!strcmp("digi,hwid,year", propnames[i]))
			sprintf(str, "20%02d", mod_hwid.year);
		else if (!strcmp("digi,hwid,month", propnames[i]))
			sprintf(str, "%02d", mod_hwid.month);
		else if (!strcmp("digi,hwid,sn", propnames[i]))
			sprintf(str, "%d", mod_hwid.sn);
		else
			continue;
		do_fixup_by_path(fdt, "/", propnames[i], str, strlen(str), 1);
	}
}
#endif /* CONFIG_PLATFORM_HAS_HWID */

void fdt_fixup_bluetooth_mac(void *fdt)
{
	char *tmp, *end;
	unsigned char mac_addr[6];
	int i;

	if ((tmp = getenv("btaddr")) != NULL) {
		for (i = 0; i < 6; i++) {
			mac_addr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
			if (tmp)
				tmp = (*end) ? end+1 : end;
		}
		do_fixup_by_path(fdt, "/bluetooth", "mac-address",
				 &mac_addr, 6, 1);
	}
}

void fdt_fixup_wireless_mac(void *fdt)
{
	char *tmp, *end;
	unsigned char mac_addr[6];
	int i;

	if ((tmp = getenv("wlanaddr")) != NULL) {
		for (i = 0; i < 6; i++) {
			mac_addr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
			if (tmp)
				tmp = (*end) ? end+1 : end;
		}
		do_fixup_by_path(fdt, "/wireless", "mac-address",
				 &mac_addr, 6, 1);
	}
}

#if defined(CONFIG_OF_BOARD_SETUP)
/*
 * Platform function to modify the FDT as needed
 */
void ft_board_setup(void *blob, bd_t *bd)
{
#if defined(CONFIG_PLATFORM_HAS_HWID)
	fdt_fixup_hwid(blob);
#endif /* CONFIG_PLATFORM_HAS_HWID */

	fdt_fixup_bluetooth_mac(blob);
	fdt_fixup_wireless_mac(blob);
}
#endif /* CONFIG_OF_BOARD_SETUP */

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	int fet_active = 1;	/* default polarity */

	/* On modules newer than V1, a FET is blocking the 3V3 supply.
	 * To have this voltage enabled to the module LCD_RS
	 * must be driven low on modules V2..V5 and driven high on modules V6.
	 * On V1 modules this line is not used so the code is harmless.
	 * On modules with blank OTP, a V6 is considered by default.
	 */
	if (mod_hwid.hv > 0 && mod_hwid.hv < 6)
		fet_active = 0;	/* active low */
	/* First set value to avoid glitches */
	gpio_direction_output(MX28_PAD_LCD_RS__GPIO_1_26, fet_active);
	mxs_iomux_setup_pad(MX28_PAD_LCD_RS__GPIO_1_26 |
			MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_PULLUP);

	return 0;
}
#endif /* CONFIG_BOARD_LATE_INIT */

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
