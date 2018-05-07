/*
 *  common/digi/cmd_nvram/mtd.h
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !Descr:      Provides interface for NAND and NOR
*/

#ifndef _MTD_H
#define _MTD_H

#ifdef CONFIG_CMD_NAND
# include <nand.h>
#endif  /* CONFIG_CMD_NAND */

#define ERROR "*** ERROR: "

extern int MtdRead(  int iChip, uint64_t ullOffs, size_t iLength, void* pvBuf );
extern int MtdRewrite( int iChip, uint64_t ullOffs, size_t iLength, const void* pvBuf );
extern int MtdWrite( int iChip, uint64_t ullOffs, size_t iLength, const void* pvBuf );
extern int MtdErase( int iChip, uint64_t ullOffs, size_t iLength );
extern int MtdInfo(  int iChip, uint64_t ullOffs, uchar* pbBad, size_t* piEraseSize );
extern uint64_t MtdSize( int iChip );
extern char     MtdBlockIsBad( int iChip, uint64_t ullOffs );
extern size_t   MtdGetEraseSize( int iChip, uint64_t ullOffs );
extern int      MtdProtect( int iChip, uint64_t ullOffs, size_t iLength, char bProtect );
extern void*    MtdVerifyAllocBuf( size_t iLength );
extern int      MtdVerifyFreeBuf( void* pvBuf );
extern int      MtdVerify( int iChip, uint64_t ullOffs, size_t iLength,
                           const void* pvSrc, void* pvBuf );
extern loff_t MemCmp(  const void* pvS1, const void* pvS2, size_t iSize );
extern void   MemDump( const void* pvBase, loff_t iOffset, size_t iLen );
static inline int get_uboot_part_size(void)
{
	if (nand_info[0].writesize < 4096)
		return PART_UBOOT_SIZE;
	else
		return PART_UBOOT_SIZE_4KPAGE;
}

#endif  /* _MTD_H */
