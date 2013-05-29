/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX51-3Stack Freescale board.
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

#ifndef __CONFIG_H
#define __CONFIG_H
#include "digi_common.h"		/* Load Digi common stuff... */

#include <asm/arch/mx51.h>

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN		(4*1024*1024)  /*  we need 2 erase blocks and the 2G part has 512KB erase sector size */
#endif


/************************************************************
 * Definition of default options when not configured by user
 ************************************************************/
#ifndef CONFIG_DIGIEL_USERCONFIG
# define CONFIG_BAUDRATE		CONFIG_PLATFORM_BAUDRATE
# define DISPLAYS_ENABLE		1
# define CONFIG_AUTOLOAD_BOOTSCRIPT	1
# ifndef CONFIG_CCIMX5X_ENABLE_EXT_ETH
#  define CONFIG_CCIMX5X_ENABLE_FEC_ETH	1
# else
#  define CONFIG_DISPLAY2_ENABLE	1
# endif
# define CONFIG_DISPLAY1_ENABLE		1
# define CONFIG_JSCCIMX51_JSK		1
# define CONFIG_MX51_UART_2		1
#endif

 /* High Level Configuration Options */
#define CONFIG_ARMV7			1	/* This is armv7 Cortex-A8 CPU core */
#define CONFIG_SYS_APCS_GNU

#define CONFIG_MXC			1
#define CONFIG_CCIMX51			1	/* in a CCIMX51 */
#define CONFIG_CCIMX5X			1	/* in a CCIMX51 or CCIMX53 */
#define CONFIG_ARCH_CPU_INIT
#define CONFIG_ARCH_MMU
//#define CONFIG_ICACHE_L1_OFF
#define CCIMX51_CPU_FREQ_600		1	/* start at 600MHz */
#define CONFIG_PLATFORM_HAS_HWID
#define CONFIG_HWID_LENGTH		6
#define CONFIG_DONT_ENABLE_ALL_CLKS	1


#define CONFIG_CCIMX51			1	/* in a CC-i.MX51 */
#define MODULE_COMPOSED_NAME		/* module name composed by parts:
					 * prefix + ... + suffix */
#define MODULENAME_PREFIX		"cc"
#define MODULENAME_SUFFIX		"mx51"
#define CONFIG_MODULE_STRING		"ConnectCore for i.MX51"

#if defined(CONFIG_UBOOT_PROMPT)
#undef CONFIG_SYS_PROMPT
#define	CONFIG_SYS_PROMPT	CONFIG_UBOOT_PROMPT_STR
#endif

#ifndef CONFIG_DOWNLOAD_BY_DEBUGGER
# define CONFIG_FLASH_HEADER		1
# define CONFIG_FLASH_HEADER_OFFSET	0x400
# define CONFIG_FLASH_HEADER_BARKER	0xB1
# define CONFIG_SOURCE			1
#else
# undef CONFIG_CMD_AUTOSCRIPT
# undef CONFIG_AUTOLOAD_BOOTSCRIPT
#endif

/* Define settings depending on the selected CCIMX51JS board */
#if defined(CONFIG_JSCCIMX51_EAK)
#define BOARD_USER_KEY1_GPIO		MX51_PIN_GPIO1_8
#elif defined(CONFIG_JSCCIMX51_JSK)
#define BOARD_USER_KEY1_GPIO		MX51_PIN_DISPB2_SER_DIO
#define BOARD_USER_KEY1_GPIO_SEL_INP	MUX_IN_GPIO3_IPP_IND_G_IN_6_SELECT_INPUT
#define BOARD_USER_KEY1_GPIO_SINP_PATH	1
#else
#endif

#define CONFIG_SKIP_RELOCATE_UBOOT

#define CONFIG_MX51_HCLK_FREQ		24000000	/* RedBoot says 26MHz */
#define CONFIG_COMPUTE_LPJ		1
#define CONFIG_PRESET_LPJ_800MHZ	3997696
#define CONFIG_PRESET_LPJ_600MHZ	2998272

//#define CONFIG_DISPLAY_CPUINFO
//#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_64BIT_VSPRINTF

#define BOARD_LATE_INIT
#define CONFIG_BOARD_CLEANUP_BEFORE_LINUX
#define CONFIG_USE_BOOT_OPTIMIZATIONS
#define CONFIG_AFTER_FLASH_INIT
#define	BOARD_BEFORE_MLOOP_INIT

/*
 * Disabled for now due to build problems under Debian and a significant
 * increase in the final file size: 144260 vs. 109536 Bytes.
 */

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1
#define CONFIG_SERIAL_TAG		1	/* hwid */


/**
 * Digi commands and defines
 */
#define PLATFORM			"js"
#define CONFIG_PRINT_SYS_INFO		1
#define	CONFIG_MODULE_NAME_WCE		"CCXMX51"
#ifndef CONFIG_UBOOT_BOARDNAME
# if defined(CONFIG_JSCCIMX51_EAK)
#  define CONFIG_UBOOT_BOARDNAME_STR	"an Early Availability Development Board"
# elif defined(CONFIG_JSCCIMX51_JSK)
#  define CONFIG_UBOOT_BOARDNAME_STR	"a JumpStart Kit Development Board"
# else
#  define CONFIG_UBOOT_BOARDNAME_STR	"a custom board"
# endif
#endif
#define CONFIG_IDENT_STRING		" - GCC " __VERSION__ "\nfor " \
					CONFIG_MODULE_STRING " on " CONFIG_UBOOT_BOARDNAME_STR

#ifndef CONFIG_CONSOLE
# define CONFIG_CONSOLE			"ttymxc"
#endif
#define CONFIG_PLATFORM_BAUDRATE	38400

/**
 * User keys
 */
#define USER_KEY1_GPIO			BOARD_USER_KEY1_GPIO
#define USER_KEY2_GPIO			MX51_PIN_GPIO1_1
#define CONFIG_USER_KEY12		1

/**
 * Hardware drivers
 */
#define CONFIG_MXC_UART			1
#define CONFIG_SERIAL_MULTI

/**
 * Serial console selection
 */
#if defined(CONFIG_MX51_UART_1)
# define CONFIG_CONS_INDEX		0
#elif defined(CONFIG_MX51_UART_2)
# define CONFIG_CONS_INDEX		1
#elif defined(CONFIG_MX51_UART_3)
# define CONFIG_CONS_INDEX		2
#endif

/**
 * Rescue silent console
 */
#ifdef CONFIG_ENABLE_SILENT_RESCUE
# define ENCON_GPIO_MUX_MODE	IOMUX_CONFIG_ALT0
# define CONSOLE_ENABLE_GPIO_STATE	0
# if defined(CONFIG_USER_BUTTON1_ENABLE_CONSOLE)
#   define ENABLE_CONSOLE_GPIO		USER_KEY1_GPIO
# elif defined(CONFIG_USER_BUTTON2_ENABLE_CONSOLE)
#   define ENABLE_CONSOLE_GPIO		USER_KEY2_GPIO
# endif
#endif


/**
 * SPI Configs
 */
#define CONFIG_CMD_SPI
#define CONFIG_IMX_ECSPI
#define CONFIG_IMX_SPI_PMIC
#define CONFIG_IMX_SPI_PMIC_CS	0
#define CONFIG_IMX_SPI_PMIC_SPEED	20000000
#define CONFIG_IMX_SPI_SS1_SPEED	2500000
#define	IMX_CSPI_VER_2_3	1
#define MAX_SPI_BYTES		(64 * 4)

/**
 * PMIC command
 */
#define CONFIG_CMD_PMIC

/**
 * RTC PMIC
 */
#define CONFIG_RTC_MC13892	1

/**
 * User and status leds
 */
#define CONFIG_USER_LED
#define USER_LED1_GPIO			MX51_PIN_NANDF_RB2
#define USER_LED2_GPIO			MX51_PIN_NANDF_RB1
#define CONFIG_STATUS_LED

/**
 * I2C Configs
 */
#define CONFIG_CMD_I2C			1
#define CONFIG_HARD_I2C			1
#define CONFIG_I2C_MXC			1
#define CONFIG_SYS_I2C_PORT		I2C2_BASE_ADDR
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		0xfe

/**
 * MMC Configs
 */
#define CONFIG_IMX_MMC		1
#define CONFIG_SYS_FSL_ESDHC_ADDR       0
#define MMC_DEVICES		{0, 2}	/* 0-SD1, 1-SD2, 2-SD3, 3-SD4 */
#define MMC_CFGS		{{MMC_SDHC1_BASE_ADDR, 1, 1}, {MMC_SDHC3_BASE_ADDR, 1, 1}}
#define CONFIG_MULTI_MMC	1
#define CONFIG_MMC              1
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC
#define CONFIG_DOS_PARTITION	1
#define CONFIG_CMD_FAT		1
#define CONFIG_CMD_EXT2		1

/**
 * Eth Configs
 */

/*
 * The MX51 3stack board seems to have a hardware "peculiarity" confirmed under
 * U-Boot, RedBoot and Linux: the ethernet Rx signal is reaching the CS8900A
 * controller inverted. The controller is capable of detecting and correcting
 * this, but it needs 4 network packets for that. Which means, at startup, you
 * will not receive answers to the first 4 packest, unless there have been some
 * broadcasts on the network, or your board is on a hub. Reducing the ARP
 * timeout from default 5 seconds to 200ms we speed up the initial TFTP
 * transfer, should the user wish one, significantly.
 */
#define CONFIG_VARIABLE_ARP_TIMEOUT		1
#ifdef CONFIG_VARIABLE_ARP_TIMEOUT
#define CONFIG_INITIAL_ARP_TIMEOUT		100UL
#define CONFIG_STANDARD_ARP_TIMEOUT		5000UL
#endif

#define CONFIG_ETH_PRIME
#define CONFIG_HAS_ETH1		/* Platform has a second NIC */
#define CONFIG_NET_MULTI			1
#define CONFIG_MII
#define CONFIG_DISCOVER_PHY
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_DNS
#define CONFIG_BOOTP_DNS2

/* Support for internal Fast Ethernet Controller */
#ifdef CONFIG_CCIMX5X_ENABLE_FEC_ETH
#define CONFIG_MXC_FEC
#define CONFIG_FEC0_IOBASE			FEC_BASE_ADDR
#define CONFIG_FEC0_PINMUX			-1
#define CONFIG_FEC0_PHY_ADDR			7
#define CONFIG_FEC0_MIIBASE 			-1
#endif

/* Support for external Ethernet Controller LAN9221 */
#ifdef CONFIG_CCIMX5X_ENABLE_EXT_ETH
#define CONFIG_SMC911X			1
#define CONFIG_SMC911X_16_BIT		1
#define CONFIG_SMC911X_BASE		mx51_io_base_addr
#endif

/**
 * Watchdog driver
 */
#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
# define CONFIG_MXC_WATCHDOG			1
#endif

/***********************************************************
 * Command definition
 ***********************************************************/

#include <config_cmd_default.h>

#define CONFIG_CMD_NAND
#define CONFIG_MXC_NAND
#undef	CONFIG_CMD_FLASH
#define CONFIG_CMD_AUTOSCRIPT
#define CONFIG_CMD_IIM
#define CONFIG_MXC_NAND_SWAP_BI		1
#define MX5X_SWAP_BI_SPARE_BB_POS	49
#define MX5X_SWAP_BI_MAIN_BB_POS	2000
#define CONFIG_NAND_OBB_ATOMIC		/* OOB area must be access in an atomic
					 * operation in combination with normal data. */
#define MX5X_SWAP_BI_SPARE_BB_POS_4K	183
#define MX5X_SWAP_BI_MAIN_BB_POS_4K	3914
/*
 * Uncomment this section to enalbe the nandrfmt command
 * #define CONFIG_CMD_NAND_RFMT_SWAPBI	1
 */

#undef CONFIG_CMD_IMLS

/**
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_AUTO_COMPLETE

#define CONFIG_SYS_MEMTEST_START	0x98000000	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		(0x90000000 + PHYS_SDRAM_1_SIZE)	/* This should be dynamic */

#undef	CONFIG_SYS_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ			1000

#define CONFIG_CMDLINE_EDITING	1

/*
 * FUSE Configs
 * */
#ifdef CONFIG_CMD_IIM
# define CONFIG_IMX_IIM
# define IMX_IIM_BASE		IIM_BASE_ADDR
# define CONFIG_IIM_HWID_BANK	1
# define CONFIG_IIM_HWID_ROW	9
#endif

/*
 * CLK commands
 * */
#define CONFIG_CMD_CLOCK
#define CONFIG_REF_CLK_FREQ CONFIG_MX51_HCLK_FREQ

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM_1			CSD0_BASE_ADDR
#ifdef CONFIG_CCIMX5_SDRAM_128MB
#define PHYS_SDRAM_1_SIZE		(128 * 1024 * 1024)
#else
#define PHYS_SDRAM_1_SIZE		(512 * 1024 * 1024)
#endif
#define iomem_valid_addr(addr, size) \
	(addr >= PHYS_SDRAM_1 && addr <= (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))

/** Load addresses */
#define CONFIG_LOADADDR		0x90800000	/* default load address	*/
/* NOTE: Digi BSP Will adjust 'loadaddr' depending on the
 * available RAM memory of the platform to
 * allow transfers of large files to RAM */
#define CONFIG_INITRD_LOADADDR_OFFSET	0x800000 /* Address where to put the
						  * the initrd: 8MiB after the
						  * kernel load address */
/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH

/*-----------------------------------------------------------------------
 * NAND FLASH driver setup
 */
#define NAND_MAX_CHIPS			8
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000

/**
 * JFFS2 partitions
 */
#undef CONFIG_JFFS2_CMDLINE
#define CONFIG_JFFS2_DEV	"nand0"

#define PART_WINCE_FS_SIZE    	0x0
#define PART_NETOS_KERNEL_SIZE 	0x002C0000
#define PART_WINCE_SIZE        	0x02800000
#define PART_SPLASH_SIZE       	0x00100000
#define PART_NVRAM_SIZE        	0x00040000	/* nvram 256 kb because 2k pagesize nands */
#define PART_UBOOT_SIZE	       	0x000c0000
#define PART_UBOOT_SIZE_4K	0x00100000
#define PART_KERNEL_SIZE       	0x00300000
#define PART_ROOTFS_SIZE	0x07800000	/* 120MB so that the default part table fits in 128MB */
#define PART_ROOTFS_SIZE_LARGE	0x0C800000	/* 200MB for flashes bigger than FLASH_SIZE_SMALLEST */
/* A flash bigger than this size will default the ROOTFS partition size to
 * the PART_ROOTFS_SIZE_LARGE value instead of PART_ROOTFS_SIZE
 */
#define FLASH_SIZE_SMALLEST	0x08000000	/* 128MB is the smallest variant */

/**
 * UBI
 */
#define CONFIG_CMD_UBI		1

#ifdef CONFIG_CMD_UBI
# define CONFIG_CMD_MTDPARTS	1
# define CONFIG_MTD_PARTITIONS	1
# define CONFIG_MTD_DEVICE	1
# define CONFIG_RBTREE		1
#endif

/**
 * Environment
 */
#if defined(CONFIG_CCIMX5X_ENABLE_FEC_ETH)
# define CONFIG_ENV_ETHPRIME		"ethprime=FEC0\0"
#elif defined(CONFIG_CCIMX5X_ENABLE_EXT_ETH)
# define CONFIG_ENV_ETHPRIME		"ethprime=smc911x-0\0"
#else
# define CONFIG_ENV_ETHPRIME		""
#endif

#define CONFIG_EXTRA_ENV_SETTINGS			\
	"silent="CFG_SET_SILENT"\0"			\
	CONFIG_ENV_ETHPRIME

/**
 * USB
 */
#define CONFIG_CMD_USB         1

#ifdef CONFIG_CMD_USB
# define CONFIG_USB_EHCI
# define CONFIG_USB_EHCI_IMX51
# define CONFIG_SYS_USB_EHCI_REGS_BASE  0x73F80300
# define CONFIG_SYS_USB_EHCI_CPU_INIT

# ifdef CONFIG_ARCH_MMU
#  define CONFIG_EHCI_DCACHE
# endif // CONFIG_ARCH_MMU

# define CONFIG_USB_STORAGE
# define CONFIG_PARTITIONS
# if !defined(CONFIG_DOS_PARTITION)
#  define CONFIG_DOS_PARTITION
# endif // CONFIG_DOS_PARTITION
#endif // CONFIG_CMD_USB

/**
 * Video support
 */
/* display1 */
#define VIDEO_OPTIONS_DISPLAY1	\
	VIDEO_MENU_OPTION("VGA", select_video_vga, NULL), \
	VIDEO_MENU_OPTION("HDMI", select_video_hdmi, NULL), \
	VIDEO_MENU_OPTION("LCD display", select_video_lcd, NULL)
/* display2 */
#define VIDEO_OPTIONS_DISPLAY2	\
	VIDEO_MENU_OPTION("VGA", select_video_vga, NULL), \
	VIDEO_MENU_OPTION("HDMI", select_video_hdmi, NULL), \
	VIDEO_MENU_OPTION("LCD display", select_video_lcd, NULL)
/* vga */
#define VIDEO_OPTIONS_VGA	\
	VIDEO_MENU_OPTION("640x480", set_video_vga, "640x480"), \
	VIDEO_MENU_OPTION("800x600", set_video_vga, "800x600"), \
	VIDEO_MENU_OPTION("1024x768", set_video_vga, "1024x768")
/* hdmi */
#define VIDEO_OPTIONS_HDMI	\
	VIDEO_MENU_OPTION("800x600", set_video_hdmi, "800x600"), \
	VIDEO_MENU_OPTION("1024x768", set_video_hdmi, "1024x768"), \
	VIDEO_MENU_OPTION("1280x720", set_video_hdmi, "1280x720"), \
	VIDEO_MENU_OPTION("1360x768", set_video_hdmi, "1360x768"), \
	VIDEO_MENU_OPTION("1366x768", set_video_hdmi, "1366x768"), \
	VIDEO_MENU_OPTION("1920x1080", set_video_hdmi, "1920x1080")
/* lcd */
#define VIDEO_OPTIONS_LCD	\
	VIDEO_MENU_OPTION("LQ070Y3DG3B (800x480 JumpStart Kit display)", set_video_lcd, "LQ070Y3DG3B"), \
	VIDEO_MENU_OPTION("LQ064V3DG01 (640x480)", set_video_lcd, "LQ064V3DG01"), \
	VIDEO_MENU_OPTION("LQ121K1LG11 and LQ121K1LG52 (1280x800)", set_video_lcd, "LQ121K1LG11"), \
	VIDEO_MENU_OPTION("LQ106K1LA05 (1280x768)", set_video_lcd, "LQ106K1LA05"), \
	VIDEO_MENU_OPTION("LQ104V1DG62 (640x480)", set_video_lcd, "LQ104V1DG62"), \
	VIDEO_MENU_OPTION("HSD101PFW2 (1024x600)", set_video_lcd, "HSD101PFW2"), \
	VIDEO_MENU_OPTION("F04B0101 (480x272)", set_video_lcd, "F04B0101"), \
	VIDEO_MENU_OPTION("F07A0102 (800x480)", set_video_lcd, "F07A0102")

/**
 * Splash screen
 */

#define CONFIG_UBOOT_LQ070Y3DG3B_TFT_LCD	1

#ifdef CONFIG_UBOOT_SPLASH
# define CONFIG_SPLASH_SCREEN	1
# define CONFIG_VIDEO_MX5
# define CONFIG_MXC_HSC
# define CONFIG_IPU_CLKRATE	133000000

# if defined(CONFIG_JSCCIMX51_EAK)
#  define CONFIG_IPU_PIX_FMT	IPU_PIX_FMT_RGB24
# elif defined(CONFIG_JSCCIMX51_JSK)
#  define CONFIG_IPU_PIX_FMT	IPU_PIX_FMT_RGB666
# else
#  error "Please define the IPU pixel format"
# endif

# define CONFIG_CMD_BMP
# define CONFIG_BMP_8BPP

# ifdef CONFIG_DISPLAY1_ENABLE
#  define CONFIG_LCD		1
#  if defined(CONFIG_UBOOT_CRT_VGA) || defined(CONFIG_UBOOT_LQ070Y3DG3B_TFT_LCD)
#   define LCD_BPP		LCD_COLOR16
#  else
#   error "Please, define LCD_BPP accordingly to your display needs"
#  endif
# else
#  error "Splash screen support is only for Display1"
# endif

#endif /* CONFIG_UBOOT_SPLASH */

/**
 * Dual boot support
 */
#ifdef CONFIG_DUAL_BOOT
/* Platform does not have persistent media
 * and NVRAM will be used instead
 */
# define CONFIG_DUAL_BOOT_PERSISTENT_IN_NVRAM
#endif

#endif				/* __CONFIG_H */
