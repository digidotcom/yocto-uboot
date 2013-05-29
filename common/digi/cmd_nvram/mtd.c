/*
 *  cmd_nvram/mtd.c
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
 *  !Descr:      Provides interface for NAND and NOR access
*/

#include <common.h>

#include <dvt.h>                /* DVTIsEnabled */

#include "mtd.h"
#include "nvram_types.h"        /* CLEAR */

#ifdef CONFIG_CMD_NAND
# define HAVE_NAND
# include <nand.h>
#else
# ifdef CONFIG_CMD_FLASH
#  undef HAVE_NAND
#  include <flash.h>
extern flash_info_t flash_info[];
# else
#  error "Select NAND or NOR support"
# endif
#endif  /* CONFIG_CMD_NAND */

/**
 * MtdRead - reads from NAND/NOR flash
 *
 * @return: 0 on failure otherwise 1
 */

int MtdRead( int iChip, uint64_t ullOffs, size_t iLength, void* pvBuf )
{
        int iRes = 0;

#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
        size_t iRetBlock;

        iRes = !pChip->read( pChip, ullOffs, iLength, &iRetBlock, pvBuf );
#else
        flash_info_t* pChip = &flash_info[ iChip ];
        memcpy32( pvBuf, ((const char*) pChip->start[ 0 ] ) + ullOffs, iLength );
        iRes = 1;
#endif

        return iRes;
}

int MtdRewrite( int iChip, uint64_t ullOffs, size_t iLength, const void* pvBuf )
{
        int iRes = 0;

#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
        size_t iRetBlock;
        struct erase_info xErase;

        CLEAR( xErase );

	xErase.mtd  = pChip;
	xErase.addr = ullOffs;
	xErase.len  = pChip->erasesize;
	xErase.callback = 0;

	iRes = !pChip->erase( pChip, &xErase );
	if (!iRes)
		printf("MtdRewrite: erase failed\n");

	iRes = !pChip->write( pChip, ullOffs, iLength, &iRetBlock, pvBuf);
	if (iRes)
		printf("MtdRewrite: rewrite succes\n");
	else
		printf("MtdRewrite: rewrite failed. Maybe this is a bad block.\n");
#else
	iRes = 1;
#endif
	return iRes;
}

int MtdWrite( int iChip, uint64_t ullOffs, size_t iLength, const void* pvBuf )
{
        int iRes = 0;

#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
        size_t iRetBlock;

        iRes = !pChip->write( pChip, ullOffs, iLength, &iRetBlock, pvBuf );
#else
        flash_info_t* pChip = &flash_info[ iChip ];
        iRes = ( flash_write( (char*) pvBuf, pChip->start[ 0 ] + ullOffs, iLength ) == ERR_OK );
#endif

        return iRes;
}

/**
 * MtdErase - erases an aligned memory area
 * @return:
 */
int MtdErase( int iChip, uint64_t ullOffs, size_t iLength )
{
        int iRes = 0;

#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
        struct erase_info xErase;

        CLEAR( xErase );

	xErase.mtd  = pChip;
	xErase.addr = ullOffs;
	xErase.len  = pChip->erasesize;
	xErase.callback = 0;
        iRes = !pChip->erase( pChip, &xErase );
#else
        flash_info_t* pChip = &flash_info[ iChip ];
        ulong ulStart = pChip->start[ 0 ] + ullOffs;
        char  bNeedErase = 1;

        DVTPrintOnlyOnce( "Sectors will be erased even if already empty\n " );

        if( !DVTIsEnabled() ) {
                /* checks if the block is already empty. Flashing is slooowwww
                 * on NOR */
                flash_info_t* pChip = &flash_info[ iChip ];
                const char* pucBlock = ( const char*) pChip->start[ 0 ] + ullOffs;
                const u32* puiBlock    = (const u32*) pucBlock;
                const u32* puiBlockEnd = (const u32*) (pucBlock + iLength );

                bNeedErase = 0;
                while( puiBlock < puiBlockEnd ) {
                        if( *puiBlock != 0xFFFFFFFF ) {
                                /* contains data, no need to go further */
                                bNeedErase = 1;
                                break;
                        }

                        puiBlock++;
                }
        } /* if( !DVTIsEnabled() */

        if( bNeedErase )
                iRes = !flash_sect_erase( ulStart, ulStart + iLength - 1, 1 );
        else
                /* not necessary */
                iRes = 1;
#endif

        return iRes;
}

uint64_t MtdSize( int iChip )
{
#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
#else
        flash_info_t* pChip = &flash_info[ iChip ];
#endif
        return pChip->size;
}

size_t MtdGetEraseSize( int iChip, uint64_t ullOffs )
{
        size_t iEraseSize;

#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
        iEraseSize = pChip->erasesize;
#else
        flash_info_t* pChip = &flash_info[ iChip ];
        flash_sect_t  sect  = find_sector( pChip, pChip->start[ 0 ] + ullOffs );

        if( sect < pChip->sector_count - 1  )
                /* there can be sectors with different erase size */
                iEraseSize = pChip->start[ sect + 1 ] - pChip->start[ sect ];
        else if( sect == pChip->sector_count - 1 )
                /* last sector */
                iEraseSize = pChip->size - ( pChip->start[ sect ] - pChip->start[ 0 ] );
        else
                iEraseSize = 0;
#endif  /* HAVE_NAND */

        return iEraseSize;
}

char MtdBlockIsBad( int iChip, uint64_t ullOffs )
{
        char bBad;

#ifdef HAVE_NAND
        nand_info_t* pChip = &nand_info[ iChip ];
        bBad = pChip->block_isbad( pChip, ullOffs );
#else
        bBad = 0;/* NOR is never bad */
#endif

        return bBad;
}

int MtdProtect( int iChip, uint64_t ullOffs, size_t iLength, char bProtect )
{
        int iRes;

#ifdef HAVE_NAND
        iRes = 1; /* no protect available */
#else
        flash_info_t* pChip = &flash_info[ iChip ];
        ulong ulStart = pChip->start[ 0 ] + ullOffs;

        flash_protect( ( bProtect ? FLAG_PROTECT_SET : FLAG_PROTECT_CLEAR ),
                       ulStart, ulStart + iLength - 1, pChip );
        iRes = 1;               /* no error available */
#endif

        return iRes;
}

/**
 * MtdVerifyAllocBuf - preallocates a buffer used in multiple verification
 *
 * For NOR technology, a buffer is not created because it can be compared
 * immediately in NOR Flash
 */
void* MtdVerifyAllocBuf( size_t iLength )
{
        void* pvBuf = NULL;

#ifdef HAVE_NAND
        pvBuf = malloc( iLength );
        if( NULL == pvBuf )
                printf( ERROR "Not enough mem for verify buffer\n" );
#endif

        return pvBuf;
}

int MtdVerifyFreeBuf( void* pvBuf )
{
        if( NULL != pvBuf )
                free( pvBuf );

        return 1;
}

int MtdVerify(
        int iChip,
        uint64_t ullOffs,
        size_t iLength,
        const void* pvSrc,
        void* pvBuf )
{
        loff_t lOffsDiff;

#ifdef HAVE_NAND
        if( !MtdRead( iChip, ullOffs, iLength, pvBuf ) ) {
                printf( ERROR "Read for verify @ 0x%08qx failed\n", ullOffs );
                goto error;
        }
#else
        /* no need to copy in RAM */
        flash_info_t* pChip = &flash_info[ iChip ];
        pvBuf = ((char*) pChip->start[ 0 ] ) + ullOffs;
#endif

        lOffsDiff = MemCmp( pvSrc, pvBuf, iLength );

        if( -1 != lOffsDiff ) {
                printf( "\n" ERROR "Difference @ 0x%08x\n",
                        (unsigned int)(ullOffs + lOffsDiff) );
                printf( "Original:\n" );
                MemDump( pvBuf, lOffsDiff, 64 );
                printf( "Flash:\n" );
                MemDump( pvSrc, lOffsDiff, 64 );
                goto error;
        }

        return 1;

error:
        return 0;
}

loff_t MemCmp8( const char* pcS1, const char* pcS2, size_t iSize )
{
        loff_t iOffset = 0;

        while( iOffset < iSize ) {
                if( *pcS2 != *pcS1 )
                        return iOffset;

                pcS2++;
                pcS1++;
                iOffset++;
        }

        return -1;
}

/**
 * MemCmp32 - performes Mem
 * @return: offset of failure or -1 if none
 */
loff_t MemCmp32( const u32* puiS1, const u32* puiS2, size_t iSize )
{
        loff_t iOffset = 0;
        int iRemaining = iSize & 0x3;
        int iRes;

        /* count in 32bit words */
        iSize >>= 2;

        while( iOffset < iSize ) {
                if( *puiS2 != *puiS1 ) {
                        /* determine exact byte position */
                        iOffset <<= 2;  /* get it in bytes */
                        /* words are different, therefore MemCmp8 will
                           return >=0 */
                        iOffset += MemCmp8( (const char*) puiS1,
                                            (const char*) puiS2,
                                            sizeof( u32 ) );
                        return iOffset;
                }
                puiS2++;
                puiS1++;
                iOffset++;
        }

        iRes = ( iRemaining ?
                 MemCmp8( (const char*) puiS1,
                          (const char*) puiS2,
                          iRemaining ) :
                 -1 );

        return iRes;
}

/**
 * MemCmp - compares memory
 * @return: offset of failure or -1 if none
 */
loff_t MemCmp(  const void* pvS1, const void* pvS2, size_t iSize )
{
        loff_t iOffs;

        if( ( ( ( int ) pvS1 ) & 0x3 ) ||
            ( ( ( int ) pvS2 ) & 0x3 ) )
                iOffs = MemCmp8( (const char*) pvS1, (const char*) pvS2, iSize );
        else
                iOffs = MemCmp32( (const u32*) pvS1, (const u32*) pvS2, iSize );

        return iOffs;
}

/**
 * MemDump - Prints memory from pvbase + iOffset to pvBase + iOffset + iLen
 */
void MemDump( const void* pvBase, loff_t iOffset, size_t iLen )
{
	const unsigned char* pucBuf = (const unsigned char*) pvBase + iOffset;
	const int COLUMN_COUNT = 16;
	int i;

	for( i = 0; i < iLen; i += COLUMN_COUNT ) {
        	/* print one row */
        	int j, iRowLen;

                if( ( i + COLUMN_COUNT ) <= iLen )
                        iRowLen = COLUMN_COUNT;
                else
                        iRowLen = iLen - i;

                printf( "%08qx  ", (long long) iOffset );

                /* print hexadecimal representation */
                for( j=0; j < iRowLen; j++ ) {
                        printf( "%02x ", *( pucBuf + j ) );
                        if( ( ( COLUMN_COUNT / 2 ) - 1 ) == j )
                                /* additional separator*/
                                printf( "   " );
                }

                printf( "  " );

                /* print character representation row */
                for( j=0; j < iRowLen; j++ ) {
                        unsigned char c = *( pucBuf + j );
                        if( ( c < 32 ) || ( c > 127 ) )
                                c = '.';

                        if( ( ( COLUMN_COUNT / 2 ) - 1 ) == j )
                                /* additional separator*/
                                printf( " " );

                        printf( "%c", c );
                }

                printf( "\r\n" );
                pucBuf += COLUMN_COUNT;
                iOffset += iRowLen;
	}
}
