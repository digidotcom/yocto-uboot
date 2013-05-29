/*
 *  common/digi/cmd_testhw/cpu/cc9m2443_adc.c
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
     defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
     (defined(CONFIG_CC9M2443) || defined(CONFIG_CCW9M2443)))

#include <cmd_testhw/testhw.h>
#include <lib/include/nvram_priv.h>  /* CE */
#include <regs.h>
#define USAGE	"adc <channel> <delta between samples us> <samples> [<clock kHz>]"

#define ADC_DEFAULT_CLOCK	1000
#define CHANNELS 9 
static u32 l_uiChannel = 0;

static inline u32 timeToMS( unsigned long ulTime )
{
        return ( ulTime ) / ( CFG_HZ / 1000);
}

/**
 * adc_clock_set - sets clock (in Hz) and keeps waitstates
 */
static int adc_clock_set( u32 uiClock )
{
        u32 uiDiv;
	
	if( uiClock < 1000 || uiClock > 2500 ) {
		eprintf( "The valid range is from 1000 to 2500 KHz.\n" );
		goto error;
	}
        
	uiDiv = ((get_PCLK() / 1000 ) / uiClock) -1;
	ADCCON_REG &= ~(0x7F << 6);
	ADCCON_REG |= (uiDiv & 0x7f) << 6;
	
	return 1;
error:
	return 0;
}

/**
 * adc_clock_get - calculates the current sampling clock
 */
static u32 adc_clock_get( void )
{
        u32 uiDiv = (ADCCON_REG >> 6) & 0x7F ;
        u32 uiClock = get_PCLK() / ( uiDiv + 1 );

        return uiClock;
}

/**
 * adc_channel_set - will use uiChannel next time and print the pins
 */
static int adc_channel_set( u32 uiChannel )
{

        /* check user input */
        if(uiChannel > CHANNELS) {
                eprintf( "Wrong channel, ignoring it\n" );
                goto error;
        }
        
        l_uiChannel = uiChannel;
	ADCMUX_REG = (l_uiChannel & 0xF);
        
	printf("Using Channel %i\n", l_uiChannel);
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
	
	ADCDLY_REG = 0x1;
	ADCCON_REG &= ~(STDBM);
	ADCCON_REG |= (PRSCEN | READ_START);
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
        sample_t* pSample = pSampleBase;

        if( NULL == pSampleBase ) {
                eprintf( "Out-Of-Memory\n" );
                goto error;
        }

	/* clear the output of adc */	
	pSample->uiLevel = (ADCDAT0_REG & 0x3FF);
	udelay(10);
        
	/* sample everything first before output */
        ulStart = get_timer( 0 );
        
	for( ; pSample < pSampleEnd; pSample++ ) {
                pSample->uiLevel     = (ADCDAT0_REG & 0x3FF);
                pSample->ulTimeStamp = get_timer( ulStart );
                udelay( uiDelta );
        }
        ulEnd = get_timer( ulStart );
        /* print results */
        printf( "Overall sampling time: %i ms\n", timeToMS( ulEnd ) );

        pSample = pSampleBase;
        printf( "Sample,     ms,  Level\n" );
        for( pSample = pSampleBase, u = 0; pSample < pSampleEnd; u++, pSample++ ) {
		printf( "%6u, %6u, %6u\n",
                       u,
                        timeToMS( pSample->ulTimeStamp),
                        pSample->uiLevel );
	}
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

        if( ( argc < 3 ) || ( argc > 4 ) )
                goto usage;

        
	CE( adc_channel_set( simple_strtoul( argv[ 0 ], NULL, 10 ) ) );
        uiDelta   = simple_strtoul( argv[ 1 ], NULL, 10 );
        uiSamples = simple_strtoul( argv[ 2 ], NULL, 10 );
        
	adc_init();

        CE( adc_clock_set( ( argc >= 4 ) ?
                           simple_strtoul( argv[ 3 ], NULL, 10 ) :
                           ADC_DEFAULT_CLOCK ) );

        printf( "Current ADC Clock is %i kHz\n",
                adc_clock_get() / 1000 );
        adc_series( uiSamples, uiDelta ) ;
     
     	return 1;

usage:
        eprintf( "Usage: %s\n", USAGE );

error:        
        return 0;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( adc, USAGE );

#endif
