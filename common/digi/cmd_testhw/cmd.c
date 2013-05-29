/*
 *  common/digi/cmd_testhw/cmd.c
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
 *  !Descr:      Provides access to various general/CPU/module/baseboard test
 *               functions/commands
 *               The list of functions can be retrieved with "testhw help".
 *               This differs from U-Boot convention, so it is hiding the stuff
 *               a little bit.
 *
 *               To add new commands:
 *                  1) Create a new C file in cpu/ or module/
 *                  2) Add obj file to common/digi/Makefile
 *                  3) Use TESTHW_CMD in C file
*/

#include <common.h>

#if defined (CONFIG_CMD_BSP) && defined(CONFIG_UBOOT_CMD_BSP_TESTHW)

#include <command.h>            /* U_BOOT_CMD */

#include <cmd_testhw/testhw.h>

/* ********** global variables ********** */

/* the linker inserts the commands after __testhw_cmd_start and
   __testhw_cmd_end */

const testhw_cmd_entry_t __testhw_cmd_start;
const testhw_cmd_entry_t __testhw_cmd_end;

/* ********** local functions ********** */

/*! \brief Locates the command entry for szName */
/*! \return NULL if szName not found, otherwise the entry for szName */
static const testhw_cmd_entry_t* FindCmd( const char* szName )
{
        const testhw_cmd_entry_t* pCmdEntry = NULL;
        const testhw_cmd_entry_t* pIt       = &__testhw_cmd_start;

        while( pIt < &__testhw_cmd_end ) {
                if( !strcmp( szName, pIt->szName ) ) {
                        /* found */
                        pCmdEntry = pIt;
                        break;
                }

                pIt++;
        }

        return pCmdEntry;
}

/*! \brief performs a test command */
/*! \return 1 on failure, otherwise 0 */
static int do_testhw( cmd_tbl_t* cmdtp, int flag, int argc, char* argv[] )
{
        const testhw_cmd_entry_t* pCmdEntry = NULL;

        if( argc < 2 ) {
                eprintf( "Usage:\n%s\n", cmdtp->usage );
                goto error;
        }

        pCmdEntry = FindCmd( argv[ 1 ] );
        if( NULL == pCmdEntry ) {
                eprintf( "Test not found: %s\n", argv[ 1 ] );
                goto error;
        }

        /* execute test command */
        if( !pCmdEntry->pfHandler( argc - 2, &argv[ 2 ] ) ) {
                TESTHW_ERROR( "Test failed: %s", argv[ 1 ] );
                goto error;
        } else
                printf( "Test OK\n" );

        return 0;

error:
        return 1;
}

/*! \brief implements "testhw help", printing all registered commands */
/*! \return always 0 (OK) */
static int do_testhw_help( int argc, char* argv[] )
{
        const testhw_cmd_entry_t* pCmdEntry = &__testhw_cmd_start;

        printf( "Commands:\n" );
        while( pCmdEntry < &__testhw_cmd_end ) {
                printf( " %-24s - %s\n", pCmdEntry->szName, pCmdEntry->szHelp);
                pCmdEntry++;
        }

        return 1;
}

/* ********** U-Boot command table ********** */

U_BOOT_CMD(
	testhw, 255, 0, do_testhw,
	"Internal hardware test functions",
        "- This is an internal command.\n"
);

/* ********** Test command implemented ********** */

TESTHW_CMD( help,
            "This help" );

#endif  /* defined (CONFIG_CMD_BSP) && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) */
