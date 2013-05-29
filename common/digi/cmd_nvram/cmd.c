/*
 *  U-Boot/common/digi/cmd_nvram/cmd.c
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
 */
/*
 *  !Revision:   $Revision$:
 *  !Author:     Markus Pietrek
 *  !Descr:      Provides
 *                "internal_nvram": the NVRAM functions to access MAC, IP etc.
 *                                  for U-Boot
 *                "partition": a GUI for modifying the partition table.
 */

#include <common.h>

#ifdef CONFIG_CMD_BSP

#include <command.h>
#include <dvt.h>

#include "nvram.h"
#include "env.h"
#include "partition.h"

extern void set_mac_from_env( void );

static int do_nvram( cmd_tbl_t* cmdtp, int flag, int argc, char* argv[] )
{
        if( !CW( NvCmdLine( argc - 1, (const char**) &argv[ 1 ] ) ) )
                return 1;

        CE( !DVTError() );

        if( ( argc > 1 ) &&
            ( !strcmp( "set", argv[ 1 ] ) ||
	      !strcmp( "reset", argv[ 1 ] ) ||
	      !strcmp( "init", argv[ 1 ] ) ) ) {
                /* update U-Boot variables */
                NvEnvUpdateFromNVRAM();

#ifdef CONFIG_CMD_NET
//		if(argc > 3 && !strncmp(argv[ 3 ],"ethaddr1",7))
//			set_mac_from_env();
#endif
	}
        /* called from U-Boot */
        return 0;

error:
        return 1;
}

/***********************************************************************
 * !Function: do_partition
 * !Return:   1 on failure otherwise 0
 * !Descr:    handles the partition command
 ***********************************************************************/

static int do_partition( cmd_tbl_t* cmdtp, int flag, int argc, char* argv[] )
{
        int iRes = 0;

        if( 1 == argc )
                iRes = PartGUI();
        else if( ( argc == 2 ) && !strcmp( "setenv_mtdparts", argv[ 1 ] ) ) {
                char szMtdParts[ 2048 ];
                szMtdParts[ 0 ] = 0;
                iRes = PartStrAppendParts( szMtdParts, sizeof( szMtdParts ) );
                if( iRes ) {
                        printf( "Setting mtdparts variable\n" );
                        setenv( "mtdparts", szMtdParts );
                }
        } else if( ( argc == 2 ) && !strcmp( "setenv_mtdroot", argv[ 1 ] ) ) {
                char szMtdRoot[ 2048 ];
                szMtdRoot[ 0 ] = 0;
                iRes = PartStrAppendRoot( szMtdRoot, sizeof(szMtdRoot), 0 );
                if( iRes ) {
                        printf( "Setting mtdroot variable\n" );
                        setenv( "mtdroot", szMtdRoot );
                }
        } else
                printf( "Usage:\n%s\n", cmdtp->usage );

        return !iRes;
}

/* ********** U-Boot command table ********** */

U_BOOT_CMD(
	intnvram, 100, 0, do_nvram,
	"displays or modifies NVRAM contents like IP or partition table",
	"[help]\n"
        "This is an internal command. Use \"intnvram help\" for detailed help\n"
);

U_BOOT_CMD(
	flpart, 2, 0, do_partition,
	"displays or modifies the partition table",
	"[ setenv_mtdparts | setenv_mtdroot ]\n"
        "Is a GUI frontend to the partition part of internal_nvram\n"
        " o if setenv_mtdparts is present, the partition table is stored in the\n"
        "   U-Boot environment variable mtdparts for use with linux\n"
        " o if setenv_mtdroot is present, the partition table configuration of rootfs\n"
        "   is stored in the U-Boot environment variable mtdroot for use with linux"
);

#endif  /* CONFIG_CMD_BSP */
