/*
 *  common/digi/cmd_testhw/common/nor.c
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
 *                   nor_pattern <start> <size> <loop>
*/

#include <common.h>

#if defined(CONFIG_CMD_BSP) &&		\
    defined(CONFIG_CMD_FLASH) &&	\
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW)

#include <nvram_types.h>        /* CLEAR */
#include <mtd.h>                /* MemDump */
#include <dvt.h>                /* DVTError */
#include <mtd.h>

#include <cmd_testhw/testhw.h>

#define USAGE "<start> <size> <count> [<stop_on_error>]"     

#define CHIP 0 /* BOOT */

/* ********** local functions ********** */

static int NORPattern(
        loff_t lStart,
        size_t iSize,
        int    iLoop,
        char   bStopOnError )
{
        uchar_t* pucBuf    = NULL;
        uchar_t* pucBufVer = NULL;
        char     bOk         = 0;
        char     bFailedOnce = 0;
        loff_t   lEnd        = lStart + iSize;
        size_t   iMaxEraseSize;
        loff_t   lAddr;
        int i;

        /* determine maximum erase size of area */
        iMaxEraseSize = 0;
        lAddr = lStart;
        while( lAddr < lEnd ) {
                size_t iEraseSize = MtdGetEraseSize( CHIP, lAddr );

                if( iMaxEraseSize < iEraseSize )
                        iMaxEraseSize = iEraseSize;
                lAddr += iEraseSize;
        }
        
        /* prepare data buffer */

        pucBuf    = malloc( iMaxEraseSize );
        pucBufVer = malloc( iMaxEraseSize );
        if( ( NULL == pucBuf ) || ( NULL == pucBufVer ) ) {
                eprintf( "\nOut of Memory for buffer\n" );
                goto out;
        }

        /* Each run the test is run +1 from previous run to distinguish from errors*/
        for( i = 0; i < iMaxEraseSize; i++ )
                pucBuf[ i ] = i + iLoop;

        printf( "Run: % 9id\r", iLoop );

        lAddr = lStart;
        /* erase region */
        while( lAddr < lEnd ) {
                size_t iEraseSize = MtdGetEraseSize( CHIP, lAddr );
                if( !MtdErase( CHIP, lAddr, iEraseSize ) ) {
                        eprintf( "\nErase failed at block 0x%08x\n", lAddr );
                        if( bStopOnError )
                                goto out;
                        else
                                bFailedOnce = 1;
                }
                lAddr += iEraseSize;
        } /* while() */

        /* verify each block that it is correctly erased */
        lAddr = lStart;
        while( lAddr < lEnd ) {
                loff_t lOffs;
                size_t iEraseSize = MtdGetEraseSize( CHIP, lAddr );
                
                if( !MtdRead( CHIP, lAddr, iEraseSize, pucBufVer ) ) {
                        eprintf( "\nRead after erase failed at block 0x%08x\n",
                                 lAddr );
                        if( bStopOnError )
                                goto out;
                        else
                                bFailedOnce = 1;
                }
                for( lOffs = 0; lOffs < iEraseSize; lOffs++ ) {
                        if( 0xff != pucBufVer[ lOffs ] ) {
                                eprintf( "\nNot erased @ 0x%08x\n", lAddr );
                                MemDump( pucBufVer, lOffs, 64 );
                                if( bStopOnError )
                                        goto out;
                                else {
                                        bFailedOnce = 1;
                                        break;
                                }
                        }
                }
                lAddr += iEraseSize;
        } /* while(  ) */

        /* write each block */
        lAddr = lStart;
        while( lAddr < lEnd ) {
                size_t iEraseSize = MtdGetEraseSize( CHIP, lAddr );
                if( !MtdWrite( CHIP, lAddr, iEraseSize, pucBuf ) ) {
                        eprintf( "\nWrite failed at block 0x%08x\n",
                                 lAddr );
                        if( bStopOnError )
                                goto out;
                        else
                                bFailedOnce = 1;
                }
                lAddr += iEraseSize;
        } /* while( ) */
        
        /* verify each block */
        lAddr = lStart;
        while( lAddr < lEnd ) {
                loff_t lOffs;
                
                size_t iEraseSize = MtdGetEraseSize( CHIP, lAddr );
                if( !MtdRead( CHIP, lAddr, iEraseSize, pucBufVer ) ) {
                        eprintf( "\nRead failed at block 0x%08x\n",
                                 lAddr );
                        if( bStopOnError )
                                goto out;
                        else
                                bFailedOnce = 1;
                }
                lOffs = MemCmp( pucBuf, pucBufVer, iEraseSize );
                if( -1 != lOffs ) {
                        eprintf( "\nDifference @ 0x%08x\n", lAddr );
                        eprintf( "Original:\n" );
                        MemDump( pucBuf,    lOffs, 64 );
                        eprintf( "Flash:\n" );
                        MemDump( pucBufVer, lOffs, 64 );
                        if( bStopOnError )
                                goto out;
                        else
                                bFailedOnce = 1;
                }
                lAddr += iEraseSize;
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

/*! \brief Writes a test pattern to flash and verifies it */
static int do_testhw_nor_pattern( int argc, char* argv[] )
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
                eprintf( "Usage: nor_pattern %s\n", USAGE );
                goto error;
        }

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
                if( !NORPattern( lStart, iLength, iLoop, bStopOnError ) ) {
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

TESTHW_CMD( nor_pattern, USAGE );

#endif /*CONFIG_CMD_BSP... */


