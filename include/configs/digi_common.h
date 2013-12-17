/*
 *  include/configs/digi_common.h
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision: 1.4 $:
 *  !Author:     Markus Pietrek
 *  !Descr:      Defines all definitions that are common to all DIGI platforms
*/

#ifndef __DIGI_COMMON_H
#define __DIGI_COMMON_H

/* global helper stuff */

#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

/* may undefine settings */
#include <digi_version.h>

/* stuff that may be undefined in userconfig.h */
/* ********** console configuration ********** */
#ifdef CONFIG_SILENT_CONSOLE
# define CFG_SET_SILENT		"yes"
#else
# define CFG_SET_SILENT		"no"
#endif
#define CONFIG_GET_CONS_FROM_CONS_VAR
#define CONFIG_GET_BAUDRATE_FROM_VAR

/* ********** System Initialization ********** */
#ifdef CONFIG_DOWNLOAD_BY_DEBUGGER
# define CONFIG_SKIP_RELOCATE_UBOOT
#endif /* CONFIG_DOWNLOAD_BY_DEBUGGER */

/*
 * If we are developing, we might want to start armboot from ram
 * so we MUST NOT initialize critical regs like mem-timing ...
 */
/* define for developing */
#undef	CONFIG_SKIP_LOWLEVEL_INIT

#define PLATFORMNAME_MAXLEN	30	/* Max length of module and
					 * platform name */
#define MAX_DYNVARS		100	/* Max number of dynamic variables */

/* ********** Prompt *********** */
#define CONFIG_PROMPT_SEPARATOR		"#"

/* ********** Rootfs *********** */
/* Delay before trying to mount the rootfs from a media */
#define ROOTFS_DELAY		10

#ifndef CONFIG_ZERO_BOOTDELAY_CHECK
# define CONFIG_ZERO_BOOTDELAY_CHECK	/* check for keypress on bootdelay==0 */
#endif  /* CONFIG_ZERO_BOOTDELAY_CHECK */

/* ********** network ********** */
#ifndef CONFIG_TFTP_RETRIES_ON_ERROR
# define CONFIG_TFTP_RETRIES_ON_ERROR	5
#endif
#define CONFIG_TFTP_UPDATE_ONTHEFLY	/* support to tftp and update on-the-fly */

/* ********** usb/mmc ********** */
#define DEFAULT_KERNEL_FS		"fat"
#define DEFAULT_KERNEL_DEVPART		"0:1"
#define DEFAULT_ROOTFS_SATA_PART	"/dev/sda2"
#define DEFAULT_ROOTFS_MMC_PART		"/dev/mmcblk0p2"
#define DEFAULT_ROOTFS_USB_PART		"/dev/sda2"

/* ********** memory sizes ********** */
#define SPI_LOADER_SIZE		8192

/* NVRAM */
#define CONFIG_ENV_IS_IN_DIGI_NVRAM
#define CONFIG_ENV_SIZE		(16 * 1024)

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#define CONFIG_SYS_GBL_DATA_SIZE	256     /* size in bytes reserved for initial data */

/* ********** misc stuff ********** */

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERRIDE
#define CONFIG_ENV_OVERWRITE

#define CONFIG_SYS_PBSIZE	 (CONFIG_SYS_CBSIZE + CONFIG_PROMPT_MAXLEN + 16)
#define CONFIG_SYS_MAXARGS	32			/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

/* In: serial, Out: serial etc. */
#define CFG_CONSOLE_INFO_QUIET

/* stuff for DVTs and special information */
#define CONFIG_DVT_PROVIDED

/* compilation */
#define CONFIG_SYS_64BIT_VSPRINTF      /* we need if for NVRAM */
#define CONFIG_SYS_64BIT_STRTOUL       /* we need if for NVRAM */

/* device tree */
#define CONFIG_FDT_MAX_SIZE	(128 * 1024)

#ifndef __ASSEMBLY__		/* put C only stuff in this section */
/* macros */
#define VIDEO_MENU_OPTION(text, hook, param)	\
	{MENU_OPTION, text, hook, param}
/* global functions */
int bsp_init(void);
#endif /* __ASSEMBLY__ */

#endif /* __DIGI_COMMON_H */
