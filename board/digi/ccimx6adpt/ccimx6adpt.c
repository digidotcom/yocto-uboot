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
#include <netdev.h>
#include <otf_update.h>
#include <part.h>
#ifdef CONFIG_OF_LIBFDT
#include <fdt_support.h>
#endif
#ifdef CONFIG_PLATFORM_HAS_HWID
#include "../common/hwid.h"
#endif
#include "../../../drivers/net/fec_mxc.h"

DECLARE_GLOBAL_DATA_PTR;

struct ccimx6_hwid my_hwid;
static u8 hwid[4 * CONFIG_HWID_WORDS_NUMBER];
static block_dev_desc_t *mmc_dev;
static int enet_xcv_type;
static int phy_addr;

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

iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_SD3_DAT7__UART1_TXD | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_SD3_DAT6__UART1_RXD | MUX_PAD_CTRL(UART_PAD_CTRL),
};

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
	/* SMSC LAN8710 PHY reset */
	MX6_PAD_RGMII_RX_CTL__GPIO_6_24		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* SMSC LAN8710 PHY interrupt */
	MX6_PAD_ENET_REF_CLK__GPIO_1_23		| MUX_PAD_CTRL(NO_PAD_CTRL),
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
	/* Micrel KSZ9031 PHY reset */
	MX6_PAD_ENET_CRS_DV__GPIO_1_25		| MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_enet(void)
{
	int enet;
	int phy_reset_gpio;
	iomux_v3_cfg_t const *enet_pads;
	int npads;

	/* iomux for Gigabit or 10/100 and PHY selection
	 * basing on env variable 'ENET'. Default to Gigabit.
	 */
	enet = (int)getenv_ulong("ENET", 10, 1000);
	if (enet == 100) {
		/* 10/100 ENET (SMSC PHY) */
		phy_reset_gpio = IMX_GPIO_NR(6, 24);
		enet_pads = enet_pads_100;
		npads = ARRAY_SIZE(enet_pads_100);
		enet_xcv_type = RMII;
		phy_addr = CONFIG_ENET_PHYADDR_SMSC;
	} else {
		/* Gigabit ENET (Micrel PHY) */
		phy_reset_gpio = IMX_GPIO_NR(1, 25);
		enet_pads = enet_pads_1000;
		npads = ARRAY_SIZE(enet_pads_1000);
		enet_xcv_type = RGMII;
		phy_addr = CONFIG_ENET_PHYADDR_MICREL;
	}
	imx_iomux_v3_setup_multiple_pads(enet_pads, npads);

	/* Assert PHY reset */
	gpio_direction_output(phy_reset_gpio , 0);
	/* Need 10ms to guarantee stable voltages */
	udelay(10 * 1000);
	/* Deassert PHY reset */
	gpio_set_value(phy_reset_gpio, 1);
	/* Need to wait 100us before accessing the MIIM (MDC/MDIO) */
	udelay(100);
}

int board_get_enet_xcv_type(void)
{
	return enet_xcv_type;
}

int board_get_enet_phy_addr(void)
{
	return phy_addr;
}

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifdef CONFIG_I2C_MXC

/* DA9063 PMIC */
#define DA9063_PAGE_CON			0x0
#define DA9063_VLDO4_CONT_ADDR		0x29
#define DA9063_VLDO4_A_ADDR		0xac
#define DA9063_VLDO4_B_ADDR		0xbd
#define DA9063_CONFIG_D_ADDR		0x109
#define DA9063_DEVICE_ID_ADDR		0x181
#define DA9063_VARIANT_ID_ADDR		0x182
#define DA9063_CUSTOMER_ID_ADDR		0x183
#define DA9063_CONFIG_ID_ADDR		0x184

static int pmic_access_page(unsigned char page)
{
	if (i2c_write(CONFIG_PMIC_I2C_ADDR, DA9063_PAGE_CON, 1, &page, 1)) {
		printf("Cannot set PMIC page!\n");
		return -1;
	}

	return 0;
}

static int pmic_read_reg(int reg, unsigned char *value)
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

static int pmic_write_reg(int reg, unsigned char value)
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

static int pmic_write_bitfield(int reg, unsigned char mask, unsigned char off,
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

static int setup_pmic_voltages(void)
{
	unsigned char dev_id, var_id, conf_id, cust_id;
#ifdef CONFIG_I2C_MULTI_BUS
	int ret;

	ret = i2c_set_bus_num(0);
	if (ret)
                return -1;
#endif

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	if (!i2c_probe(CONFIG_PMIC_I2C_ADDR)) {
		/* Read and print PMIC identification */
		if (pmic_read_reg(DA9063_DEVICE_ID_ADDR, &dev_id) ||
		    pmic_read_reg(DA9063_VARIANT_ID_ADDR, &var_id) ||
		    pmic_read_reg(DA9063_CUSTOMER_ID_ADDR, &cust_id) ||
		    pmic_read_reg(DA9063_CONFIG_ID_ADDR, &conf_id)) {
			printf("Could not read PMIC ID registers\n");
			return -1;
		}
		printf("PMIC:  DA9063, Device: 0x%02x, Variant: 0x%02x, "
			"Customer: 0x%02x, Config: 0x%02x\n", dev_id, var_id,
			cust_id, conf_id);

#if defined(CONFIG_FEC_MXC)
		/* Both NVCC_ENET and NVCC_RGMII come from LDO4 (2.5V) */
		/* Config LDO4 voltages A and B at 2.5V, then enable VLDO4 */
		if (pmic_write_reg(DA9063_VLDO4_A_ADDR, 0x50) ||
		    pmic_write_reg(DA9063_VLDO4_B_ADDR, 0x50) ||
		    pmic_write_bitfield(DA9063_VLDO4_CONT_ADDR, 0x1, 0, 0x1))
			printf("Could not configure VLDO4\n");
#endif
		/* PWR_EN on the ccimx6adpt enables the +5V suppy and comes
		 * from GP_FB_2. Configure this as high level active by setting
		 * pin 6.
		 */
		if (pmic_write_bitfield(DA9063_CONFIG_D_ADDR, 0x1, 6, 0x0))
			printf("Could not enable PWR_EN\n");
	}
	return 0;
}
#endif


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
#endif

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
#endif

#if defined(CONFIG_PHY_MICREL)
int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif

int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_enet();

	ret = cpu_eth_init(bis);
	if (ret)
		printf("FEC MXC: %s:failed\n", __func__);

	return 0;
}

int board_early_init_f(void)
{
	setup_iomux_uart();

#ifdef CONFIG_CMD_SATA
	setup_sata();
#endif
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	return 0;
}

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

int board_late_init(void)
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
	printf("    Month:         %02d\n", (hwid[0] >> 16) & 0xff);
	printf("    S/N:           %d\n", hwid[0] & 0xffff);
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
	fdt_fixup_mac(blob, "wlanaddr", "/wireless");
	fdt_fixup_mac(blob, "btaddr", "/bluetooth");
}
#endif /* CONFIG_OF_BOARD_SETUP */

static int write_chunk(struct mmc *mmc, otf_data_t *otfd, unsigned int dstblk,
			unsigned int chunklen)
{
	int sectors;

	/* Check WP */
	if (mmc_getwp(mmc) == 1) {
		printf("Error: card is write protected!\n");
		return -1;
	}

	/* Sectors to write. If less bytes than one sector, make it one */
	sectors = chunklen / mmc_dev->blksz;
	if (sectors == 0)
		sectors = 1;

	/* Check if chunk fits */
	if (sectors + dstblk > otfd->part->start + otfd->part->size) {
		printf("Error: length of data exceeds partition size\n");
		return -1;
	}

	/* Write chunklen bytes of chunk to media */
	debug("writing chunk of %d bytes from 0x%x to block %d\n",
		chunklen, otfd->loadaddr, dstblk);
	return mmc->block_dev.block_write(CONFIG_SYS_STORAGE_DEV, dstblk,
					  chunklen / mmc_dev->blksz,
					  (const void *)otfd->loadaddr);
}

/* writes a chunk of data from RAM to main storage media (eMMC) */
int board_update_chunk(otf_data_t *otfd)
{
	static unsigned int chunk_len = 0;
	static unsigned int dstblk = 0;
	struct mmc *mmc = find_mmc_device(CONFIG_SYS_STORAGE_DEV);

	/* Initialize dstblk */
	if (dstblk == 0)
		dstblk = otfd->part->start;

	/* The flush flag is set when the download process has finished
	 * meaning we must write the remaining bytes in RAM to the storage
	 * media. After this, we must quit the function. */
	if (otfd->flags & OTF_FLAG_FLUSH) {
		/* Write chunk with remaining bytes */
		if (chunk_len) {
			printf("- Writing chunk\n");
			if (write_chunk(mmc, otfd, dstblk, chunk_len) < 0) {
				printf("Error: not all data written to media\n");
				return -1;
			}
		}
		/* Reset all static variables if offset == 0 (starting chunk) */
		chunk_len = 0;
		dstblk = 0;
		return 0;
	}

	/* Buffer otfd in RAM (if not there already) until we reach the
	 * configured limit to write it to media
	 */
	if (otfd->loadaddr != (unsigned int)otfd->buf) {
		memcpy((void *)(otfd->loadaddr + otfd->offset), otfd->buf,
			otfd->len);
	}
	chunk_len += otfd->len;
	if (chunk_len >= CONFIG_OTF_CHUNK) {
		unsigned int remaining;
		unsigned int written;
		/* We have CONFIG_OTF_CHUNK (or more) bytes in RAM.
		 * Let's proceed to write as many as multiples of blksz
		 * as possible.
		 */
		printf("- Writing chunk\n");
		written = write_chunk(mmc, otfd, dstblk, chunk_len) *
			  mmc_dev->blksz;
		if (written < 0)
			return -1;
		remaining = chunk_len - written;
		/* increment destiny block */
		dstblk += (written / mmc_dev->blksz);
		/* copy excess of bytes from previous chunk to offset 0 */
		if (remaining) {
			memcpy((void *)otfd->loadaddr,
			       (void *)(otfd->loadaddr + written),
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
