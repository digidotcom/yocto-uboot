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

#ifndef __CCIMX6_COMMON_CONFIG_H
#define __CCIMX6_COMMON_CONFIG_H

#define CONFIG_MX6

#include "mx6_common.h"
#include "digi_common.h"		/* Load Digi common stuff... */

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO_LATE

#include <asm/arch/imx-regs.h>

#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 2 * 1024 * 1024)
#define CONFIG_SYS_L2CACHE_OFF

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_MXC_GPIO

#define CONFIG_MXC_UART

/* MMC Configs */
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR      0

#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_EMMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_DOS_PARTITION
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_GPT
#define CONFIG_PARTITION_UUIDS
#define CONFIG_CMD_PART

#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_ETHPRIME			"FEC"
#define CONFIG_NO_MAC_FROM_OTP

#define CONFIG_PHYLIB

/* protected environment variables (besides ethaddr and serial#) */
#define CONFIG_ENV_FLAGS_LIST_STATIC	\
	"wlanaddr:mo,"			\
	"btaddr:mo,"			\
	"bootargs_once:sr"

#define CONFIG_CONS_INDEX              1
#define CONFIG_BAUDRATE                        115200

/* Command definition */
#include <config_cmd_default.h>

#define CONFIG_CMD_BMODE
#define CONFIG_CMD_BOOTZ
#undef CONFIG_CMD_IMLS
#define CONFIG_CMD_SETEXPR

#define CONFIG_CMD_FUSE
#ifdef CONFIG_CMD_FUSE
#define CONFIG_MXC_OCOTP
#endif
#define CONFIG_PLATFORM_HAS_HWID
#define CONFIG_HWID_BANK		4
#define CONFIG_HWID_START_WORD		2
#define CONFIG_HWID_WORDS_NUMBER	2

#define CONFIG_CMD_UPDATE
/* On the fly update chunk (must be a multiple of mmc block size) */
#define CONFIG_OTF_CHUNK		(32 * 1024 * 1024)
#define CONFIG_CMD_DBOOT

#define CONFIG_SUPPORTED_SOURCES	((1 << SRC_TFTP) | \
					 (1 << SRC_NFS) | \
					 (1 << SRC_MMC))
#define CONFIG_SUPPORTED_SOURCES_NET	"tftp|nfs"
#define CONFIG_SUPPORTED_SOURCES_BLOCK	"mmc"
#define CONFIG_SUPPORTED_SOURCES_LIST	CONFIG_SUPPORTED_SOURCES_NET "|" \
					CONFIG_SUPPORTED_SOURCES_BLOCK
#define CONFIG_SUPPORTED_SOURCES_ARGS	DIGICMD_SRC_NET_ARGS \
					DIGICMD_SRC_BLOCK_ARGS

#define CONFIG_BOOTDELAY               1

#define CONFIG_LOADADDR			0x12000000
#define CONFIG_SYS_TEXT_BASE		0x17800000

/* Pool of randomly generated UUIDs at host machine */
#define RANDOM_UUIDS	\
	"uuid_disk=075e2a9b-6af6-448c-a52a-3a6e69f0afff\0" \
	"part1_uuid=43f1961b-ce4c-4e6c-8f22-2230c5d532bd\0" \
	"part2_uuid=f241b915-4241-47fd-b4de-ab5af832a0f6\0" \
	"part3_uuid=1c606ef5-f1ac-43b9-9bb5-d5c578580b6b\0" \
	"part4_uuid=c7d8648b-76f7-4e2b-b829-e95a83cc7b32\0" \
	"part5_uuid=ebae5694-6e56-497c-83c6-c4455e12d727\0" \
	"part6_uuid=3845c9fc-e581-49f3-999f-86c9bab515ef\0" \
	"part7_uuid=3fcf7bf1-b6fe-419d-9a14-f87950727bc0\0" \
	"part8_uuid=12c08a28-fb40-430a-a5bc-7b4f015b0b3c\0" \
	"part9_uuid=dc83dea8-c467-45dc-84eb-5e913daec17e\0"

/* Helper strings for extra env settings */
#define CALCULATE_FILESIZE_IN_BLOCKS	\
	"setexpr filesizeblks ${filesize} / 200; " \
	"setexpr filesizeblks ${filesizeblks} + 1; "

#if defined(CONFIG_SYS_BOOT_NAND)
	/*
	 * The partions' layout for NAND is:
	 *     mtd0: 16M      (uboot)
	 *     mtd1: 16M      (kernel)
	 *     mtd2: 16M      (dtb)
	 *     mtd3: left     (rootfs)
	 */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_addr=0x18000000\0" \
	"fdt_high=0xffffffff\0"	  \
	"bootargs=console=" CONFIG_CONSOLE_DEV ",115200 ubi.mtd=3 "  \
		"root=ubi0:rootfs rootfstype=ubifs "		     \
		"mtdparts=gpmi-nand:16m(boot),16m(kernel),16m(dtb),-(rootfs)\0"\
	"bootcmd=nand read ${loadaddr} 0x1000000 0x800000;"\
		"nand read ${fdt_addr} 0x2000000 0x100000;"\
		"bootm ${loadaddr} - ${fdt_addr}\0"

#elif defined(CONFIG_SYS_BOOT_SATA)

#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_addr=0x18000000\0" \
	"fdt_high=0xffffffff\0"   \
	"bootargs=console=" CONFIG_CONSOLE_DEV ",115200 \0"\
	"bootargs_sata=setenv bootargs ${bootargs} " \
		"root=/dev/sda1 rootwait rw \0" \
	"bootcmd_sata=run bootargs_sata; sata init; " \
		"sata read ${loadaddr} 0x800  0x4000; " \
		"sata read ${fdt_addr} 0x8000 0x800; " \
		"bootm ${loadaddr} - ${fdt_addr} \0" \
	"bootcmd=run bootcmd_sata \0"
#else
#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_DEFAULT_NETWORK_SETTINGS \
	RANDOM_UUIDS \
	"script=boot.scr\0" \
	"loadscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script}\0" \
	"uimage=uImage-" CONFIG_SYS_BOARD "\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"fdt_addr=0x18000000\0" \
	"initrd_addr=0x19000000\0" \
	"initrd_file=uramdisk.img\0" \
	"boot_fdt=try\0" \
	"bootscript=" CONFIG_BOOTSCRIPT "\0" \
	"ip_dyn=yes\0" \
	"console=" CONFIG_CONSOLE_DEV "\0" \
	"fdt_high=0xffffffff\0"	  \
	"initrd_high=0xffffffff\0" \
	"mmcbootpart=" __stringify(CONFIG_SYS_BOOT_PART) "\0" \
	"mmcpart=1\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} ${smp} " \
		"root=/dev/mmcblk0p2 rootwait rw\0" \
	"loaduimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${uimage}\0" \
	"loadinitrd=fatload mmc ${mmcdev}:${mmcpart} ${initrd_addr} ${initrd_file}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if run loaduimage; then " \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if run loadfdt; then " \
					"bootm ${loadaddr} - ${fdt_addr}; " \
				"else " \
					"if test ${boot_fdt} = try; then " \
						"bootm; " \
					"else " \
						"echo WARN: Cannot load the DT; " \
					"fi; " \
				"fi; " \
			"else " \
				"bootm; " \
			"fi;" \
		"else " \
			"echo ERR: Cannot load the kernel; " \
		"fi;\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${rootpath},v3,tcp \0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${uimage}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"bootm ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootm; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootm; " \
		"fi;\0" \
	"uboot_file=u-boot-" CONFIG_SYS_BOARD ".imx\0" \
	"parts_android=\"uuid_disk=${uuid_disk};" \
		"start=2MiB," \
		"name=android,size=64MiB,uuid=${part1_uuid};" \
		"name=android2,size=64MiB,uuid=${part2_uuid};" \
		"name=system,size=512MiB,uuid=${part3_uuid};" \
		"name=system2,size=512MiB,uuid=${part4_uuid};" \
		"name=cache,size=32MiB,uuid=${part5_uuid};" \
		"name=data,size=-,uuid=${part6_uuid};" \
		"\"\0" \
	"android_file=boot.img\0" \
	"system_file=system.img\0" \
	"partition_mmc_android=mmc rescan;" \
		"if mmc dev ${mmcdev} 0;then;else mmc dev ${mmcdev};fi;" \
		"gpt write mmc ${mmcdev} ${parts_android};" \
		"mmc rescan\0" \
	"bootargs_android=\"androidboot.console=ttymxc0 " \
		"androidboot.hardware=freescale fbmem=28M vmalloc=400M\"\0" \
	"bootargs_mmc_android=setenv bootargs console=${console},${baudrate} " \
		"${bootargs_android} androidboot.mmcdev=${mmcdev} " \
		"video=mxcfb0:${video0} " \
		"video=mxcfb1:${video1} " \
		"video=mxcfb2:${video2} " \
		"${bootargs_once} ${std_bootargs}\0" \
	"bootargs_tftp_android=setenv bootargs console=${console},${baudrate} " \
		"${bootargs_android} root=/dev/nfs " \
		"video=mxcfb0:${video0} " \
		"video=mxcfb1:${video1} " \
		"video=mxcfb2:${video2} " \
		"ip=dhcp nfsroot=${serverip}:${rootpath},v3,tcp " \
		"${bootargs_once} ${std_bootargs}\0" \
	"bootargs_nfs_android=run bootargs_tftp_android\0" \
	"mmcroot=PARTUUID=1c606ef5-f1ac-43b9-9bb5-d5c578580b6b\0" \
	"bootargs_mmc_linux=setenv bootargs console=${console},${baudrate} " \
		"${bootargs_linux} root=${mmcroot} rootwait rw " \
		"video=mxcfb0:${video0} " \
		"video=mxcfb1:${video1} " \
		"video=mxcfb2:${video2} " \
		"${bootargs_once} ${std_bootargs}\0" \
	"bootargs_tftp_linux=setenv bootargs console=${console},${baudrate} " \
		"${bootargs_linux} root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${rootpath},v3,tcp " \
		"${bootargs_once} ${std_bootargs}\0" \
	"bootargs_nfs_linux=run bootargs_tftp_linux\0" \
	"parts_linux=\"uuid_disk=${uuid_disk};" \
		"start=2MiB," \
		"name=linux,size=64MiB,uuid=${part1_uuid};" \
		"name=linux2,size=64MiB,uuid=${part2_uuid};" \
		"name=rootfs,size=1GiB,uuid=${part3_uuid};" \
		"name=rootfs2,size=1GiB,uuid=${part4_uuid};" \
		"name=userfs,size=-,uuid=${part5_uuid};" \
		"\"\0" \
	"linux_file=dey-image-graphical-ccimx6adpt.boot.vfat\0" \
	"rootfs_file=dey-image-graphical-ccimx6adpt.rootfs.ext4\0" \
	"partition_mmc_linux=mmc rescan;" \
		"if mmc dev ${mmcdev} 0;then;else mmc dev ${mmcdev};fi;" \
		"gpt write mmc ${mmcdev} ${parts_linux};" \
		"mmc rescan\0" \
	"video0=dev=ldb,LDB-HSD101PFW2,bpp=32\0" \
	"video1=dev=hdmi,1920x1080M@60,bpp=32\0" \
	"video2=off\0" \
	""	/* end line */

#define CONFIG_BOOTCOMMAND \
	"if run loadscript; then " \
		"source ${loadaddr};" \
	"else " \
		"dboot android mmc ${mmcdev}:${mmcpart}; " \
	"fi;"
#endif
#define CONFIG_ARP_TIMEOUT     200UL

/* Miscellaneous configurable options */
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2     "> "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE              1024

#define CONFIG_SYS_MEMTEST_START       0x10000000
#define CONFIG_SYS_MEMTEST_END         0x10010000
#define CONFIG_SYS_MEMTEST_SCRATCH     0x10800000

#define CONFIG_SYS_LOAD_ADDR           CONFIG_LOADADDR
#define CONFIG_SYS_HZ                  1000

#define CONFIG_CMDLINE_EDITING

/* Physical Memory Map */
#define CONFIG_NR_DRAM_BANKS           1
#define PHYS_SDRAM                     MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE          PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR       IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE       IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* FLASH and environment organization */
#define CONFIG_SYS_NO_FLASH

#if defined CONFIG_SYS_BOOT_SPINOR
#define CONFIG_SYS_USE_SPINOR
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_EIMNOR
#define CONFIG_SYS_USE_EIMNOR
#define CONFIG_ENV_IS_IN_FLASH
#elif defined CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
#elif defined CONFIG_SYS_BOOT_SATA
#define CONFIG_ENV_IS_IN_SATA
#define CONFIG_CMD_SATA
#else
#define CONFIG_ENV_IS_IN_MMC
#endif

#ifdef CONFIG_CMD_SATA
#define CONFIG_DWC_AHSATA
#define CONFIG_SYS_SATA_MAX_DEVICE	1
#define CONFIG_DWC_AHSATA_PORT_ID	0
#define CONFIG_DWC_AHSATA_BASE_ADDR	SATA_ARB_BASE_ADDR
#define CONFIG_LBA48
#define CONFIG_LIBATA
#endif

#ifdef CONFIG_SYS_USE_SPINOR
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_STMICRO
#define CONFIG_MXC_SPI
#define CONFIG_SF_DEFAULT_BUS  0
#define CONFIG_SF_DEFAULT_SPEED 20000000
#define CONFIG_SF_DEFAULT_MODE (SPI_MODE_0)
#endif

#ifdef CONFIG_SYS_USE_EIMNOR
#undef CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_FLASH_BASE           WEIM_ARB_BASE_ADDR
#define CONFIG_SYS_FLASH_SECT_SIZE	(128 * 1024)
#define CONFIG_SYS_MAX_FLASH_BANKS 1    /* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT 256   /* max number of sectors on one chip */
#define CONFIG_SYS_FLASH_CFI            /* Flash memory is CFI compliant */
#define CONFIG_FLASH_CFI_DRIVER         /* Use drivers/cfi_flash.c */
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE /* Use buffered writes*/
#define CONFIG_SYS_FLASH_EMPTY_INFO
#define CONFIG_SYS_FLASH_PROTECTION
#endif

#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_CMD_NAND
#define CONFIG_CMD_NAND_TRIMFFS

/* NAND stuff */
#define CONFIG_NAND_MXS
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION

/* DMA stuff, needed for GPMI/MXS NAND support */
#define CONFIG_APBH_DMA
#define CONFIG_APBH_DMA_BURST
#define CONFIG_APBH_DMA_BURST8
#endif

#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(1792 * 1024)	/* 256kB below 2MiB */
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + (128 * 1024))
#define CONFIG_ENV_SIZE_REDUND		CONFIG_ENV_SIZE
#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_OFFSET		(768 * 1024)
#define CONFIG_ENV_SECT_SIZE		(8 * 1024)
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED
#elif defined(CONFIG_ENV_IS_IN_FLASH)
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_SIZE			CONFIG_SYS_FLASH_SECT_SIZE
#define CONFIG_ENV_SECT_SIZE		CONFIG_SYS_FLASH_SECT_SIZE
#define CONFIG_ENV_OFFSET		(4 * CONFIG_SYS_FLASH_SECT_SIZE)
#elif defined(CONFIG_ENV_IS_IN_NAND)
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET		(8 << 20)
#define CONFIG_ENV_SECT_SIZE		(128 << 10)
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#elif defined(CONFIG_ENV_IS_IN_SATA)
#define CONFIG_ENV_OFFSET		(768 * 1024)
#define CONFIG_SATA_ENV_DEV		0
#define CONFIG_SYS_DCACHE_OFF /* remove when sata driver support cache */
#endif

#define CONFIG_OF_LIBFDT
#define CONFIG_OF_BOARD_SETUP

#ifndef CONFIG_SYS_DCACHE_OFF
#define CONFIG_CMD_CACHE
#endif

/*
 * I2C configs
 */
#define CONFIG_CMD_I2C
#define CONFIG_HARD_I2C         1
#define CONFIG_I2C_MXC          1
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_BASE             I2C2_BASE_ADDR
#define CONFIG_SYS_I2C_SPEED            100000
#define CONFIG_SYS_I2C_SLAVE            0	/* unused */
#define CONFIG_PMIC_I2C_ADDR		0x58	/* DA9063 PMIC i2c address */

#endif                         /* __CCIMX6_COMMON_CONFIG_H */
