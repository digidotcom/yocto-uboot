/*
 *  common/helper.c
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
 *  !Descr:      Various helper stuff, like waiting for Yes Pressed etc.
*/

#include <common.h>
#include "helper.h"

extern int RunningAutoScript;

/*! \brief waits until something is pressed on console */
/*! \return 1 if yes is pressed otherwise 1
 */
int WaitForYesPressed( const char* szWhat, const char* szWhere )
{
        int iRes = 0;

	/**
	 * From autoscript we shouldn't expect user's confirmations.
	 * Assume yes is the correct answer here to avoid halting the script.
	 */
	if (RunningAutoScript)
		return 1;

        printf( "%s(y/n)", szWhat );

        while( 1 ) {
                if( tstc() ) {
                        char c = getc();
                        putc( c );

                        if( 'y' == c ) {
                                iRes = 1;
                                break;
                        } else {
                                eprintf( "\n%s aborted\n", szWhere );
                                break;
                        }
                }
        } /* while( true ) */

        printf( "\n" );

        return iRes;
}

long get_input(const char *cp)
{
	ulong res = 0;
	char * endp;
	res = simple_strtoul(cp, &endp, 16);
	if(strlen(cp) != (endp - cp)) {
		printf("input not valid\n");
		return -1;
	}
	return res;
}
