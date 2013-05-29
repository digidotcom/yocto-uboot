/*
 *  common/digi/cmd_nvram/env.h
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
 *  !Descr:      Defines the functions to combine the U-Boot environment with
 *               NVRAM
 */

#ifndef __DIGI_CMD_NVRAM_ENV_H
#define __DIGI_CMD_NVRAM_ENV_H

#include <jffs2/load_kernel.h>  /* part_info */

# define CW( cmd ) \
        ({                          \
                int _iRes = (cmd);               \
                if( !_iRes ) \
                        NvEnvPrintError( #cmd );      \
                (_iRes); \
        })

#define CE( sCmd ) \
        do { \
                if( !(sCmd) )       \
                        goto error; \
        } while( 0 )

struct nv_param_part;

extern int NvEnvUpdateFromNVRAM( void );
extern int NvEnvGetFirstJFFS2Part( struct part_info* pPartInfo );
extern void NvEnvUseDefault( void );
extern void NvEnvPrintError( const char* szFormat, ... );
extern int NvEnvUpdateLCDConfig(void);

#endif /* __DIGI_CMD_NVRAM_ENV_H */
