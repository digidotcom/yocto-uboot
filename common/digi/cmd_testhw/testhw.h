/*
 *  common/digi/cmd_testhw/testhw.h
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
 *  !Descr:      Provides TESTHW_CMD
*/

/* in convention with U-Boot returns 1 on failure and 0 on ok */
typedef int ( *testhw_cmd_func_t )( int argc, char* argv[] );

typedef struct {
        const char*       szName;
        const char*       szHelp;
        testhw_cmd_func_t pfHandler;
} testhw_cmd_entry_t;
    
extern const testhw_cmd_entry_t __testhw_cmd_start;
extern const testhw_cmd_entry_t __testhw_cmd_end;

/* the handler function needs to be named do_testhw_#name */
#define TESTHW_CMD( name, help ) \
        testhw_cmd_entry_t __testhw_cmd_##name \
        __attribute__ ((unused,section (".testhw_cmd"))) = {       \
                .szName    = #name, \
                .szHelp    = help,  \
                .pfHandler = do_testhw_##name \
        }

#define TESTHW_ERROR( szFormat, ... ) \
        eprintf( "** ERROR: " szFormat "\n", __VA_ARGS__ )

#define TESTHW_WARNING( szFormat, ... ) \
        eprintf( "** WARNING: " szFormat "\n", __VA_ARGS__ )
