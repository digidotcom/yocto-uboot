/*
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on m28evk.h:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
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
 */
#ifndef __CCARDIMX28JS_CONFIG_H__
#define __CCARDIMX28JS_CONFIG_H__

#include "digi_common.h"		/* Load Digi common stuff... */

/*
 * SoC configurations
 */
#define CONFIG_MX28				/* i.MX28 SoC */

#define CONFIG_MXS_GPIO			/* GPIO control */
#define CONFIG_SYS_HZ		1000		/* Ticks per second */

#define CONFIG_MACH_TYPE	MACH_TYPE_CCARDWMX28JS

#include <asm/arch/regs-base.h>

#define CONFIG_SYS_NO_FLASH
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_ARCH_MISC_INIT
#define CONFIG_BOARD_LATE_INIT

/*
 * Digi custom
 */
#define MODULE_COMPOSED_NAME		/* module name composed by parts:
					 * prefix + ... + suffix + platform */
#define MODULENAME_PREFIX		"ccard"
#define MODULENAME_SUFFIX		"mx28"
#define CONFIG_MODULE_STRING		"ConnectCard for i.MX28"
#define PLATFORM			"js"

#define CONFIG_CMD_BOOTSTREAM
#define CONFIG_HAB_ENABLED
#define	CONFIG_CMD_BSP
#define CONFIG_IDENT_STRING             " - GCC " __VERSION__ "\nfor " \
                                        CONFIG_MODULE_STRING
#define CONFIG_DIGI_CMD
#define CONFIG_MXS_OTP
#define CONFIG_HAS_HWID
#define CONFIG_HWID_LENGTH		8
#define CONFIG_BOARD_BEFORE_MLOOP_INIT
#define CONFIG_CMD_NVRAM
#define CONFIG_ENV_IS_IN_DIGI_NVRAM
#define CONFIG_TFTP_UPDATE_ONTHEFLY	/* support to tftp and update on-the-fly */

/*
 * SPL
 */
#define CONFIG_SPL
#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH	"arch/arm/cpu/arm926ejs/mxs"
#define CONFIG_SPL_LDSCRIPT	"arch/arm/cpu/arm926ejs/mxs/u-boot-spl.lds"
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT

/*
 * CPU frequency configuration (determines EMI/HBUS frequencies)
 */
#define CONFIG_CPU_FREQ		454

/*
 * U-Boot Commands
 */
#include <config_cmd_default.h>
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DOS_PARTITION

#define CONFIG_CMD_CACHE
#define CONFIG_CMD_DATE
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_FAT
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_MII
#define CONFIG_CMD_MMC
#define CONFIG_CMD_NET
#define CONFIG_CMD_NFS
#define CONFIG_CMD_PING
#define CONFIG_CMD_SETEXPR
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI
#define CONFIG_CMD_USB
#define CONFIG_CMD_BOOTZ
#define CONFIG_CMD_I2C
#define	CONFIG_CMD_SNTP

/*
 * Memory configurations
 */
#define CONFIG_NR_DRAM_BANKS		1		/* 1 bank of DRAM */
#define PHYS_SDRAM_1			0x40000000	/* Base address */
#define PHYS_SDRAM_1_SIZE		0x40000000	/* Max 1 GB RAM */
#define CONFIG_SYS_MALLOC_LEN		0x00400000	/* 4 MB for malloc */
#define CONFIG_SYS_MEMTEST_START	0x40000000	/* Memtest start adr */
#define CONFIG_SYS_MEMTEST_END		0x40400000	/* 4 MB RAM test */
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
/* Point initial SP in SRAM so SPL can use it too. */

#define CONFIG_SYS_INIT_RAM_ADDR	0x00000000
#define CONFIG_SYS_INIT_RAM_SIZE	(128 * 1024)

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/*
 * We need to sacrifice first 4 bytes of RAM here to avoid triggering some
 * strange BUG in ROM corrupting first 4 bytes of RAM when loading U-Boot
 * binary. In case there was more of this mess, 0x100 bytes are skipped.
 */
#define CONFIG_SYS_TEXT_BASE	0x40000100

#define CONFIG_ENV_OVERWRITE
/*
 * U-Boot general configurations
 */
#define	CONFIG_SYS_LONGHELP
#define CONFIG_SYS_CBSIZE	2048			/* Console I/O Buffer Size */
#define CONFIG_VERSION_VARIABLE	/* U-BOOT version */
#define CONFIG_AUTO_COMPLETE		/* Command auto complete */
#define CONFIG_CMDLINE_EDITING		/* Command history etc */
#define CONFIG_SYS_HUSH_PARSER

/*
 * Serial Driver
 */
#define CONFIG_PL011_SERIAL
#define CONFIG_PL011_CLOCK		24000000
#define CONFIG_PL01x_PORTS		{ (void *)MXS_UARTDBG_BASE }
#define CONFIG_CONS_INDEX		0
#define CONFIG_BAUDRATE			115200	/* Default baud rate */
#define CONFIG_UARTDBG_CONSOLE		"ttyAMA"

#define CONFIG_CONSOLE			CONFIG_UARTDBG_CONSOLE
#define CONFIG_CONS_INDEX		0

/*
 * DMA
 */
#define CONFIG_APBH_DMA

/*
 * MMC Driver
 */
#ifdef CONFIG_ENV_IS_IN_MMC
 #define CONFIG_ENV_OFFSET	(256 * 1024)
 #define CONFIG_ENV_SIZE	(16 * 1024)
 #define CONFIG_SYS_MMC_ENV_DEV 0
#endif
#define CONFIG_CMD_SAVEENV
#ifdef	CONFIG_CMD_MMC
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_MXS_MMC
#define CONFIG_MMC_NO_CARDDETECT
#endif

/*
 * NAND Driver
 */
#define CONFIG_CMD_NAND
#ifdef CONFIG_CMD_NAND
#define CONFIG_NAND_MXS
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x60000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define PART_NVRAM_SIZE        	0x00040000	/* nvram 256 kb because 2k pagesize nands */
#define PART_UBOOT_SIZE	       	0x00300000	/* 3 MB */
#define PART_KERNEL_SIZE       	0x00500000	/* 5 MB */
#define PART_FDT_SIZE       	0x00080000	/* 512 KiB */
#define PART_ROOTFS_SIZE       	0x07300000	/* 115 MB */
#define CONFIG_FLASH_DRIVER_NAME	"gpmi-nand"
#define CONFIG_NAND_OBB_ATOMIC	/* OOB area must be access in an atomic
				 * operation in combination with normal data. */
#endif

/*
 * Ethernet on SOC (FEC)
 */
#ifdef	CONFIG_CMD_NET
#define CONFIG_NET_MULTI
#define CONFIG_ETHPRIME	"FEC0"
#define CONFIG_FEC_MXC
#define CONFIG_FEC_MXC_MULTI
#define CONFIG_MII
#define CONFIG_FEC_XCV_TYPE	RMII
#define CONFIG_FEC1_INIT_ONLY_MAC
#endif

/*
 * RTC
 */
#ifdef	CONFIG_CMD_DATE
#define	CONFIG_RTC_MXS
#endif

/*
 * USB
 */
#ifdef	CONFIG_CMD_USB
#define	CONFIG_USB_EHCI
#define	CONFIG_USB_EHCI_MXS
#define	CONFIG_EHCI_MXS_PORT 1
#define	CONFIG_EHCI_IS_TDI
#define	CONFIG_USB_STORAGE
#define	CONFIG_USB_HOST_ETHER
#define	CONFIG_USB_ETHER_ASIX
#define	CONFIG_USB_ETHER_SMSC95XX
#endif

/* I2C */
#ifdef CONFIG_CMD_I2C
#define CONFIG_I2C_MXS
#define CONFIG_HARD_I2C
#define CONFIG_SYS_I2C_SPEED	400000
#endif

/*
 * SPI
 */
#ifdef CONFIG_CMD_SPI
#define CONFIG_HARD_SPI
#define CONFIG_MXS_SPI
#define CONFIG_MXS_SPI_DMA_ENABLE
#define CONFIG_SPI_HALF_DUPLEX
#define CONFIG_DEFAULT_SPI_BUS		2
#define CONFIG_DEFAULT_SPI_MODE		SPI_MODE_0

/* User keys */
#define USER_KEY1_GPIO			MX28_PAD_AUART2_RTS__GPIO_3_11
#define USER_KEY2_GPIO			MX28_PAD_SSP0_DETECT__GPIO_2_9

/* SPI Flash */
#ifdef CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SF_DEFAULT_BUS	2
#define CONFIG_SF_DEFAULT_CS	0
/* this may vary and depends on the installed chip */
#define CONFIG_SPI_FLASH_SST
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		24000000

/* (redundant) environemnt in SPI flash */
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_SIZE			0x1000		/* 4KB */
#define CONFIG_ENV_OFFSET		0x40000		/* 256K */
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_ENV_SECT_SIZE		0x1000
#define CONFIG_ENV_SPI_CS		0
#define CONFIG_ENV_SPI_BUS		2
#define CONFIG_ENV_SPI_MAX_HZ		24000000
#define CONFIG_ENV_SPI_MODE		SPI_MODE_0
#endif
#endif
#endif

/**
 * UBI
 */
#define CONFIG_CMD_UBI

#ifdef CONFIG_CMD_UBI
# define CONFIG_CMD_MTDPARTS
# define CONFIG_MTD_PARTITIONS
# define CONFIG_MTD_DEVICE
# define CONFIG_RBTREE
#endif

/*
 * Boot Linux
 */
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_BOOTDELAY	4
#define CONFIG_BOOTFILE		"uImage"
#define CONFIG_FDT_LOADADDR	0x40700000
#define CONFIG_LOADADDR		0x40800000
/* NOTE: Digi BSP Will adjust 'loadaddr' depending on the
 * available RAM memory of the platform to
 * allow transfers of large files to RAM */
#define CONFIG_INITRD_LOADADDR_OFFSET	0x800000 /* Address where to put the
						  * the initrd: 8MiB after the
						  * kernel load address */
#define CONFIG_FDT_PREFIX	"imx28-"
#define CONFIG_FDT_SUFFIX	".dtb"
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR
#define CONFIG_OF_LIBFDT
#define CONFIG_OF_BOARD_SETUP

/*
 * Extra Environments
 */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"update_nand_full_filename=u-boot.nand\0" \
	"update_nand_firmware_filename=u-boot.sb\0"	\
	"update_sd_firmware_filename=u-boot.sd\0" \
	"update_nand_firmware_maxsz=0x100000\0"	\
	"update_nand_stride=0x40\0"	/* MX28 datasheet ch. 12.12 */ \
	"update_nand_count=0x4\0"	/* MX28 datasheet ch. 12.12 */ \
	"update_nand_get_fcb_size="	/* Get size of FCB blocks */ \
		"nand device 0 ; " \
		"nand info ; " \
		"setexpr fcb_sz ${update_nand_stride} * ${update_nand_count};" \
		"setexpr update_nand_fcb ${fcb_sz} * ${nand_writesize}\0" \
	"update_nand_full="		    /* Update FCB, DBBT and FW */ \
		"if tftp ${update_nand_full_filename} ; then " \
		"run update_nand_get_fcb_size ; " \
		"nand scrub -y 0x0 ${filesize} ; " \
		"nand write.raw ${loadaddr} 0x0 ${update_nand_fcb} ; " \
		"setexpr update_off ${loadaddr} + ${update_nand_fcb} ; " \
		"setexpr update_sz ${filesize} - ${update_nand_fcb} ; " \
		"nand write ${update_off} ${update_nand_fcb} ${update_sz} ; " \
		"fi\0" \
	"update_nand_firmware="		/* Update only firmware */ \
		"if tftp ${update_nand_firmware_filename} ; then " \
		"run update_nand_get_fcb_size ; " \
		"setexpr fcb_sz ${update_nand_fcb} * 2 ; " /* FCB + DBBT */ \
		"setexpr fw_sz ${update_nand_firmware_maxsz} * 2 ; " \
		"setexpr fw_off ${fcb_sz} + ${update_nand_firmware_maxsz};" \
		"nand erase ${fcb_sz} ${fw_sz} ; " \
		"nand write ${loadaddr} ${fcb_sz} ${filesize} ; " \
		"nand write ${loadaddr} ${fw_off} ${filesize} ; " \
		"fi\0" \
	"update_sd_firmware="		/* Update the SD firmware partition */ \
		"if mmc rescan ; then "	\
		"if tftp ${update_sd_firmware_filename} ; then " \
		"setexpr fw_sz ${filesize} / 0x200 ; "	/* SD block size */ \
		"setexpr fw_sz ${fw_sz} + 1 ; "	\
		"mmc write ${loadaddr} 0x800 ${fw_sz} ; " \
		"fi ; "	\
		"fi\0" \
	"script=boot.scr\0"	\
	"uimage=uImage\0" \
	"console_fsl=ttyAM0\0" \
	"console_mainline=ttyAMA0\0" \
	"fdtaddr=" MK_STR(CONFIG_FDT_LOADADDR) "\0" \
	"mmcdev=0\0" \
	"mmcpart=2\0" \
	"mmcroot=/dev/mmcblk0p3 rw\0" \
	"mmcrootfstype=ext3 rootwait\0"	\
	"mmcargs=setenv bootargs console=${console_mainline},${baudrate} " \
		"root=${mmcroot} " \
		"rootfstype=${mmcrootfstype}\0"	\
	"loadbootscript="  \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; "	\
		"source\0" \
	"loaduimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${uimage}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; "	\
		"if fatload mmc ${mmcdev}:${mmcpart} ${fdtaddr} ${fdtimg}; then " \
			"bootm ${loadaddr} - ${fdtaddr}; " \
		"else " \
			"bootm; " \
		"fi;\0" \
	"netargs=setenv bootargs console=${console_mainline},${baudrate} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; "	\
		"dhcp ${uimage}; " \
		"if dhcp ${fdtaddr} ${fdtimg}; then " \
			"bootm ${loadaddr} - ${fdtaddr}; " \
		"else " \
			"bootm; " \
		"fi;\0"

#define CONFIG_BOOTCOMMAND \
	"if mmc rescan ${mmcdev}; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loaduimage; then " \
				"run mmcboot; " \
			"else run netboot; " \
			"fi; " \
		"fi; " \
	"else run netboot; fi"

#endif /* __CCARDIMX28JS_CONFIG_H__ */
