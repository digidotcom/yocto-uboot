/*
 *  common/digi/cmd_compat.c
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
 *  !Descr:      Provides the compatibility functions to migrate U-Boot 1.1.3
 *               environment variables to U-Boot 1.1.4 NVRAM
 *  !References: [1] common/env_eeprom.c:env_init
 *               [2] common/digi/cmd_nvram/env.c
*/

#include <common.h>
#if defined(CONFIG_CMD_BSP) && defined(CONFIG_UBOOT_CMD_BSP_COMPAT)

#include <command.h>            /* U_BOOT_CMD */
#include <environment.h>        /* env_t */

#include "cmd_bsp.h"            /* GetEnvVar */
#include "nvram.h"              /* NvCriticalGet */
#include "env.h"                /* CE */
#include "helper.h"             /* WaitForYesPressed */

DECLARE_GLOBAL_DATA_PTR;

#define CONSOLE_VAR "console="

/* ********** local typedefs ********** */

/*! what should be done with a 1.1.3 environment variable */
typedef enum {
        ENV_DROP, /*! isn't used any longer */
        ENV_CONV, /*! format or usage has changed. Requires env_conv_e */
} env_action_e;

/*! when action is ENV_CONV, describe ay of conversion */
typedef enum {
        ENVC_ETHADDR,    /*! ethaddr can't be written directly any longer */
        ENVC_STDBOOTARG, /*! console= is in printenv_dynamic */
        ENVC_BOOTCMD,    /*! he will probably want a completely different
                          *  bootcmd */
        ENVC_GATEWAY,    /*! new variable name */
        ENVC_DNS,        /*! new variable name */
} env_conv_e;

/*! Configures an action for a specific environment variable */
typedef struct {
        const char*  szEnvVar;  /*! variable to set */
        env_action_e eAction;   /*! action for it */
        env_conv_e   eConv;     /*! when converting, how to convert.*/
} env_action_t;

/* ********** global stuff ********** */

extern int _do_orig_setenv( int flag, int argc, char *argv[] );

/* ********** local functions ********** */

static const env_action_t* FindConvertAction( const char* szEnvVar );
static void SetEnvIfNotSet( const char* szEnvVar, const char* szEnvVal );
static void HandleOldEnv( const char* szEnvVar, const char* szEnvVal );
static int  GetI2CEnv( void );
static int  do_digi_upgrade( cmd_tbl_t* cmdtp, int flag,int argc,char* argv[]);

/* ********** local variables ********** */

/* crc32 + data( zero terminated szString list of "env=value" )*/
static char  l_acEnv[ CMD_BSP_COMPAT_ENV_SIZE ];
/* zero terminated szString list of "env=value" */
static char* l_pcEnvData = &l_acEnv[ offsetof( env_t, data ) ];

#define MK_DROP( szVar )    { .szEnvVar = szVar, .eAction = ENV_DROP  }
#define MK_CONV( szVar, eHowToConv ) \
        { .szEnvVar = szVar, .eAction = ENV_CONV, .eConv = eHowToConv  }
/*! these environment variables are taken directly from 1.1.3 and
 *  copied to this U-Boot's NVRAM with setenv */
static env_action_t l_axEnvAction[] = {
        /* auto generated */
        MK_DROP( "bootfile"       ),
        MK_DROP( "filesize"       ),
        MK_DROP( "fileaddr"       ),
        MK_DROP( "wcesize"       ),

        /* needs to be adjusted to new syntax */
        MK_CONV( "std_bootarg", ENVC_STDBOOTARG  ),
        MK_CONV( "ethaddr",     ENVC_ETHADDR     ),
        MK_CONV( "bootcmd",     ENVC_BOOTCMD     ),
        MK_CONV( "gw",          ENVC_GATEWAY     ),
        MK_CONV( "dns",         ENVC_DNS     ),

        /* 1.1.3 stuff replaced by U-Boot 'dboot' and 'update' */
        MK_DROP( "update_kernel_tftp" ),
        MK_DROP( "update_rootfs_tftp" ),
        MK_DROP( "update_uboot_tftp"  ),
        MK_DROP( "update_wce_tftp"    ),
        MK_DROP( "update_eboot_tftp"  ),
        MK_DROP( "update_kernel_usb"  ),
        MK_DROP( "update_rootfs_usb"  ),
        MK_DROP( "update_uboot_usb"   ),
        MK_DROP( "update_wce_usb"     ),
        MK_DROP( "update_eboot_usb"   ),
        MK_DROP( "update_ns_usb"      ),
        MK_DROP( "update_ns_tftp"     ),
        MK_DROP( "boot_flash"         ),
        MK_DROP( "boot_ns_flash"      ),
        MK_DROP( "boot_net"           ),
        MK_DROP( "boot_ns_net"        ),
        MK_DROP( "boot_usb"           ),
        MK_DROP( "boot_ns_usb"        ),
        MK_DROP( "boot_wce_flash"     ),
        MK_DROP( "boot_wce_net"       ),
        MK_DROP( "boot_wce_usb"       ),
        MK_DROP( "connect_to_pb"      ),
        MK_DROP( "set_esize"          ),
        MK_DROP( "set_ebsize"         ),
        MK_DROP( "set_wcesize"        ),
        MK_DROP( "ffxaddr"            ),
        MK_DROP( "ffxsize"            ),
        MK_DROP( "pou"                ),
        MK_DROP( "splashimage"        ),
        MK_DROP( "gtk"                ),
        MK_DROP( "gtr"                ),
        MK_DROP( "gtu"                ),
        MK_DROP( "gtn"                ),
        MK_DROP( "gtw"                ),
        MK_DROP( "gte"                ),
        MK_DROP( "gfk"                ),
        MK_DROP( "gfn"                ),
        MK_DROP( "gfe"                ),
        MK_DROP( "gfw"                ),
        MK_DROP( "guk"                ),
        MK_DROP( "gur"                ),
        MK_DROP( "guu"                ),
        MK_DROP( "gun"                ),
        MK_DROP( "guw"                ),
        MK_DROP( "gue"                ),
        MK_DROP( "wfr"                ),
        MK_DROP( "wfk"                ),
        MK_DROP( "wfu"                ),
        MK_DROP( "wfn"                ),
        MK_DROP( "wfe"                ),
        MK_DROP( "wfw"                ),
        MK_DROP( "cfr"                ),
        MK_DROP( "cfk"                ),
        MK_DROP( "cfu"                ),
        MK_DROP( "cfw"                ),
        MK_DROP( "cfe"                ),
        MK_DROP( "smtd"               ),
        MK_DROP( "snfs"               ),
        MK_DROP( "nsldr"              ),
        MK_DROP( "nsexe"              ),

        /* replaced by flpart */
        MK_DROP( "flashsize"          ),
        MK_DROP( "flashpartsize"      ),
        MK_DROP( "loadaddr"           ),
};
#undef MK_DROP
#undef MK_CONV

/* ********** local functions ********** */

/*! \brief determines what to do with I2C szEnv */
static const env_action_t* FindConvertAction( const char* szEnvVar )
{
        const env_action_t* pAction = NULL;
        int i = 0;

        while( i < ARRAY_SIZE( l_axEnvAction ) ) {
                const env_action_t* pIt = &l_axEnvAction[ i ];
                if( !strcmp( pIt->szEnvVar, szEnvVar ) ) {
                        /* found action */
                        pAction = pIt;
                        break;
                }

                i++;
        } /* while( i < ARRAY_SIZE ) */

        return pAction;
}

/*! don't store the same value. This skips internal handling stuff like setting
 *  baudrate.
 * When a variable is kept, it is printed. */
static void SetEnvIfNotSet( const char* szEnvVar, const char* szEnvVal )
{
        const char* szDefault = GetEnvVar( szEnvVar, 1 );

        if( ( NULL == szDefault ) || strcmp( szEnvVal, szDefault ) ) {
                /* print what we keep */
                printf( "  %s=%s\n", szEnvVar, szEnvVal );
                /* keep it */
                setenv( (char*) szEnvVar, (char*) szEnvVal );
        }
}

/*! \brief takes an old environment variable and converts/drops/reuses it */
/*! If no special handling is defined, the environment variables is reused */
static void HandleOldEnv( const char* szEnvVar, const char* szEnvVal )
{
        const env_action_t* pAction = FindConvertAction( szEnvVar );
        char  bDrop = 0;

        if( NULL != pAction ) {
                switch( pAction->eAction ) {
                    case ENV_DROP: bDrop = 1; break;
                    case ENV_CONV:
                        bDrop = 1;
                        switch( pAction->eConv ) {
                            case ENVC_GATEWAY:
                                /* gw->gateway. Use new variable */
                                bDrop = 0;
                                szEnvVar = "gateway";
                                break;
                            case ENVC_DNS:
                                /* dns->dnsip. Use new variable */
                                bDrop = 0;
                                szEnvVar = "dnsip";
                                break;
                            case ENVC_BOOTCMD:
                                eprintf( " Can't upgrade \"bootcmd=%s\" automatically. Please provide the correct \"dboot\"\n",
                                         szEnvVal );
                                break;
                            case ENVC_STDBOOTARG:
                            {
                                    /* try to extract the console= part
                                     which is now in printenv_dynamic*/
                                    char  szBootArg[ 512 ];
                                    char  szConsole[ 64 ];
                                    char* szOrigConsole = strstr( szEnvVal, CONSOLE_VAR );

                                    if( NULL != szOrigConsole ) {
                                            /* extract console= */
                                            char* szEnd = strchr( szOrigConsole, ' ' );
                                            int   iLen;
                                            int   iLenAssign = strlen( CONSOLE_VAR );

                                            if( NULL == szEnd )
                                                    iLen = strlen( szOrigConsole );
                                            else
                                                    iLen = szEnd - szOrigConsole;

                                            iLen -= iLenAssign;  /* length of argument */

                                            strncpy( szConsole, szOrigConsole + iLenAssign,
                                                     min( ARRAY_SIZE( szConsole ) - 1, iLen ) );
                                            szBootArg[ 0 ] = 0;

                                            /* remove console= from old argument */
                                            /* stuff preceeding console= */
                                            strncat( szBootArg,
                                                     szEnvVal,
                                                     min( ARRAY_SIZE( szBootArg ),
                                                          ( szOrigConsole - szEnvVal ) ) );
                                            if( NULL != szEnd ) {
                                                    /* stuff following space after console=...*/
                                                    if( szOrigConsole != szEnvVal )
                                                            /* console= was in middle, add space  */
                                                            strncat( szBootArg,
                                                                     " ",
                                                                     ARRAY_SIZE( szBootArg ) - strlen( szBootArg ) );
                                                    strncat( szBootArg,
                                                             szEnd + 1,
                                                             ARRAY_SIZE( szBootArg ) - strlen( szBootArg ) );
                                            }

                                            SetEnvIfNotSet( "std_bootarg", szBootArg );
                                            SetEnvIfNotSet( "console",     szConsole );
                                    } else
                                            /* try to keep it */
                                            bDrop = 0;
                                    break;
                            } /* ENVC_STDBOOTARG */
                            case ENVC_ETHADDR:
                            {
                                    /* ethaddr is write protected, see [2] */
                                    nv_critical_t* pCritical = NULL;
                                    nv_mac_t       xMAC;
                                    if( NvCriticalGet( &pCritical ) &&
                                        NvToMAC( &xMAC, szEnvVal ) ) {
                                            pCritical->s.p.xID.axMAC[ 0 ] = xMAC;
                                            /* param 0 (flag) not used */
                                            const char* argv[] = { "setenv", "ethaddr", szEnvVal };
                                            _do_orig_setenv( 0, ARRAY_SIZE( argv ), (char**) argv );
                                    }
                                    else
                                            eprintf( " Couldn't convert MAC %s\n",
                                                     szEnvVal );
                                    break;
                            } /* ENVC_ETHADDR */
                        } /* switch( pAction->eConv ) */
                        break;  /* ENV_CONV */
                } /* switch( pAction->eAction ) */
        } /* NULL != pAction */
        /* if not found, must be from user, keep it */

        if( !bDrop )
                SetEnvIfNotSet( szEnvVar, szEnvVal );
}

/*! \brief copies the I2C EEPROM to if valid */
/*! \return 0 if I2C EEPROM not present or wrong CRC */
static int GetI2CEnv( void )
{
        ulong ulCRC32Calc;
        ulong ulCRC32Read;

        /* copy data to RAM */

        if( eeprom_read( CONFIG_I2C_EEPROM_ADDR, CMD_BSP_COMPAT_ENV_OFFSET,
                         (uchar*) l_acEnv, sizeof( l_acEnv ) ) ) {
                eprintf( "I2C Read Error\n" );
                goto error;
        }

        /* get CRC32 in EEPROM */
        memcpy( &ulCRC32Read, &l_acEnv[ offsetof( env_t, crc ) ], sizeof( ulong ) );

        /* calculate new CRC32, only of remaining data, see [1] */
        ulCRC32Calc = crc32( 0, (uchar*) l_pcEnvData,
                             l_acEnv + sizeof( l_acEnv ) - l_pcEnvData );

        if( ulCRC32Calc != ulCRC32Read ) {
                eprintf( "CRC Error: I2C not initialized by U-Boot 1.1.3?\n" );
                goto error;
        }

        /* I2C Data is valid */

        return 1;

error:
        return 0;
}

/*! \brief Upgrades I2C Environment to NVRAM */
/*! \return 1 on failure otherwise 0 */
static int do_digi_upgrade(
        cmd_tbl_t* cmdtp,
        int flag,
        int argc, char* argv[] )
{
        char* szEnvLine = l_pcEnvData;
        const nv_info_t* pInfo = NULL;

        /* copy I2C EEPROM area to RAM if present and valid */

        if( !CW( GetI2CEnv() ) )
            goto error;

        /* I2C exists, now check whether we already have NVRAM */
        /* does  */
        pInfo = NvInfo();
        if( ( NULL != pInfo ) && pInfo->bGood )
                CE( WaitForYesPressed( "NVRAM in Flash already exists. Overwrite it", "Upgrade" ) );

        /* we can upgrade from now */

        /* scan all environment variables and handle them
           (store, convert, drop) */

        printf( "Keeping non-default environment variables:\n" );
        while( szEnvLine < ( l_acEnv + sizeof( l_acEnv ) ) ) {
                const char* szAssign;
                char  szEnvVar[ 64 ];
                int   iLen = strlen( szEnvLine );
                int   iLenName;

                if( !iLen )
                        /* no more entries */
                        break;

                /* extract only name part  */
                szAssign = strchr( szEnvLine, '=' );
                iLenName = szAssign - szEnvLine;  /* may be <0 if szAssign == NULL*/
                /* convert/reuse/drop variable */
                if( ( NULL != szAssign ) && ( iLenName < sizeof( szEnvVar ))){
                        strncpy( szEnvVar, szEnvLine, iLenName );
                        szEnvVar[ iLenName ] = 0;
                        HandleOldEnv( szEnvVar, szAssign + 1 ); /* skip '=' */
                } else
                        eprintf( "Can't handle line %s\n", szEnvLine );

                /* next line */
                szEnvLine += iLen + 1;
        } /* while( szEnvLine ) */

        if( !CW( NvSave() ) )
            goto error;

        /* save nvram */

        printf( "I2C EEPROM upgraded to NVRAM\n" );

        return 0;

error:
        return 1;
}

U_BOOT_CMD(
	upgrade, 1, 0, do_digi_upgrade,
	"Upgrades U-Boot Environment variables to 1.1.4",
	"Converts U-Boot 1.1.3 Environment to U_Boot 1.1.4 NVRAM Structure\n"
);

#endif /* defined(CONFIG_CMD_BSP) && defined(CONFIG_UBOOT_CMD_BSP_COMPAT) */
