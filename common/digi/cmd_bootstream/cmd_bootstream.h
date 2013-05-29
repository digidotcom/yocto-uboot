/*
 *  Copyright (C) 2011 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

/*
 * Bootstream support for Freescale platforms
 */

#ifndef __DIGI_CMD_BOOTSTREAM_H
#define __DIGI_CMD_BOOTSTREAM_H

#include "BootControlBlocks.h"
#include "nvram.h"

/* flags */
#define F_VERBOSE	(1 << 0)
#define F_DRYRUN	(1 << 1)

struct mtd_config {
	int chip_count;
	int chip_0_offset;
	int chip_0_size;
	int chip_1_offset;
	int chip_1_size;
	int search_exponent;
	int data_setup_time;
	int data_hold_time;
	int address_setup_time;
	int data_sample_time;
	int row_address_size;
	int column_address_size;
	int read_command_code1;
	int read_command_code2;
	int boot_stream_major_version;
	int boot_stream_minor_version;
	int boot_stream_sub_version;
	int ncb_version;
	int boot_stream_1_address;
	int boot_stream_2_address;
	int flags;
};

//------------------------------------------------------------------------------
// This structure represents an MTD device in which we will write boot
// information.
//------------------------------------------------------------------------------

struct mtd_part {
	// A bit set where each bit corresponds to a block in a given MTD.
	uint32_t *bad_blocks;

	// The number of bad blocks appearing in this MTD.
	int nrbad;

        int oobinfochanged;
	struct nand_oobinfo old_oobinfo;
	int ecc;
};

/* partially implements mtd_data in kobs-ng */
struct mtd_bootblock {
	struct mtd_part part[2];
	/* writesize + oobsize buffer */
	void *buf;

	/* NCBs */
	NCB_BootBlockStruct_t *curr_ncb;
	NCB_BootBlockStruct_t ncb[2];
	loff_t ncb_ofs[2];
	int ncb_version;	/* 0, 1, or 3. Negative means error */

	/* LDLBs */
	NCB_BootBlockStruct_t *curr_ldlb;
	NCB_BootBlockStruct_t ldlb[2];
	loff_t ldlb_ofs[2];

	/* DBBTs */
	NCB_BootBlockStruct_t *curr_dbbt;
	NCB_BootBlockStruct_t dbbt[2];
	loff_t dbbt_ofs[2];
	/* the 2 NANDs */
	BadBlockTableNand_t *bbtn[2];

	/* In fact, we can reuse the boot block
	 * struct for mx53 on mx28, it's compatible
	 */

	/* FCB */
	BCB_ROM_BootBlockStruct_t  fcb;

	/* DBBT */
	BCB_ROM_BootBlockStruct_t  dbbt28;
};

#define PAGES_PER_STRIDE	64

/* Functions */
int ncb_get_version(void *ncb_candidate, NCB_BootBlockStruct_t **result);
int fcb_encrypt(BCB_ROM_BootBlockStruct_t *fcb, void *target, size_t size, int version);
int write_bootstream(const nv_param_part_t* part,
		     unsigned long bs_start_address,
		     int bs_size);
#endif	/* __DIGI_CMD_BOOTSTREAM_H */
