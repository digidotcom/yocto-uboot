/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2013 Digi International, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 * Author: Jason Liu <r64343@freescale.com>
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
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#if CONFIG_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif
#include <mmc.h>
#include <fsl_esdhc.h>
#include <fuse.h>
#include <miiphy.h>
#include <otf_update.h>
#include <part.h>
#ifdef CONFIG_OF_LIBFDT
#include <fdt_support.h>
#endif
#ifdef CONFIG_PLATFORM_HAS_HWID
#include "../common/hwid.h"
#endif
#include "../ccimx6/ccimx6.h"
#include "../../../drivers/net/fec_mxc.h"

DECLARE_GLOBAL_DATA_PTR;

struct ccimx6_hwid my_hwid;
static u8 hwid[4 * CONFIG_HWID_WORDS_NUMBER];
static block_dev_desc_t *mmc_dev;
static int enet_xcv_type;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |               \
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED   |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL	(PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#if CONFIG_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C2 Camera, MIPI, pfuze */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX6_PAD_KEY_COL3__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_KEY_COL3__GPIO_4_12 | PC,
		.gp = IMX_GPIO_NR(4, 12)
	},
	.sda = {
		.i2c_mode = MX6_PAD_KEY_ROW3__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_KEY_ROW3__GPIO_4_13 | PC,
		.gp = IMX_GPIO_NR(4, 13)
	}
};
#ifdef CONFIG_I2C_MULTI_BUS
struct i2c_pads_info i2c_pad_info2 = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO_3__I2C3_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO_3__GPIO_1_3| PC,
		.gp = IMX_GPIO_NR(1, 3)
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO_6__I2C3_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO_6__GPIO_1_6 | PC,
		.gp = IMX_GPIO_NR(1, 6)
	}
};
#endif
#endif

int dram_init(void)
{
	gd->ram_size = ((ulong)CONFIG_DDR_MB * 1024 * 1024);

	return 0;
}

iomux_v3_cfg_t const enet_pads_100[] = {
	MX6_PAD_ENET_MDIO__ENET_MDIO		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_MDC__ENET_MDC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_TXD0__ENET_TDATA_0		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_TXD1__ENET_TDATA_1		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_RXD0__ENET_RDATA_0		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_RXD1__ENET_RDATA_1		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TX_CTL__ENET_REF_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_RX_ER__ENET_RX_ER		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_TX_EN__ENET_TX_EN		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_CRS_DV__ENET_RX_EN		| MUX_PAD_CTRL(ENET_PAD_CTRL),
};

iomux_v3_cfg_t const enet_pads_1000[] = {
	MX6_PAD_ENET_MDIO__ENET_MDIO		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_MDC__ENET_MDC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TXC__ENET_RGMII_TXC	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD0__ENET_RGMII_TD0	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD1__ENET_RGMII_TD1	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD2__ENET_RGMII_TD2	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD3__ENET_RGMII_TD3	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RXC__ENET_RGMII_RXC	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD0__ENET_RGMII_RD0	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD1__ENET_RGMII_RD1	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD2__ENET_RGMII_RD2	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD3__ENET_RGMII_RD3	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL),
};

void setup_iomux_enet(void)
{
	int enet;

	/* iomux for Gigabit or 10/100 and PHY selection
	 * basing on env variable 'ENET'. Default to Gigabit.
	 */
	enet = (int)getenv_ulong("ENET", 10, 1000);
	if (enet == 100) {
		/* 10/100 ENET */
		enet_xcv_type = RMII;
		imx_iomux_v3_setup_multiple_pads(enet_pads_100,
						 ARRAY_SIZE(enet_pads_100));
	} else {
		/* Gigabit ENET */
		enet_xcv_type = RGMII;
		imx_iomux_v3_setup_multiple_pads(enet_pads_1000,
						 ARRAY_SIZE(enet_pads_1000));
	}
}

int board_get_enet_xcv_type(void)
{
	return enet_xcv_type;
}

#ifdef CONFIG_I2C_MXC

static int pmic_access_page(unsigned char page)
{
	if (i2c_write(CONFIG_PMIC_I2C_ADDR, DA9063_PAGE_CON, 1, &page, 1)) {
		printf("Cannot set PMIC page!\n");
		return -1;
	}

	return 0;
}

int pmic_read_reg(int reg, unsigned char *value)
{
	unsigned char page = reg / 0x80;

	if (pmic_access_page(page))
		return -1;

	if (i2c_read(CONFIG_PMIC_I2C_ADDR, reg, 1, value, 1))
		return -1;

	/* return to page 0 by default */
	pmic_access_page(0);
	return 0;
}

int pmic_write_reg(int reg, unsigned char value)
{
	unsigned char page = reg / 0x80;

	if (pmic_access_page(page))
		return -1;

	if (i2c_write(CONFIG_PMIC_I2C_ADDR, reg, 1, &value, 1))
		return -1;

	/* return to page 0 by default */
	pmic_access_page(0);
	return 0;
}

int pmic_write_bitfield(int reg, unsigned char mask, unsigned char off,
			       unsigned char bfval)
{
	unsigned char value;

	if (pmic_read_reg(reg, &value) == 0) {
		value &= ~(mask << off);
		value |= (bfval << off);
		return pmic_write_reg(reg, value);
	}

	return -1;
}
#endif /* CONFIG_I2C_MXC */

#ifdef CONFIG_FSL_ESDHC

/* The order of MMC controllers here must match that of CONFIG_MMCDEV_USDHCx
 * in the platform header
 */
struct fsl_esdhc_cfg usdhc_cfg[CONFIG_SYS_FSL_USDHC_NUM] = {
	{USDHC4_BASE_ADDR},
	{USDHC2_BASE_ADDR},
};

iomux_v3_cfg_t const usdhc4_pads[] = {
	MX6_PAD_SD4_CLK__USDHC4_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_CMD__USDHC4_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT0__USDHC4_DAT0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT1__USDHC4_DAT1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT2__USDHC4_DAT2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT3__USDHC4_DAT3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT4__USDHC4_DAT4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT5__USDHC4_DAT5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT6__USDHC4_DAT6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT7__USDHC4_DAT7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_SD2_CLK__USDHC2_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__USDHC2_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT0__USDHC2_DAT0	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT1__USDHC2_DAT1	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT2__USDHC2_DAT2	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT3__USDHC2_DAT3	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

int mmc_get_env_devno(void)
{
	u32 soc_sbmr = readl(SRC_BASE_ADDR + 0x4);

	/* BOOT_CFG2[4] and BOOT_CFG2[3] denote boot media:
	 * 01:	SDHC2 (uSD card)
	 * 11:	SDHC4 (eMMC)
	 */
	switch((soc_sbmr & 0x00001800) >> 11) {
	case 1:
		return CONFIG_MMCDEV_USDHC2;	/* SDHC2 (uSD) */
	case 3:
		return CONFIG_MMCDEV_USDHC4;	/* SDHC4 (eMMC) */
	}

	return -1;
}

int mmc_get_env_partno(void)
{
	u32 soc_sbmr = readl(SRC_BASE_ADDR + 0x4);

	/* BOOT_CFG2[4] and BOOT_CFG2[3] denote boot media:
	 * 01:	SDHC2 (uSD card)
	 * 11:	SDHC4 (eMMC)
	 */
	switch((soc_sbmr & 0x00001800) >> 11) {
	case 1:
		return 0;	/* When booting from SDHC2 (uSD) the
				 * environment will be saved to the unique
				 * hardware partition: 0 */
	case 3:
		return 2;	/* When booting from SDHC4 (eMMC) the
				 * environment will be saved to boot
				 * partition 2 to protect it from
				 * accidental overwrite during U-Boot update */
	}

	return -1;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC2_BASE_ADDR:
		ret = 1; /* uSD/uSDHC2 does not connect CD. Assume present */
		break;
	case USDHC4_BASE_ADDR:
		ret = 1; /* eMMC/uSDHC4 is always present */
		break;
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
	int i;

	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			/* USDHC4 (eMMC) */
			imx_iomux_v3_setup_multiple_pads(
					usdhc4_pads, ARRAY_SIZE(usdhc4_pads));
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
			break;
		case 1:
			/* USDHC2 (uSD) */
			imx_iomux_v3_setup_multiple_pads(
					usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}

		if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
			printf("Warning: failed to initialize mmc dev %d\n", i);
	}

	mmc_dev = mmc_get_dev(CONFIG_SYS_STORAGE_DEV);
	if (NULL == mmc_dev)
		printf("Warning: failed to get sys storage device\n");

	return 0;
}
#endif /* CONFIG_FSL_ESDHC */

#ifdef CONFIG_CMD_SATA
int setup_sata(void)
{
	struct iomuxc_base_regs *const iomuxc_regs
		= (struct iomuxc_base_regs *) IOMUXC_BASE_ADDR;
	int ret = enable_sata_clock();
	if (ret)
		return ret;

	clrsetbits_le32(&iomuxc_regs->gpr[13],
			IOMUXC_GPR13_SATA_MASK,
			IOMUXC_GPR13_SATA_PHY_8_RXEQ_3P0DB
			|IOMUXC_GPR13_SATA_PHY_7_SATA2M
			|IOMUXC_GPR13_SATA_SPEED_3G
			|(3<<IOMUXC_GPR13_SATA_PHY_6_SHIFT)
			|IOMUXC_GPR13_SATA_SATA_PHY_5_SS_DISABLED
			|IOMUXC_GPR13_SATA_SATA_PHY_4_ATTEN_9_16
			|IOMUXC_GPR13_SATA_PHY_3_TXBOOST_0P00_DB
			|IOMUXC_GPR13_SATA_PHY_2_TX_1P104V
			|IOMUXC_GPR13_SATA_PHY_1_SLOW);

	return 0;
}
#endif /* CONFIG_CMD_SATA */

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd2",	 MAKE_CFGVAL(0x40, 0x28, 0x00, 0x00)},
	{"sd3",	 MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	/* 8 bit bus width */
	{"emmc", MAKE_CFGVAL(0x40, 0x38, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

int ccimx6_late_init(void)
{
	int ret = 0;
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_I2C_MXC
	/* Setup I2C2 (PMIC, Kinetis) */
	setup_i2c(1, CONFIG_SYS_I2C_SPEED,
			CONFIG_SYS_I2C_SLAVE, &i2c_pad_info1);
#ifdef CONFIG_I2C_MULTI_BUS
	/* Setup I2C3 (HDMI, Audio...) */
	setup_i2c(2, CONFIG_SYS_I2C_SPEED,
			CONFIG_SYS_I2C_SLAVE, &i2c_pad_info2);
#endif
	ret = setup_pmic_voltages();
	if (ret)
		return -1;
#endif

#ifdef CONFIG_CMD_MMC
	/* If undefined, determine 'mmcdev' variable depending on boot media */
	if (NULL == getenv("mmcdev")) {
		u32 soc_sbmr = readl(SRC_BASE_ADDR + 0x4);

		switch((soc_sbmr & 0x00001800) >> 11) {
		case 1:
			setenv_ulong("mmcdev", 1);	/* SDHC2 (uSD) */
			break;
		case 3:
			setenv_ulong("mmcdev", 0);	/* SDHC4 (eMMC) */
			break;
		}
	}
#endif

	return 0;
}

#ifdef CONFIG_PLATFORM_HAS_HWID
void board_print_hwid(u32 *hwid)
{
	int i;

	for (i = CONFIG_HWID_WORDS_NUMBER - 1; i >= 0; i--)
		printf(" %.8x", hwid[i]);
	printf("\n");
	/* Formatted printout */
	printf("    TF (location): 0x%02x\n", (hwid[1] >> 16) & 0xf);
	printf("    Variant:       0x%02x\n", (hwid[1] >> 8) & 0xff);
	printf("    HW Version:    %d\n", (hwid[1] >> 4) & 0xf);
	printf("    Cert:          0x%x\n", hwid[1] & 0xf);
	printf("    Year:          20%02d\n", (hwid[0] >> 24) & 0xff);
	printf("    Month:         %02d\n", (hwid[0] >> 20) & 0xf);
	printf("    S/N:           %d\n", hwid[0] & 0xfffff);
}

static int is_valid_hwid(u8 variant)
{
	if (variant < ARRAY_SIZE(ccimx6_variants))
		if (ccimx6_variants[variant].cpu != IMX6_NONE)
			return 1;

	return 0;
}

int array_to_hwid(u8 *hwid)
{
	/*
	 *       | 31..                  MAC1            ..0 | 31..          MAC0                    ..0 |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * HWID: |       --      | TF  | variant  | HV |Cert |   Year   | Mon |     Serial Number        |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * Byte:            7          6          5          4          3          2          1          0
	 */

	if (!is_valid_hwid(hwid[5]))
		return -EINVAL;

	my_hwid.tf = hwid[6] & 0xf;
	my_hwid.variant = hwid[5];
	my_hwid.hv = (hwid[4] & 0xf0) >> 4;
	my_hwid.cert = hwid[4] & 0xf;
	my_hwid.year = hwid[3];
	my_hwid.month = (hwid[2] & 0xf0) >> 4;
	my_hwid.sn = ((hwid[2] & 0xf) << 16) | (hwid[1] << 8) | hwid[0];

	return  0;
}

int get_hwid(void)
{
	u32 bank = CONFIG_HWID_BANK;
	u32 word = CONFIG_HWID_START_WORD;
	u32 cnt = CONFIG_HWID_WORDS_NUMBER;
	u32 *val = (u32 *)hwid;
	int ret, i;

	for (i = 0; i < cnt; i++, word++) {
		ret = fuse_read(bank, word, &val[i]);
		if (ret)
			return -1;
	}

	return 0;
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
			sprintf(str, "0x%02x", my_hwid.tf);
		else if (!strcmp("digi,hwid,variant", propnames[i]))
			sprintf(str, "0x%02x", my_hwid.variant);
		else if (!strcmp("digi,hwid,hv", propnames[i]))
			sprintf(str, "0x%x", my_hwid.hv);
		else if (!strcmp("digi,hwid,cert", propnames[i]))
			sprintf(str, "0x%x", my_hwid.cert);
		else if (!strcmp("digi,hwid,year", propnames[i]))
			sprintf(str, "20%02d", my_hwid.year);
		else if (!strcmp("digi,hwid,month", propnames[i]))
			sprintf(str, "%02d", my_hwid.month);
		else if (!strcmp("digi,hwid,sn", propnames[i]))
			sprintf(str, "%d", my_hwid.sn);
		else
			continue;

		do_fixup_by_path(fdt, "/", propnames[i], str,
				 strlen(str) + 1, 1);
	}
}
#endif /* CONFIG_PLATFORM_HAS_HWID */

int checkboard(void)
{
	const char *bootdevice;

	printf("Board:   %s\n", CONFIG_BOARD_DESCRIPTION);
#ifdef CONFIG_PLATFORM_HAS_HWID
	if (!array_to_hwid(hwid))
		printf("Variant: 0x%02x - %s\n", my_hwid.variant,
			ccimx6_variants[my_hwid.variant].id_string);
#endif /* CONFIG_PLATFORM_HAS_HWID */
	bootdevice = boot_mode_string();
	printf("Boot device: %s", bootdevice);
	if (!strcmp(bootdevice, "esdhc2"))
		printf(" (uSD card)\n");
	else if (!strcmp(bootdevice, "esdhc4"))
		printf(" (eMMC)\n");
	else
		printf("\n");
	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP)
void fdt_fixup_mac(void *fdt, char *varname, char *node)
{
	char *tmp, *end;
	unsigned char mac_addr[6];
	int i;

	if ((tmp = getenv(varname)) != NULL) {
		for (i = 0; i < 6; i++) {
			mac_addr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
			if (tmp)
				tmp = (*end) ? end+1 : end;
		}
		do_fixup_by_path(fdt, node, "mac-address", &mac_addr, 6, 1);
	}
}

/* Platform function to modify the FDT as needed */
void ft_board_setup(void *blob, bd_t *bd)
{

#ifdef CONFIG_PLATFORM_HAS_HWID
	/* Re-read HWID which could have been overriden by U-Boot commands */
	get_hwid();
	if (!array_to_hwid(hwid))
		fdt_fixup_hwid(blob);
#endif /* CONFIG_PLATFORM_HAS_HWID */

	fdt_fixup_mac(blob, "wlanaddr", "/wireless");
	fdt_fixup_mac(blob, "btaddr", "/bluetooth");
}
#endif /* CONFIG_OF_BOARD_SETUP */

static int write_chunk(struct mmc *mmc, otf_data_t *otfd, unsigned int dstblk,
			unsigned int chunklen)
{
	int sectors;
	unsigned long written, read, verifyaddr;

	printf("\nWriting chunk...\n");
	/* Check WP */
	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	/* We can only write whole sectors (multiples of mmc_dev->blksz bytes)
	 * so we need to check if the chunk is a whole multiple or else add 1
	 */
	sectors = chunklen / mmc_dev->blksz;
	if (chunklen % mmc_dev->blksz)
		sectors++;

	/* Check if chunk fits */
	if (sectors + dstblk > otfd->part->start + otfd->part->size) {
		printf("Error: length of data exceeds partition size\n");
		return -1;
	}

	/* Write chunklen bytes of chunk to media */
	debug("writing chunk of 0x%x bytes (0x%x sectors) "
		"from 0x%x to block 0x%x\n",
		chunklen, sectors, otfd->loadaddr, dstblk);
	written = mmc->block_dev.block_write(CONFIG_SYS_STORAGE_DEV, dstblk,
					     sectors,
					     (const void *)otfd->loadaddr);
	if (written != sectors)
		return -1;

	/* Verify written chunk if $loadaddr + chunk size does not overlap
	 * $verifyaddr (where the read-back copy will be placed)
	 */
	verifyaddr = getenv_ulong("verifyaddr", 16, CONFIG_VERIFYADDR);
	if (otfd->loadaddr + sectors * mmc_dev->blksz < verifyaddr) {
		/* Read back data... */
		printf("Reading back chunk...\n");
		read = mmc->block_dev.block_read(CONFIG_SYS_STORAGE_DEV, dstblk,
						 sectors, (void *)verifyaddr);
		if (read != sectors)
			return -1;
		/* ...then compare */
		printf("Verifying chunk...\n");
		return memcmp((const void *)otfd->loadaddr,
			      (const void *)verifyaddr,
			      sectors * mmc_dev->blksz);
	} else {
		printf("Cannot verify chunk. It overlaps $verifyaddr!\n");
		return 0;
	}
}

/* writes a chunk of data from RAM to main storage media (eMMC) */
int board_update_chunk(otf_data_t *otfd)
{
	static unsigned int chunk_len = 0;
	static unsigned int dstblk = 0;
	struct mmc *mmc = find_mmc_device(CONFIG_SYS_STORAGE_DEV);

	/* Initialize dstblk and local variables */
	if (otfd->flags & OTF_FLAG_INIT) {
		chunk_len = 0;
		dstblk = otfd->part->start;
		otfd->flags &= ~OTF_FLAG_INIT;
	}

	/* The flush flag is set when the download process has finished
	 * meaning we must write the remaining bytes in RAM to the storage
	 * media. After this, we must quit the function. */
	if (otfd->flags & OTF_FLAG_FLUSH) {
		/* Write chunk with remaining bytes */
		if (chunk_len) {
			if (write_chunk(mmc, otfd, dstblk, chunk_len))
				return -1;
		}
		/* Reset all static variables if offset == 0 (starting chunk) */
		chunk_len = 0;
		dstblk = 0;
		return 0;
	}

	/* Buffer otfd in RAM until we reach the configured limit to write it
	 * to media
	 */
	memcpy((void *)(otfd->loadaddr + otfd->offset), otfd->buf, otfd->len);
	chunk_len += otfd->len;
	if (chunk_len >= CONFIG_OTF_CHUNK) {
		unsigned int remaining;
		/* We have CONFIG_OTF_CHUNK (or more) bytes in RAM.
		 * Let's proceed to write as many as multiples of blksz
		 * as possible.
		 */
		remaining = chunk_len % mmc_dev->blksz;
		chunk_len -= remaining;	/* chunk_len is now multiple of blksz */

		if (write_chunk(mmc, otfd, dstblk, chunk_len))
			return -1;

		/* increment destiny block */
		dstblk += (chunk_len / mmc_dev->blksz);
		/* copy excess of bytes from previous chunk to offset 0 */
		if (remaining) {
			memcpy((void *)otfd->loadaddr,
			       (void *)(otfd->loadaddr + chunk_len),
			       remaining);
			debug("Copying excess of %d bytes to offset 0\n",
			      remaining);
		}
		/* reset chunk_len to excess of bytes from previous chunk
		 * (or zero, if that's the case) */
		chunk_len = remaining;
	}
	/* Set otfd offset pointer to offset in RAM where new bytes would
	 * be written. This offset may be reused by caller */
	otfd->offset = chunk_len;

	return 0;
}
