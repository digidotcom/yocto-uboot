/*
 *  common/digi/cmd_nvram/env.c
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
 *  !Descr:      Replaces do_saveenv by an implementation that works with the
 *               nvram library.
 */

#include <common.h>

#ifdef CONFIG_CMD_NAND
# include <nand.h>
#endif  /* CONFIG_CMD_NAND */

#include <command.h>
#include <environment.h>        /* env_t */

#include "nvram.h"
#include "env.h"

#undef DBG_ENV

#define OFFS( x ) \
        .iOffs = ( (char*) &((nv_critical_t*) NULL)->s.p.x -       \
                   (char*) NULL )

#undef DEBUG                    /* defined by nand.h */
#ifdef DBG_ENV
# define DEBUG( szFormat, ... ) \
        printf( szFormat, ##__VA_ARGS__ )
#else
# define DEBUG( szFormat, ... ) do { } while( 0 )
#endif  /* DBG_ENV */

#if defined(CONFIG_CMD_UBI)
# include "partition.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

/* references to names in env_common.c */
extern uchar default_environment[];

env_t* env_ptr = NULL;
char * env_name_spec = "Digi NVRAM";

/* ********** local typedefs **********/


typedef enum {
        ENV_IP,
        ENV_BOOL_ON,            /* should be printed ON/OFF */
        ENV_MAC
} env_type_e;

typedef struct env_param {
        const char* szName;
        env_type_e  eType;
        int         iOffs;
        char        bReadOnly;
} env_param_t;

/* it is not static so we can set it manually for debugging */
char g_bOptDetailed = 0;

/* ********** extern stuff **********/

extern env_t* env_ptr;          /* common/env_<nand/eeprom>.c */
extern int _do_setenv( int flag, int argc, char *argv[] );  /* common/cmd_nvedit.c */
extern int _do_orig_setenv (int flag, int argc, char *argv[]);

/* ********** local functions **********/

static const env_param_t* EnvVarInNVRAM( const char* szArg );
static int EnvVarStoreToNVRAM( const env_param_t* pParam, const char* szVal );

/* ********** local variables **********/

static const env_param_t l_axParam[] = {
        { "ethaddr",      ENV_MAC, OFFS( xID.axMAC[ 0 ] ), 1 },
        { "wlanaddr",     ENV_MAC, OFFS( xID.axMAC[ 1 ] ), 1 },
        { "eth1addr",     ENV_MAC, OFFS( eth1addr ), 1 },
        { "btaddr",       ENV_MAC, OFFS( btaddr1 ), 1 },
        { "ipaddr",       ENV_IP,  OFFS( xIP.axDevice[ 0 ].uiIP ) },
        { "ipaddr_wlan",  ENV_IP,  OFFS( xIP.axDevice[ 1 ].uiIP ) },
        { "ipaddr1",      ENV_IP,  OFFS( eth1dev.uiIP ) },
        { "netmask",      ENV_IP,  OFFS( xIP.axDevice[ 0 ].uiNetMask ) },
        { "netmask_wlan", ENV_IP,  OFFS( xIP.axDevice[ 1 ].uiNetMask ) },
        { "netmask1",     ENV_IP,  OFFS( eth1dev.uiNetMask ) },
        { "serverip",     ENV_IP,  OFFS( xIP.uiIPServer ) },
        { "gatewayip",    ENV_IP,  OFFS( xIP.uiIPGateway) },
        { "dnsip",        ENV_IP,  OFFS( xIP.auiIPDNS[ 0 ] ) },
        { "dnsip2",       ENV_IP,  OFFS( xIP.auiIPDNS[ 1 ] ) },
        { "dhcp",         ENV_BOOL_ON, OFFS( xIP.axDevice[ 0 ].flags.bDHCP ) },
        { "dhcp_wlan",    ENV_BOOL_ON, OFFS( xIP.axDevice[ 1 ].flags.bDHCP ) },
        { "dhcp1",        ENV_BOOL_ON, OFFS( eth1dev.flags.bDHCP ) },
};



/* ********** global functions **********/
int _do_setenv( int flag, int argc, char* argv[] )
{
        int iRes = 1;
        const env_param_t* pParam;

        if( argc >= 2 ) {
                pParam = EnvVarInNVRAM( argv[ 1 ] );
                if( NULL != pParam ) {
                        /* variable is maintained in NVRAM and not by U-Boot
                         * environment */
                        if( !pParam->bReadOnly ) {
                                if( argc != 2 )
                                        iRes = EnvVarStoreToNVRAM( pParam, argv[ 2 ] );
                                else {
                                        iRes = 0;
                                        eprintf( "Variable %s can't be deleted\n",
                                                 argv[ 1 ] );
                                }
                        } else {
                                iRes  = 0;
                                eprintf( "Variable %s can't be changed by setenv\n",
                                         argv[ 1 ] );
                        } /* if( !pParam->bReadOnly ) */
                } /* if( NULL != pParam */
        } /* if( argc >= 2 ) */

        if( iRes )
                iRes = ( _do_orig_setenv( flag, argc, argv ) >= 0 );

        return ( iRes ? 0 : -1 );
}

int saveenv( void )
{
        int iRes;
        int i;
        char* argvSetEnv[ 4 ] = { "setenv", NULL, NULL };

        /* remove environment variables used by us */
        for( i = 0; i < ARRAY_SIZE( l_axParam ); i++ ) {
                argvSetEnv[ 1 ] = (char*) l_axParam[ i ].szName;
                DEBUG( "Removing from U-Boot NVRAM %s\n",
                         argvSetEnv[ 1 ] );
                /* ignore if it works or not */
                _do_orig_setenv( 0, ARRAY_SIZE( argvSetEnv ), argvSetEnv );
        }

        iRes = CW( NvOSCfgSet( NVOS_UBOOT, env_ptr, CONFIG_ENV_SIZE ) );
        if( iRes )
                iRes &= CW( NvSave() );

        CW( NvEnvUpdateFromNVRAM() );

        return ( iRes ? 0 : -1 );
}

/*! \brief adds the NVRAM variables to U-Boot Environment variables */
/*! \return 0 on failure otherwise 1 */

int NvEnvUpdateFromNVRAM( void )
{
        int iRes = 1;
        int i;
        nv_critical_t* pCrit;
        char szTmp[ 40 ];

        if( !CW( NvCriticalGet( &pCrit ) ) )
            return 0;

        for( i = 0; i < ARRAY_SIZE( l_axParam ); i++ ) {
                char* argvSetEnv[] = { "setenv", NULL, NULL };
                const env_param_t* pParam = &l_axParam[ i ];
                void* pvAddr;

                argvSetEnv[ 1 ] = (char*) pParam->szName;
                argvSetEnv[ 2 ] = szTmp;

                pvAddr = (char*) pCrit + pParam->iOffs;
                switch( pParam->eType ) {
                    case ENV_MAC:
                        strncpy( szTmp,
                                 NvToStringMAC( *(nv_mac_t*) pvAddr ),
                                 sizeof( szTmp ));
                        break;
                    case ENV_IP:
                    {
                            nv_ip_t ip = *(nv_ip_t*) pvAddr;
                            strncpy( szTmp,
                                     NvToStringIP( ip ),
                                     sizeof( szTmp ));
                    }
                    break;
                    case ENV_BOOL_ON:
                        sprintf( szTmp, "%s",
                                 ( *(char*) pvAddr ? "on" : "off" ) );
                        break;
                }

                DEBUG( "Adding to U-Boot from NVRAM %s=%s\n",
                       argvSetEnv[ 1 ], argvSetEnv[ 2 ] );

                _do_orig_setenv( 0, ARRAY_SIZE( argvSetEnv ), argvSetEnv );
        }

#if defined(CONFIG_CMD_UBI)
	if ( !PartSetMtdParts() ) {
		return 0;
	}
#endif

        return iRes;
}

/*! \brief returns the partition information of the first JFFS2 partition */
/*! \param pPartInfo offset and size are modified when a partition is found.
 * \return 1 if there is a JFFS2 partition in the partition table  */

int NvEnvGetFirstJFFS2Part( struct part_info* pPartInfo )
{
        int iRes = 0;
        const nv_param_part_t* pPart;

        iRes = NvParamPartFind(
                &pPart,
                NVPT_FILESYSTEM,
                NVFS_JFFS2,
                0,
                1 );

        if( iRes ) {
                pPartInfo->offset = pPart->ullStart;
                pPartInfo->size   = pPart->ullSize;
        }

        return iRes;
}

/*! \brief checks whether szArg is maintained by NVRAM */
/*! When maintained in NVRAM, it shouldn't be stored in U-Boot Environment
 *  variables persistent
* \return 1 if szArg is in NVRAM */

static const env_param_t* EnvVarInNVRAM( const char* szArg )
{
        int i;
        const env_param_t* pParam = NULL;

        for( i = 0; i < ARRAY_SIZE( l_axParam ); i++ ) {

                if( !strcmp( szArg, l_axParam[ i ].szName ) ) {
                        pParam = &l_axParam[ i ];
                        break;
                }
        }

        return pParam;
}

/*! \brief adds the U-Boot environment variables to NVRAM */
/*! \return 0 on failure otherwise 1 */

static int EnvVarStoreToNVRAM( const env_param_t* pParam, const char* szVal )
{
        int iRes = 0;
        nv_critical_t* pCrit;
        void* pvAddr;

        if( !CW( NvCriticalGet( &pCrit ) ) )
            return 0;

        pvAddr = (char*) pCrit + pParam->iOffs;

        switch( pParam->eType ) {
            case ENV_MAC:
                iRes = NvToMAC( (nv_mac_t*) pvAddr, szVal );
                break;

            case ENV_IP:
                iRes = NvToIP( (nv_ip_t*) pvAddr, szVal );
                break;

            case ENV_BOOL_ON:
                if( !strcmp( szVal, "no" ) || !strcmp( szVal, "off" ) ) {
                        *(char*) pvAddr = 0;
                        iRes = 1;
                } else if( !strcmp( szVal, "yes" ) || !strcmp( szVal, "on" ) ) {
                        *(char*) pvAddr = 1;
                        iRes = 1;
                }
                break;
        }

        /* we did change it, printall should reflect it */
        CW( iRes );

        return iRes;
}

uchar env_get_char_spec (int index)
{
	return ( *((uchar *)(gd->env_addr + index)) );
}

/* this is called before nand_init()
 * so we can't read Nand to validate env data.
 * Mark it OK for now. env_relocate() in env_common.c
 * will call our relocate function which will does
 * the real validation.
 */

int env_init( void )
{
	gd->env_addr  = (ulong)&default_environment[0];
        /* to call env_relocate_spec */
	gd->env_valid = 1;

	return (0);
}

void env_relocate_spec( void )
{
        size_t iSize;
        static char bAlreadyInitialized = 0;
        int iRes = 1;
        nv_repair_e eMode = NVR_AUTO;

#ifdef CONFIG_DOWNLOAD_BY_DEBUGGER
        /* if debugger is used, we always want to use defaults, e.g. ops */
        eMode = NVR_IGNORE;
#endif

        if( !bAlreadyInitialized ) {
                iRes = CW( NvInit( eMode ) );
                /* env_relocate sets env_addr on return. This invalidates all
                 * variables we use  */
                gd->env_addr = (ulong)&(env_ptr->data);
                bAlreadyInitialized = 1;
        }

#ifdef CONFIG_UBOOT_IGNORE_NVRAM_ENV
        puts("*** Warning - ignoring environment in NVRAM, using default environment\n\n");
        NvEnvUseDefault();
#else
        if( iRes )
                iRes = CW( NvOSCfgGet( NVOS_UBOOT, env_ptr, CONFIG_ENV_SIZE, &iSize ) );

	if( !iRes || ( CONFIG_ENV_SIZE != iSize ) ||
            ( crc32( 0, env_ptr->data, ENV_SIZE ) != env_ptr->crc ) ) {
                puts ("*** Warning - bad CRC or NAND, using default environment\n\n");
                NvEnvUseDefault();
        } else {
                gd->env_valid = 1;
                CW( NvEnvUpdateFromNVRAM() );
        }
#endif /* CONFIG_UBOOT_IGNORE_NVRAM_ENV */

#ifdef CFG_HAS_WIRELESS
	{
		/*
		 * For wireless modules, read also the mac addr and calibration information.
		 * That information is placed at address 0x1000 to allow the OS accessing
		 * to that information.
		 */
		unsigned char *pMac = (unsigned char *)(gd->bd->bi_boot_params + 0xf00);
		wcd_data_t *pWCal = (wcd_data_t *)(gd->bd->bi_boot_params + 0xf08);
		nv_critical_t *pNvram = NULL;

		iRes = CW( NvOSCfgGet( NVOS_WCAL_DATA, pWCal, sizeof( wcd_data_t ), &iSize ) );
		if( iRes && ( iSize == sizeof( wcd_data_t ) ) ) {
			iRes = NvPrivWirelessCalDataCheck( pWCal );
			if ( !iRes )
				puts ("*** Warning - Wireless Calibration Data corrupted!\n\n");
		}

		iRes = NvCriticalGet( &pNvram );
		if ( iRes != 0 ) {
			memcpy((unsigned char *)pMac, &pNvram->s.p.xID.axMAC[1], 6);
		}
	}
#endif

#if defined(CONFIG_DISPLAY1_ENABLE) || defined(CONFIG_DISPLAY2_ENABLE)
	iRes = NvEnvUpdateLCDConfig();
#endif
}

void NvEnvUseDefault( void )
{
	memset( env_ptr, 0, sizeof(env_t) );
	memcpy( env_ptr->data,
                default_environment,
                CONFIG_ENV_SIZE );

	env_ptr->crc = crc32( 0, env_ptr->data, CONFIG_ENV_SIZE );
	gd->env_valid = 1;

        CW( NvEnvUpdateFromNVRAM() );
}

void NvEnvPrintError( const char* szFormat, ... )
{
        const char* szError    = NULL;
        const char* szWhat = NULL;
        const char* szFunc = NULL;
        const char* szFile = NULL;
        int iLine;

        va_list ap;

        /*eprintf may not be present here because no stderr set */
        printf( "*** Error: " );
        if( g_bOptDetailed ) {
                /*@-formatconst@*/
                va_start( ap, szFormat );
                vprintf( szFormat, ap );
                va_end( ap );
                printf( ": " );
                /*@+formatconst@*/
        }

        if( NVE_GOOD != NvErrorMsg( &szError, &szWhat, &szFunc, &szFile, &iLine ) ) {
                if( g_bOptDetailed )
                        printf( " %s: (%s) @ %s:%i (%s)",
                                 szError, szWhat, szFile, iLine, szFunc );
                else
                        printf( " %s: (%s)",
                                szError, szWhat);
        }

        printf( "\n" );
}

#if defined(CONFIG_DISPLAY1_ENABLE) || defined(CONFIG_DISPLAY2_ENABLE)
int NvEnvUpdateLCDConfig(void)
{
	int iRes;
	size_t iSize;;

	nv_lcd_config_t *pLCDConfig = (nv_lcd_config_t *)(gd->bd->bi_boot_params + 0x1980);

	iRes = CW( NvOSCfgGet( NVOS_LCD_CONFIG, pLCDConfig, sizeof( nv_lcd_config_t ), &iSize ) );

	return iRes;
}
#endif
