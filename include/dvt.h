/*
 *  /targets/U-Boot-cc9p9360js.cvs/include/dvt.h
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision: 1.1 $
 *  !Author:     Markus Pietrek
 *  !Descr:      Stuff for Device Verification Tests
 *               When doing DVTs, we sometimes want a different behaviour than
 *               in production. E.g. when writing to the NAND flash, we don't
 *               want the MTD layer to abort on the first verify failed
 *               error. We might want to check all bytes of the page and print
 *               all errors.
 *
 *               This header files provides functions that enable specific
 *               behaviour. 
 *
 *               If DVT  is off, then DVTWarning and DVTError always return 0
 *             
*/

#ifndef _DVT_H
#define _DVT_H

#include <common.h>
#include <stdarg.h>

#define DVT_PREFIX     "\n*** DVT: "
#define ERROR_PREFIX   DVT_PREFIX "ERROR:   "
#define WARNING_PREFIX DVT_PREFIX "WARNING: "

#if defined(CONFIG_CMD_BSP) &&			\
    defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&	\
    defined(CONFIG_DVT_PROVIDED)

#define DVTPrintOnlyOnce( ... )					\
	{                       				\
		static char bAlreadyPrinted = 0;		\
		if( DVTIsEnabled() && !bAlreadyPrinted ) {	\
			bAlreadyPrinted = 1;			\
			eprintf( DVT_PREFIX __VA_ARGS__ );	\
		}						\
	}

extern char g_bDVTIsEnabled;
extern char g_bDVTStatusChanged;
extern char g_bDVTHadError;
extern char g_bDVTHadWarning;

/**
 * DVTIsEnabled - 
 *
 * @return: returns 1 if DVT test is on otherwise 0
 */
static inline char DVTIsEnabled( void ) 
{
        return g_bDVTIsEnabled;
}

/**
 * DVTError - 
 *
 * @return: returns 1 if DVT test had an error. It also prints warnings
 */
static inline char DVTError( void ) 
{
        if( g_bDVTHadError )
                eprintf( ERROR_PREFIX "occured\n" );

        if( g_bDVTHadWarning )
                eprintf( WARNING_PREFIX "occured\n" );
        
        return g_bDVTHadError;
}

/**
 * DVTStatusChanged -
 * @bAck - when 1, the status changed is acknowledged, so a next call doesn't
 *         return 1.
 *         when two DVTStatusChanged( 0 ) following each other, only the first
 *         will return 1.
 *         A DVTStatusChanged( 1 ) will also return 1 and reset it.
 *         This way, DVTStatusChanged( 0 ) can be used on the same level
 *         without interferring each other,
 *         but an upper level will also detect it.
 * @return: if an error or warning is new
 */
static inline char DVTStatusChanged( char bFullAck ) 
{
        char bRes = 0;

        if( bFullAck ) {
                bRes = ( g_bDVTStatusChanged > 0 );
                g_bDVTStatusChanged = 0;
        } else if( g_bDVTStatusChanged > 1 ) {
                bRes = 1;
                g_bDVTStatusChanged = 1;
        }
        
        return bRes;
}

static inline void DVTReset( void )
{
        g_bDVTStatusChanged = 0;
        g_bDVTHadError      = 0;
        g_bDVTHadWarning    = 0;
}

/*
 * DVTCountError - remembers an error
 */
static inline void DVTCountError( void )
{
	if( DVTIsEnabled() ) {
		g_bDVTStatusChanged = 2;
		g_bDVTHadError      = 1;
	}
}

/**
 * DVTSetError - remembers an error and returns with 1 if DVTIsEnabled
 *
 * Use it like:
 * if( DVTSetError( "" ) ) { DVT Handling } else { normal handling }
 */
static inline char DVTSetError( const char* szFormat, ... ) 
{
        if( !DVTIsEnabled() )
                return 0;

        if( NULL != szFormat ) {
                va_list ap;
                
                /* it's not stderr, put there is no vfprintf( stderr )*/
                va_start( ap, szFormat );
                eprintf( ERROR_PREFIX );
                vprintf( szFormat, ap );
                eprintf( "\n" );
                va_end( ap );
        }
        
        g_bDVTStatusChanged = 2;
        g_bDVTHadError      = 1;
        
        return 1;
}

/*
 * DVTCountWarning - remembers a warning
 */
static inline void  DVTCountWarning( void )
{
	if( DVTIsEnabled() ) {
		g_bDVTStatusChanged = 2;
		g_bDVTHadWarning    = 1;
	}
}

/**
 * DVTSetWarning - remembers a warning and returns with 1 if DVTIsEnabled
 *
 * Use it like:
 * if( DVTSetWarning() ) { DVT Handling } else { normal handling }
 */
static inline char DVTSetWarning( const char* szFormat, ... ) 
{
        if( !DVTIsEnabled() )
                return 0;
        
        if( NULL != szFormat ) {
                va_list ap;
                
                /* it's not stderr, put there is no vfprintf( stderr )*/
                va_start( ap, szFormat );
                eprintf( WARNING_PREFIX );
                vprintf( szFormat, ap );
                eprintf( "\n" );
                va_end( ap );
        }
        
        g_bDVTStatusChanged = 2;
        g_bDVTHadWarning    = 1;

        return 1;
}

#else
/* don't have negative impact on performance */
# define DVTPrintOnlyOnce( ... )      do {} while( 0 )
# define DVTIsEnabled()               ( 0 )
# define DVTEnable( bEnabled )        ( 0 )
# define DVTError()                   ( 0 )
# define DVTStatusChanged(bAck)       ( 0 )
# define DVTReset()                   do {} while( 0 )
# define DVTCountError()    	      do {} while( 0 )
# define DVTSetError(szFormat,...)    ( 0 )
# define DVTCountWarning()            do {} while( 0 )
# define DVTSetWarning(szFormat,...)  ( 0 )
#endif  /* CONFIG_DVT_PROVIDED */
#endif  /* _DVT_H */
