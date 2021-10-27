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
#include <asm/arch/mx6-ddr.h>
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
#include <micrel.h>
#include <miiphy.h>
#include <otf_update.h>
#include <part.h>
#include <recovery.h>
#ifdef CONFIG_OF_LIBFDT
#include <fdt_support.h>
#endif
#include "../common/hwid.h"
#include "ccimx6.h"
#include "../../../drivers/net/fec_mxc.h"

DECLARE_GLOBAL_DATA_PTR;

struct ccimx6_hwid my_hwid;
static block_dev_desc_t *mmc_dev;
static int mmc_dev_index = -1;
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

struct addrvalue {
	u32 address;
	u32 value;
};

/**
 * To add new valid variant ID, append new lines in this array with its configuration
 */
struct ccimx6_variant ccimx6_variants[] = {
/* 0x00 */ { IMX6_NONE,	0, 0, "Unknown"},
/* 0x01 - 55001818-01 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_KINETIS | CCIMX6_HAS_EMMC,
		"Consumer quad-core 1.2GHz, 4GB eMMC, 1GB DDR3, 0/+70C, Wireless, Bluetooth, Kinetis",
	},
/* 0x02 - 55001818-02 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_KINETIS | CCIMX6_HAS_EMMC,
		"Consumer quad-core 1.2GHz, 4GB eMMC, 1GB DDR3, -20/+70C, Wireless, Bluetooth, Kinetis",
	},
/* 0x03 - 55001818-03 */
	{
		IMX6Q,
		MEM_512MB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_EMMC,
		"Industrial quad-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C, Wireless, Bluetooth",
	},
/* 0x04 - 55001818-04 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_EMMC,
		"Industrial dual-core 800MHz, 4GB eMMC, 1GB DDR3, -40/+85C, Wireless, Bluetooth",
	},
/* 0x05 - 55001818-05 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_EMMC,
		"Consumer dual-core 1GHz, 4GB eMMC, 1GB DDR3, 0/+70C, Wireless",
	},
/* 0x06 - 55001818-06 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_EMMC,
		"Consumer dual-core 1GHz, 4GB eMMC, 512MB DDR3, 0/+70C, Wireless, Bluetooth",
	},
/* 0x07 - 55001818-07 */
	{
		IMX6S,
		MEM_256MB,
		CCIMX6_HAS_WIRELESS,
		"Consumer mono-core 1GHz, no eMMC, 256MB DDR3, 0/+70C, Wireless",
	},
/* 0x08 - 55001818-08 */
	{
		IMX6D,
		MEM_512MB,
		CCIMX6_HAS_EMMC,
		"Consumer dual-core 1GHz, 4GB eMMC, 512MB DDR3, 0/+70C",
	},
/* 0x09 - 55001818-09 */
	{
		IMX6S,
		MEM_256MB,
		0,
		"Consumer mono-core 1GHz, no eMMC, 256MB DDR3, 0/+70C",
	},
/* 0x0A - 55001818-10 */
	{
		IMX6DL,
		MEM_512MB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_EMMC,
		"Industrial DualLite-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C, Wireless",
	},
/* 0x0B - 55001818-11 */
	{
		IMX6DL,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_EMMC,
		"Consumer DualLite-core 1GHz, 4GB eMMC, 1GB DDR3, 0/+70C, Wireless, Bluetooth",
	},
/* 0x0C - 55001818-12 */
	{
		IMX6DL,
		MEM_512MB,
		CCIMX6_HAS_EMMC,
		"Industrial DualLite-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C",
	},
/* 0x0D - 55001818-13 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_KINETIS | CCIMX6_HAS_EMMC,
		"Industrial dual-core 800MHz, 8GB eMMC, 1GB DDR3, -40/+85C, Wireless, Bluetooth, Kinetis",
	},
/* 0x0E - 55001818-14 */
	{
		IMX6D,
		MEM_512MB,
		CCIMX6_HAS_EMMC,
		"Industrial dual-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C",
	},
/* 0x0F - 55001818-15 */
	{
		IMX6Q,
		MEM_512MB,
		CCIMX6_HAS_EMMC,
		"Industrial quad-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C",
	},
/* 0x10 - 55001818-16 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_KINETIS | CCIMX6_HAS_EMMC,
		"Industrial quad-core 800MHz, 4GB eMMC, 1GB DDR3, -40/+85C, Wireless, Bluetooth, Kinetis",
	},
/* 0x11 - 55001818-17 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_KINETIS | CCIMX6_HAS_EMMC,
		"Industrial quad-core 800MHz, 8GB eMMC, 1GB DDR3, -40/+85C, Wireless, Bluetooth, Kinetis",
	},
/* 0x12 - 55001818-18 */
	{
		IMX6Q,
		MEM_2GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_EMMC,
		"Consumer quad-core 1.2GHz, 4GB eMMC, 2GB DDR3, -20/+70C, Wireless, Bluetooth",
	},
/* 0x13 - 55001818-19 */
	{
		IMX6DL,
		MEM_512MB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH |
		CCIMX6_HAS_EMMC,
		"Industrial DualLite-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C, Wireless, Bluetooth",
	},
};

#define NUM_VARIANTS	19

const char *cert_regions[] = {
	"U.S.A.",
	"International",
	"Japan",
};

#define DDR3_CAL_REGS	12
/* DDR3 calibration values for the different CC6 variants */
struct addrvalue ddr3_calibration[NUM_VARIANTS + 1][DDR3_CAL_REGS] = {
	/* Variant 0x02 */
	[0x02] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00070012},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x002C0020},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x001F0035},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x002E0030},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x432C0331},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03250328},
		{MX6_MMDC_P1_MPDGCTRL0, 0x433E0346},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0336031C},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x382B2F35},
		{MX6_MMDC_P1_MPRDDLCTL, 0x31332A3B},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3938403A},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4430453D},
	},
	/* Variant 0x03 (same as variant 0x0F) */
	[0x03] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000C0019},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310024},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43450348},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03330339},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3F38393C},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3A3B433F},
		{0, 0},
	},
	/* Variant 0x04 (same as variant 0x0D) */
	[0x04] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0018},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00320023},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00200038},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300033},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43430345},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03370339},
		{MX6_MMDC_P1_MPDGCTRL0, 0x43500356},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0348032A},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3E33353A},
		{MX6_MMDC_P1_MPRDDLCTL, 0x37383141},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3B3A433D},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4633483E},
	},
	/* Variant 0x05 */
	[0x05] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00080014},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00300022},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00200035},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300032},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x432F0332},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03250328},
		{MX6_MMDC_P1_MPDGCTRL0, 0x433D0345},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0339031C},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3B303438},
		{MX6_MMDC_P1_MPRDDLCTL, 0x32342D3C},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3938433C},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4433463D},
	},
	/* Variant 0x06 (same as 0x08) */
	[0x06] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000A0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x002E0020},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43360337},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0329032B},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x39303338},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x37373F3A},
		{0, 0},
	},
	/* Variant 0x07 (same as 0x09) */
	[0x07] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00290036},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42540247},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x40404847},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x40402D31},
		{0, 0},
	},
	/* Variant 0x08 (same as 0x06) */
	[0x08] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000A0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x002E0020},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43360337},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0329032B},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x39303338},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x37373F3A},
		{0, 0},
	},
	/* Variant 0x09 (same as 0x07) */
	[0x09] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00290036},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42540247},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x40404847},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x40402D31},
		{0, 0},
	},
	/* Variant 0x0A (same as variants 0x0C, 0x13) */
	[0x0A] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00270036},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310033},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42520243},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0236023F},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x45474B4A},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x28282326},
		{0, 0},
	},
	/* Variant 0x0B */
	[0x0B] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x002C0038},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00360038},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x001B001F},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x002B0034},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x423F0235},
		{MX6_MMDC_P0_MPDGCTRL1, 0x02360241},
		{MX6_MMDC_P1_MPDGCTRL0, 0x42340236},
		{MX6_MMDC_P1_MPDGCTRL1, 0x02250238},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x41454848},
		{MX6_MMDC_P1_MPRDDLCTL, 0x45464B43},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x36352D31},
		{MX6_MMDC_P1_MPWRDLCTL, 0x3130332D},
	},
	/* Variant 0x0C (same as variants 0x0A, 0x13) */
	[0x0C] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00270036},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310033},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42520243},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0236023F},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x45474B4A},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x28282326},
		{0, 0},
	},
	/* Variant 0x0D (same as variant 0x04) */
	[0x0D] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0018},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00320023},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00200038},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300033},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43430345},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03370339},
		{MX6_MMDC_P1_MPDGCTRL0, 0x43500356},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0348032A},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3E33353A},
		{MX6_MMDC_P1_MPRDDLCTL, 0x37383141},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3B3A433D},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4633483E},
	},
	/* Variant 0x0E */
	[0x0E] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x0011001B},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00370029},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x4348034A},
		{MX6_MMDC_P0_MPDGCTRL1, 0x033C033E},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3F36383E},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3D3C4440},
		{0, 0},
	},
	/* Variant 0x0F (same as variant 0x03) */
	[0x0F] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000C0019},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310024},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43450348},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03330339},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3F38393C},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3A3B433F},
		{0, 0},
	},
	/* Variant 0x11 */
	[0x11] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x0013001E},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x003B002D},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00280041},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x0037003E},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x434C034F},
		{MX6_MMDC_P0_MPDGCTRL1, 0x033F0344},
		{MX6_MMDC_P1_MPDGCTRL0, 0x4358035F},
		{MX6_MMDC_P1_MPDGCTRL1, 0x034E0332},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3D33353A},
		{MX6_MMDC_P1_MPRDDLCTL, 0x37383240},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3C3B453E},
		{MX6_MMDC_P1_MPWRDLCTL, 0x47374A40},
	},
	/* Variant 0x12 */
	[0x12] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00060015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x002F001F},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00220035},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300031},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43220325},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0318031F},
		{MX6_MMDC_P1_MPDGCTRL0, 0x4334033C},
		{MX6_MMDC_P1_MPDGCTRL1, 0x032F0314},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3E31343B},
		{MX6_MMDC_P1_MPRDDLCTL, 0x38363040},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3939423B},
		{MX6_MMDC_P1_MPWRDLCTL, 0x46354840},
	},
	/* Variant 0x13 (same as variants 0x0A, 0x0C) */
	[0x13] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00270036},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310033},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42520243},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0236023F},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x45474B4A},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x28282326},
		{0, 0},
	},
};

/* DDR3 calibration values for the different CC6N variants */
struct addrvalue ddr3_cal_cc6n[NUM_VARIANTS + 1][DDR3_CAL_REGS] = {
	/* Variant 0x02 (same as variants 0x11 and 0x12) */
	[0x02] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x0027001D},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x001C002B},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00240028},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x423D0241},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0238023A},
		{MX6_MMDC_P1_MPDGCTRL0, 0x4249024D},
		{MX6_MMDC_P1_MPDGCTRL1, 0x02440234},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3B323437},
		{MX6_MMDC_P1_MPRDDLCTL, 0x3436323D},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x36393D3B},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4033423C},
	},
	/* Variant 0x03 (same as variant 0x0F) */
	[0x03] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000C0019},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310024},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43450348},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03330339},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3F38393C},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3A3B433F},
		{0, 0},
	},
	/* Variant 0x04 (same as variant 0x0D) */
	[0x04] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0018},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00320023},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00200038},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300033},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43430345},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03370339},
		{MX6_MMDC_P1_MPDGCTRL0, 0x43500356},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0348032A},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3E33353A},
		{MX6_MMDC_P1_MPRDDLCTL, 0x37383141},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3B3A433D},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4633483E},
	},
	/* Variant 0x05 */
	[0x05] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00080014},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00300022},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00200035},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300032},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x432F0332},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03250328},
		{MX6_MMDC_P1_MPDGCTRL0, 0x433D0345},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0339031C},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3B303438},
		{MX6_MMDC_P1_MPRDDLCTL, 0x32342D3C},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3938433C},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4433463D},
	},
	/* Variant 0x06 (same as 0x08) */
	[0x06] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000A0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x002E0020},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43360337},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0329032B},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x39303338},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x37373F3A},
		{0, 0},
	},
	/* Variant 0x07 (same as 0x09) */
	[0x07] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00290036},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42540247},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x40404847},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x40402D31},
		{0, 0},
	},
	/* Variant 0x08 (same as 0x06) */
	[0x08] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000A0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x002E0020},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43360337},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0329032B},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x39303338},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x37373F3A},
		{0, 0},
	},
	/* Variant 0x09 (same as 0x07) */
	[0x09] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00290036},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42540247},
		{0, 0},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x40404847},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x40402D31},
		{0, 0},
	},
	/* Variant 0x0A (same as variants 0x0C, 0x13) */
	[0x0A] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00270036},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310033},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42520243},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0236023F},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x45474B4A},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x28282326},
		{0, 0},
	},
	/* Variant 0x0B */
	[0x0B] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x002C0038},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00360038},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x001B001F},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x002B0034},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x423F0235},
		{MX6_MMDC_P0_MPDGCTRL1, 0x02360241},
		{MX6_MMDC_P1_MPDGCTRL0, 0x42340236},
		{MX6_MMDC_P1_MPDGCTRL1, 0x02250238},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x41454848},
		{MX6_MMDC_P1_MPRDDLCTL, 0x45464B43},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x36352D31},
		{MX6_MMDC_P1_MPWRDLCTL, 0x3130332D},
	},
	/* Variant 0x0C (same as variants 0x0A, 0x13) */
	[0x0C] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00270036},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310033},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42520243},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0236023F},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x45474B4A},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x28282326},
		{0, 0},
	},
	/* Variant 0x0D (same as variant 0x04) */
	[0x0D] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0018},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00320023},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x00200038},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00300033},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43430345},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03370339},
		{MX6_MMDC_P1_MPDGCTRL0, 0x43500356},
		{MX6_MMDC_P1_MPDGCTRL1, 0x0348032A},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3E33353A},
		{MX6_MMDC_P1_MPRDDLCTL, 0x37383141},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3B3A433D},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4633483E},
	},
	/* Variant 0x0E */
	[0x0E] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x0011001B},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00370029},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x4348034A},
		{MX6_MMDC_P0_MPDGCTRL1, 0x033C033E},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3F36383E},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3D3C4440},
		{0, 0},
	},
	/* Variant 0x0F (same as variant 0x03) */
	[0x0F] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000C0019},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310024},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x43450348},
		{MX6_MMDC_P0_MPDGCTRL1, 0x03330339},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3F38393C},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x3A3B433F},
		{0, 0},
	},
	/* Variant 0x11 (same as variants 0x02 and 0x12) */
	[0x11] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x0027001D},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x001C002B},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00240028},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x423D0241},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0238023A},
		{MX6_MMDC_P1_MPDGCTRL0, 0x4249024D},
		{MX6_MMDC_P1_MPDGCTRL1, 0x02440234},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3B323437},
		{MX6_MMDC_P1_MPRDDLCTL, 0x3436323D},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x36393D3B},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4033423C},
	},
	/* Variant 0x12 (same as variants 0x02 and 0x11) */
	[0x12] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x000B0015},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x0027001D},
		{MX6_MMDC_P1_MPWLDECTRL0, 0x001C002B},
		{MX6_MMDC_P1_MPWLDECTRL1, 0x00240028},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x423D0241},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0238023A},
		{MX6_MMDC_P1_MPDGCTRL0, 0x4249024D},
		{MX6_MMDC_P1_MPDGCTRL1, 0x02440234},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x3B323437},
		{MX6_MMDC_P1_MPRDDLCTL, 0x3436323D},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x36393D3B},
		{MX6_MMDC_P1_MPWRDLCTL, 0x4033423C},
	},
	/* Variant 0x13 (same as variants 0x0A, 0x0C) */
	[0x13] = {
		/* Write leveling */
		{MX6_MMDC_P0_MPWLDECTRL0, 0x00270036},
		{MX6_MMDC_P0_MPWLDECTRL1, 0x00310033},
		{0, 0},
		{0, 0},
		/* Read DQS gating */
		{MX6_MMDC_P0_MPDGCTRL0, 0x42520243},
		{MX6_MMDC_P0_MPDGCTRL1, 0x0236023F},
		{0, 0},
		{0, 0},
		/* Read delay */
		{MX6_MMDC_P0_MPRDDLCTL, 0x45474B4A},
		{0, 0},
		/* Write delay */
		{MX6_MMDC_P0_MPWRDLCTL, 0x28282326},
		{0, 0},
	},
};

static int mx6_rgmii_rework(struct phy_device *phydev)
{
	char *phy_mode;

	/*
	 * Micrel PHY KSZ9031 has four MMD registers to configure the clock skew
	 * of different signals. In U-Boot we're having Ethernet issues on
	 * certain boards which work fine in Linux. We examined these MMD clock
	 * skew registers in Linux which have different values than the reset
	 * defaults:
	 * 			Reset default		Linux
	 * ------------------------------------------------------------------
	 *  Control data pad	0077 (no skew)		0000 (-0.42 ns)
	 *  RX data pad		7777 (no skew)		0000 (-0.42 ns)
	 *  TX data pad		7777 (no skew)		7777 (no skew)
	 *  Clock pad		3def (no skew)		03ff (+0.96 ns)
	 *
	 *  Setting the skews used in Linux solves the issues in U-Boot.
	 */

	/* control data pad skew - devaddr = 0x02, register = 0x04 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* rx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* tx data pad skew - devaddr = 0x02, register = 0x05 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x7777);
	/* gtx and rx clock pad skew - devaddr = 0x02, register = 0x08 */
	ksz9031_phy_extended_write(phydev, 0x02,
				   MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
				   MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x03ff);

	phy_mode = getenv("phy_mode");
	if (!strcmp("master", phy_mode)) {
		unsigned short reg;

		/*
		 * Micrel PHY KSZ9031 takes up to 5 seconds to autonegotiate
		 * with Gigabit switches. This time can be reduced by forcing
		 * the PHY to work as master during master-slave negotiation.
		 * Forcing master mode may cause autonegotiation to fail if
		 * the other end is also forced as master, or using a direct
		 * cable connection.
		 */
		reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_CTRL1000);
		reg |= MSTSLV_MANCONFIG_ENABLE | MSTSLV_MANCONFIG_MASTER;
		phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, reg);
	}

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	mx6_rgmii_rework(phydev);
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

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

int is_ccimx6n(void)
{
	/* If HWID is empty, default to CC6N */
	return my_hwid.hv ? my_hwid.hv >= CCIMX6N_BASE_HV : 1;
}

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

int mmc_get_bootdevindex(void)
{
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned reg;

	if (readl(&psrc->gpr10) & (1 << 28))
		reg = readl(&psrc->gpr9);
	else
		reg = readl(&psrc->sbmr1);

	/* BOOT_CFG2[4] and BOOT_CFG2[3] denote boot media:
	 * 01:	SDHC2 (uSD card)
	 * 11:	SDHC4 (eMMC)
	 */
	switch((reg & 0x00001800) >> 11) {
	case 1:
		/* SDHC2 (uSD) */
		if (board_has_emmc())
			return 1;	/* index of USDHC2 if SOM has eMMC */
		else
			return 0;	/* index of USDHC2 if SOM has no eMMC */
	case 3:
		return 0;	/* index of SDHC4 (eMMC) */
	}

	return -1;
}

int mmc_get_env_devno(void)
{
	return mmc_get_bootdevindex();
}

int mmc_get_env_partno(void)
{
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned reg;

	if (readl(&psrc->gpr10) & (1 << 28))
		reg = readl(&psrc->gpr9);
	else
		reg = readl(&psrc->sbmr1);

	/* BOOT_CFG2[4] and BOOT_CFG2[3] denote boot media:
	 * 01:	SDHC2 (uSD card)
	 * 11:	SDHC4 (eMMC)
	 */
	switch((reg & 0x00001800) >> 11) {
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

int board_mmc_init(bd_t *bis)
{
	int i;

	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			if (board_has_emmc()) {
				/* USDHC4 (eMMC) */
				imx_iomux_v3_setup_multiple_pads(usdhc4_pads,
						ARRAY_SIZE(usdhc4_pads));
				usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
				if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
					printf("Warning: failed to initialize USDHC4\n");
			}
			break;
		case 1:
			/* USDHC2 (uSD) */

			/*
			 * On CC6N enable LDO9 regulator powering USDHC2
			 * (microSD)
			 */
			pmic_write_bitfield(DA9063_LDO9_CONT, 0x1, 0, 0x1);

			imx_iomux_v3_setup_multiple_pads(
					usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			if (fsl_esdhc_initialize(bis, &usdhc_cfg[i]))
				printf("Warning: failed to initialize USDHC2\n");
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}

	}

	/* Get mmc system device */
	mmc_dev = mmc_get_dev(mmc_get_bootdevindex());
	if (NULL == mmc_dev)
		printf("Warning: failed to get mmc sys storage device\n");

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
	{"emmc", MAKE_CFGVAL(0x60, 0x58, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

void update_ddr3_calibration(u8 variant)
{
	int i;
	volatile u32 *addr;
	struct addrvalue *ddr3_cal;

	if (is_ccimx6n()) {
		if (variant == 0 || variant >= ARRAY_SIZE(ddr3_cal_cc6n))
			return;
		ddr3_cal = ddr3_cal_cc6n[variant];
	} else {
		if (variant <= 0 || variant > NUM_VARIANTS)
			return;
		ddr3_cal = ddr3_calibration[variant];
	}

	for (i = 0; i < DDR3_CAL_REGS; i++) {
		addr = (volatile u32 *)(ddr3_cal[i].address);
		if (addr != NULL)
			writel(ddr3_cal[i].value, addr);
	}
}

static void ccimx6_detect_spurious_wakeup(void) {
	unsigned int carrierboard_ver = get_carrierboard_version();
	unsigned char event_a, event_b, event_c, event_d, fault_log;

	/* Check whether we come from a shutdown state */
	pmic_read_reg(DA9063_FAULT_LOG_ADDR, &fault_log);
	debug("DA9063 fault_log 0x%08x\n", fault_log);

	if (fault_log & (DA9063_E_nSHUT_DOWN |
			 DA9063_E_nKEY_RESET)) {
		/* Clear fault log nSHUTDOWN or nKEY_RESET bit */
		pmic_write_reg(DA9063_FAULT_LOG_ADDR, fault_log &
			      (DA9063_E_nSHUT_DOWN | DA9063_E_nKEY_RESET));

		pmic_read_reg(DA9063_EVENT_A_ADDR, &event_a);
		pmic_read_reg(DA9063_EVENT_B_ADDR, &event_b);
		pmic_read_reg(DA9063_EVENT_C_ADDR, &event_c);
		pmic_read_reg(DA9063_EVENT_D_ADDR, &event_d);

		/* Clear event registers */
		pmic_write_reg(DA9063_EVENT_A_ADDR, event_a);
		pmic_write_reg(DA9063_EVENT_B_ADDR, event_b);
		pmic_write_reg(DA9063_EVENT_C_ADDR, event_c);
		pmic_write_reg(DA9063_EVENT_D_ADDR, event_d);

		/* Return if the wake up is valid */
		if (event_a) {
			/* Valid wake up sources include RTC ticks and alarm,
			 * onKey and ADC measurement */
			if (event_a & (DA9063_E_TICK | DA9063_E_ALARM |
				       DA9063_E_nONKEY | DA9063_E_ADC_RDY)) {
				debug("WAKE: Event A: 0x%02x\n", event_a);
				return;
			}
		}

		/* All events in B are wake-up capable  */
		if (event_b) {
			unsigned int valid_mask = 0xFF;

			/* Any event B is valid, except E_WAKE on SBCv1 which
			 * is N/C */
			if (carrierboard_ver == 1)
				valid_mask &= ~DA9063_E_WAKE;

			if (event_b & valid_mask) {
				debug("WAKE: Event B: 0x%02x wake-up valid 0x%02x\n",
						event_b, valid_mask);
				return;
			}
		}

		/* The only wake-up OTP enabled GPIOs in event C are:
		 *   - GPIO5, valid on BT variants
		 *   - GPIO6, valid on wireless variants
		 */
		if (event_c) {
			unsigned int valid_mask = 0;

			/* On variants with bluetooth the BT_HOST_WAKE
			 * (GPIO5) pin is valid. */
			if (board_has_bluetooth())
				valid_mask |= DA9063_E_GPIO5;
			/* On variants with wireless the WLAN_HOST_WAKE
			 * (GPIO6) pin is valid. */
			if (board_has_wireless())
				valid_mask |= DA9063_E_GPIO6;

			if (event_c & valid_mask) {
				debug("WAKE: Event C: 0x%02x wake-up valid 0x%02x\n",
						event_c, valid_mask);
				return;
			}
		}

		/* The only wake-up OTP enabled GPIOs in event D are:
		 *  - GPIO8, valid on MCA variants
		 *  - GPIO9, N/C on SBCs, never valid
		 */
		if (event_d) {
			unsigned int valid_mask = 0;

			/* On variants with kinetis GPIO8/SYS_EN is valid */
			if (board_has_kinetis())
			       valid_mask |= DA9063_E_GPIO8;

			if (event_d & valid_mask) {
				debug("WAKE: Event D: 0x%02x wake-up valid 0x%02x\n",
						event_d, valid_mask);
				return;
			}
		}

		/* If we reach here the event is spurious */
		printf("Spurious wake, back to standby.\n");
		debug("Events A:%02x B:%02x C:%02x D:%02x\n", event_a, event_b,
			event_c, event_d);

		/* Make sure nRESET is asserted when waking up */
		pmic_write_bitfield(DA9063_CONTROL_B_ADDR, 0x1, 3, 0x1);

		/* De-assert SYS_EN to get to powerdown mode. The OTP
		 * is not reread when coming up so the wake-up
		 * supression configuration will be preserved .*/
		pmic_write_bitfield(DA9063_CONTROL_A_ADDR, 0x1, 0, 0x0);

		/* Don't come back */
		while (1)
			udelay(1000);
	}

	return;
}

static int ccimx6_fixup(void)
{
	if (!board_has_bluetooth()) {
		/* Avoid spurious wake ups */
		if (pmic_write_bitfield(DA9063_GPIO4_5_ADDR, 0x1, 7, 0x1)) {
			printf("Failed to suppress GPIO5 wakeup.");
			return -1;
		}
	}

	if (!board_has_wireless()) {
		/* Avoid spurious wake ups */
		if (pmic_write_bitfield(DA9063_GPIO6_7_ADDR, 0x1, 3, 0x1)) {
			printf("Failed to suppress GPIO6 wakeup.");
			return -1;
		}
	}

	if (!board_has_kinetis()) {
		/* Avoid spurious wake ups */
		if (pmic_write_bitfield(DA9063_GPIO8_9_ADDR, 0x1, 3, 0x1)) {
			printf("Failed to suppress GPIO8 wakeup.");
			return -1;
		}
	}

	ccimx6_detect_spurious_wakeup();

	return 0;
}

void pmic_bucks_synch_mode(void)
{
#ifdef CONFIG_I2C_MULTI_BUS
	if (i2c_set_bus_num(0))
                return;
#endif

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (!i2c_probe(CONFIG_PMIC_I2C_ADDR)) {
		if (pmic_write_bitfield(DA9063_BCORE2_CONF_ADDR, 0x3, 6, 0x2))
			printf("Could not set BCORE2 in synchronous mode\n");
		if (pmic_write_bitfield(DA9063_BCORE1_CONF_ADDR, 0x3, 6, 0x2))
			printf("Could not set BCORE1 in synchronous mode\n");
		if (pmic_write_bitfield(DA9063_BPRO_CONF_ADDR, 0x3, 6, 0x2))
			printf("Could not set BPRO in synchronous mode\n");
		if (pmic_write_bitfield(DA9063_BIO_CONF_ADDR, 0x3, 6, 0x2))
			printf("Could not set BIO in synchronous mode\n");
		if (pmic_write_bitfield(DA9063_BMEM_CONF_ADDR, 0x3, 6, 0x2))
			printf("Could not set BMEM in synchronous mode\n");
		if (pmic_write_bitfield(DA9063_BPERI_CONF_ADDR, 0x3, 6, 0x2))
			printf("Could not set BPERI in synchronous mode\n");
	} else {
		printf("Could not set bucks in synchronous mode\n");
	}
}

int ccimx6_late_init(void)
{
	int ret = 0;
#ifdef CONFIG_CMD_MMC
	char cmd[80];
#endif
	char var[5];

	/* Override DDR3 calibration values basing on HWID variant */
	update_ddr3_calibration(my_hwid.variant);

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_I2C_MXC
#ifdef CONFIG_I2C_MULTI_BUS
	/* Setup I2C3 (HDMI, Audio...) */
	setup_i2c(2, CONFIG_SYS_I2C_SPEED,
			CONFIG_SYS_I2C_SLAVE, &i2c_pad_info2);
#endif
	/* Operate all PMIC's bucks in "synchronous" mode (PWM) since the
	 * default "auto" mode may change them to operate in "sleep" mode (PFD)
	 * which might result in malfunctioning on certain custom boards with
	 * low loads under extreme stress conditions.
	 */
	pmic_bucks_synch_mode();

	ret = setup_pmic_voltages();
	if (ret)
		return -1;
#endif

#ifdef CONFIG_CMD_MMC
	/* Set $mmcbootdev to MMC boot device index */
	sprintf(cmd, "setenv -f mmcbootdev %x", mmc_get_bootdevindex());
	run_command(cmd, 0);
#endif

	/* Set $module_variant variable */
	sprintf(var, "0x%02x", my_hwid.variant);
	setenv("module_variant", var);

#ifdef CONFIG_ANDROID_RECOVERY
	if (recovery_check_and_clean_flag()) {
		char *recoverycmd;

		recoverycmd = getenv("recoverycmd");
		if (recoverycmd)
			run_command(recoverycmd, 0);
	}
#endif

	return ccimx6_fixup();
}

void board_print_hwid(u32 *hwid)
{
	int i;
	int cert;

	for (i = CONFIG_HWID_WORDS_NUMBER - 1; i >= 0; i--)
		printf(" %.8x", hwid[i]);
	printf("\n");
	/* Formatted printout */
	printf("    Year:          20%02d\n", (hwid[1] >> 26) & 0x3f);
	printf("    Week:          %02d\n", (hwid[1] >> 20) & 0x3f);
	printf("    Variant:       0x%02x\n", (hwid[1] >> 8) & 0xff);
	printf("    HW Version:    0x%x\n", (hwid[1] >> 4) & 0xf);
	cert = hwid[1] & 0xf;
	printf("    Cert:          0x%x (%s)\n", cert,
	       cert < ARRAY_SIZE(cert_regions) ? cert_regions[cert] : "??");
	printf("    Location:      %c\n", ((hwid[0] >> 27) & 0x1f) + 'A');
	printf("    Generator ID:  %02d\n", (hwid[0] >> 20) & 0x7f);
	printf("    S/N:           %06d\n", hwid[0] & 0xfffff);
}

void board_print_manufid(u32 *hwid)
{
	int i;

	for (i = CONFIG_HWID_WORDS_NUMBER - 1; i >= 0; i--)
		printf(" %.8x", hwid[i]);
	printf("\n");
	/* Formatted printout */
	printf(" Manufacturing ID: %c%02d%02d%02d%06d %02x%x%x\n",
	       ((hwid[0] >> 27) & 0x1f) + 'A',
	       (hwid[1] >> 26) & 0x3f,
	       (hwid[1] >> 20) & 0x3f,
	       (hwid[0] >> 20) & 0x7f,
	       hwid[0] & 0xfffff,
	       (hwid[1] >> 8) & 0xff,
	       (hwid[1] >> 4) & 0xf,
	       hwid[1] & 0xf);
}

static int is_valid_hwid(struct ccimx6_hwid *hwid)
{
	if (hwid->variant < ARRAY_SIZE(ccimx6_variants))
		if (ccimx6_variants[hwid->variant].cpu != IMX6_NONE)
			return 1;

	return 0;
}

static void array_to_hwid(u32 *hwid)
{
	/*
	 *                      MAC1 (Bank 4 Word 3)
	 *
	 *       | 31..26 | 25..20 |   |  15..8  | 7..4 | 3..0 |
	 *       +--------+--------+---+---------+------+------+
	 * HWID: |  Year  |  Week  | - | Variant |  HV  | Cert |
	 *       +--------+--------+---+---------+------+------+
	 *
	 *                      MAC0 (Bank 4 Word 2)
	 *
	 *       |  31..27  | 26..20 |         19..0           |
	 *       +----------+--------+-------------------------+
	 * HWID: | Location |  GenID |      Serial number      |
	 *       +----------+--------+-------------------------+
	 */
	my_hwid.year = (hwid[1] >> 26) & 0x3f;
	my_hwid.week = (hwid[1] >> 20) & 0x3f;
	my_hwid.variant = (hwid[1] >> 8) & 0xff;
	my_hwid.hv = (hwid[1] >> 4) & 0xf;
	my_hwid.cert = hwid[1] & 0xf;
	my_hwid.location = (hwid[0] >> 27) & 0x1f;
	my_hwid.genid = (hwid[0] >> 20) & 0x7f;
	my_hwid.sn = hwid[0] & 0xfffff;
}

int manufstr_to_hwid(int argc, char *const argv[], u32 *val)
{
	u32 *mac0 = val;
	u32 *mac1 = val + 1;
	char tmp[13];
	unsigned long num;

	/* Initialize HWID words */
	*mac0 = 0;
	*mac1 = 0;

	if (argc != 2)
		goto err;

	/*
	 * Digi Manufacturing team produces a string in the form
	 *     LYYWWGGXXXXXX
	 * where:
	 *  - L:	location, an uppercase letter [A..Z]
	 *  - YY:	year (last two digits of XXI century, in decimal)
	 *  - WW:	week of year (in decimal)
	 *  - GG:	generator ID (in decimal)
	 *  - XXXXXX:	serial number (in decimal)
	 * this information goes into the following places on the HWID:
	 *  - L:	OCOTP_MAC0 bits 31..27 (5 bits)
	 *  - YY:	OCOTP_MAC1 bits 31..26 (6 bits)
	 *  - WW:	OCOTP_MAC1 bits 25..20 (6 bits)
	 *  - GG:	OCOTP_MAC0 bits 26..20 (7 bits)
	 *  - XXXXXX:	OCOTP_MAC0 bits 19..0 (20 bits)
	 */
	if (strlen(argv[0]) != 13)
		goto err;

	/*
	 * Additionally a second string in the form VVHC must be given where:
	 *  - VV:	variant (in hex)
	 *  - H:	hardware version (in hex)
	 *  - C:	wireless certification (in hex)
	 * this information goes into the following places on the HWID:
	 *  - VV:	OCOTP_MAC1 bits 15..8 (8 bits)
	 *  - H:	OCOTP_MAC1 bits 7..4 (4 bits)
	 *  - C:	OCOTP_MAC1 bits 3..0 (4 bits)
	 */
	if (strlen(argv[1]) != 4)
		goto err;

	/* Location */
	if (argv[0][0] < 'A' || argv[0][0] > 'Z')
		goto err;
	*mac0 |= (argv[0][0] - 'A') << 27;
	printf("    Location:      %c\n", argv[0][0]);

	/* Year (only 6 bits: from 0 to 63) */
	strncpy(tmp, &argv[0][1], 2);
	tmp[2] = 0;
	num = simple_strtol(tmp, NULL, 10);
	if (num < 0 || num > 63)
		goto err;
	*mac1 |= num << 26;
	printf("    Year:          20%02d\n", (int)num);

	/* Week */
	strncpy(tmp, &argv[0][3], 2);
	tmp[2] = 0;
	num = simple_strtol(tmp, NULL, 10);
	if (num < 1 || num > 54)
		goto err;
	*mac1 |= num << 20;
	printf("    Week:          %02d\n", (int)num);

	/* Generator ID */
	strncpy(tmp, &argv[0][5], 2);
	tmp[2] = 0;
	num = simple_strtol(tmp, NULL, 10);
	if (num < 0 || num > 99)
		goto err;
	*mac0 |= num << 20;
	printf("    Generator ID:  %02d\n", (int)num);

	/* Serial number */
	strncpy(tmp, &argv[0][7], 6);
	tmp[6] = 0;
	num = simple_strtol(tmp, NULL, 10);
	if (num < 0 || num > 999999)
		goto err;
	*mac0 |= num;
	printf("    S/N:           %06d\n", (int)num);

	/* Variant */
	strncpy(tmp, &argv[1][0], 2);
	tmp[2] = 0;
	num = simple_strtol(tmp, NULL, 16);
	if (num < 0 || num > 0xff)
		goto err;
	*mac1 |= num << 8;
	printf("    Variant:       0x%02x\n", (int)num);

	/* Hardware version */
	strncpy(tmp, &argv[1][2], 1);
	tmp[1] = 0;
	num = simple_strtol(tmp, NULL, 16);
	if (num < 0 || num > 0xf)
		goto err;
	*mac1 |= num << 4;
	printf("    HW version:    0x%x\n", (int)num);

	/* Cert */
	strncpy(tmp, &argv[1][3], 1);
	tmp[1] = 0;
	num = simple_strtol(tmp, NULL, 16);
	if (num < 0 || num > 0xf)
		goto err;
	*mac1 |= num;
	printf("    Cert:          0x%x (%s)\n", (int)num,
	       num < ARRAY_SIZE(cert_regions) ? cert_regions[num] : "??");

	return 0;

err:
	printf("Invalid manufacturing string.\n"
		"Manufacturing information must be in the form: "
		CONFIG_MANUF_STRINGS_HELP "\n");
	return -EINVAL;
}

int get_hwid(void)
{
	u32 hwid[CONFIG_HWID_WORDS_NUMBER];
	u32 bank = CONFIG_HWID_BANK;
	u32 word = CONFIG_HWID_START_WORD;
	u32 cnt = CONFIG_HWID_WORDS_NUMBER;
	int ret, i;

	for (i = 0; i < cnt; i++, word++) {
		ret = fuse_read(bank, word, &hwid[i]);
		if (ret)
			return -1;
	}

	array_to_hwid(hwid);

	return 0;
}

void fdt_fixup_hwid(void *fdt)
{
	const char *propnames[] = {
		"digi,hwid,location",
		"digi,hwid,genid",
		"digi,hwid,sn",
		"digi,hwid,year",
		"digi,hwid,week",
		"digi,hwid,variant",
		"digi,hwid,hv",
		"digi,hwid,cert",
	};
	char str[20];
	int i;

	/* Register the HWID as main node properties in the FDT */
	for (i = 0; i < ARRAY_SIZE(propnames); i++) {
		/* Convert HWID fields to strings */
		if (!strcmp("digi,hwid,location", propnames[i]))
			sprintf(str, "%c", my_hwid.location + 'A');
		else if (!strcmp("digi,hwid,genid", propnames[i]))
			sprintf(str, "%02d", my_hwid.genid);
		else if (!strcmp("digi,hwid,sn", propnames[i]))
			sprintf(str, "%06d", my_hwid.sn);
		else if (!strcmp("digi,hwid,year", propnames[i]))
			sprintf(str, "20%02d", my_hwid.year);
		else if (!strcmp("digi,hwid,week", propnames[i]))
			sprintf(str, "%02d", my_hwid.week);
		else if (!strcmp("digi,hwid,variant", propnames[i]))
			sprintf(str, "0x%02x", my_hwid.variant);
		else if (!strcmp("digi,hwid,hv", propnames[i]))
			sprintf(str, "0x%x", my_hwid.hv);
		else if (!strcmp("digi,hwid,cert", propnames[i]))
			sprintf(str, "0x%x", my_hwid.cert);
		else
			continue;

		do_fixup_by_path(fdt, "/", propnames[i], str,
				 strlen(str) + 1, 1);
	}
}

int board_has_emmc(void)
{
	if (is_valid_hwid(&my_hwid))
		return (ccimx6_variants[my_hwid.variant].capabilities &
				    CCIMX6_HAS_EMMC);
	else
		return 1; /* assume it has if invalid HWID */
}

int board_has_wireless(void)
{
	if (is_valid_hwid(&my_hwid))
		return (ccimx6_variants[my_hwid.variant].capabilities &
				    CCIMX6_HAS_WIRELESS);
	else
		return 1; /* assume it has if invalid HWID */
}

int board_has_bluetooth(void)
{
	if (is_valid_hwid(&my_hwid))
		return (ccimx6_variants[my_hwid.variant].capabilities &
				    CCIMX6_HAS_BLUETOOTH);
	else
		return 1; /* assume it has if invalid HWID */
}

int board_has_kinetis(void)
{
	if (is_valid_hwid(&my_hwid))
		return (ccimx6_variants[my_hwid.variant].capabilities &
				    CCIMX6_HAS_KINETIS);
	else
		return 1; /* assume it has if invalid HWID */
}

int get_carrierboard_version(void)
{
#ifdef CONFIG_HAS_CARRIERBOARD_VERSION
	u32 version;

	if (fuse_read(CONFIG_CARRIERBOARD_VERSION_BANK,
		      CONFIG_CARRIERBOARD_VERSION_WORD, &version))
		return CARRIERBOARD_VERSION_UNDEFINED;

	version >>= CONFIG_CARRIERBOARD_VERSION_OFFSET;
	version &= CONFIG_CARRIERBOARD_VERSION_MASK;

	return((int)version);
#else
	return CARRIERBOARD_VERSION_UNDEFINED;
#endif /* CONFIG_HAS_CARRIERBOARD_VERSION */
}

#ifdef CONFIG_HAS_CARRIERBOARD_VERSION
void fdt_fixup_carrierboard(void *fdt)
{
	char str[20];

	sprintf(str, "%d", get_carrierboard_version());
	do_fixup_by_path(fdt, "/", "digi,carrierboard,version", str,
			 strlen(str) + 1, 1);
}
#endif /* CONFIG_HAS_CARRIERBOARD_VERSION */

int checkboard(void)
{
	const char *bootdevice;
	int board_ver = get_carrierboard_version();

	printf("Board: %s ", CONFIG_BOARD_DESCRIPTION);
	if (CARRIERBOARD_VERSION_UNDEFINED == board_ver)
		printf("(undefined version)\n");
	else
		printf("v%d\n", board_ver);
	if (is_valid_hwid(&my_hwid))
		printf("Variant: 0x%02x - %s\n", my_hwid.variant,
			ccimx6_variants[my_hwid.variant].id_string);

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
void fdt_fixup_mac(void *fdt, char *varname, char *node, char *property)
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
		do_fixup_by_path(fdt, node, property, &mac_addr, 6, 1);
	}
}

/* Platform function to modify the FDT as needed */
void ft_board_setup(void *blob, bd_t *bd)
{

	/* Re-read HWID which could have been overriden by U-Boot commands */
	fdt_fixup_hwid(blob);

#ifdef CONFIG_HAS_CARRIERBOARD_VERSION
	fdt_fixup_carrierboard(blob);
#endif /* CONFIG_HAS_CARRIERBOARD_VERSION */

	if (board_has_wireless())
		/* Wireless MACs */
		fdt_fixup_mac(blob, "wlanaddr", "/wireless", "mac-address");
		if (is_ccimx6n()) {
			fdt_fixup_mac(blob, "wlan1addr", "/wireless", "mac-address1");
			fdt_fixup_mac(blob, "wlan2addr", "/wireless", "mac-address2");
			fdt_fixup_mac(blob, "wlan3addr", "/wireless", "mac-address3");
		}
	if (board_has_bluetooth())
		fdt_fixup_mac(blob, "btaddr", "/bluetooth", "mac-address");
}
#endif /* CONFIG_OF_BOARD_SETUP */

static int write_chunk(struct mmc *mmc, otf_data_t *otfd, unsigned int dstblk,
			unsigned int chunklen)
{
	int sectors;
	unsigned long written, read, verifyaddr;

	printf("\nWriting chunk...");
	/* Check WP */
	if (mmc_getwp(mmc) == 1) {
		printf("[Error]: card is write protected!\n");
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
		printf("[Error]: length of data exceeds partition size\n");
		return -1;
	}

	/* Write chunklen bytes of chunk to media */
	debug("writing chunk of 0x%x bytes (0x%x sectors) "
		"from 0x%x to block 0x%x\n",
		chunklen, sectors, otfd->loadaddr, dstblk);
	written = mmc->block_dev.block_write(mmc_dev_index, dstblk, sectors,
					     (const void *)otfd->loadaddr);
	if (written != sectors) {
		printf("[Error]: written sectors != sectors to write\n");
		return -1;
	}
	printf("[OK]\n");

	/* Verify written chunk if $loadaddr + chunk size does not overlap
	 * $verifyaddr (where the read-back copy will be placed)
	 */
	verifyaddr = getenv_ulong("verifyaddr", 16, 0);
	if (otfd->loadaddr + sectors * mmc_dev->blksz < verifyaddr) {
		/* Read back data... */
		printf("Reading back chunk...");
		read = mmc->block_dev.block_read(mmc_dev_index, dstblk, sectors,
						 (void *)verifyaddr);
		if (read != sectors) {
			printf("[Error]: read sectors != sectors to read\n");
			return -1;
		}
		printf("[OK]\n");
		/* ...then compare */
		printf("Verifying chunk...");
		if (memcmp((const void *)otfd->loadaddr,
			      (const void *)verifyaddr,
			      sectors * mmc_dev->blksz)) {
			printf("[Error]\n");
			return -1;
		} else {
			printf("[OK]\n");
			return 0;
		}
	} else {
		printf("[Warning]: Cannot verify chunk. "
			"It overlaps $verifyaddr!\n");
		return 0;
	}
}

/* writes a chunk of data from RAM to main storage media (eMMC) */
int board_update_chunk(otf_data_t *otfd)
{
	static unsigned int chunk_len = 0;
	static unsigned int dstblk = 0;
	struct mmc *mmc;

	if (mmc_dev_index == -1)
		mmc_dev_index = getenv_ulong("mmcdev", 16,
					     mmc_get_bootdevindex());
	mmc = find_mmc_device(mmc_dev_index);
	if (NULL == mmc)
		return -1;

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

int ccimx6_init(void)
{
	if (get_hwid()) {
		printf("Cannot read HWID\n");
		return -1;
	}

	/*
	 * Override DDR3 calibration values basing on HWID variant.
	 * NOTE: Re-writing the DDR3 calibration values is a delicate operation
	 * that must be done before other systems (like VPU) are enabled and can
	 * be accessing the RAM on their own.
	 */
	update_ddr3_calibration(my_hwid.variant);

#ifdef CONFIG_I2C_MXC
	/* Setup I2C2 (PMIC, Kinetis) */
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
#endif

	return 0;
}
