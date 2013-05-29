/*
 *  U-Boot/common/digi/cmd_nvram/partition.h
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision$
 *  !Author:     Markus Pietrek
 *  !Descr:      Provides a GUI frontend to "internal_nvram".
*/

#ifndef __DIGI_CMD_NVRAM_PARTITION_H
#define __DIGI_CMD_NVRAM_PARTITION_H

struct nv_param_part;

extern void PrintPartition(const struct nv_param_part* pPart);

extern int PartErase( const struct nv_param_part* pPart );
extern int PartRead(
        const struct nv_param_part* pPart,
        void* pvBuf,
        size_t iSize,
        char bSilent );
extern int PartReadAndDecompress(
        const struct nv_param_part* pPart,
        void* pvBuf,
        size_t iSize);
extern int PartHasBadBlock(
	const struct nv_param_part* pPart,
	uint64_t ullStartAddress);
extern int PartWrite(
        const struct nv_param_part* pPart,
        uint64_t ullStartAddress,
        const void* pvBuf,
        size_t iSize,
        char bOnTheFly);
extern int PartVerify(
        const struct nv_param_part* pPart,
        uint64_t ullStartAddress,
        const void* pvBuf,
        size_t iSize,
        char bOnTheFly );
extern int PartProtect(
        const struct nv_param_part* pPart,
        char bProtect );
extern 	void PrintProgress( int iPercentage, int iThrottle,
                            const char* szFmt, ... );
#ifdef CONFIG_PARTITION_SWAP
extern int PartSwap(
        const struct nv_param_part* pPart,
        void* pvBuf,
        size_t iSize );
#endif
extern uint64_t PartSize( const struct nv_param_part* pPart );

extern int PartGUI( void );
extern int PartStrAppendParts( char* szCmdLine, size_t iMaxSize );
extern int append_rootfs_parameters(char* szCmdLine, size_t iMaxSize, const struct nv_param_part* pPart);
extern int PartStrAppendRoot(char* szCmdLine, size_t iMaxSize, uint8_t boot_part);

extern int PartSetMtdParts( void );
extern int is_system_partition(const struct nv_param_part *pPart);

#endif  /* __DIGI_CMD_NVRAM_PARTITION_H */
