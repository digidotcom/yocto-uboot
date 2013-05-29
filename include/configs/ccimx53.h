/*
 * Copyright (C) 2011 Digi International, Inc.
 *
 * Configuration settings for the ConnectCore Wi-i.MX53 and
 * ConnectCore i.MX53 Digi Embedded Module.
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
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN		(4*1024*1024)  /*  we need 2 erase blocks and the 2G part has 512KB erase sector size */
#endif

#define CONFIG_UBOOT_LQ070Y3DG3B_TFT_LCD
#define CONFIG_UBOOT_LQ121K1LG11_TFT_LCD

/************************************************************
 * Definition of default options when not configured by user
 ************************************************************/
#ifndef CONFIG_DIGIEL_USERCONFIG
# define CONFIG_BAUDRATE		CONFIG_PLATFORM_BAUDRATE
# define DISPLAYS_ENABLE		1
# define CONFIG_AUTOLOAD_BOOTSCRIPT	1
# ifndef CONFIG_CCIMX5X_ENABLE_EXT_ETH
#  define CONFIG_CCIMX5X_ENABLE_FEC_ETH	1
# endif
# define CONFIG_DISPLAY1_ENABLE		1
# define CONFIG_DISPLAY2_ENABLE		1
# define CONFIG_DISPLAY_LVDS_ENABLE	1
# define CONFIG_JSCCIMX53_JSK		1
# define CONFIG_MX53_UART_1		1
#endif

 /* High Level Configuration Options */
#define CONFIG_ARMV7		/* This is armv7 Cortex-A8 CPU core */
#define CONFIG_MXC
#define CONFIG_MX53
#define CONFIG_CCIMX53			1	/* in a CCIMX53 */
#define CONFIG_CCIMX5X			1	/* in a CCIMX51 or CCIMX53 */
#define CONFIG_PLATFORM_HAS_HWID		/* HWID programmed on the eFuses */
#define CONFIG_HWID_LENGTH		6

#define CONFIG_MX53_CLK32		32768

#define CONFIG_CCIMX53			1	/* in a CCIMX53 */
#define MODULE_COMPOSED_NAME		/* module name composed by parts:
					 * prefix + ... + suffix */
#define MODULENAME_PREFIX		"cc"
#define MODULENAME_SUFFIX		"mx53"

#define CONFIG_MODULE_STRING		"ConnectCore for i.MX53"

#if defined(CONFIG_UBOOT_PROMPT)
#undef CONFIG_SYS_PROMPT
#define	CONFIG_SYS_PROMPT	CONFIG_UBOOT_PROMPT_STR
#endif

#ifndef CONFIG_DOWNLOAD_BY_DEBUGGER
# define CONFIG_FLASH_HEADER		1
# define CONFIG_FLASH_HEADER_OFFSET	0x400
# define CONFIG_SOURCE			1
#else
# undef CONFIG_CMD_AUTOSCRIPT
# undef CONFIG_AUTOLOAD_BOOTSCRIPT
#endif

#define CONFIG_SKIP_RELOCATE_UBOOT

#define CONFIG_ARCH_CPU_INIT
#define CONFIG_ARCH_MMU

#define CONFIG_MX53_HCLK_FREQ		24000000
#define CONFIG_SYS_PLL2_FREQ    	400
#define CONFIG_SYS_AHB_PODF     	2
#define CONFIG_SYS_AXIA_PODF    	0
#define CONFIG_SYS_AXIB_PODF    	1
#define CONFIG_COMPUTE_LPJ		1
#define CONFIG_PRESET_LPJ_1000MHZ	4997120
#define CONFIG_PRESET_LPJ_800MHZ	3997696

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
#define	CONFIG_MODULE_NAME_WCE		"CCXMX53"
#ifndef CONFIG_UBOOT_BOARDNAME
# if defined(CONFIG_JSCCIMX53_JSK)
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
#define CONFIG_PLATFORM_BAUDRATE	115200		/* Default baud rate */

/**
 * User keys
 */
#define USER_KEY1_GPIO			MX53_PIN_GPIO_10
#define USER_KEY2_GPIO			MX53_PIN_GPIO_11
#define CONFIG_USER_KEY12		1

/**
 * Hardware drivers
 */
#define CONFIG_MXC_UART			1
#define CONFIG_SERIAL_MULTI

/**
 * User and status leds
 */
/* #define CONFIG_USER_LED */
#define USER_LED1_GPIO			MX53_PIN_CSI0_DATA_EN
#define USER_LED2_GPIO			MX53_PIN_GPIO_17

/**
 * Serial console selection
 */
#if defined(CONFIG_MX53_UART_1)
# define CONFIG_CONS_INDEX		0
#elif defined(CONFIG_MX53_UART_2)
# define CONFIG_CONS_INDEX		1
#elif defined(CONFIG_MX53_UART_3)
# define CONFIG_CONS_INDEX		2
#elif defined(CONFIG_MX53_UART_4)
# define CONFIG_CONS_INDEX		3
#elif defined(CONFIG_MX53_UART_5)
# define CONFIG_CONS_INDEX		4
#endif

/**
 * PMIC command
 */
#define CONFIG_CMD_PMIC
#define DA905x_I2C_ADDR			0x48
#define DA905x_I2C_ALT_ADDR		0x68
#define CONFIG_USE_PMIC_I2C_WORKARROUND	1

/**
 * RTC PMIC
 */
#define CONFIG_RTC_DA905x		1

/**
 * Watchdog driver
 */
#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
# define CONFIG_MXC_WATCHDOG			1
#endif

/**
 * USB support
 */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX5
#define CONFIG_USB_STORAGE	1
#define CONFIG_PARTITIONS

#define CONFIG_MXC_USB_PORT	1
#define CONFIG_USB_HUB_RESET	MX53_PIN_CSI0_D11 /* GPIO[5, 29] */
#define CONFIG_MXC_USB_PORTSC	(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS	0

#ifdef CONFIG_USB_STORAGE
	#define CONFIG_DOS_PARTITION	1
	#define CONFIG_CMD_FAT		1
	#define CONFIG_CMD_EXT2		1
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
#define MX5X_SWAP_BI_SPARE_BB_POS_4K   183
#define MX5X_SWAP_BI_MAIN_BB_POS_4K    3914
/*
 * Uncomment this section to enalbe the nandrfmt command
 * #define CONFIG_CMD_NAND_RFMT_SWAPBI	1
 */


#define CONFIG_CMD_MMC
#define CONFIG_CMD_ENV

#define CONFIG_CMD_CLOCK
#define CONFIG_REF_CLK_FREQ CONFIG_MX53_HCLK_FREQ

#define CONFIG_CMD_SATA
#undef CONFIG_CMD_IMLS


#define CONFIG_PRIME	"FEC0"

#define CONFIG_ARP_TIMEOUT	200UL


/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_AUTO_COMPLETE

#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM_1	/* memtest works on */
#define CONFIG_SYS_MEMTEST_END		(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE)

#undef	CONFIG_SYS_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_HZ				1000

#define CONFIG_CMDLINE_EDITING	1


/**
 * Eth Configs
 */

/*
 * The i.MX51 and i.MX53 seems to have a hardware "peculiarity" confirmed under
 * U-Boot, RedBoot and Linux: the ethernet Rx signal is reaching the PHY
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

#define CONFIG_NET_MULTI			1
#define CONFIG_ETH_PRIME
#define CONFIG_HAS_ETH1		/* Platform has a second NIC */
#define CONFIG_MII
#define CONFIG_MII_GASKET
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
#define CONFIG_SMC911X_BASE		mx53_io_base_addr
#endif

/*
 * FUSE Configs
 * */
#ifdef CONFIG_CMD_IIM
	#define CONFIG_IMX_IIM
	#define IMX_IIM_BASE		IIM_BASE_ADDR
	#define CONFIG_IIM_HWID_BANK	1
	#define CONFIG_IIM_HWID_ROW	9
#endif

/*
 * I2C Configs
 */
#define CONFIG_CMD_I2C			1
#define CONFIG_HARD_I2C			1
#define CONFIG_I2C_MXC			1
#define CONFIG_SYS_I2C_PORT		I2C3_BASE_ADDR
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		0xfe
#define CONFIG_I2C_SDA_GPIO		MX53_PIN_GPIO_6
#define CONFIG_I2C_SCL_GPIO		MX53_PIN_GPIO_5
#define CONFIG_I2C_RECOVER_STUCK	1
#define CONFIG_CAMERA_RESET_WORKAROUND
#define CONFIG_CAMERA0_RESET_GPIO	MX53_PIN_CSI0_D9	/* GPIO5_27 */
#define CONFIG_CAMERA1_RESET_GPIO	MX53_PIN_CSI0_D8	/* GPIO5_26 */

/*
 * MMC Configs
 */
#ifdef CONFIG_CMD_MMC
	#define CONFIG_MMC				1
	#define CONFIG_GENERIC_MMC
	#define CONFIG_IMX_MMC
	#define CONFIG_SYS_FSL_ESDHC_NUM        2
	#define CONFIG_SYS_FSL_ESDHC_ADDR       0
	#define MMC_CFGS			{{MMC_SDHC2_BASE_ADDR, 1, 1}, {MMC_SDHC3_BASE_ADDR, 1, 1}}
	#define CONFIG_SYS_MMC_ENV_DEV  0
	#define CONFIG_DOS_PARTITION	1
	#define CONFIG_CMD_FAT		1
	#define CONFIG_CMD_EXT2		1
	#define CONFIG_BOOT_PARTITION_ACCESS
	#define CONFIG_EMMC_DDR_PORT_DETECT
	#define CONFIG_EMMC_DDR_MODE
	/* port 1 (ESDHC3) is 8 bit */
	#define CONFIG_MMC_8BIT_PORTS   0x2
	/*
	 * This could be used to use 8 data bits on the SD2
	 * #define CONFIG_MMC_8BIT_PORTS	0x2
	 */
#endif

/*
 * SATA Configs
 */
#ifdef CONFIG_CMD_SATA
  #define CONFIG_DWC_AHSATA
  #define CONFIG_SYS_SATA_MAX_DEVICE      1
  #define CONFIG_DWC_AHSATA_PORT_ID       0
  #define CONFIG_DWC_AHSATA_BASE_ADDR     SATA_BASE_ADDR
  #define CONFIG_LBA48
  #define CONFIG_LIBATA
  #define CONFIG_DOS_PARTITION		1
  #define CONFIG_CMD_FAT		1
  #define CONFIG_CMD_EXT2		1
#endif


/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#define PHYS_SDRAM_1_MAX_SIZE	(512 * 1024 * 1024)
#ifdef CONFIG_CCIMX5_SDRAM_128MB
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1_SIZE		(128 * 1024 * 1024)
#else
#define CONFIG_NR_DRAM_BANKS	2
#define PHYS_SDRAM_1_SIZE		(256 * 1024 * 1024)
#endif

#if CONFIG_NR_DRAM_BANKS > 1
#define PHYS_SDRAM_2		CSD1_BASE_ADDR
#define PHYS_SDRAM_2_SIZE	PHYS_SDRAM_1_SIZE
#define PHYS_SDRAM_2_MAX_SIZE	(512 * 1024 * 1024)
#define iomem_valid_addr(addr, size) \
	((addr >= PHYS_SDRAM_1 && addr <= (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE)) \
	|| (addr >= PHYS_SDRAM_2 && addr <= (PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE)))
#else
#define iomem_valid_addr(addr, size) \
	(addr >= PHYS_SDRAM_1 && addr <= (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))
#endif

/** Load addresses */
#define CONFIG_LOADADDR		0x70800000	/* default load address	*/
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

#if 0
#if defined(CONFIG_FSL_ENV_IN_NAND)
	#define CONFIG_ENV_IS_IN_NAND 1
	#define CONFIG_ENV_OFFSET	0x100000
#elif defined(CONFIG_FSL_ENV_IN_MMC)
	#define CONFIG_ENV_IS_IN_MMC	1
	#define CONFIG_ENV_OFFSET	(768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SATA)
	#define CONFIG_ENV_IS_IN_SATA   1
	#define CONFIG_SATA_ENV_DEV     0
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#elif defined(CONFIG_FSL_ENV_IN_SF)
	#define CONFIG_ENV_IS_IN_SPI_FLASH	1
	#define CONFIG_ENV_SPI_CS		1
	#define CONFIG_ENV_OFFSET       (768 * 1024)
#else
	#define CONFIG_ENV_IS_NOWHERE	1
#endif
#endif


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
#define PART_UBOOT_SIZE	       	0x00100000
#define PART_UBOOT_SIZE_4K	0x00100000
#define PART_KERNEL_SIZE       	0x00300000
#define PART_ROOTFS_SIZE       	0x07800000	/* 120MB so that the default part table fits in 128MB */
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
 * Video support
 */
/* display1 */
#define VIDEO_OPTIONS_DISPLAY1	\
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
	VIDEO_MENU_OPTION("1024x768", set_video_vga, "1024x768"), \
	VIDEO_MENU_OPTION("1280x1024", set_video_vga, "1280x1024"), \
	VIDEO_MENU_OPTION("1680x1050", set_video_vga, "1680x1050")
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
#ifdef CONFIG_UBOOT_SPLASH
# define CONFIG_SPLASH_SCREEN	1
# define CONFIG_VIDEO_MX5
# define CONFIG_MXC_HSC
# define CONFIG_IPU_CLKRATE	200000000

# define CONFIG_IPU_PIX_FMT	IPU_PIX_FMT_RGB24
//# define CONFIG_IPU_PIX_FMT	IPU_PIX_FMT_RGB666

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

/* For the splash support, on big screens we need to malloc a big chunk of memory */
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN		(2048*1024)

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
