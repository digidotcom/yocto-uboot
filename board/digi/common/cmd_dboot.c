/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#include <common.h>
#include <part.h>
#include "helper.h"

enum {
	OS_UNDEFINED = -1,
	OS_LINUX,
	OS_ANDROID,
};

static const char *os_strings[] = {
	[OS_LINUX] =	"linux",
	[OS_ANDROID] =	"android",
};

static int get_os(char *os)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(os_strings); i++) {
		if (!strncmp(os_strings[i], os, strlen(os)))
			return i;
	}

	return OS_UNDEFINED;
}

static const char *get_os_string(int os)
{
	if (OS_UNDEFINED != os && os < ARRAY_SIZE(os_strings))
		return os_strings[os];

	return "";
}

static int set_bootargs(int os, int src)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";

	/* Run script at variable 'bootargs_<src>_<os>' */
	sprintf(cmd, "run bootargs_%s_%s", get_source_string(src),
		get_os_string(os));

	return run_command(cmd, 0);
}

static int boot_os(int has_initrd, int has_fdt)
{
	char cmd[CONFIG_SYS_CBSIZE] = "";

	sprintf(cmd, "bootm $loadaddr %s %s",
		has_initrd ? "$initrd_addr" : "-",
		has_fdt ? "$fdt_addr" : "");

	return run_command(cmd, 0);
}

static int do_dboot(cmd_tbl_t* cmdtp, int flag, int argc, char * const argv[])
{
	int src = SRC_TFTP;	/* default to TFTP */
	int os = SRC_UNDEFINED;
	char *devpartno = NULL;
	char *fs = NULL;
	int ret;
	char filename[256] = "";
	char *varload;
	int has_fdt = 0;
	int has_initrd = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	/* Get OS to boot */
	os = get_os(argv[1]);
	if (OS_UNDEFINED == os) {
		printf("'%s' is not a valid operating system\n", argv[1]);
		return CMD_RET_FAILURE;
	}

	/* Get source of firmware file */
	if (argc > 2) {
		src = get_source(argc, argv, &devpartno, &fs);
		if (src == SRC_UNSUPPORTED) {
			printf("'%s' is not supported as source\n",
				argv[2]);
			return CMD_RET_USAGE;
		}
		else if (src == SRC_UNDEFINED) {
			printf("Error: undefined source\n");
			return CMD_RET_USAGE;
		}
	}

	/* Get firmware file name */
	ret = get_fw_filename(argc, argv, src, filename);
	if (ret) {
		/* Filename was not provided. Look for default one */
		ret = get_default_filename(argv[1], filename, CMD_DBOOT);
		if (ret) {
			printf("Error: need a filename\n");
			return CMD_RET_FAILURE;
		}
	}

	/* Load firmware file to RAM */
	ret = load_firmware(src, filename, devpartno, fs, "$loadaddr", NULL);
	if (ret == LDFW_ERROR) {
		printf("Error loading firmware file to RAM\n");
		return CMD_RET_FAILURE;
	}

	/* Get flattened Device Tree */
	varload = getenv("boot_fdt");
	if (NULL == varload && OS_ANDROID == os)
		varload = (char *)"no";	/* Android default */
	ret = load_firmware(src, "$fdt_file", devpartno, fs,
			    "$fdt_addr", varload);
	if (ret == LDFW_LOADED) {
		has_fdt = 1;
	} else if (ret == LDFW_ERROR) {
		printf("Error loading FDT file\n");
		return CMD_RET_FAILURE;
	}

	/* Get init ramdisk */
	varload =  getenv("boot_initrd");
	if (NULL == varload && OS_LINUX == os)
		varload = (char *)"no";	/* Linux default */
	ret = load_firmware(src, "$initrd_file", devpartno, fs,
			    "$initrd_addr", varload);
	if (ret == LDFW_LOADED) {
		has_initrd = 1;
	} else if (ret == LDFW_ERROR) {
		printf("Error loading init ramdisk file\n");
		return CMD_RET_FAILURE;
	}

	/* Set boot arguments */
	ret = set_bootargs(os, src);
	if (ret) {
		printf("Error setting boot arguments\n");
		return CMD_RET_FAILURE;
	}

	/* Boot OS */
	return boot_os(has_initrd, has_fdt);
}

U_BOOT_CMD(
	dboot,	6,	0,	do_dboot,
	"Digi modules boot command",
	"<os> [source] [extra-args...]\n"
	" Description: Boots <os> via <source>\n"
	" Arguments:\n"
	"   - os:           one of the operating systems reserved names: \n"
	"                   linux|android\n"
	"   - [source]:     " CONFIG_DBOOT_SUPPORTED_SOURCES_LIST "\n"
	"   - [extra-args]: extra arguments depending on 'source'\n"
	"\n"
	CONFIG_DBOOT_SUPPORTED_SOURCES_ARGS_HELP
);
