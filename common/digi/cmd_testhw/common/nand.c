/*
 *  /common/digi/cmd_testhw/common/nand.o
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
 *  !Descr:      Implements commands:
 *                   nand_pattern <start> <size> <loop>
*/

#include <common.h>

#if defined(CONFIG_CMD_BSP) &&		\
    defined(CONFIG_CMD_NAND) &&		\
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW)

#include <nand.h>
#include <nvram_types.h>        /* CLEAR */
#include <mtd.h>                /* MemDump */
#include <dvt.h>                /* DVTError */

#include <cmd_testhw/testhw.h>

#define USAGE "<start> <size> <count> [<stop_on_error>]"

extern long get_input(const char *cp);

static int inline rewrite( nand_info_t* pChip, struct erase_info xErase, uint64_t ullOffs, size_t iLength, uchar_t* pvBuf )
{
	        int iRes = 0;
		size_t iRetBlock;
		xErase.addr = ullOffs;
		xErase.callback = 0;

		printf("try to rewrite block ...\n");
		iRes = !pChip->erase( pChip, &xErase );
		if (!iRes) {
			printf("NandPattern: erase for rewrite failed\n");
			return iRes;
		}

		iRes = !pChip->write( pChip, ullOffs, iLength, &iRetBlock, pvBuf);
		if (iRes)
			printf("NandPattern: rewrite successful\n");
		else
			printf("NandPattern: rewrite failed. Maybe this is a bad block.\n");

		return iRes;
}

/* ********** local functions ********** */

static int NandPattern(
        loff_t lStart,
        size_t iSize,
        int    iLoop,
        char   bStopOnError )
{
        struct erase_info xErase;
        nand_info_t*      pChip = &nand_info[ 0 ];
        uchar_t*          pucBuf    = NULL;
        uchar_t*          pucBufVer = NULL;
        loff_t            lAddr;
        char   bOk         = 0;
        char   bFailedOnce = 0;
        int i;

        CLEAR( xErase );

        xErase.mtd  = pChip;
        xErase.addr = lStart;
        xErase.len  = xErase.mtd->erasesize;

        /* prepare data buffer */

        pucBuf    = malloc( xErase.mtd->erasesize );
        pucBufVer = malloc( xErase.mtd->erasesize );
        if( ( NULL == pucBuf ) || ( NULL == pucBufVer ) ) {
                eprintf( "\nOut of Memory for buffer\n" );
                goto out;
        }

        /* Each page starts with +1 offset from next.
         * Each run the test is run +1 from previous run to distinguish from errors*/
        for( i = 0; i < xErase.len; i++ )
                pucBuf[ i ] = i + iLoop + ( i / pChip->writesize );

        printf( "Run: % 9id\r", iLoop );

        /* erase region */
        while( xErase.addr < lStart + iSize ) {
                if( !pChip->block_isbad( pChip, xErase.addr ) ) {
                        if( pChip->erase( pChip, &xErase ) ) {
                                eprintf( "\nErase failed at block 0x%08llx\n",
                                         xErase.addr );
                                if( bStopOnError )
                                        goto out;
                                else
                                        bFailedOnce = 1;
                        }
                }
                xErase.addr += xErase.mtd->erasesize;
        } /* while() */

        /* verify each block that it is correctly erased */
        lAddr = lStart;
        while( lAddr < lStart + iSize ) {
                if( !pChip->block_isbad( pChip, lAddr ) ) {
                        size_t iRetBlock;
                        loff_t lOffs;

                        if( pChip->read( pChip, lAddr, xErase.mtd->erasesize, &iRetBlock, pucBufVer ) ) {
                                eprintf( "\nRead after erase failed at block 0x%08x\n",
                                         (uint32_t)lAddr );
                                if( bStopOnError )
                                        goto out;
                                else
                                        bFailedOnce = 1;
                        }
                        for( lOffs = 0; lOffs < xErase.mtd->erasesize; lOffs++ ) {
                                if( 0xff != pucBufVer[ lOffs ] ) {
                                        eprintf( "\nNot erased @ 0x%08x\n", (uint32_t)lAddr );
                                        MemDump( pucBufVer, lOffs, 64 );
                                        if( bStopOnError )
                                                goto out;
                                        else {
                                                bFailedOnce = 1;
                                                break;
                                        }
                                }
                        }
                }
                lAddr += xErase.mtd->erasesize;
        } /* while(  ) */

        /* write each block */
        lAddr = lStart;
        while( lAddr < lStart + iSize ) {
                if( !pChip->block_isbad( pChip, lAddr ) ) {
                        size_t iRetBlock;

                        if( pChip->write( pChip, lAddr, xErase.mtd->erasesize, &iRetBlock, pucBuf ) ) {
                                eprintf( "\nWrite failed at block 0x%08x\n",
                                         (uint32_t)lAddr );
				if ( !rewrite( pChip, xErase, lAddr, xErase.mtd->erasesize, pucBuf )){
					if( bStopOnError )
						goto out;
					else
						bFailedOnce = 1;
				}
			}
                }
                lAddr += xErase.mtd->erasesize;
        } /* while( ) */

        /* verify each block */
        lAddr = lStart;
        while( lAddr < lStart + iSize ) {
                if( !pChip->block_isbad( pChip, lAddr ) ) {
                        size_t iRetBlock;
                        loff_t lOffs;

                        if( pChip->read( pChip, lAddr, xErase.mtd->erasesize, &iRetBlock, pucBufVer ) ) {
                                eprintf( "\nRead failed at block 0x%08x\n",
                                         (uint32_t)lAddr );
                                if( bStopOnError )
                                        goto out;
                                else
                                        bFailedOnce = 1;
                        }
                        lOffs = MemCmp( pucBuf, pucBufVer, xErase.mtd->erasesize );
                        if( -1 != lOffs ) {
                                eprintf( "\nDifference @ 0x%08x\n", (uint32_t)lAddr );
                                eprintf( "Original:\n" );
                                MemDump( pucBuf,    lOffs, 64 );
                                eprintf( "Flash:\n" );
                                MemDump( pucBufVer, lOffs, 64 );
                                if( bStopOnError )
                                        goto out;
                                else
                                        bFailedOnce = 1;
                        }
                }
                lAddr += xErase.mtd->erasesize;
        } /* while(  ) */

        bOk = !bFailedOnce;

out:
        printf( "\r" );

        if( NULL != pucBuf )
                free( pucBuf );
        if( NULL != pucBufVer )
                free( pucBufVer );

        return bOk;
}

/*! \brief Writes a  */
static int do_testhw_nand_pattern( int argc, char* argv[] )
{
        loff_t lStart;
        size_t iLength;
        int    iCount;
        char   bStopOnError = 0;
        char   bOk   = 1;
        int    iLoop = 1;
	long check_val;

        DVTReset();

        if( ( argc < 3 ) || ( argc > 4 ) ) {
                eprintf( "Usage: nand_pattern %s\n", USAGE );
                goto error;
        }

#ifndef CONFIG_MTD_NAND_VERIFY_WRITE
        printf( "Write Verification without ECC is disabled\n" );
#endif

        lStart  = get_input(argv[ 0 ]);
        iLength = get_input(argv[ 1 ]);
	if((long) lStart == -1 || (long) iLength == -1)
		return 0;
        iCount  = simple_strtoul( argv[ 2 ], NULL, 10 );
        if( argc == 4 ) {
                check_val = get_input(argv[ 3 ]);
		if(check_val == -1)
			return 0;
		bStopOnError = (char) check_val;
	}

        while( !iCount || ( iLoop <= iCount ) ) {
                if( !NandPattern( lStart, iLength, iLoop, bStopOnError ) ) {
                        bOk = 0;
                        if( bStopOnError )
                                break;
                }

                if( ctrlc() ) {
                        clear_ctrlc();
                        printf( "\nAborted by User\n" );
                        break;
                }

                iLoop++;
        }

        printf( "\n" );

        if( DVTError() )
                goto error;

        return bOk;

error:

        return 0;
}


/* ********** Test command implemented ********** */

TESTHW_CMD( nand_pattern, USAGE );

#endif /*(CONFIG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW))*/


