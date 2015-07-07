/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#include <common.h>
#include <asm/imx-common/boot_mode.h>
#include <otf_update.h>
#include <part.h>
#include "helper.h"

DECLARE_GLOBAL_DATA_PTR;

static block_dev_desc_t *mmc_dev;
static int mmc_dev_index;

extern int mmc_get_bootdevindex(void);
extern int board_update_chunk(otf_data_t *oftd);
extern void register_tftp_otf_update_hook(int (*hook)(otf_data_t *oftd),
					  disk_partition_t*);
extern void unregister_tftp_otf_update_hook(void);
extern void register_mmc_otf_update_hook(int (*hook)(otf_data_t *oftd),
					  disk_partition_t*);
extern void unregister_mmc_otf_update_hook(void);

int register_otf_hook(int src, int (*hook)(otf_data_t *oftd),
		       disk_partition_t *partition)
{
	switch (src) {
	case SRC_TFTP:
		register_tftp_otf_update_hook(hook, partition);
		return 1;
	case SRC_MMC:
		//register_mmc_otf_update_hook(hook, partition);
		break;
	case SRC_USB:
	case SRC_SATA:
	case SRC_NFS:
		//TODO
		break;
	}

	return 0;
}

void unregister_otf_hook(int src)
{
	switch (src) {
	case SRC_TFTP:
		unregister_tftp_otf_update_hook();
		break;
	case SRC_MMC:
		//unregister_mmc_otf_update_hook();
		break;
	case SRC_USB:
	case SRC_SATA:
	case SRC_NFS:
		//TODO
		break;
	}

}

enum {
	ERR_WRITE = 1,
	ERR_READ,
	ERR_VERIFY,
};

static int write_firmware(char *partname, unsigned long loadaddr,
			  unsigned long filesize, disk_partition_t *info)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";
	unsigned long size_blks, verifyaddr, u, m;

	size_blks = (filesize / mmc_dev->blksz) + (filesize % mmc_dev->blksz != 0);

	if (size_blks > info->size) {
		printf("File size (%lu bytes) exceeds partition size (%lu bytes)!\n",
			filesize,
			info->size * mmc_dev->blksz);
		return -1;
	}

	/* Prepare command to change to storage device */
	sprintf(cmd, "%s dev %d", CONFIG_SYS_STORAGE_MEDIA, mmc_dev_index);

	/*
	 * If updating U-Boot on eMMC
	 * append the hardware partition where U-Boot lives.
	 */
	if (!strcmp(partname, "uboot") &&
	    !strcmp(CONFIG_SYS_STORAGE_MEDIA, "mmc") &&
	    board_has_emmc() && (mmc_dev_index == 0))
		strcat(cmd, " $mmcbootpart");

	/* Change to storage device */
	if (run_command(cmd, 0)) {
		debug("Cannot change to storage device\n");
		return -1;
	}

	/* Write firmware command */
	sprintf(cmd, "%s write %lx %lx %lx", CONFIG_SYS_STORAGE_MEDIA,
		loadaddr, info->start, size_blks);
	if (run_command(cmd, 0))
		return ERR_WRITE;

	/* If there is enough RAM to hold two copies of the firmware,
	 * verify written firmware.
	 * +--------|---------------------|------------------|--------------+
	 * |        L                     V                  | U-Boot+Stack |
	 * +--------|---------------------|------------------|--------------+
	 * P                                                 U              M
	 *
	 *  P = PHYS_SDRAM (base address of SDRAM)
	 *  L = $loadaddr
	 *  V = $verifyaddr
	 *  M = last address of SDRAM (CONFIG_DDR_MB (size of SDRAM) + P)
	 *  U = SDRAM address where U-Boot is located (plus margin)
	 */
	verifyaddr = getenv_ulong("verifyaddr", 16, 0);
	m = PHYS_SDRAM + (CONFIG_DDR_MB * 1024 * 1024);
	u = m - CONFIG_UBOOT_RESERVED;

	/* ($loadaddr + firmware size) must not exceed $verifyaddr
	 * ($verifyaddr + firmware size) must not exceed U
	 */
	if ((loadaddr + size_blks * mmc_dev->blksz) < verifyaddr &&
	    (verifyaddr + size_blks * mmc_dev->blksz) < u) {
		unsigned long filesize_padded;
		int i;

		/* Read back data... */
		printf("Reading back firmware...\n");
		sprintf(cmd, "%s read %lx %lx %lx", CONFIG_SYS_STORAGE_MEDIA,
			verifyaddr, info->start, size_blks);
		if (run_command(cmd, 0))
			return ERR_READ;
		/* ...then compare by 32-bit words (faster than by bytes)
		 * padding with zeros any bytes at the end to make the size
		 * be a multiple of 4.
		 *
		 * Reference: http://stackoverflow.com/a/2022252
		 */
		printf("Verifying firmware...\n");
		filesize_padded = (filesize + (4 - 1)) & ~(4 - 1);

		for (i = filesize; i < filesize_padded; i++) {
			*((char *)loadaddr + i) = 0;
			*((char *)verifyaddr + i) = 0;
		}
		sprintf(cmd, "cmp.l %lx %lx %lx", loadaddr, verifyaddr,
			(filesize_padded / 4));
		if (run_command(cmd, 0))
			return ERR_VERIFY;
		printf("Update was successful\n");
	} else {
		printf("Firmware updated but not verified "
		       "(not enough available RAM to verify)\n");
	}

	return 0;
}

static int write_file(char *targetfilename, char *targetfs, int part)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";
	unsigned long loadaddr, filesize;

	loadaddr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);
	filesize = getenv_ulong("filesize", 16, 0);

	/* Change to storage device */
	sprintf(cmd, "%s dev %d", CONFIG_SYS_STORAGE_MEDIA, mmc_dev_index);
	if (run_command(cmd, 0)) {
		debug("Cannot change to storage device\n");
		return -1;
	}

	/* Prepare write command */
	sprintf(cmd, "%swrite %s %d:%d %lx %s %lx", targetfs,
		CONFIG_SYS_STORAGE_MEDIA, mmc_dev_index, part,
		loadaddr, targetfilename, filesize);

	return run_command(cmd, 0);
}

#define ECSD_PARTITION_CONFIG		179
#define BOOT_ACK			(1 << 6)
#define BOOT_PARTITION_ENABLE_OFF	3

static int emmc_bootselect(void)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";
	int bootpart;

	/* Prepare command to change to storage device */
	sprintf(cmd, "mmc dev %d", mmc_dev_index);

	/* Change to storage device */
	if (run_command(cmd, 0)) {
		debug("Cannot change to storage device\n");
		return -1;
	}

	/* Select boot partition and enable boot acknowledge */
	bootpart = getenv_ulong("mmcbootpart", 16, CONFIG_SYS_BOOT_PART_EMMC);
	sprintf(cmd, "mmc ecsd write %x %x", ECSD_PARTITION_CONFIG,
		BOOT_ACK | (bootpart << BOOT_PARTITION_ENABLE_OFF));

	return run_command(cmd, 0);
}

/*
 * This function returns the size of available RAM holding a firmware transfer.
 * This size depends on:
 *   - The total RAM available
 *   - The loadaddr
 *   - The RAM occupied by U-Boot and its location
 */
static unsigned int get_available_ram_for_update(void)
{
	unsigned int loadaddr;
	unsigned int la_off;

	loadaddr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);
	la_off = loadaddr - gd->bd->bi_dram[0].start;

	return (gd->bd->bi_dram[0].size - CONFIG_UBOOT_RESERVED - la_off);
}

static int init_mmc_globals(void)
{
	/* Use the device in $mmcdev or else, the boot media */
	mmc_dev_index = getenv_ulong("mmcdev", 16, mmc_get_bootdevindex());
	mmc_dev = mmc_get_dev(mmc_dev_index);
	if (NULL == mmc_dev) {
		debug("Cannot determine sys storage device\n");
		return -1;
	}

	return 0;
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
	int otf_enabled = 0;
	char cmd[CONFIG_SYS_CBSIZE] = "";
	unsigned long loadaddr;
	unsigned long verifyaddr;
	unsigned long filesize;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (init_mmc_globals())
		return CMD_RET_FAILURE;

	/* Get data of partition to be updated */
	if (!strcmp(argv[1], "uboot")) {
		/* Simulate partition data for U-Boot */
		info.start = CONFIG_SYS_BOOT_PART_OFFSET / mmc_dev->blksz;
		info.size = CONFIG_SYS_BOOT_PART_SIZE / mmc_dev->blksz;
		strcpy((char *)info.name, argv[1]);
	} else {
		/* Not a reserved name. Must be a partition name or index */
		char dev_index_str[2];

		/* Look up partition on the device */
		sprintf(dev_index_str, "%d", mmc_dev_index);
		if (get_partition_bynameorindex(CONFIG_SYS_STORAGE_MEDIA,
					dev_index_str, argv[1], &info) < 0) {
			printf("Error: partition '%s' not found\n", argv[1]);
			return CMD_RET_FAILURE;
		}
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
		src = get_source(argc, argv, &devpartno, &fs);
		if (src == SRC_UNSUPPORTED) {
			printf("Error: '%s' is not supported as source\n",
				argv[2]);
			return CMD_RET_USAGE;
		}
		else if (src == SRC_UNDEFINED) {
			printf("Error: undefined source\n");
			return CMD_RET_USAGE;
		}
	}

	loadaddr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);
	/*
	 * If undefined, calculate 'verifyaddr' as halfway through the RAM
	 * from $loadaddr.
	 */
	if (NULL == getenv("verifyaddr")) {
		verifyaddr = loadaddr +
			     ((gd->ram_size - (loadaddr - PHYS_SDRAM)) / 2);
		if (verifyaddr > loadaddr &&
		    verifyaddr < (PHYS_SDRAM + gd->ram_size))
			setenv_hex("verifyaddr", verifyaddr);
	}

	if (src == SRC_RAM) {
		/* Get address in RAM where firmware file is */
		if (argc > 3)
			loadaddr = simple_strtol(argv[3], NULL, 16);

		/* Get filesize */
		if (argc > 4)
			filesize = simple_strtol(argv[4], NULL, 16);
	} else {
		if (getenv_yesno("otf-update") == -1) {
			/*
			 * If otf-update is undefined, check if there is enough
			 * RAM to hold the largest possible file that fits into
			 * the destiny partition.
			 */
			unsigned long avail = get_available_ram_for_update();

			if (avail <= info.size * mmc_dev->blksz) {
				printf("Partition to update is larger (%d MiB) than the\n"
				       "available RAM memory (%d MiB, starting at $loadaddr=0x%08x).\n",
				       (int)(info.size * mmc_dev->blksz / (1024 * 1024)),
				       (int)(avail / (1024 * 1024)),
				       (unsigned int)loadaddr);
				printf("Activating On-the-fly update mechanism.\n");
				otf_enabled = 1;
			}
		}

		/* Get firmware file name */
		ret = get_fw_filename(argc, argv, src, filename);
		if (ret) {
			/* Filename was not provided. Look for default one */
			ret = get_default_filename(argv[1], filename, CMD_UPDATE);
			if (ret) {
				printf("Error: need a filename\n");
				return CMD_RET_USAGE;
			}
		}
	}

	/* Activate on-the-fly update if needed */
	if (otf_enabled || (getenv_yesno("otf-update") == 1)) {
		if (!strcmp((char *)info.name, "uboot")) {
			/* Do not activate on-the-fly update for U-Boot */
			printf("On-the-fly mechanism disabled for U-Boot "
				"for security reasons\n");
		} else {
			/* register on-the-fly update mechanism */
			otf = register_otf_hook(src, board_update_chunk, &info);
		}
	}

	if (otf) {
		/* Prepare command to change to storage device */
		sprintf(cmd, CONFIG_SYS_STORAGE_MEDIA " dev %d", mmc_dev_index);
		/* Change to storage device */
		if (run_command(cmd, 0)) {
			printf("Error: cannot change to storage device\n");
			ret = CMD_RET_FAILURE;
			goto _ret;
		}
	}

	if (src != SRC_RAM) {
		/* Load firmware file to RAM */
		ret = load_firmware(src, filename, devpartno, fs, "$loadaddr",
				    NULL);
		if (ret == LDFW_ERROR) {
			printf("Error loading firmware file to RAM\n");
			ret = CMD_RET_FAILURE;
			goto _ret;
		} else if (ret == LDFW_LOADED && otf) {
			ret = CMD_RET_SUCCESS;
			goto _ret;
		}
	}

	/* Write firmware file from RAM to storage */
	filesize = getenv_ulong("filesize", 16, 0);
	ret = write_firmware(argv[1], loadaddr, filesize, &info);
	if (ret) {
		if (ret == ERR_READ)
			printf("Error while reading back written firmware!\n");
		else if (ret == ERR_VERIFY)
			printf("Error while verifying written firmware!\n");
		else
			printf("Error writing firmware!\n");
		ret = CMD_RET_FAILURE;
		goto _ret;
	}

	/*
	 * If updating U-Boot into eMMC, instruct the eMMC to boot from
	 * special hardware partition.
	 */
	if (!strcmp(argv[1], "uboot") &&
	    !strcmp(CONFIG_SYS_STORAGE_MEDIA, "mmc") &&
	    board_has_emmc() && (mmc_dev_index == 0)) {
		ret = emmc_bootselect();
		if (ret) {
			printf("Error changing eMMC boot partition\n");
			ret = CMD_RET_FAILURE;
			goto _ret;
		}
	}

_ret:
	unregister_otf_hook(src);
	return ret;
}

U_BOOT_CMD(
	update,	6,	0,	do_update,
	"Digi modules update command",
	"<partition>  [source] [extra-args...]\n"
	" Description: updates (raw writes) <partition> in $mmcdev via <source>\n"
	" Arguments:\n"
	"   - partition:    a partition index, a GUID partition name, or one\n"
	"                   of the reserved names: uboot\n"
	"   - [source]:     " CONFIG_UPDATE_SUPPORTED_SOURCES_LIST "\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	CONFIG_UPDATE_SUPPORTED_SOURCES_ARGS_HELP
);

/* Certain command line arguments of 'update' command may be at different
 * index depending on the selected <source>. This function returns in 'arg'
 * the argument at <index> plus an offset that depends on the selected <source>
 * Upon calling, the <index> must be given as if <source> was SRC_RAM.
 */
static int get_arg_src(int argc, char * const argv[], int src, int index,
		       char **arg)
{
	switch (src) {
	case SRC_TFTP:
	case SRC_NFS:
		index += 1;
		break;
	case SRC_MMC:
	case SRC_USB:
	case SRC_SATA:
		index += 3;
		break;
	case SRC_RAM:
		break;
	default:
		return -1;
	}

	if (argc > index) {
		*arg = (char *)argv[index];

		return 0;
	}

	return -1;
}

static int do_updatefile(cmd_tbl_t* cmdtp, int flag, int argc,
			 char * const argv[])
{
	int src = SRC_TFTP;	/* default to TFTP */
	char *devpartno = NULL;
	char *fs = NULL;
	disk_partition_t info;
	char *srcfilename = NULL;
	char *targetfilename = NULL;
	char *targetfs = NULL;
	const char *default_fs = "fat";
	int part;
	int i;
	char *supported_fs[] = {
#ifdef CONFIG_FAT_WRITE
		"fat",
#endif
#ifdef CONFIG_EXT4_WRITE
		"ext4",
#endif
	};
	char dev_index_str[2];

	if (argc < 2)
		return CMD_RET_USAGE;

	if (init_mmc_globals())
		return CMD_RET_FAILURE;

	/* Get data of partition to be updated */
	sprintf(dev_index_str, "%d", mmc_dev_index);
	part = get_partition_bynameorindex(CONFIG_SYS_STORAGE_MEDIA,
					   dev_index_str, argv[1], &info);
	if (part < 0) {
		printf("Error: partition '%s' not found\n", argv[1]);
		return CMD_RET_FAILURE;
	}

	/* Get source of update firmware file */
	if (argc > 2) {
		src = get_source(argc, argv, &devpartno, &fs);
		if (src == SRC_UNSUPPORTED) {
			printf("Error: '%s' is not supported as source\n",
				argv[2]);
			return CMD_RET_USAGE;
		}
		else if (src == SRC_UNDEFINED) {
			printf("Error: undefined source\n");
			return CMD_RET_USAGE;
		}
	}

	/* Get file name */
	if (get_arg_src(argc, argv, src, 2, &srcfilename)) {
		printf("Error: need a filename\n");
		return CMD_RET_USAGE;
	}

	/* Get target file name. If not provided use srcfilename by default */
	if (get_arg_src(argc, argv, src, 3, &targetfilename))
		targetfilename = srcfilename;

	/* Get target filesystem. If not provided use 'fat' by default */
	if (get_arg_src(argc, argv, src, 4, &targetfs))
		targetfs = (char *)default_fs;

	/* Check target fs is supported */
	for (i = 0; i < ARRAY_SIZE(supported_fs); i++)
		if (!strcmp(targetfs, supported_fs[i]))
			break;

	if (i >= ARRAY_SIZE(supported_fs)) {
		printf("Error: target file system '%s' is unsupported for "
			"write operation.\n"
			"Valid file systems are: ", targetfs);
		for (i = 0; i < ARRAY_SIZE(supported_fs); i++)
			printf("%s ", supported_fs[i]);
		printf("\n");
		return CMD_RET_FAILURE;
	}

	/* Load firmware file to RAM */
	if (LDFW_ERROR == load_firmware(src, srcfilename, devpartno, fs,
					"$loadaddr", NULL)) {
		printf("Error loading firmware file to RAM\n");
		return CMD_RET_FAILURE;
	}

	/* Write file from RAM to storage partition */
	if (write_file(targetfilename, targetfs, part)) {
		printf("Error writing file\n");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	updatefile,	8,	0,	do_updatefile,
	"Digi modules updatefile command",
	"<partition>  [source] [extra-args...]\n"
	" Description: updates/writes a file in <partition> in $mmcdev via\n"
	"              <source>\n"
	" Arguments:\n"
	"   - partition:    a partition index or a GUID partition name where\n"
	"                   to upload the file\n"
	"   - [source]:     " CONFIG_UPDATE_SUPPORTED_SOURCES_LIST "\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	CONFIG_UPDATEFILE_SUPPORTED_SOURCES_ARGS_HELP
);
