/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#include <common.h>
#include <otf_update.h>
#include <part.h>
#include "helper.h"

extern int board_update_chunk(otf_data_t *oftd);
extern void register_tftp_otf_update_hook(int (*hook)(otf_data_t *oftd),
					  disk_partition_t*);
extern void unregister_tftp_otf_update_hook(void);
extern void register_mmc_otf_update_hook(int (*hook)(otf_data_t *oftd),
					  disk_partition_t*);
extern void unregister_mmc_otf_update_hook(void);

void register_otf_hook(int src, int (*hook)(otf_data_t *oftd),
		       disk_partition_t *partition)
{
	switch (src) {
	case SRC_TFTP:
		register_tftp_otf_update_hook(hook, partition);
		break;
	case SRC_MMC:
		register_mmc_otf_update_hook(hook, partition);
		break;
	case SRC_USB:
	case SRC_SATA:
	case SRC_NFS:
		//TODO
		break;
	}
}

void unregister_otf_hook(int src)
{
	switch (src) {
	case SRC_TFTP:
		unregister_tftp_otf_update_hook();
		break;
	case SRC_MMC:
		unregister_mmc_otf_update_hook();
		break;
	case SRC_USB:
	case SRC_SATA:
	case SRC_NFS:
		//TODO
		break;
	}

}

static int write_firmware(char *partname, disk_partition_t *info)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";
	char *filesize = getenv("filesize");
	unsigned long size;
	block_dev_desc_t *mmc_dev;

	mmc_dev = mmc_get_dev(CONFIG_SYS_STORAGE_DEV);
	if (NULL == mmc_dev) {
		debug("Cannot determine sys storage device\n");
		return -1;
	}

	if (NULL == filesize) {
		debug("Cannot determine filesize\n");
		return -1;
	}
	size = simple_strtoul(filesize, NULL, 16) / mmc_dev->blksz;
	if (simple_strtoul(filesize, NULL, 16) % mmc_dev->blksz)
		size++;

	if (size > info->size) {
		printf("File size (%lu bytes) exceeds partition size (%lu bytes)!\n",
			size * mmc_dev->blksz,
			info->size * mmc_dev->blksz);
		return -1;
	}

	/* Prepare command to change to storage device */
	sprintf(cmd, "%s dev %d", CONFIG_SYS_STORAGE_MEDIA,
		CONFIG_SYS_STORAGE_DEV);

#ifdef CONFIG_SYS_BOOT_PART
	/* If U-Boot and special partition, append the hardware partition */
	if (!strcmp(partname, "uboot"))
		strcat(cmd, " $mmcbootpart");
#endif

	/* Change to storage device */
	if (run_command(cmd, 0)) {
		debug("Cannot change to storage device\n");
		return -1;
	}

	/* Prepare write command */
	sprintf(cmd, "%s write $loadaddr %lx %lx", CONFIG_SYS_STORAGE_MEDIA,
		info->start, size);

	return run_command(cmd, 0);
}

#define ECSD_PARTITION_CONFIG		179
#define BOOT_ACK			(1 << 6)
#define BOOT_PARTITION_ENABLE_OFF	3

static int emmc_bootselect(void)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";

	/* Prepare command to change to storage device */
	sprintf(cmd, "mmc dev %d", CONFIG_SYS_STORAGE_DEV);

	/* Change to storage device */
	if (run_command(cmd, 0)) {
		debug("Cannot change to storage device\n");
		return -1;
	}

	/* Select boot partition and enable boot acknowledge */
	sprintf(cmd, "mmc ecsd write %x %x", ECSD_PARTITION_CONFIG,
		BOOT_ACK | (CONFIG_SYS_BOOT_PART << BOOT_PARTITION_ENABLE_OFF));

	return run_command(cmd, 0);
}

static int do_update(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
	int src = SRC_TFTP;	/* default to TFTP */
	char *devpartno = NULL;
	char *fs = NULL;
	disk_partition_t info;
	int ret;
	char filename[256] = "";
	int otf = 0;
	char cmd[CONFIG_SYS_CBSIZE] = "";

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Get data of partition to be updated */
	ret = get_target_partition(argv[1], &info);
	if (ret) {
		printf("Partition '%s' not found\n", argv[1]);
		return CMD_RET_FAILURE;
	}

	/* Ask for confirmation if needed */
	if (getenv_yesno("forced_update") != 1) {
		/* Confirm programming */
		if (!strcmp((char *)info.name, "uboot") &&
		    !confirm_msg("Do you really want to program "
				 "the boot loader? <y/N> "))
			return CMD_RET_FAILURE;
	}

	/* Get source of update firmware file */
	if (argc > 2) {
		src = get_source(argv[2]);
		if (src == SRC_UNSUPPORTED) {
			printf("'%s' is not supported as source\n",
				argv[2]);
			return CMD_RET_USAGE;
		}
		else if (src == SRC_UNDEFINED) {
			return CMD_RET_USAGE;
		}

		if (src == SRC_USB || src == SRC_MMC || src == SRC_SATA) {
			/* Get device:partition and file system */
			if (argc > 3)
				devpartno = argv[3];
			if (argc > 4)
				fs = argv[4];
		}
	}

	/* Get firmware file name */
	ret = get_fw_filename(argc, argv, src, filename);
	if (ret) {
		/* Filename was not provided. Look for default one */
		ret = get_default_filename(argv[1], filename, CMD_UPDATE);
		if (ret) {
			printf("Error: need a filename\n");
			return CMD_RET_FAILURE;
		}
	}

	/* Activate on-the-fly update if needed */
	otf = (getenv_yesno("otf-update") == 1);
	if (otf) {
		if (!strcmp((char *)info.name, "uboot")) {
			/* Do not activate on-the-fly update for U-Boot */
			printf("On-the-fly mechanism disabled for U-Boot "
				"for security reasons\n");
			otf = 0;
		} else {
			/* register on-the-fly update mechanism */
			register_otf_hook(src, board_update_chunk, &info);
			/* Prepare command to change to storage device */
			sprintf(cmd, "mmc dev %d", CONFIG_SYS_STORAGE_DEV);
			/* Change to storage device */
			if (run_command(cmd, 0)) {
				printf("Cannot change to storage device\n");
				ret = -1;
				goto _ret;
			}
		}
	}

	/* Load firmware file to RAM */
	ret = load_firmware(src, filename, devpartno, fs, "$loadaddr", NULL);
	if (ret == LDFW_ERROR) {
		printf("Error loading firmware file to RAM\n");
		ret = CMD_RET_FAILURE;
		goto _ret;
	} else if (ret == LDFW_LOADED && otf) {
		ret = 0;
		goto _ret;
	}

	/* Write firmware file from RAM to storage */
	ret = write_firmware(argv[1], &info);
	if (ret) {
		printf("Error writing firmware\n");
		ret = CMD_RET_FAILURE;
		goto _ret;
	}
#ifdef CONFIG_SYS_BOOT_PART
	/* If U-Boot and special partition, instruct the eMMC
	 * to boot from it */
	if (!strcmp(argv[1], "uboot") &&
	    !strcmp(CONFIG_SYS_STORAGE_MEDIA, "mmc")) {
		ret = emmc_bootselect();
		if (ret) {
			printf("Error changing eMMC boot partition\n");
			ret = CMD_RET_FAILURE;
			goto _ret;
		}
	}
#endif

_ret:
	unregister_otf_hook(src);
	return ret;
}

U_BOOT_CMD(
	update,	6,	0,	do_update,
	"Digi modules update commands",
	"<partition>  [source] [extra-args...]\n"
	" Description: updates flash <partition> via <source>\n"
	" Arguments:\n"
	"   - partition:    a partition name or one of the reserved names: \n"
	"                   uboot\n"
	"   - [source]:     " CONFIG_SUPPORTED_SOURCES_LIST "\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	CONFIG_SUPPORTED_SOURCES_ARGS
);
