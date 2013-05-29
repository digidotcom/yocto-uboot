/*
 *  common/digi/cmd_testhw/cpu/ns9215_adc.c
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
 *  !Descr:      Provides commands:
 *                 adc
 *  !References: [1] NS9215 Hardware Reference Manual, Preliminary January 2007
*/

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     defined(CONFIG_NS9215))

#include <cmd_testhw/testhw.h>
#include <lib/include/nvram_priv.h>  /* CE */

#include <asm-arm/arch-ns9xxx/ns921x_sys.h>
#include <asm-arm/arch-ns9xxx/ns9215_adc.h>
#include <asm-arm/arch-ns9xxx/io.h>  /* adc_readl */

#define USAGE	"adc <channel> <delta between samples us> <samples> [<clock kHz>]"

#define ADC_DEFAULT_CLOCK	14000000
#define ADC_DEFAULT_WAIT_EXT	0

static u32 l_uiChannel = 0;

static inline u32 timeToMS( unsigned long ulTime )
{
        return ( 1000 * ulTime ) / CFG_HZ;
}

/**
 * adc_int_clear - clears the interrupt to be able to detect new samples
 */

static inline void adc_int_clear( void )
{
        adc_rmw32( ADC_CFG, | ADC_CFG_INTCLR );
        adc_rmw32( ADC_CFG, & (~ADC_CFG_INTCLR) );
}

/**
 * adc_clock_wait_set - sets clock (in Hz) and waitstates
 */
static void adc_clock_wait_set( u32 uiClock, u32 uiWait )
{
        u32 uiDiv = ( ( sys_clock_freq() / uiClock ) - 1 ) / 2;

        adc_writel( ADC_CLOCK_CFG_N_SET( uiDiv ) |
                       ADC_CLOCK_CFG_WAIT_SET( uiWait ), ADC_CLOCK_CFG );
}

/**
 * adc_clock_set - sets clock (in Hz) and keeps waitstates
 */
static int adc_clock_set( u32 uiClock )
{
        if( !uiClock ) {
                eprintf( "0 Hz not allowed\n" );
                goto error;
        }
        
        adc_clock_wait_set( uiClock,
                            ADC_CLOCK_CFG_WAIT_GET( adc_readl( ADC_CLOCK_CFG ) ) );

        return 1;

error:
        return 0;
}

/**
 * adc_clock_get - calculates the current sampling clock
 */
static u32 adc_clock_get( void )
{
        u32 uiDiv = ADC_CLOCK_CFG_N_GET( adc_readl( ADC_CLOCK_CFG ) );
        u32 uiClock = sys_clock_freq() / ( 2 * ( uiDiv + 1 ) );

        return uiClock;
}

/**
 * adc_level - returns the level after the next sampling
 */
static inline u32 adc_level( void )
{
        /* wait until the channel has been latched into output register */
        do {
                adc_int_clear();
        } while( ADC_CFG_INTSTAT( adc_readl( ADC_CFG ) ) != l_uiChannel );

        /* return the sample */
        return adc_readl( ADC_OUTPUT( l_uiChannel ) );
}

/**
 * adc_channel_set - will use uiChannel next time and print the pins
 */
static int adc_channel_set( u32 uiChannel )
{
#define MK( ch, szPin ) [ ch ] = szPin
        static const char* aszPin[ 8 ] = {
                MK( 0, "X21-C17" ),
                MK( 1, "X21-D17" ),
                MK( 2, "X20-A18" ),
                MK( 3, "X20-B18" ),
                MK( 4, "X21-C18" ),
                MK( 5, "X21-D18" ),
                MK( 6, "X20-A19" ),
                MK( 7, "X20-B19" ),
        };
#undef MK

        /* check user input */
        if( l_uiChannel >= ARRAY_SIZE( aszPin ) ) {
                eprintf( "Wrong channel, ignoring it" );
                goto error;
        }
        
        l_uiChannel = uiChannel;

        /* avoig FAQ "where the heck is that channel" */
        printf( "Using Channel %i on JumpStart %s\n",
                l_uiChannel, aszPin[ l_uiChannel ] );

        return 1;

error:
        return 0;
}

/**
 * adc_init - low level initialization
 */
static void adc_init( void )
{
        static char bAlreadyInitialized = 0;

        if( bAlreadyInitialized )
                return;

        bAlreadyInitialized = 1;

        /* power it on */ 
        sys_rmw32( SYS_CLOCK, | SYS_CLOCK_ADC );
        sys_rmw32( SYS_RESET, | SYS_RESET_ADC );

        /* configure ADC */
        adc_writel( ADC_CFG_EN     |
                       ADC_CFG_INTCLR |
                       ADC_CFG_SEL_SET( 0x7 ), ADC_CFG );
        adc_clock_wait_set( ADC_DEFAULT_CLOCK, ADC_DEFAULT_WAIT_EXT );

        adc_int_clear();

        printf( "Ensure that VREF_ADC is connected (X21-D19), e.g. to 3.3V (X20-A20)\n" );

        adc_channel_set( 0 );
}

static int adc_series( u32 uiSamples, u32 uiDelta )
{
        typedef struct {
                u32 uiLevel;
                unsigned long ulTimeStamp;
        } sample_t;
        
        u32 u;
        ulong ulStart;
        ulong ulEnd;
        sample_t* pSampleBase = (sample_t*) malloc( uiSamples * sizeof( sample_t ) );
        sample_t* pSampleEnd = pSampleBase + uiSamples;
        sample_t* pSample;

        if( NULL == pSampleBase ) {
                eprintf( "Out-Of-Memory\n" );
                goto error;
        }

        /* sample everything first before output */
        ulStart = get_timer( 0 );
        for( pSample = pSampleBase; pSample < pSampleEnd; pSample++ ) {
                pSample->uiLevel     = adc_level();
                pSample->ulTimeStamp = get_timer( ulStart );
                udelay( uiDelta );
        }
        ulEnd = get_timer( ulStart );

        /* print results */
        printf( "Overall sampling time: %i ms\n", timeToMS( ulEnd ) );

        pSample = pSampleBase;
        printf( "Sample,     ms,  Level\n" );
        for( pSample = pSampleBase, u = 0; pSample < pSampleEnd; u++, pSample++ )
                printf( "%6u, %6u, %6u\n",
                        u,
                        timeToMS( pSample->ulTimeStamp ),
                        pSample->uiLevel );

        free( pSampleBase );
        
        return 1;
        
error:
        return 0;
}

/**
 * do_testhw_adc - provides adc commands
 */
static int do_testhw_adc( int argc, char* argv[] )
{
        u32 uiDelta;
        u32 uiSamples;
        
        adc_init();

        if( ( argc < 3 ) || ( argc > 4 ) )
                goto usage;

        CE( adc_channel_set( simple_strtoul( argv[ 0 ], NULL, 10 ) ) );
        uiDelta   = simple_strtoul( argv[ 1 ], NULL, 10 );
        uiSamples = simple_strtoul( argv[ 2 ], NULL, 10 );

        CE( adc_clock_set( ( argc >= 4 ) ?
                           simple_strtoul( argv[ 3 ], NULL, 10 ) * 1000 :
                           ADC_DEFAULT_CLOCK ) );

        printf( "Current ADC Clock is %i kHz\n",
                adc_clock_get() / 1000 );
        CE( adc_series( uiSamples, uiDelta ) );
        
        return 1;

usage:
        eprintf( "Usage: %s\n", USAGE );

error:        
        return 0;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( adc, USAGE );

#endif /*(CONFIG_COMMANDS & CFG_CMD_BSP && defined(CONFIG_UBOOT_CMD_BSP_TESTHW) && defined(CONFIG_NS921X))*/

