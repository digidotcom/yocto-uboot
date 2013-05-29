/*
 *  common/digi/cmd_chkimg.c
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
 *  !Descr:      Provides chkimg and CheckCRC32OfImageInFlash
 *               The linker file may contain an entry __u_boot_crc32. This
 *               needs to be exactly 4 Bytes before .bss.
 *               u-boot.bin will not have these values set, so run append_crc32
 *               on it (or a copy of it). Then you can use chkimg to check a
 *               running U-Boot against it's CRC32.
 *               This CRC32 is different to the one reported by update_flash,
 *               because they
 *               This checks that the U-Boot image in NOR/NAND flash matches
 *               it.
*/

#include <common.h>
#if defined(CFG_APPEND_CRC32) && !defined(CFG_NO_FLASH)

#include <command.h>
#include "cmd_chkimg.h"

/* set from linker file */
extern const ulong __u_boot_crc32;
extern const void* _start;

int CheckCRC32OfImageInFlash( char bSilent )
{
        ulong ulCRC;
        int   iRes = 0;

#if !defined(CFG_NO_FLASH)
	ulCRC = crc32( 0, (const unsigned char*) CFG_FLASH_BASE,
                       (int) &__u_boot_crc32 - (int) &_start );
#else
# error How should I determine crc32 on nand? PartRead?, PartCRC32?
#endif

        iRes = ( ulCRC == __u_boot_crc32 );

        if( iRes ) {
                if( !bSilent )
                        printf( "CRC32: match: 0x%08x\n", ulCRC );
        } else
                printf( "CRC32: expected U-Boot to have 0x%08x, is 0x%08x\n",
                        __u_boot_crc32, ulCRC );

        return iRes;
}

static int do_digi_chkimg( cmd_tbl_t* cmdtp, int flag, int argc, char* argv[] )
{
        return !CheckCRC32OfImageInFlash( 0 );
}

U_BOOT_CMD(
	chkimg,	1,	0,	do_digi_chkimg,
	"Checks that the U-Boot image in flash matches its CRC",
        NULL
);

#endif  /* CFG_APPEND_CRC32 */
