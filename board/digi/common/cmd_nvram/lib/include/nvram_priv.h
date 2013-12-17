/*
 *  nvram/lib/include/nvram_priv.h
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
 *  !Descr:      Defines the private functions needed by the nvram core to
 *               access I2C, Flash and a console.
 */

#ifndef NVRAM_PRIV_H
#define NVRAM_PRIV_H

#include "nvram.h"

#define RETURN_NOT_IMPLEMENTED() \
        return NV_SET_ERROR( NVE_NOT_IMPLEMENTED, "" )

#define NV_SET_ERROR( eError, szWhat ) \
        NvPrivSetError( eError, szWhat, __FUNCTION__, __FILE__, __LINE__ )

#define CE( sCmd ) \
        do { \
                if( !(sCmd) )       \
                        goto error; \
        } while( 0 )

#define CE_SET_ERROR_EXT( sCmd, iError, szWhat )           \
        do { \
                if( !(sCmd) ) {   \
                        NV_SET_ERROR( (iError), (szWhat) );   \
                        goto error; \
                } \
        } while( 0 )
#define CE_SET_ERROR( sCmd, iError )           \
        CE_SET_ERROR_EXT( sCmd, iError, "" )

#define CE_WRONG_VALUE( sCmd, szWhat )       \
        CE_SET_ERROR_EXT( sCmd, NVE_WRONG_VALUE, szWhat )

#define REQUIRE_INITIALIZED() \
        CE_SET_ERROR_EXT( g_bInitialized, NVE_NOT_INITIALIZED, "" )


#ifdef __cplusplus
extern "C" {
#endif

/* ********** global types ********** */

typedef struct nv_priv_flash_status {
        size_t  iEraseSize;
        uint8_t bBad;
	uint8_t type;   /* flash type */
} nv_priv_flash_status_t;

extern char      g_bInitialized;
extern nv_info_t g_xInfo;
extern nv_critical_t* g_pWorkcopy;
extern nv_dualb_t *g_pWorkcopyDualb;
extern nv_prodinfo_t *g_pWorkcopyProdinfo;

/* ********** global private functions implemented in nvram.c already ********** */

extern int NvPrivSetError( nv_error_e eError, const char* szWhat,
                           const char* szFunc,
                           const char* szFile, int iLine );
extern char NvPrivIsOutputEnabled( void );

/* ********** global private functions that are os specific ********** */

extern int NvPrivOSInit( void );
extern int NvPrivOSPostInit( void );
extern int NvPrivOSFinish( void );

extern int NvPrivOSCriticalPostReset(
        struct nv_critical* pCrit );
extern int NvPrivOSCriticalPartReset( /*@out@*/ struct nv_critical* pCrit,
                                      nv_os_type_e eForOS );

extern int NvPrivOSFlashOpen( char bForWrite );
extern int NvPrivOSFlashClose( void );
// iOffs must be on a sector of it's own and iLength must be < iEraseSize
extern int NvPrivOSFlashRead(  /*@out@*/ void* pvBuf,
                               loff_t iOffs, size_t iLength );
extern int NvPrivOSFlashErase(
        loff_t iOffs );
extern int NvPrivOSFlashWrite( /*@in@*/ const void* pvBuf,
                               loff_t iOffs, size_t iLength );
extern int NvPrivOSFlashInfo(
        loff_t iOffs,
        /*@out@*/ struct nv_priv_flash_status* pStatus );
extern int NvPrivOSFlashProtect( loff_t iOffs, size_t iLength, char bProtect );

extern void NvPrivOSPrintf( const char* szFormat, ...);
extern void NvPrivOSPrintfError( const char* szFormat, ...);
extern int  NvPrivImageRepair( void );

#ifdef CONFIG_PLATFORM_HAS_HWID
/* This functions are implemented in the platform code */
extern void NvPrintHwID(void);
extern int array_to_hwid(u8 *hwid);
#endif

/* ********** global private variables ********** */

#ifdef __cplusplus
}
#endif

#endif  /* NVRAM_PRIV_H */
