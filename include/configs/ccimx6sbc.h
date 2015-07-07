/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2013 Digi International, Inc.
 *
 * Configuration settings for the Freescale i.MX6Q SabreSD board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the
 * GNU General Public License for more details.
 */

#ifndef __CCIMX6SBC_CONFIG_H
#define __CCIMX6SBC_CONFIG_H

#include "ccimx6_common.h"
#include <asm/imx-common/gpio.h>

#define CONFIG_MACH_TYPE		4899
#define CONFIG_BOARD_DESCRIPTION	"ConnectCore 6 SBC"
#define CONFIG_MXC_UART_BASE		UART4_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc3"
#if defined(CONFIG_MX6DL) || defined(CONFIG_MX6S)
#define CONFIG_DEFAULT_FDT_FILE		"uImage-imx6dl-" CONFIG_SYS_BOARD ".dtb"
#elif defined(CONFIG_MX6Q)
#define CONFIG_DEFAULT_FDT_FILE		"uImage-imx6q-" CONFIG_SYS_BOARD ".dtb"
#endif

#define CONFIG_SYS_FSL_USDHC_NUM	2

/* MMC device and partition where U-Boot image is */
#define CONFIG_SYS_BOOT_PART_EMMC	1	/* Boot part 1 on eMMC */
#define CONFIG_SYS_BOOT_PART_OFFSET	1024
#define CONFIG_SYS_BOOT_PART_SIZE	((2 * 1024 * 1024) - CONFIG_SYS_BOOT_PART_OFFSET)

/* Media type for firmware updates */
#define CONFIG_SYS_STORAGE_MEDIA	"mmc"

/* Ethernet PHY */
#define CONFIG_PHY_MICREL
#define CONFIG_ENET_PHYADDR_MICREL	3

/* Carrier board version in OTP bits */
#define CONFIG_HAS_CARRIERBOARD_VERSION
#ifdef CONFIG_HAS_CARRIERBOARD_VERSION
/* For the SBC, the carrier board version is stored in Bank 4 Word 6 (GP1)
 * in the lower 4 bits */
#define CONFIG_CARRIERBOARD_VERSION_BANK	4
#define CONFIG_CARRIERBOARD_VERSION_WORD	6
#define CONFIG_CARRIERBOARD_VERSION_MASK	0xf	/* 4 OTP bits */
#define CONFIG_CARRIERBOARD_VERSION_OFFSET	0	/* lower 4 OTP bits */
#endif /* CONFIG_HAS_CARRIERBOARD_VERSION */

/* Celsius degrees below CPU's max die temp at which boot should be attempted */
#define CONFIG_BOOT_TEMP_BELOW_MAX		10

#endif                         /* __CCIMX6SBC_CONFIG_H */
