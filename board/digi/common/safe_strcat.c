/*
 *  Copyright (C) 2011 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
 */

#include "safe_strcat.h"
#include <common.h>

char *safe_strcat(char *dest, const char *src, int dest_size)
{
	return(strncat(dest, src, dest_size - strlen(dest) - 1));
}
