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

#include <common.h>
#include <asm/io.h>
#if defined(CONFIG_CMD_BOOTSTREAM) && defined(CONFIG_CMD_NAND)
#include <div64.h>
#include <command.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <net.h>                /* DHCP */
#include "partition.h"          /* MtdGetEraseSize */
#include "mtd.h"
#include "nand_device_info.h"
#ifdef CONFIG_MX28
#include "gpmi_nfc_gpmi.h"
#endif
#include "cmd_bootstream.h"
#include "BootControlBlocks.h"

const struct mtd_config default_mtd_config = {
	.chip_count = 1,
	.chip_0_offset = 0,
	.chip_0_size = 0,
	.chip_1_offset = 0,
	.chip_1_size = 0,
	.search_exponent = 2,
	.data_setup_time = 80,
	.data_hold_time = 60,
	.address_setup_time = 25,
	.data_sample_time = 6,
	.row_address_size = 3,
	.column_address_size = 2,
	.read_command_code1 = 0x00,
	.read_command_code2 = 0x30,
	.boot_stream_major_version = 1,
	.boot_stream_minor_version = 0,
	.boot_stream_sub_version = 0,
	.ncb_version = 3,
	.boot_stream_1_address = 0,
	.boot_stream_2_address = 0,
	.flags = 0,
};

int v1_rom_mtd_init(struct mtd_info *mtd,
		    struct mtd_config *cfg,
		    struct mtd_bootblock *bootblock,
		    unsigned int boot_stream_size_in_bytes,
		    uint64_t part_size)
{
#ifdef CONFIG_MX28
	unsigned int  stride_size_in_bytes;
	unsigned int  search_area_size_in_bytes;
	unsigned int  search_area_size_in_pages;
	unsigned int  max_boot_stream_size_in_bytes;
	unsigned int  boot_stream_size_in_pages;
	unsigned int  boot_stream1_pos;
	unsigned int  boot_stream2_pos;
	BCB_ROM_BootBlockStruct_t  *fcb;
	BCB_ROM_BootBlockStruct_t  *dbbt;
	struct nand_chip *chip = mtd->priv;
	struct gpmi_nfc_info *gpmi_info = chip->priv;

	//----------------------------------------------------------------------
	// Compute the geometry of a search area.
	//----------------------------------------------------------------------

	stride_size_in_bytes = PAGES_PER_STRIDE * mtd->writesize;
	search_area_size_in_bytes = (1 << cfg->search_exponent) * stride_size_in_bytes;
	search_area_size_in_pages = (1 << cfg->search_exponent) * PAGES_PER_STRIDE;

	//----------------------------------------------------------------------
	// Check if the target MTD is too small to even contain the necessary
	// search areas.
	//
	// Recall that the boot area for the i.MX28 appears at the beginning of
	// the first chip and contains two search areas: one each for the FCB
	// and DBBT.
	//----------------------------------------------------------------------

	if ((search_area_size_in_bytes * 2) > mtd->size) {
		fprintf(stderr, "mtd: mtd size too small\n");
		return -1;
	}

	//----------------------------------------------------------------------
	// Figure out how large a boot stream the target MTD could possibly
	// hold.
	//
	// The boot area will contain both search areas and two copies of the
	// boot stream.
	//----------------------------------------------------------------------

	max_boot_stream_size_in_bytes =

		lldiv(part_size - search_area_size_in_bytes * 2,
		//--------------------------------------------//
					2);

	//----------------------------------------------------------------------
	// Figure out how large the boot stream is.
	//----------------------------------------------------------------------

	boot_stream_size_in_pages =

		(boot_stream_size_in_bytes + (mtd->writesize - 1)) /
		//---------------------------------------------------//
				mtd->writesize;

	if (cfg->flags & F_VERBOSE) {
		printf("mtd: max_boot_stream_size_in_bytes = %d\n", max_boot_stream_size_in_bytes);
		printf("mtd: boot_stream_size_in_bytes = %d\n", boot_stream_size_in_bytes);
	}

	//----------------------------------------------------------------------
	// Check if the boot stream will fit.
	//----------------------------------------------------------------------

	if (boot_stream_size_in_bytes >= max_boot_stream_size_in_bytes) {
		fprintf(stderr, "mtd: bootstream too large\n");
		return -1;
	}

	//----------------------------------------------------------------------
	// Compute the positions of the boot stream copies.
	//----------------------------------------------------------------------

	boot_stream1_pos = 2 * search_area_size_in_bytes;
	boot_stream2_pos = boot_stream1_pos + max_boot_stream_size_in_bytes;

	if (cfg->flags & F_VERBOSE) {
		printf("mtd: #1 0x%08x - 0x%08x (0x%08x)\n",
				boot_stream1_pos, boot_stream1_pos + max_boot_stream_size_in_bytes,
				boot_stream1_pos + boot_stream_size_in_bytes);
		printf("mtd: #2 0x%08x - 0x%08x (0x%08x)\n",
				boot_stream2_pos, boot_stream2_pos + max_boot_stream_size_in_bytes,
				boot_stream2_pos + boot_stream_size_in_bytes);
	}

	//----------------------------------------------------------------------
	// Fill in the FCB.
	//----------------------------------------------------------------------

	fcb = &(bootblock->fcb);
	memset(fcb, 0, sizeof(*fcb));

	fcb->m_u32FingerPrint                        = FCB_FINGERPRINT;
	fcb->m_u32Version                            = FCB_VERSION_1;

	fcb->FCB_Block.m_NANDTiming.m_u8DataSetup    = cfg->data_setup_time;
	fcb->FCB_Block.m_NANDTiming.m_u8DataHold     = cfg->data_hold_time;
	fcb->FCB_Block.m_NANDTiming.m_u8AddressSetup = cfg->address_setup_time;
	fcb->FCB_Block.m_NANDTiming.m_u8DSAMPLE_TIME = cfg->data_sample_time;

	fcb->FCB_Block.m_u32PageDataSize             = mtd->writesize;
	fcb->FCB_Block.m_u32TotalPageSize            = mtd->writesize + mtd->oobsize;
	fcb->FCB_Block.m_u32SectorsPerBlock          = mtd->erasesize / mtd->writesize;

	if (mtd->writesize == 2048) {
                fcb->FCB_Block.m_u32NumEccBlocksPerPage      = mtd->writesize / 512 - 1;
                fcb->FCB_Block.m_u32MetadataBytes            = 10;
                fcb->FCB_Block.m_u32EccBlock0Size            = 512;
                fcb->FCB_Block.m_u32EccBlockNSize            = 512;
		fcb->FCB_Block.m_u32EccBlock0EccType         = ROM_BCH_Ecc_8bit;
		fcb->FCB_Block.m_u32EccBlockNEccType         = ROM_BCH_Ecc_8bit;

	} else if (mtd->writesize == 4096) {
		fcb->FCB_Block.m_u32NumEccBlocksPerPage      = (mtd->writesize / 512) - 1;
		fcb->FCB_Block.m_u32MetadataBytes            = 10;
		fcb->FCB_Block.m_u32EccBlock0Size            = 512;
		fcb->FCB_Block.m_u32EccBlockNSize            = 512;
		if (mtd->oobsize == 218) {
			fcb->FCB_Block.m_u32EccBlock0EccType = ROM_BCH_Ecc_16bit;
			fcb->FCB_Block.m_u32EccBlockNEccType = ROM_BCH_Ecc_16bit;
		} else if ((mtd->oobsize == 128)){
			fcb->FCB_Block.m_u32EccBlock0EccType = ROM_BCH_Ecc_8bit;
			fcb->FCB_Block.m_u32EccBlockNEccType = ROM_BCH_Ecc_8bit;
		}
	} else {
		fprintf(stderr, "Illegal page size %d\n", mtd->writesize);
	}

	fcb->FCB_Block.m_u32BootPatch                  = 0; // Normal boot.

	fcb->FCB_Block.m_u32Firmware1_startingPage   = boot_stream1_pos / mtd->writesize;
	fcb->FCB_Block.m_u32Firmware2_startingPage   = boot_stream2_pos / mtd->writesize;
	fcb->FCB_Block.m_u32PagesInFirmware1         = boot_stream_size_in_pages;
	fcb->FCB_Block.m_u32PagesInFirmware2         = boot_stream_size_in_pages;
#ifdef CONFIG_USE_NAND_DBBT
	fcb->FCB_Block.m_u32DBBTSearchAreaStartAddress = search_area_size_in_pages;
#else
	fcb->FCB_Block.m_u32DBBTSearchAreaStartAddress = 0;
#endif
	fcb->FCB_Block.m_u32BadBlockMarkerByte         = gpmi_info->m_u32BlkMarkByteOfs;
	fcb->FCB_Block.m_u32BadBlockMarkerStartBit     = gpmi_info->m_u32BlkMarkBitStart;
	fcb->FCB_Block.m_u32BBMarkerPhysicalOffset     = mtd->writesize;

	//----------------------------------------------------------------------
	// Fill in the DBBT.
	//----------------------------------------------------------------------

	dbbt = &(bootblock->dbbt28);
	memset(dbbt, 0, sizeof(*dbbt));

	dbbt->m_u32FingerPrint                = DBBT_FINGERPRINT2;
	dbbt->m_u32Version                    = 1;

	dbbt->DBBT_Block.v2.m_u32NumberBB        = 0;
	dbbt->DBBT_Block.v2.m_u32Number2KPagesBB = 0;
#endif /* CONFIG_MX28 */
	return 0;

}

int v2_rom_mtd_init(struct mtd_info *mtd,
		    struct mtd_config *cfg,
		    struct mtd_bootblock *bootblock,
		    unsigned int boot_stream_size_in_bytes,
		    uint64_t part_size)
{
	unsigned int  stride_size_in_bytes;
	unsigned int  search_area_size_in_bytes;
	unsigned int  search_area_size_in_pages;
	unsigned int  max_boot_stream_size_in_bytes;
	unsigned int  boot_stream_size_in_pages;
	unsigned int  boot_stream1_pos;
	unsigned int  boot_stream2_pos;
	BCB_ROM_BootBlockStruct_t  *fcb;
	BCB_ROM_BootBlockStruct_t  *dbbt;
#ifdef CONFIG_USE_NAND_DBBT
	struct mtd_part *mp;
	int j, k , thisbad, badmax,currbad;
	BadBlockTableNand_t *bbtn;
#endif

	//----------------------------------------------------------------------
	// Compute the geometry of a search area.
	//----------------------------------------------------------------------

	stride_size_in_bytes = mtd->erasesize;
	search_area_size_in_bytes = 4 * stride_size_in_bytes;
	search_area_size_in_pages = search_area_size_in_bytes / mtd->writesize;

	//----------------------------------------------------------------------
	// Check if the target MTD is too small to even contain the necessary
	// search areas.
	//
	// the first chip and contains two search areas: one each for the FCB
	// and DBBT.
	//----------------------------------------------------------------------

	if ((search_area_size_in_bytes * 2) > mtd->size) {
		fprintf(stderr, "mtd: mtd size too small\n");
		return -1;
	}

	//----------------------------------------------------------------------
	// Figure out how large a boot stream the target MTD could possibly
	// hold.
	//
	// The boot area will contain both search areas and two copies of the
	// boot stream.
	//----------------------------------------------------------------------

	max_boot_stream_size_in_bytes =

		lldiv(part_size - search_area_size_in_bytes * 2,
		//--------------------------------------------//
					2);

	//----------------------------------------------------------------------
	// Figure out how large the boot stream is.
	//----------------------------------------------------------------------

	boot_stream_size_in_pages =

		(boot_stream_size_in_bytes + (mtd->writesize - 1)) /
		//---------------------------------------------------//
				mtd->writesize;

	if (cfg->flags & F_VERBOSE) {
		printf("mtd: max_boot_stream_size_in_bytes = %d\n", max_boot_stream_size_in_bytes);
		printf("mtd: boot_stream_size_in_bytes = %d\n", boot_stream_size_in_bytes);
	}

	//----------------------------------------------------------------------
	// Check if the boot stream will fit.
	//----------------------------------------------------------------------

	if (boot_stream_size_in_bytes >= max_boot_stream_size_in_bytes) {
		fprintf(stderr, "mtd: bootstream too large\n");
		return -1;
	}

	//----------------------------------------------------------------------
	// Compute the positions of the boot stream copies.
	//----------------------------------------------------------------------

	boot_stream1_pos = 2 * search_area_size_in_bytes;
	boot_stream2_pos = boot_stream1_pos + max_boot_stream_size_in_bytes;

	if (cfg->flags & F_VERBOSE) {
		printf("mtd: #1 0x%08x - 0x%08x (0x%08x)\n",
				boot_stream1_pos, boot_stream1_pos + max_boot_stream_size_in_bytes,
				boot_stream1_pos + boot_stream_size_in_bytes);
		printf("mtd: #2 0x%08x - 0x%08x (0x%08x)\n",
				boot_stream2_pos, boot_stream2_pos + max_boot_stream_size_in_bytes,
				boot_stream2_pos + boot_stream_size_in_bytes);
	}

	//----------------------------------------------------------------------
	// Fill in the FCB.
	//----------------------------------------------------------------------

	fcb = &(bootblock->fcb);
	memset(fcb, 0, sizeof(*fcb));

	fcb->m_u32FingerPrint                        = FCB_FINGERPRINT;
	fcb->m_u32Version                            = 0x00000001;

	fcb->FCB_Block.m_u32Firmware1_startingPage = boot_stream1_pos / mtd->writesize;
	fcb->FCB_Block.m_u32Firmware2_startingPage = boot_stream2_pos / mtd->writesize;
	fcb->FCB_Block.m_u32PagesInFirmware1         = boot_stream_size_in_pages;
	fcb->FCB_Block.m_u32PagesInFirmware2         = boot_stream_size_in_pages;
#ifdef CONFIG_USE_NAND_DBBT
	fcb->FCB_Block.m_u32DBBTSearchAreaStartAddress = search_area_size_in_pages;
#else
	fcb->FCB_Block.m_u32DBBTSearchAreaStartAddress = 0;
#endif

#ifdef CONFIG_MXC_NAND_SWAP_BI
	/* Enable BI_SWAP */
	{
		unsigned int nand_sections =  mtd->writesize >> 9;
		unsigned int nand_oob_per_section = ((mtd->oobsize / nand_sections) >> 1) << 1;
		unsigned int nand_trunks =  mtd->writesize / (512 + nand_oob_per_section);
		fcb->FCB_Block.m_u32DISBBM = 1;
		fcb->FCB_Block.m_u32BadBlockMarkerByte =
			mtd->writesize - nand_trunks  * nand_oob_per_section;
		fcb->FCB_Block.m_u32BBMarkerPhysicalOffsetInSpareData
			= (nand_sections - 1) * (512 + nand_oob_per_section) + 512 + 1;
	}
#endif
	//----------------------------------------------------------------------
	// Fill in the DBBT.
	//----------------------------------------------------------------------

	dbbt = &(bootblock->dbbt28);
	memset(dbbt, 0, sizeof(*dbbt));

	dbbt->m_u32FingerPrint                = DBBT_FINGERPRINT2;
	dbbt->m_u32Version                    = 1;

	/* Only check boot partition that ROM support */

#ifdef CONFIG_USE_NAND_DBBT
	mp = &md->part[0];
	if (mp->nrbad == 0)
		return 0;

	md->bbtn[0] = malloc(2048); /* single page */
	if (md->bbtn[0] == NULL) {
		fprintf(stderr, "mtd: failed to allocate BBTN#%d\n", 2048);
		return -1;
	}

	bbtn = md->bbtn[0];
	memset(bbtn, 0, sizeof(*bbtn));

	badmax = ARRAY_SIZE(bbtn->u32BadBlock);
	thisbad = mp->nrbad;
	if (thisbad > badmax)
		thisbad = badmax;


	dbbt->DBBT_Block.v2.m_u32NumberBB = thisbad;
	dbbt->DBBT_Block.v2.m_u32Number2KPagesBB = 1; /* one page should be enough*/

	bbtn->uNumberBB = thisbad;

	/* fill in BBTN */
	j = mtd->size / mtd->erasesize;
	currbad = 0;
	for (k = 0; k < j && currbad < thisbad; k++) {
		if ((mp->bad_blocks[k >> 5] & (1 << (k & 31))) == 0)
			continue;
		bbtn->u32BadBlock[currbad++] = k;
	}
#endif
	return 0;

}

//------------------------------------------------------------------------------
// This function writes the search areas for a given BCB. It will write *two*
// search areas for a given BCB. If there are multiple chips, it will write one
// search area on each chip. If there is one chip, it will write two search
// areas on the first chip.
//
// md         A pointer to the current struct mtd_data.
// bcb_name   A pointer to a human-readable string that indicates what kind of
//            BCB we're writing. This string will only be used in log messages.
// ofs1       If there is one chips, the index of the
// ofs2
// ofs_mchip  If there are multiple chips, the index of the search area to write
//            on both chips.
// end        The number of consecutive search areas to be written.
// size       The size of the BCB data to be written.
// ecc        Indicates whether or not to use hardware ECC.
//------------------------------------------------------------------------------

int mtd_commit_bcb(struct mtd_info *mtd,
		   struct mtd_config *cfg,
		   struct mtd_bootblock *bootblock,
		   char *bcb_name,
		   loff_t ofs1, loff_t ofs2, loff_t ofs_mchip,
		   loff_t end, size_t size)
{
	int chip;
	loff_t end_index, search_area_indices[2], o;
	int err = 0, r = -1;
	int i;
	int j;
	unsigned stride_size_in_bytes;
	unsigned search_area_size_in_strides;
	unsigned search_area_size_in_bytes;
	struct nand_chip *nandchip = mtd->priv;
	size_t nbytes = 0;
	char *readbuf = NULL;
	unsigned count;

	readbuf = malloc(mtd->writesize);
	if (NULL == readbuf)
		return -1;

	//----------------------------------------------------------------------
	// Compute some important facts about geometry.
	//----------------------------------------------------------------------
#ifdef MX_USE_SINGLE_PAGE_STRIDE	/* mx23, mx28 */
		stride_size_in_bytes        = mtd->erasesize;
		search_area_size_in_strides = 4;
		search_area_size_in_bytes   = search_area_size_in_strides * stride_size_in_bytes;
		count = 2; //write two copy on mx23/28
#else
		stride_size_in_bytes        = PAGES_PER_STRIDE * mtd->writesize;
		search_area_size_in_strides = 1 << cfg->search_exponent;
		search_area_size_in_bytes   = search_area_size_in_strides * stride_size_in_bytes;
		count = 1; //only write one copy
#endif
	/* NOTE (hpalacio): For i.MX28 we are not defining MX_USE_SINGLE_PAGE_STRIDE and
	 * so we are getting into the 'else' above. The calculated figures result the same
	 * except for the 'count' field. Having the 'count' field to a value of 2 simply
	 * loops to write twice the four FCB or DBBT copies.
	 */

	//----------------------------------------------------------------------
	// Check whether there are multiple chips and set up the two search area
	// indices accordingly.
	//----------------------------------------------------------------------

	if (cfg->chip_count > 1)
		search_area_indices[0] = search_area_indices[1] = ofs_mchip;
	else {
		search_area_indices[0] = ofs1;
		search_area_indices[1] = ofs2;
	}

	//----------------------------------------------------------------------
	// Loop over search areas for this BCB.
	//----------------------------------------------------------------------

	for (i = 0; !err && i < count; i++) {

		//--------------------------------------------------------------
		// Compute the search area index that marks the end of the
		// writing on this chip.
		//--------------------------------------------------------------

		end_index = search_area_indices[i] + end;

		//--------------------------------------------------------------
		// Figure out which chip we're writing.
		//--------------------------------------------------------------

		chip = (cfg->chip_count > 1) ? i : 0;

		//--------------------------------------------------------------
		// Loop over consecutive search areas to write.
		//--------------------------------------------------------------

		for (; search_area_indices[i] < end_index; search_area_indices[i]++) {

			//------------------------------------------------------
			// Compute the byte offset of the beginning of this
			// search area.
			//------------------------------------------------------

			o = search_area_indices[i] * search_area_size_in_bytes;

			//------------------------------------------------------
			// Loop over strides in this search area.
			//------------------------------------------------------

			for (j = 0; j < search_area_size_in_strides; j++, o += stride_size_in_bytes) {

				//----------------------------------------------
				// If we're crossing into a new block, erase it
				// first.
				//----------------------------------------------

				if (llmod(o, mtd->erasesize) == 0) {
					if (cfg->flags & F_VERBOSE) {
						fprintf(stdout, "erasing block at 0x%llx\n", o);
					}
					if (!(cfg->flags & F_DRYRUN)) {
						r = MtdErase(chip, o, mtd->erasesize);
						if (r < 0) {
							fprintf(stderr, "mtd: Failed to erase block @0x%llx\n", o);
							err++;
							continue;
						}
					}
				}

				//----------------------------------------------
				// Write & verify the page.
				//----------------------------------------------

				if (cfg->flags & F_VERBOSE)
					fprintf(stdout, "mtd: Writing %s%d @%d:0x%llx(%x)\n",
								bcb_name, j, chip, o, size);

				if (!(cfg->flags & F_DRYRUN)) {
					if (size == mtd->writesize + mtd->oobsize) {
						/* We're going to write a raw page (data+oob).
						 * Change the mode to RAW. Then restore it. */
						int old_mode = nandchip->ops.mode;

						nandchip->ops.datbuf = bootblock->buf;
						nandchip->ops.mode = MTD_OOB_RAW;
						r = mtd->write_oob(mtd, o, &nandchip->ops);
						if (r) {
							fprintf(stderr, "mtd: Failed to write %s @%d: 0x%llx (%d)\n",
								bcb_name, chip, o, r);
							err ++;
						}
						//------------------------------------------------------
						// Verify the written data
						//------------------------------------------------------
						nandchip->ops.datbuf = (void *)readbuf;
						r = mtd->read_oob(mtd, o, &nandchip->ops);
						if (r) {
							fprintf(stderr, "mtd: Failed to read @0x%llx (%d)\n", o, r);
							err++;
							goto _error;
						}
						if (memcmp(bootblock->buf, readbuf, mtd->writesize)) {
							fprintf(stderr, "mtd: Verification error @0x%llx\n", o);
							err++;
							goto _error;
						}

						if (cfg->flags & F_VERBOSE)
							fprintf(stdout, "mtd: Verified %s%d @%d:0x%llx(%x)\n",
										bcb_name, j, chip, o, size);

						/* restore mode */
						nandchip->ops.mode = old_mode;
					}
					else {
						r = mtd->write(mtd, o, size, &nbytes, (const void *)bootblock->buf);
						if (r || nbytes != size) {
							fprintf(stderr, "mtd: Failed to write %s @%d: 0x%llx (%d)\n",
								bcb_name, chip, o, r);
							err ++;
						}
						//------------------------------------------------------
						// Verify the written data
						//------------------------------------------------------
						r = mtd->read(mtd, o, size, &nbytes, (void *)readbuf);
						if (r || nbytes != size) {
							fprintf(stderr, "mtd: Failed to read @0x%llx (%d)\n", o, r);
							err++;
							goto _error;
						}
						if (memcmp(bootblock->buf, readbuf, size)) {
							fprintf(stderr, "mtd: Verification error @0x%llx\n", o);
							err++;
							goto _error;
						}
						if (cfg->flags & F_VERBOSE)
							fprintf(stdout, "mtd: Verified %s%d @%d:0x%llx(%x)\n",
										bcb_name, j, chip, o, size);
					}
				}
			}

		}

	}

	if (cfg->flags & F_VERBOSE)
		printf("%s(%s): status %d\n", __func__, bcb_name, err);

_error:
	free(readbuf);
	return err;
}

int v1_rom_mtd_commit_structures(struct mtd_info *mtd,
				 struct mtd_config *cfg,
				 struct mtd_bootblock *bootblock,
				 unsigned long bs_start_address,
				 unsigned int boot_stream_size_in_bytes)
{
	int startpage, start, size;
	unsigned int search_area_size_in_bytes, stride_size_in_bytes;
	int i, r, chunk;
	loff_t ofs, end;
	int chip = 0;
	unsigned long read_addr;
	size_t nbytes = 0;
	char *readbuf = NULL;

	readbuf = malloc(mtd->writesize);
	if (NULL == readbuf)
		return -1;

	//----------------------------------------------------------------------
	// Compute some important facts about geometry.
	//----------------------------------------------------------------------

	stride_size_in_bytes = PAGES_PER_STRIDE * mtd->writesize;
	search_area_size_in_bytes = (1 << cfg->search_exponent) * stride_size_in_bytes;

	//----------------------------------------------------------------------
	// Construct the ECC decorations and such for the FCB.
	//----------------------------------------------------------------------

	size = mtd->writesize + mtd->oobsize;

	if (cfg->flags & F_VERBOSE) {
		if (bootblock->ncb_version != cfg->ncb_version)
			printf("NCB versions differ, %d is used.\n", cfg->ncb_version);
	}

	r = fcb_encrypt(&bootblock->fcb, bootblock->buf, size, 1);
	if (r < 0)
		goto _error;;

	//----------------------------------------------------------------------
	// Write the FCB search area.
	//----------------------------------------------------------------------
	mtd_commit_bcb(mtd, cfg, bootblock, "FCB", 0, 0, 0, 1, size);

	//----------------------------------------------------------------------
	// Write the DBBT search area.
	//----------------------------------------------------------------------

	memset(bootblock->buf, 0, size);
	memcpy(bootblock->buf, &(bootblock->dbbt28), sizeof(bootblock->dbbt28));

	mtd_commit_bcb(mtd, cfg, bootblock, "DBBT", 1, 1, 1, 1, mtd->writesize);

	//----------------------------------------------------------------------
	// Loop over the two boot streams.
	//----------------------------------------------------------------------

	for (i = 0; i < 2; i++) {
		/* Set start address where bootstream is in RAM */
		read_addr = bs_start_address;

		//--------------------------------------------------------------
		// Figure out where to put the current boot stream.
		//--------------------------------------------------------------

		if (i == 0) {
			startpage = bootblock->fcb.FCB_Block.m_u32Firmware1_startingPage;
			size      = bootblock->fcb.FCB_Block.m_u32PagesInFirmware1;
			end       = bootblock->fcb.FCB_Block.m_u32Firmware2_startingPage;
		} else {
			startpage = bootblock->fcb.FCB_Block.m_u32Firmware2_startingPage;
			size      = bootblock->fcb.FCB_Block.m_u32PagesInFirmware2;
			end       = lldiv(mtd->size, mtd->writesize);
		}

		//--------------------------------------------------------------
		// Compute the byte addresses corresponding to the page
		// addresses.
		//--------------------------------------------------------------

		start = startpage * mtd->writesize;
		size  = size      * mtd->writesize;
		end   = end       * mtd->writesize;

		if (cfg->flags & F_VERBOSE)
			printf("mtd: Writting firmware image #%d @%d: 0x%08x - 0x%08x\n", i,
					chip, start, start + size);

		//--------------------------------------------------------------
		// Loop over pages as we write them.
		//--------------------------------------------------------------

		ofs = start;
		while (ofs < end && size > 0) {

			//------------------------------------------------------
			// Check if the current block is bad.
			//------------------------------------------------------

			while (mtd->block_isbad(mtd, ofs) == 1) {
				if (cfg->flags & F_VERBOSE)
					fprintf(stdout, "mtd: Skipping bad block at 0x%llx\n", ofs);
				ofs += mtd->erasesize;
			}

			chunk = size;

			//------------------------------------------------------
			// Check if we've entered a new block and, if so, erase
			// it before beginning to write it.
			//------------------------------------------------------

			if (llmod(ofs, mtd->erasesize) == 0) {
				if (cfg->flags & F_VERBOSE) {
					fprintf(stdout, "erasing block at 0x%llx\n", ofs);
				}
				if (!(cfg->flags & F_DRYRUN)) {
					r = MtdErase(chip, ofs, mtd->erasesize);
					if (r < 0) {
						fprintf(stderr, "mtd: Failed to erase block @0x%llx\n", ofs);
						ofs += mtd->erasesize;
						continue;
					}
				}
			}

			if (chunk > mtd->writesize)
				chunk = mtd->writesize;

			//------------------------------------------------------
			// Write the current chunk to the medium.
			//------------------------------------------------------

			if (cfg->flags & F_VERBOSE) {
				fprintf(stdout, "Writing bootstream file from 0x%lx to offset 0x%llx\n", read_addr, ofs);
			}
			if (!(cfg->flags & F_DRYRUN)) {
				r = mtd->write(mtd, ofs, chunk, &nbytes, (const void *)read_addr);
				if (r || nbytes != chunk) {
					fprintf(stderr, "mtd: Failed to write BS @0x%llx (%d)\n", ofs, r);
				}
			}
			//------------------------------------------------------
			// Verify the written data
			//------------------------------------------------------
			r = mtd->read(mtd, ofs, chunk, &nbytes, (void *)readbuf);
			if (r || nbytes != chunk) {
				fprintf(stderr, "mtd: Failed to read BS @0x%llx (%d)\n", ofs, r);
				goto _error;
			}
			if (memcmp((void *)read_addr, readbuf, chunk)) {
				fprintf(stderr, "mtd: Verification error @0x%llx\n", ofs);
				goto _error;
			}

			ofs += mtd->writesize;
			read_addr += mtd->writesize;
			size -= chunk;
		}
		if (cfg->flags & F_VERBOSE)
			printf("mtd: Verified firmware image #%d @%d: 0x%08x - 0x%08x\n", i,
					chip, start, start + size);

		//--------------------------------------------------------------
		// Check if we ran out of room.
		//--------------------------------------------------------------

		if (ofs >= end) {
			fprintf(stderr, "mtd: Failed to write BS#%d\n", i);
			goto _error;
		}
	}

	free(readbuf);
	return 0;
_error:
	free(readbuf);
	return -1;
}

int v2_rom_mtd_commit_structures(struct mtd_info *mtd,
		 struct mtd_config *cfg,
		 struct mtd_bootblock *bootblock,
		 unsigned long bs_start_address,
		 unsigned int boot_stream_size_in_bytes)
{
	int startpage, start, size;
	unsigned int search_area_size_in_bytes, stride_size_in_bytes;
	int i, r, chunk;
	loff_t ofs, end;
	int chip = 0;
	unsigned long read_addr;
	size_t nbytes = 0;
	char *readbuf = NULL;

	readbuf = malloc(mtd->writesize);
	if (NULL == readbuf)
		return -1;

	//----------------------------------------------------------------------
	// Compute some important facts about geometry.
	//----------------------------------------------------------------------

	stride_size_in_bytes = mtd->erasesize;
	search_area_size_in_bytes = 4 * stride_size_in_bytes;

	//----------------------------------------------------------------------
	// Construct the ECC decorations and such for the FCB.
	//----------------------------------------------------------------------

	size = mtd->writesize + mtd->oobsize;

	if (cfg->flags & F_VERBOSE) {
		if (bootblock->ncb_version != cfg->ncb_version)
			printf("NCB versions differ, %d is used.\n", cfg->ncb_version);
	}

	//----------------------------------------------------------------------
	// Write the FCB search area.
	//----------------------------------------------------------------------
	memset(bootblock->buf, 0, size);
	memcpy(bootblock->buf, &(bootblock->fcb), sizeof(bootblock->fcb));

	mtd_commit_bcb(mtd, cfg, bootblock, "FCB", 0, 0, 0, 1, mtd->writesize);

	//----------------------------------------------------------------------
	// Write the DBBT search area.
	//----------------------------------------------------------------------

	memset(bootblock->buf, 0, size);
	memcpy(bootblock->buf, &(bootblock->dbbt28), sizeof(bootblock->dbbt28));

	mtd_commit_bcb(mtd, cfg, bootblock, "DBBT", 1, 1, 1, 1, mtd->writesize);

	//----------------------------------------------------------------------
	// Write the DBBT table area.
	//----------------------------------------------------------------------

	memset(bootblock->buf, 0, size);

#ifdef CONFIG_USE_NAND_DBBT
	if (bootblock->dbbt28.DBBT_Block.v2.m_u32Number2KPagesBB> 0 && md->bbtn[0] != NULL) {
		memcpy(md->buf, md->bbtn[0], sizeof(*md->bbtn[0]));

		ofs = search_area_size_in_bytes;

		for (i=0; i < 4; i++, ofs += stride_size_in_bytes) {

			if (md->flags & F_VERBOSE)
				printf("mtd: PUTTING down DBBT%d BBTN%d @0x%llx (0x%x)\n", i, 0,
					ofs + 4 * mtd->writesize, mtd->writesize);

			r = mtd_write_page(md, chip, ofs + 4 * mtd->writesize, 1);
			if (r != mtd->writesize) {
				fprintf(stderr, "mtd: Failed to write BBTN @0x%llx (%d)\n", ofs, r);
			}
		}
	}
#endif
	//----------------------------------------------------------------------
	// Loop over the two boot streams.
	//----------------------------------------------------------------------

	for (i = 0; i < 2; i++) {
		/* Set start address where bootstream is in RAM */
		read_addr = bs_start_address;

		//--------------------------------------------------------------
		// Figure out where to put the current boot stream.
		//--------------------------------------------------------------
		if (i == 0) {
			startpage = bootblock->fcb.FCB_Block.m_u32Firmware1_startingPage;
			size      = bootblock->fcb.FCB_Block.m_u32PagesInFirmware1;
			end       = bootblock->fcb.FCB_Block.m_u32Firmware2_startingPage;
		} else {
			startpage = bootblock->fcb.FCB_Block.m_u32Firmware2_startingPage;
			size      = bootblock->fcb.FCB_Block.m_u32PagesInFirmware2;
			end       = lldiv(mtd->size, mtd->writesize);
		}

		//--------------------------------------------------------------
		// Compute the byte addresses corresponding to the page
		// addresses.
		//--------------------------------------------------------------

		start = startpage * mtd->writesize;
		size  = size      * mtd->writesize;
		end   = end       * mtd->writesize;

		if (cfg->flags & F_VERBOSE)
			printf("mtd: Writting firmware image #%d @%d: 0x%08x - 0x%08x\n", i,
					chip, start, start + size);

		//--------------------------------------------------------------
		// Loop over pages as we write them.
		//--------------------------------------------------------------

		ofs = start;
		while (ofs < end && size > 0) {

			//------------------------------------------------------
			// Check if the current block is bad.
			//------------------------------------------------------

			while (mtd->block_isbad(mtd, ofs) == 1) {
				if (cfg->flags & F_VERBOSE)
					printf("mtd: Skipping bad block at 0x%llx\n", ofs);
				ofs += mtd->erasesize;
			}

			chunk = size;

			//------------------------------------------------------
			// Check if we've entered a new block and, if so, erase
			// it before beginning to write it.
			//------------------------------------------------------

			if (llmod(ofs, mtd->erasesize) == 0) {
				if (cfg->flags & F_VERBOSE) {
					fprintf(stdout, "erasing block at 0x%llx\n", ofs);
				}
				if (!(cfg->flags & F_DRYRUN)) {
					r = MtdErase(chip, ofs, mtd->erasesize);
					if (r < 0) {
						fprintf(stderr, "mtd: Failed to erase block @0x%llx\n", ofs);
						ofs += mtd->erasesize;
						continue;
					}
				}
			}

			if (chunk > mtd->writesize)
				chunk = mtd->writesize;

			//------------------------------------------------------
			// Write the current chunk to the medium.
			//------------------------------------------------------
			if (cfg->flags & F_VERBOSE) {
				fprintf(stdout, "Writing bootstream file from 0x%lx to offset 0x%llx\n", read_addr, ofs);
			}
			if (!(cfg->flags & F_DRYRUN)) {
				r = mtd->write(mtd, ofs, chunk, &nbytes, (const void *)read_addr);
				if (r || nbytes != chunk) {
					fprintf(stderr, "mtd: Failed to write BS @0x%llx (%d)\n", ofs, r);
				}
			}

			//------------------------------------------------------
			// Verify the written data
			//------------------------------------------------------
			r = mtd->read(mtd, ofs, chunk, &nbytes, (void *)readbuf);
			if (r || nbytes != chunk) {
				fprintf(stderr, "mtd: Failed to read BS @0x%llx (%d)\n", ofs, r);
				goto _error;
			}
			if (memcmp((void *)read_addr, readbuf, chunk)) {
				fprintf(stderr, "mtd: Verification error @0x%llx\n", ofs);
				goto _error;
			}

			ofs += mtd->writesize;
			read_addr += mtd->writesize;
			size -= chunk;
		}

		//--------------------------------------------------------------
		// Check if we ran out of room.
		//--------------------------------------------------------------

		if (ofs >= end) {
			fprintf(stderr, "mtd: Failed to write BS#%d\n", i);
			return -1;
		}

	}

	free(readbuf);
	return 0;
_error:
	free(readbuf);
	return -1;
}

int write_bootstream(const nv_param_part_t* part,
		     unsigned long bs_start_address,
		     int bs_size)
{
	/* TODO: considering chip = 0 */
	int chip = 0;
	uint8_t id_bytes[NAND_DEVICE_ID_BYTE_COUNT];
	struct mtd_info *mtd = &nand_info[chip];
	struct nand_chip *this = mtd->priv;
	struct nand_device_info  *dev_info;
	int i, r = -1;
	struct mtd_config cfg;
	struct mtd_bootblock bootblock;

	/* copy defaults */
	memcpy(&cfg, &default_mtd_config, sizeof(cfg));

	/* flags (TODO: parse command arguments?) */
	//cfg.flags |= F_VERBOSE;
	//cfg.flags |= F_DRYRUN;

	/* alloc buffer */
	bootblock.buf = malloc(mtd->writesize + mtd->oobsize);
	if (NULL == bootblock.buf) {
		fprintf(stderr, "mtd: unable to allocate page buffer\n");
		return -1;
	}

	/* Read ID bytes from the first NAND Flash chip. */
	this->select_chip(mtd, chip);

	this->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	for (i = 0; i < NAND_DEVICE_ID_BYTE_COUNT; i++)
		id_bytes[i] = this->read_byte(mtd);

	/* Get information about this device, based on the ID bytes. */
	dev_info = nand_device_get_info(id_bytes);

	/* Check if we understand this device. */
	if (dev_info) {
		/* Update configuration with values for detected chip */
		cfg.data_setup_time = dev_info->data_setup_in_ns;
		cfg.data_hold_time = dev_info->data_hold_in_ns;
		cfg.address_setup_time = dev_info->address_setup_in_ns;
		cfg.data_sample_time = dev_info->gpmi_sample_delay_in_ns;
	}
	else {
		printf("Unrecognized NAND Flash device. Using default values.\n");
	}

	printf("Writing bootstream...");
#if defined(CONFIG_MX28)
	r = v1_rom_mtd_init(mtd, &cfg, &bootblock, bs_size, part->ullSize);
#elif defined(CONFIG_MX53)
	r = v2_rom_mtd_init(mtd, &cfg, &bootblock, bs_size, part->ullSize);
#endif
	if (r < 0) {
		printf("mtd_init failed!\n");
	}
	else {
#if defined(CONFIG_MX28)
		r = v1_rom_mtd_commit_structures(mtd, &cfg, &bootblock, bs_start_address, bs_size);
#elif defined(CONFIG_MX53)
		r = v2_rom_mtd_commit_structures(mtd, &cfg, &bootblock, bs_start_address, bs_size);
#endif
	}

	/* free buffer */
	free(bootblock.buf);
	if (r)
		printf("FAILED\n");
	else
		printf("OK\n");

	return r;
}

#endif	/* CONFIG_CMD_BOOTSTREAM && CONFIG_CMD_NAND */
