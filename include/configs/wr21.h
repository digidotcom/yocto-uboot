/*
 * Copyright (C) 2011 Digi International, Inc.
 *
 * Configuration settings for the ConnectPort X2.
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

#ifndef __WR21_H
#define __WR21_H
#include "digi_common.h"		/* Load Digi common stuff... */

#include <asm/arch/mx28.h>
#include <asm/arch/regs-enet.h>

/************************************************************
 * Definition of default options when not configured by user
 ************************************************************/
#ifndef CONFIG_DIGIEL_USERCONFIG
# define CONFIG_BAUDRATE		CONFIG_PLATFORM_BAUDRATE
/**
 * Dual boot support
 */
//# define CONFIG_DUAL_BOOT			1
# define CONFIG_DUAL_BOOT_MODE_PEER		1
# define CONFIG_DUAL_BOOT_STARTUP_GUARANTEE_PER_BOOT	1
# define CONFIG_DUAL_BOOT_RETRIES		3
# define CONFIG_DUAL_BOOT_WDT_ENABLE		1
# define CONFIG_DUAL_BOOT_WDT_TIMEOUT		30
# define CONFIG_DUAL_BOOT_RETRY_FOREVER		1
/* Silent console */
//# define CONFIG_SILENT_CONSOLE			1
#endif /* CONFIG_DIGIEL_USERCONFIG */

#ifdef CONFIG_SILENT_CONSOLE
# define CONFIG_ENABLE_SILENT_RESCUE		1
#endif

/*
 * SoC configurations
 */
#define CONFIG_MX28				/* i.MX28 SoC */
#define CONFIG_MX28_TO1_2
#define CONFIG_SYS_HZ		1000		/* Ticks per second */
/* ROM loads UBOOT into DRAM */
#define CONFIG_SKIP_RELOCATE_UBOOT

/*
 * Memory configurations
 */
#define CONFIG_NR_DRAM_BANKS	1		/* 1 bank of DRAM */
#define PHYS_SDRAM_1		0x40000000	/* Base address */
#define PHYS_SDRAM_1_SIZE	0x08000000	/* 128 MB */
#define CONFIG_SYS_MEMTEST_START 0x40000000	/* Memtest start address */
#define CONFIG_SYS_MEMTEST_END	 0x40400000	/* 4 MB RAM test */

/*
 * U-Boot general configurations
 */
#define CONFIG_WR21			1	/* in a WR21 */
#define CONFIG_MODULE_NAME		"wr21"
#define CONFIG_MODULE_STRING		"Digi TransPort WR21"
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE
						/* Boot argument buffer size */
#define CONFIG_VERSION_VARIABLE			/* U-BOOT version */
#define CONFIG_AUTO_COMPLETE			/* Command auto complete */
#define CONFIG_CMDLINE_EDITING			/* Command history etc */

#define CONFIG_SYS_64BIT_VSPRINTF

#if defined(CONFIG_UBOOT_PROMPT)
#undef CONFIG_SYS_PROMPT
#define	CONFIG_SYS_PROMPT	CONFIG_UBOOT_PROMPT_STR
#endif

/**
 * Digi commands and defines
 */
#define CONFIG_IDENT_STRING		" - GCC " __VERSION__ "\nfor " \
					CONFIG_MODULE_STRING
#define CONFIG_CMD_BOOTSTREAM		1
#define CONFIG_HAB_ENABLED
#define CONFIG_NVRAM_PRODUCT_INFO_SECTION

/* Unsupported common options for WR21 must be disabled */
#undef CONFIG_CMD_CACHE
#undef CONFIG_CMD_DATE
#undef CONFIG_CMD_SNTP

/*
 * Boot Linux
 */
#define CONFIG_BOOTFILE		"uImage"
#define CONFIG_BOOTCOMMAND	"dboot linux flash"
#define CONFIG_LOADADDR		0x40800000
/* NOTE: Digi BSP Will adjust 'loadaddr' depending on the
 * available RAM memory of the platform to
 * allow transfers of large files to RAM */
#define CONFIG_INITRD_LOADADDR_OFFSET	0x800000 /* Address where to put the
						  * the initrd: 8MiB after the
						  * kernel load address */
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/*
 * U-Boot Commands
 */
#define CONFIG_SYS_NO_FLASH
#include <config_cmd_default.h>
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_DISPLAY_CPUINFO

/*
 * Serial Driver
 */
#define CONFIG_SERIAL_MULTI
#define CONFIG_DEFAULT_SERIAL_AUART	1		/* Use AUART by default (DUART if commented) */
#define CONFIG_UARTDBG_CLK		24000000
#define CONFIG_UARTDBG_CONSOLE		"ttyAM"
#define CONFIG_UARTAPP_CLK		24000000
#define CONFIG_UARTAPP_CONSOLE		"ttySP"
#define CONFIG_PLATFORM_BAUDRATE	115200		/* Default baud rate */

#ifndef CONFIG_CONSOLE
# if defined(CONFIG_DEFAULT_SERIAL_AUART)
#  define CONFIG_CONSOLE			CONFIG_UARTAPP_CONSOLE
# else
#  define CONFIG_CONSOLE			CONFIG_UARTDBG_CONSOLE
# endif /* CONFIG_DEFAULT_SERIAL_AUART */
#endif
#define CONFIG_CONS_INDEX		1

/*
 * FEC Driver
 */
#define CONFIG_MXC_FEC
#define CONFIG_FEC0_IOBASE		REGS_ENET_BASE
#define CONFIG_FEC0_PHY_ADDR		0

#define CONFIG_FEC1_ENABLED		1
#ifdef CONFIG_FEC1_ENABLED
# define CONFIG_FEC1_INIT_ONLY_MAC
#define CONFIG_FEC1_IOBASE		(REGS_ENET_BASE + HW_ENET_MAC1_EIR)
#define CONFIG_FEC1_PHY_ADDR		3
#endif

#define CONFIG_NET_MULTI
#define CONFIG_ETH_PRIME
#define CONFIG_RMII
#define CONFIG_IPADDR			192.168.1.103
#define CONFIG_SERVERIP			192.168.1.101
#define CONFIG_NETMASK			255.255.255.0
#define CONFIG_STARTAGAIN_TIMEOUT	10000UL
#define CONFIG_PHY_POWER_GPIO		PINID_SAIF0_SDATA0	/* for WR21 rev 2 on */
#define CONFIG_PHY_RESET_GPIO		PINID_SSP0_DATA5	/* for WR21 rev 2 on */
#define CONFIG_PHY_CLOCK		50000000		/* 50MHz */

/* Add for working with "strict" DHCP server */
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_DNS

/*
 * MMC Driver
 */
//#define CONFIG_CMD_MMC

#ifdef CONFIG_CMD_MMC
	#define CONFIG_MMC
	#define CONFIG_IMX_SSP_MMC		/* MMC driver based on SSP */
	#define CONFIG_GENERIC_MMC
	#define CONFIG_DYNAMIC_MMC_DEVNO
	#define CONFIG_DOS_PARTITION
	#define CONFIG_CMD_FAT
	#define CONFIG_SYS_SSP_MMC_NUM 2

	/* environment */
	#define CONFIG_FSL_ENV_IN_MMC

	/* if there is no flash do not support Digi BSP */
	#undef CONFIG_CMD_BSP
#endif

/*
 * GPMI Nand Configs
 */
#ifndef CONFIG_CMD_MMC	/* NAND conflict with MMC */

#define CONFIG_CMD_NAND

#ifdef CONFIG_CMD_NAND
	#define CONFIG_NAND_GPMI
	#define CONFIG_GPMI_NFC_SWAP_BLOCK_MARK
	#define CONFIG_GPMI_NFC_V1

	#define CONFIG_GPMI_REG_BASE	GPMI_BASE_ADDR
	#define CONFIG_BCH_REG_BASE	BCH_BASE_ADDR

	#define NAND_MAX_CHIPS		8
	#define CONFIG_SYS_NAND_BASE		0x40000000
	#define CONFIG_SYS_MAX_NAND_DEVICE	1
	#define CONFIG_NAND_OBB_ATOMIC	/* OOB area must be access in an atomic
					 * operation in combination with normal data. */

	/* environment */
	#define CONFIG_FSL_ENV_IN_NAND
	#define CONFIG_FLASH_DRIVER_NAME	"gpmi-nfc-main"
	#define CONFIG_BOOTARGS		"gpmi"

	/* partitions */
	#undef CONFIG_JFFS2_CMDLINE
	#define CONFIG_JFFS2_DEV	"nand0"

	#define PART_WINCE_FS_SIZE    	0x0
	#define PART_NETOS_KERNEL_SIZE 	0x002C0000
	#define PART_WINCE_SIZE        	0x02800000
	#define PART_SPLASH_SIZE       	0x00100000
	#define PART_NVRAM_SIZE        	0x00040000	/* nvram 256 kb because 2k pagesize nands */
	#define PART_UBOOT_SIZE	       	0x00300000	/* 3 MB */
	#define PART_KERNEL_SIZE       	0x00500000	/* 5 MB */
	#define PART_ROOTFS_SIZE       	0x01400000	/* 20 MB */
#endif /* CONFIG_CMD_NAND */

/*
 * APBH DMA Configs
 */
#define CONFIG_APBH_DMA

#ifdef CONFIG_APBH_DMA
	#define CONFIG_APBH_DMA_V1
	#define CONFIG_MXS_DMA_REG_BASE ABPHDMA_BASE_ADDR
#endif

#endif

/*
 * Watchdog driver
 */
#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
# define CONFIG_MXS_WATCHDOG			1
#endif

/*
 * Persistent bits driver
 */
//#define CONFIG_MXS_PERSISTENT		1

/**
 * Rescue silent console
 */
#ifdef CONFIG_ENABLE_SILENT_RESCUE
# define ENABLE_CONSOLE_GPIO		PINID_LCD_RESET	/* (3, 30) */
# define CONSOLE_ENABLE_GPIO_STATE	0
#endif

/* Do not check for keypress if bootdelay=0 */
#ifdef CONFIG_ZERO_BOOTDELAY_CHECK
# undef CONFIG_ZERO_BOOTDELAY_CHECK
#endif

/*
 * Environments
 */
#define CONFIG_CMD_ENV

#if defined(CONFIG_FSL_ENV_IN_NAND)
	/* all defines in digi_common.h */
#elif defined(CONFIG_FSL_ENV_IN_MMC)
	#undef CONFIG_ENV_SIZE
	#define CONFIG_ENV_IS_IN_MMC	1
	/* Assoiated with the MMC layout defined in mmcops.c */
	#define CONFIG_ENV_OFFSET               (0x400) /* 1 KB */
	#define CONFIG_ENV_SIZE                 (0x20000 - 0x400) /* 127 KB */
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif

#define CONFIG_EXTRA_ENV_SETTINGS			\
	"silent=no\0"					\
	"ethprime=FEC0\0"

/* The global boot mode will be detected by ROM code and
 * a boot mode value will be stored at fixed address:
 * TO1.0 addr 0x0001a7f0
 * TO1.2 addr 0x00019BF0
 */
#ifndef MX28_EVK_TO1_0
 #define GLOBAL_BOOT_MODE_ADDR 0x00019BF0
#else
 #define GLOBAL_BOOT_MODE_ADDR 0x0001a7f0
#endif
#define BOOT_MODE_SD0 0x9
#define BOOT_MODE_SD1 0xa

#endif /* __WR21_H */
