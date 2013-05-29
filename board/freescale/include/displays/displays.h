/*
 * displays.h
 *
 * Copyright (C) 2011, 2012 by Digi International Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _DISPLAYS_H
#define _DISPLAYS_H

/*#include <common.h>
#include <lcd.h>*/
#include <linux/fb.h>
#ifdef CONFIG_LCD

/* List of displays */
struct fb_videomode ipu_displays[] = {
	{
		/* LQ070Y3DG3B */
		.name           = "LCD@LQ070Y3DG3B",
		.refresh        = 60,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 44000,
		.left_margin    = 10,
		.right_margin   = 100,
		.upper_margin   = 25,
		.lower_margin   = 10,
		.hsync_len      = 128,
		.vsync_len      = 10,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED,
		.flag           = 0,
	}, {
		/* LQ121K1LG11 */
		.name           = "LCD@LQ121K1LG11",
		.refresh        = 60,
		.xres           = 1280,
		.yres           = 800,
		.pixclock       = 22500,
		.left_margin    = 10,
		.right_margin   = 370,
		.upper_margin   = 10,
		.lower_margin   = 10,
		.hsync_len      = 10,
		.vsync_len      = 10,
		.sync           = FB_SYNC_CLK_LAT_FALL,
		.vmode          = FB_VMODE_NONINTERLACED,
		.flag           = 0,
	}
};
#endif

#endif // _DISPLAYS_H
