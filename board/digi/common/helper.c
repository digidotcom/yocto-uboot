/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#include <common.h>
#include "helper.h"

DECLARE_GLOBAL_DATA_PTR;

enum {
	FWLOAD_NO,
	FWLOAD_YES,
	FWLOAD_TRY,
};

static const char *src_strings[] = {
	[SRC_TFTP] =	"tftp",
	[SRC_NFS] =	"nfs",
	[SRC_FLASH] =	"flash",
	[SRC_USB] =	"usb",
	[SRC_MMC] =	"mmc",
	[SRC_RAM] =	"ram",
	[SRC_SATA] =	"sata",
};

#ifdef CONFIG_AUTO_BOOTSCRIPT
#define AUTOSCRIPT_TFTP_MSEC		100
#define AUTOSCRIPT_TFTP_CNT		15
#define AUTOSCRIPT_START_AGAIN		100
extern ulong TftpRRQTimeoutMSecs;
extern int TftpRRQTimeoutCountMax;
extern unsigned long NetStartAgainTimeout;
int DownloadingAutoScript = 0;
int RunningAutoScript = 0;
#endif

int confirm_msg(char *msg)
{
#ifdef CONFIG_AUTO_BOOTSCRIPT
        /* From autoscript we shouldn't expect user's confirmations.
         * Assume yes is the correct answer here to avoid halting the script.
         */
	if (RunningAutoScript)
		return 1;
#endif

	printf(msg);
	if (getc() == 'y') {
		int c;

		putc('y');
		c = getc();
		putc('\n');
		if (c == '\r')
			return 1;
	}

	puts("Operation aborted by user\n");
	return 0;
}

int get_source(int argc, char * const argv[], char **devpartno, char **fs)
{
	int i;
	char *src = argv[2];

	for (i = 0; i < ARRAY_SIZE(src_strings); i++) {
		if (!strncmp(src_strings[i], src, strlen(src))) {
			if (1 << i & CONFIG_SUPPORTED_SOURCES)
				break;
			else
				return SRC_UNSUPPORTED;
		}
	}

	if (i >= ARRAY_SIZE(src_strings))
		return SRC_UNDEFINED;

	if (i == SRC_USB || i == SRC_MMC || i == SRC_SATA) {
		/* Get device:partition and file system */
		if (argc > 3)
			*devpartno = (char *)argv[3];
		if (argc > 4)
			*fs = (char *)argv[4];
	}

	return i;
}

const char *get_source_string(int src)
{
	if (SRC_UNDEFINED != src && src < ARRAY_SIZE(src_strings))
		return src_strings[src];

	return "";
}

int get_fw_filename(int argc, char * const argv[], int src, char *filename)
{
	switch (src) {
	case SRC_TFTP:
	case SRC_NFS:
		if (argc > 3) {
			strcpy(filename, argv[3]);
			return 0;
		}
		break;
	case SRC_MMC:
	case SRC_USB:
	case SRC_SATA:
		if (argc > 5) {
			strcpy(filename, argv[5]);
			return 0;
		}
		break;
	case SRC_RAM:
		return 0;	/* No file is needed */
	default:
		return -1;
	}

	return -1;
}

int get_default_filename(char *partname, char *filename, int cmd)
{
	switch(cmd) {
	case CMD_DBOOT:
		if (!strcmp(partname, "linux") ||
		    !strcmp(partname, "android")) {
			strcpy(filename, "$uimage");
			return 0;
		}
		break;

	case CMD_UPDATE:
		if (!strcmp(partname, "uboot")) {
			strcpy(filename, "$uboot_file");
			return 0;
		} else {
			/* Read the default filename from a variable called
			 * after the partition name: <partname>_file
			 */
			char varname[100];
			char *varvalue;

			sprintf(varname, "%s_file", partname);
			varvalue = getenv(varname);
			if (varvalue != NULL) {
				strcpy(filename, varvalue);
				return 0;
			}
		}
		break;
	}

	return -1;
}

int get_default_devpartno(int src, char *devpartno)
{
	char *dev, *part;

	switch (src) {
	case SRC_MMC:
		dev = getenv("mmcdev");
		if (dev == NULL)
			return -1;
		part = getenv("mmcpart");
		/* If mmcpart not defined, default to 1 */
		if (part == NULL)
			sprintf(devpartno, "%s:1", dev);
		else
			sprintf(devpartno, "%s:%s", dev, part);
		break;
	case SRC_USB:	// TODO
	case SRC_SATA:	// TODO
	default:
		return -1;
	}

	return 0;
}

/* A variable determines if the file must be loaded.
 * The function returns:
 *	LDFW_LOADED if the file was loaded successfully
 *	LDFW_NOT_LOADED if the file was not loaded, but isn't required
 *	LDFW_ERROR on error
 */
int load_firmware(int src, char *filename, char *devpartno,
		  char *fs, char *loadaddr, char *varload)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";
	char def_devpartno[] = "0:1";
	char def_fs[] = "fat";
	int ret;
	int fwload = FWLOAD_YES;

	/* Variable 'varload' determines if the file must be loaded:
	 * - yes|NULL: the file must be loaded. Return error otherwise.
	 * - try: the file may be loaded. Return ok even if load fails.
	 * - no: skip the load.
	 */
	if (NULL != varload) {
		if (!strcmp(varload, "no"))
			return LDFW_NOT_LOADED;	/* skip load and return ok */
		else if (!strcmp(varload, "try"))
			fwload = FWLOAD_TRY;
	}

	/* Use default values if not provided */
	if (NULL == devpartno) {
		if (get_default_devpartno(src, def_devpartno))
			strcpy(def_devpartno, "0:1");
		devpartno = def_devpartno;
	}
	if (NULL == fs)
		fs = def_fs;

	switch (src) {
	case SRC_TFTP:
		sprintf(cmd, "tftpboot %s %s", loadaddr, filename);
		break;
	case SRC_NFS:
		sprintf(cmd, "nfs %s $rootpath/%s", loadaddr, filename);
		break;
	case SRC_MMC:
	case SRC_USB:
	case SRC_SATA:
		sprintf(cmd, "%sload %s %s %s %s", fs,
			src_strings[src], devpartno, loadaddr, filename);
		break;
	case SRC_RAM:
		ret = LDFW_NOT_LOADED;	/* file is already in RAM */
		goto _ret;
	default:
		return -1;
	}

	ret = run_command(cmd, 0);
_ret:
	if (FWLOAD_TRY == fwload) {
		if (ret)
			return LDFW_NOT_LOADED;
		else
			return LDFW_LOADED;
	}

	if (ret)
		return LDFW_ERROR;

	return LDFW_LOADED;	/* ok, file was loaded */
}

#if defined(CONFIG_SOURCE) && defined(CONFIG_AUTO_BOOTSCRIPT)
void run_auto_bootscript(void)
{
#ifdef CONFIG_CMD_NET
	int ret;
	char *bootscript;
	/* Save original timeouts */
        ulong saved_rrqtimeout_msecs = TftpRRQTimeoutMSecs;
        int saved_rrqtimeout_count = TftpRRQTimeoutCountMax;
	ulong saved_startagain_timeout = NetStartAgainTimeout;
	unsigned long saved_flags = gd->flags;
	char *retrycnt = getenv("netretry");

	bootscript = getenv("bootscript");
	if (bootscript) {
		printf("Bootscript from TFTP... ");

		/* Silence console */
		gd->flags |= GD_FLG_SILENT;
		/* set timeouts for bootscript */
		TftpRRQTimeoutMSecs = AUTOSCRIPT_TFTP_MSEC;
		TftpRRQTimeoutCountMax = AUTOSCRIPT_TFTP_CNT;
		NetStartAgainTimeout = AUTOSCRIPT_START_AGAIN;
		/* set retrycnt */
		setenv("netretry", "no");

		/* Silence net commands during the bootscript download */
		DownloadingAutoScript = 1;
		ret = run_command("tftp ${loadaddr} ${bootscript}", 0);
		/* First restore original values of global variables
		 * and then evaluate the result of the run_command */
		DownloadingAutoScript = 0;
		/* Restore original timeouts */
		TftpRRQTimeoutMSecs = saved_rrqtimeout_msecs;
		TftpRRQTimeoutCountMax = saved_rrqtimeout_count;
		NetStartAgainTimeout = saved_startagain_timeout;
		/* restore retrycnt */
		if (retrycnt)
			setenv("netretry", retrycnt);
		else
			setenv("netretry", "");
		/* Restore flags */
		gd->flags = saved_flags;

		if (ret)
			goto error;

		printf("[ready]\nRunning bootscript...\n");
		RunningAutoScript = 1;
		/* Launch bootscript */
		run_command("source ${loadaddr}", 0);
		RunningAutoScript = 0;
		return;
error:
		printf( "[not available]\n" );
	}
#endif
}
#endif
