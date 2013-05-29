/*
 *  cmd_nvram/nvram_priv_uboot.c
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
 *  !Descr:      Defines the private functions needed by the nvram core to
 *               access Flash and a console for U-Boot.
 */

#include <common.h>

#include "nvram_priv.h"
#include "env.h"
#include "mtd.h"
#include "partition.h"

size_t uboot_part_size = PART_UBOOT_SIZE;
extern uint8_t modified_parts[NV_MAX_PARTITIONS];

#define OFFS( x ) ( uboot_part_size + ( x ) )

#define CE( sCmd ) \
        do { \
                if( !(sCmd) )       \
                        goto error;    \
        } while( 0 )

#ifndef PART_FPGA_SIZE
# define PART_FPGA_SIZE 0
#endif

#define NVRAM_CHIP 0

DECLARE_GLOBAL_DATA_PTR;

extern size_t nvram_part_size;

/* ********** local functions ********** */

/* Returns the total size of the partitions before the
 * one passed as parameter */
uint64_t sizeof_partsbelow(nv_param_part_t *table, int part)
{
	uint64_t size = 0;

	while (part-- > 0)
		size += table[part].ullSize;

	return size;
}

static int NvPrivOSIntInRange( loff_t iOffs, size_t iLength )
{
        int iRes = 1;

        if( ( iOffs + iLength ) > nvram_part_size )
                iRes = NV_SET_ERROR( NVE_NO_SPACE, "" );

        return iRes;
}

static void NvPrivOSAddPartitions(
        nv_critical_t* pCrit,
        const nv_param_part_t* pTable,
        size_t iEntries )
{
        int i;
        uint64_t ullSizeFromEnd = 0;

        /* the partition table in flash only knows positive start addresses.
         * To support placing partitions from the end, we allow that
         * pTable->ullStart is actual signed and negative. If < 0 then
         * partitions runs from end. If partitions grow big enough that this
         * becomes a problem, I'll be happy that it survived that long. */

        /* determine how much space are counted from end-of-flash*/
        for( i = 0; i < iEntries; i++ ) {
                if( (int64_t) pTable[ i ].ullStart < 0 )
                        ullSizeFromEnd += pTable[ i ].ullSize;
        }

        /* now do all partitions coming from 0 */
        for( i = 0; i < iEntries; i++ ) {
                nv_param_part_t* pPart = &pCrit->s.p.xPartTable.axEntries[ pCrit->s.p.xPartTable.uiEntries ];

                if( (int64_t) pTable[ i ].ullStart < 0 )
                        continue;

                ASSERT( i < ARRAY_SIZE( pCrit->s.p.xPartTable.axEntries ) - 1 );
                *pPart = pTable[ i ];

                if( !pPart->ullSize )
                        /* convert 0 to to-end-of-flash */
                        pPart->ullSize = PartSize( pPart ) - ullSizeFromEnd;

                pCrit->s.p.xPartTable.uiEntries++;
        }

        /* now do all partitions coming from end of flash */
        for( i = 0; i < iEntries; i++ ) {
                nv_param_part_t* pPart = &pCrit->s.p.xPartTable.axEntries[ pCrit->s.p.xPartTable.uiEntries ];

                if( (int64_t) pTable[ i ].ullStart >= 0 )
                        continue;

                ASSERT( i < ARRAY_SIZE( pCrit->s.p.xPartTable.axEntries ) - 1 );
                *pPart = pTable[ i ];

                if( (int64_t) pPart->ullStart < 0 ) {
                        /* convert 0 to to-end-of-flash */
                        pPart->ullStart += MtdSize( pPart->uiChip );
                }

                pCrit->s.p.xPartTable.uiEntries++;
        }
}

/* ********** global functions ********** */

int NvPrivOSInit( void )
{
        return 1;
}

int NvPrivOSPostInit( void )
{
        int i;
        const nv_critical_t* pCrit;

        /* protects all fixed and read-only partitions against accidently
         * modifications.
         * This is called right after NVRAM/Partition table has been read, so
         * protects it early */
        CE( NvCriticalGet( (nv_critical_t**) &pCrit ) );

        for( i = 0; i < pCrit->s.p.xPartTable.uiEntries; i++ ) {
                /* each parttition */
                const nv_param_part_t* pPart = &pCrit->s.p.xPartTable.axEntries[ i ];

                if( pPart->flags.bFixed || pPart->flags.bReadOnly )
                        /* avoid accidently changes */
                        PartProtect( pPart, 1 );
        }

        return 1;

error:
        return 0;
}

int NvPrivOSFinish( void )
{
        return 1;
}

int NvPrivOSCriticalPostReset(
        struct nv_critical* pCrit )
{
        CE( NvOSCfgAdd( NVOS_UBOOT, CONFIG_ENV_SIZE ) );

        /* nothing to do */
        return 1;

error:
        return 0;
}

int NvPrivOSCriticalPartReset(
        struct nv_critical* pCrit,
        nv_os_type_e eForOS )
{
        static nv_param_part_t axPartitionTable[10];
	int part;
	int sys_parts = 1;
	int i;
	int rootfs_part_size = PART_ROOTFS_SIZE;

#ifdef CONFIG_DUAL_BOOT
	sys_parts = 2;
#endif

	/* reset modified partitions array */
	memset(modified_parts, 0, sizeof(modified_parts));
#ifndef CONFIG_NETOS_BRINGUP
	/* Loader partitions */
#ifdef CONFIG_CMD_BOOTSTREAM
	/* U-Boot bootstream */
	part = 0;
	strcpy(axPartitionTable[part].szName, "Bstrm-U-Boot");
	axPartitionTable[part].eType = NVPT_BOOTSTREAM;
	axPartitionTable[part].flags.bFixed = 1;
	axPartitionTable[part].ullStart = 0;
	axPartitionTable[part].ullSize = PART_UBOOT_SIZE;
#else
	/* U-Boot */
	part = 0;
	strcpy(axPartitionTable[part].szName, "U-Boot");
	axPartitionTable[part].eType = NVPT_UBOOT;
	axPartitionTable[part].flags.bFixed = 1;
	axPartitionTable[part].ullStart = 0;
	axPartitionTable[part].ullSize = uboot_part_size;
#endif
	modified_parts[part] = 1;
	/* NVRAM */
	part++;
	strcpy(axPartitionTable[part].szName, "NVRAM");
	axPartitionTable[part].eType = NVPT_NVRAM;
	axPartitionTable[part].flags.bFixed = 1;
	axPartitionTable[part].ullStart  = sizeof_partsbelow(axPartitionTable, part);
	axPartitionTable[part].ullSize = nvram_part_size;
	modified_parts[part] = 1;
# if PART_FPGA_SIZE > 0
	/* FPGA */
	part++;
	strcpy(axPartitionTable[part].szName, "FPGA");
	axPartitionTable[part].eType = NVPT_FPGA;
	axPartitionTable[part].flags.bFixed = 1;
	axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
	axPartitionTable[part].ullSize = PART_FPGA_SIZE;
	modified_parts[part] = 1;
# endif
#else
	/* NETOS bringup partitions */
	/* Loader */
	part = 0;
	strcpy(axPartitionTable[part].szName, "NET+OS-Loader");
	axPartitionTable[part].eType = NVPT_NETOS_LOADER;
	axPartitionTable[part].flags.bFixed = 1;
	axPartitionTable[part].ullStart = 0;
	axPartitionTable[part].ullSize  = PART_NETOS_LOADER_SIZE;
	modified_parts[part] = 1;
	/* Kernel */
	part++;
	strcpy(axPartitionTable[part].szName, PART_NAME_NETOS);
	axPartitionTable[part].eType = NVPT_NETOS;
	axPartitionTable[part].flags.bFixed = 0;
	axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
	axPartitionTable[part].ullSize  = PART_NETOS_KERNEL_SIZE;
	modified_parts[part] = 1;
	/* NVRAM */
	part++;
	strcpy(axPartitionTable[part].szName, "NET+OS-NVRAM");
	axPartitionTable[part].eType = NVPT_NETOS_NVRAM;
	axPartitionTable[part].flags.bFixed = 1;
	/* negative start means start from end of flash*/
	axPartitionTable[part].ullStart = (int64_t) -PART_NETOS_NVRAM_SIZE;
	axPartitionTable[part].ullSize  = PART_NETOS_NVRAM_SIZE;
	modified_parts[part] = 1;
#endif

	switch( eForOS ) {
		case NVOS_NONE:
		break;
#ifndef CONFIG_NETOS_BRINGUP
		case NVOS_LINUX:
		case NVOS_ANDROID:
			/* Linux partitions */

#if defined(PART_ROOTFS_SIZE_LARGE) && defined(FLASH_SIZE_SMALLEST)
			/* Compute dynamic size of ROOTFS partition.
			 * Depending on Flash size, ROOTFS partition will have
			 * more or less space. For Flash size bigger than 128MB
			 * use a ROOTFS of 160MB. Other
			 */
			if (MtdSize(0) > FLASH_SIZE_SMALLEST)
				rootfs_part_size = PART_ROOTFS_SIZE_LARGE;
#endif /* PART_ROOTFS_SIZE_LARGE */

#ifdef CONFIG_UBOOT_SPLASH
			/* Splash */
			part++;
			strcpy(axPartitionTable[part].szName, "Splash");
			axPartitionTable[part].eType = NVPT_SPLASH_SCREEN;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = PART_SPLASH_SIZE;
			modified_parts[part] = 1;
#endif
			for (i=0; i < sys_parts; i++) {
				/* Check if there is enough space in Flash to hold dual boot partitions */
				if (i > 0) {
					if (MtdSize(0) <
					    (sizeof_partsbelow(axPartitionTable, part + 1) +
					     PART_KERNEL_SIZE + rootfs_part_size)) {
						printf("ERROR: Not enough space in flash for dual boot partitions\n");
						break;
					}
				}
				/* Kernel */
				part++;
				if (1 == sys_parts)
					strcpy(axPartitionTable[part].szName, PART_NAME_LINUX);
				else
					sprintf(axPartitionTable[part].szName, PART_NAME_LINUX "%d", i);
				axPartitionTable[part].eType = NVPT_LINUX;
				axPartitionTable[part].flags.bFixed = 0;
				axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
				axPartitionTable[part].ullSize  = PART_KERNEL_SIZE;
				modified_parts[part] = 1;
				/* Rootfs */
				part++;
				if (1 == sys_parts)
					strcpy(axPartitionTable[part].szName, PART_NAME_ROOTFS);
				else
					sprintf(axPartitionTable[part].szName, PART_NAME_ROOTFS "%d", i);
				axPartitionTable[part].eType = NVPT_FILESYSTEM;
				axPartitionTable[part].flags.bFixed = 0;
				axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
				axPartitionTable[part].ullSize  = rootfs_part_size;
				if (NVOS_LINUX == eForOS) {
#ifdef SQUASH_ROOTFS
					axPartitionTable[part].flags.fs.eType = NVFS_SQUASHFS;
#else
					axPartitionTable[part].flags.fs.eType = NVFS_JFFS2;
#endif
				}
				else if (NVOS_ANDROID == eForOS) {
					axPartitionTable[part].flags.fs.eType = NVFS_UBIFS;
				}
				axPartitionTable[part].flags.fs.bRoot = 1;
				modified_parts[part] = 1;
			}
#if PART_ROOTFS_SIZE > 0
			/* UserFS */
			part++;
			strcpy(axPartitionTable[part].szName, "UserFS");
			axPartitionTable[part].eType = NVPT_FILESYSTEM;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = 0;	/* if 0, means to end-of-flash */
			if (NVOS_LINUX == eForOS) {
				axPartitionTable[part].flags.fs.eType = NVFS_JFFS2;
			}
			else if (NVOS_ANDROID == eForOS) {
				axPartitionTable[part].flags.fs.eType = NVFS_UBIFS;
			}
			modified_parts[part] = 1;
#endif
		break;

#if PART_WINCE_SIZE > 0
		case NVOS_WINCE:
			/* WinCE partitions */
#ifdef PART_WINCE_REG_SIZE
			/* Registry */
			part++;
			strcpy(axPartitionTable[part].szName, "Registry");
			axPartitionTable[part].eType = NVPT_WINCE_REGISTRY;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = PART_WINCE_REG_SIZE;
			axPartitionTable[part].flags.fs.eType = NVFS_NONE;
			axPartitionTable[part].flags.fs.bRoot = 0;
			modified_parts[part] = 1;
#endif
#ifdef CONFIG_UBOOT_SPLASH
			/* Splash */
			part++;
			strcpy(axPartitionTable[part].szName, "Splash");
			axPartitionTable[part].eType = NVPT_SPLASH_SCREEN;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = PART_SPLASH_SIZE;
			modified_parts[part] = 1;
#endif
			for (i=0; i < sys_parts; i++) {
				/* Check if there is enough space in Flash to hold dual boot partitions */
				if (i > 0) {
					if (MtdSize(0) <
					    (sizeof_partsbelow(axPartitionTable, part + 1) +
					     PART_WINCE_SIZE)) {
						printf("ERROR: Not enough space in flash for dual boot partitions\n");
						break;
					}
				}
				/* Kernel */
				part++;
				if (1 == sys_parts)
					strcpy(axPartitionTable[part].szName, PART_NAME_WINCE);
				else
					sprintf(axPartitionTable[part].szName, PART_NAME_WINCE "%d", i);
				axPartitionTable[part].eType = NVPT_WINCE;
				axPartitionTable[part].flags.bFixed = 0;
				axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
				axPartitionTable[part].ullSize  = PART_WINCE_SIZE;
				axPartitionTable[part].flags.fs.eType = NVFS_NONE;
				modified_parts[part] = 1;
			}

			/* File system */
			part++;
			strcpy(axPartitionTable[part].szName, "Filesys");
			axPartitionTable[part].eType = NVPT_FILESYSTEM;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = PART_WINCE_FS_SIZE;
			axPartitionTable[part].flags.fs.eType = NVFS_EXFAT;
			modified_parts[part] = 1;
		break;
#endif /* PART_WINCE_SIZE */

#if PART_NETOS_KERNEL_SIZE > 0
		case NVOS_NETOS:
			/* NETOS partitions */
			/* Kernel */
			part++;
			strcpy(axPartitionTable[part].szName, PART_NAME_NETOS);
			axPartitionTable[part].eType = NVPT_NETOS;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = PART_NETOS_KERNEL_SIZE;
			modified_parts[part] = 1;
			/* File system */
			part++;
			strcpy(axPartitionTable[part].szName, "NET+OS-FS");
			axPartitionTable[part].eType = NVPT_FILESYSTEM;
			axPartitionTable[part].flags.bFixed = 0;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize = 0;	/* to end of flash */
			axPartitionTable[part].flags.fs.eType = NVFS_YAFFS2;
			axPartitionTable[part].flags.fs.bRoot = 0;
			modified_parts[part] = 1;
		break;
#endif

#ifdef CONFIG_PARTITION
		case NVOS_USER_DEFINED:
#if defined(CONFIG_PARTITION_2) && !(PART_FPGA_SIZE > 0)
			part++;
			strcpy(axPartitionTable[part].szName, CONFIG_PARTITION_NAME_2);
			axPartitionTable[part].eType = CONFIG_PARTITION_TYPE_2;
			axPartitionTable[part].flags.bFixed = PARTITION_FIXED_2;
			axPartitionTable[part].flags.bReadOnly = PARTITION_READONLY_2;
			axPartitionTable[part].flags.fs.eType = CONFIG_PARTITION_FS_2;
			axPartitionTable[part].flags.fs.bRoot = PARTITION_ROOTFS_2;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize = CONFIG_PARTITION_SIZE_2 * 1024;
			modified_parts[part] = 1;
#endif
#ifdef CONFIG_PARTITION_3
			part++;
			strcpy(axPartitionTable[part].szName, CONFIG_PARTITION_NAME_3);
			axPartitionTable[part].eType = CONFIG_PARTITION_TYPE_3;
			axPartitionTable[part].flags.bFixed = PARTITION_FIXED_3;
			axPartitionTable[part].flags.bReadOnly = PARTITION_READONLY_3;
			axPartitionTable[part].flags.fs.eType = CONFIG_PARTITION_FS_3;
			axPartitionTable[part].flags.fs.bRoot = PARTITION_ROOTFS_3;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = CONFIG_PARTITION_SIZE_3 * 1024;
			modified_parts[part] = 1;
#endif
#ifdef CONFIG_PARTITION_4
			part++;
			strcpy(axPartitionTable[part].szName, CONFIG_PARTITION_NAME_4);
			axPartitionTable[part].eType = CONFIG_PARTITION_TYPE_4;
			axPartitionTable[part].flags.bFixed = PARTITION_FIXED_4;
			axPartitionTable[part].flags.bReadOnly = PARTITION_READONLY_4;
			axPartitionTable[part].flags.fs.eType = CONFIG_PARTITION_FS_4;
			axPartitionTable[part].flags.fs.bRoot = PARTITION_ROOTFS_4;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = CONFIG_PARTITION_SIZE_4 * 1024;
			modified_parts[part] = 1;
#endif
#ifdef CONFIG_PARTITION_5
			part++;
			strcpy(axPartitionTable[part].szName, CONFIG_PARTITION_NAME_5);
			axPartitionTable[part].eType = CONFIG_PARTITION_TYPE_5;
			axPartitionTable[part].flags.bFixed = PARTITION_FIXED_5;
			axPartitionTable[part].flags.bReadOnly = PARTITION_READONLY_5;
			axPartitionTable[part].flags.fs.eType = CONFIG_PARTITION_FS_5;
			axPartitionTable[part].flags.fs.bRoot = PARTITION_ROOTFS_5;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = CONFIG_PARTITION_SIZE_5 * 1024;
			modified_parts[part] = 1;
#endif
#ifdef CONFIG_PARTITION_6
			part++;
			strcpy(axPartitionTable[part].szName, CONFIG_PARTITION_NAME_6);
			axPartitionTable[part].eType = CONFIG_PARTITION_TYPE_6;
			axPartitionTable[part].flags.bFixed = PARTITION_FIXED_6;
			axPartitionTable[part].flags.bReadOnly = PARTITION_READONLY_6;
			axPartitionTable[part].flags.fs.eType = CONFIG_PARTITION_FS_6;
			axPartitionTable[part].flags.fs.bRoot = PARTITION_ROOTFS_6;
			axPartitionTable[part].ullStart = sizeof_partsbelow(axPartitionTable, part);
			axPartitionTable[part].ullSize  = CONFIG_PARTITION_SIZE_6 * 1024;
			modified_parts[part] = 1;
#endif
		break;
#endif /* CONFIG_PARTITION */
#endif /* NETOS_BRINGUP */
		default:
			NV_SET_ERROR( NVE_NOT_IMPLEMENTED, NvToStringOS( eForOS ) );
			goto error;
		break;
	}

	/* Add partition table to NVRAM */
        NvPrivOSAddPartitions( pCrit, axPartitionTable, part + 1 );

        return 1;

error:
        return 0;
}
int NvPrivOSFlashOpen( char bForWrite )
{
        /* not necessary */
        return 1;
}

int NvPrivOSFlashClose( void )
{
        /* not necessary */
        return 1;
}

int NvPrivOSFlashRead( void* pvBuf, loff_t iOffs, size_t iLength )
{
        CE( NvPrivOSIntInRange( iOffs, iLength ) );

        if( !MtdRead( NVRAM_CHIP, OFFS( iOffs ), iLength, pvBuf ) ) {
                char szErr[ 64 ];
                sprintf( szErr, "Flash Read @ 0x%08qx, length = %i",
                         OFFS( iOffs ), iLength );

                NV_SET_ERROR( NVE_IO, szErr );
                goto error;
        }

        return 1;

error:
        return 0;
}

int NvPrivOSFlashErase( loff_t iOffs )
{
        nv_priv_flash_status_t xStatus;

        CE( NvPrivOSFlashInfo( iOffs, &xStatus ) );
        if( !MtdErase( NVRAM_CHIP, OFFS( iOffs ), xStatus.iEraseSize ) ) {
                char szErr[ 64 ];
                sprintf( szErr, "Flash Erase @ 0x%08qx, length = %i",
                         OFFS( iOffs ), xStatus.iEraseSize );

                goto error;
        }

        return 1;

error:
        return 0;
}

int NvPrivOSFlashWrite( /*@in@*/ const void* pvBuf, loff_t iOffs, size_t iLength )
{
        CE( NvPrivOSIntInRange( iOffs, iLength ) );
        if( !MtdWrite( NVRAM_CHIP, OFFS( iOffs ), iLength, pvBuf ) ) {
                char szErr[ 64 ];
                sprintf( szErr, "Flash Write @ 0x%08qx, length = %i",
                         OFFS( iOffs ), iLength );

                NV_SET_ERROR( NVE_IO, szErr );
                goto error;
        }

        return 1;

error:
        return 0;
}

int NvPrivOSFlashProtect( loff_t iOffs, size_t iLength, char bProtect )
{
        CE( NvPrivOSIntInRange( iOffs, iLength ) );

        if( !MtdProtect( NVRAM_CHIP, OFFS( iOffs ), iLength, bProtect ) ) {
                char szErr[ 64 ];
                sprintf( szErr, "%s failed @ 0x%08qx, length = %i",
                         ( bProtect ? "Protect" : "Unprotect" ),
                         OFFS( iOffs ), iLength );

                NV_SET_ERROR( NVE_IO, szErr );
                goto error;
        }

        return 1;

error:
        return 0;
}

int NvPrivOSFlashInfo(
        loff_t iOffs,
        /*@out@*/ struct nv_priv_flash_status* pStatus )
{
        if( !NvPrivOSIntInRange( iOffs, 1 ) )
                /* the sector needs to be present */
                return 0;

        CLEAR( *pStatus );
        pStatus->bBad       = MtdBlockIsBad( NVRAM_CHIP, OFFS( iOffs ) );
        pStatus->iEraseSize = MtdGetEraseSize(  NVRAM_CHIP, OFFS( iOffs ) );

        return 1;
}

void NvPrivOSPrintf( const char* szFormat, ...)
{
        va_list ap;

        if( !NvPrivIsOutputEnabled() )
                return;

        va_start( ap, szFormat );
        vprintf( szFormat, ap );
        va_end( ap );
}

void NvPrivOSPrintfError( const char* szFormat, ...)
{
        va_list ap;

        if( !NvPrivIsOutputEnabled() )
                return;

        /* it's not stderr, put there is no vfprintf( stderr )*/
        va_start( ap, szFormat );
        vprintf( szFormat, ap );
        va_end( ap );
}
