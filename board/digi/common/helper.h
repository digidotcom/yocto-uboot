/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#ifndef __DIGI_HELPER_H
#define __DIGI_HELPER_H

enum {
	SRC_UNDEFINED = -2,
	SRC_UNSUPPORTED = -1,
	SRC_TFTP,
	SRC_NFS,
	SRC_FLASH,
	SRC_USB,
	SRC_MMC,
	SRC_RAM,
	SRC_SATA,
};

enum {
	CMD_DBOOT,
	CMD_UPDATE,
};

int confirm_msg(char *msg);
int get_source(int argc, char * const argv[], char **devpartno, char **fs);
const char *get_source_string(int src);
int get_fw_filename(int argc, char * const argv[], int src, char *filename);
int get_default_filename(char *partname, char *filename, int cmd);

enum {
	LDFW_ERROR = -1,
	LDFW_NOT_LOADED,
	LDFW_LOADED,
};

int load_firmware(int src, char *filename, char *devpartno,
		  char *fs, char *loadaddr, char *varload);

#endif  /* __DIGI_HELPER_H */
