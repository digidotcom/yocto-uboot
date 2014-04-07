/*
 *  include/configs/digi_common_post.h
 *
 *  Copyright (C) 2006 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
/*
 *  !Revision:   $Revision: 1.1 $:
 *  !Author:     Markus Pietrek
 *  !Descr:      Defines all definitions that are common to all DIGI platforms
 *               and needs to be processed after <platform.h>
*/

#ifndef __DIGI_COMMON_POST_H
#define __DIGI_COMMON_POST_H

/* ********** stack sizes ********** */
#ifdef CONFIG_USE_IRQ
# define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
# define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif


/* Video settings, define video variable */
/* VGA defaults if supported */
#if defined(CONFIG_UBOOT_CRT_VGA)
#define VIDEO_DISPLAY	"displayfb:VGA"
/* LQ057Q3DC12I defaults if VGA not supported */
#elif !defined(CONFIG_UBOOT_CRT_VGA) && defined(CONFIG_UBOOT_LQ057Q3DC12I_TFT_LCD)
#define VIDEO_DISPLAY	"displayfb:LQ057Q3DC12I"
/* LQ064V3DG01 defaults if neither VGA nor LQ057 supported */
#elif !defined(CONFIG_UBOOT_CRT_VGA) && !defined(CONFIG_UBOOT_LQ057Q3DC12I_TFT_LCD) && \
    defined(CONFIG_UBOOT_LQ064V3DG01_TFT_LCD)
#define VIDEO_DISPLAY	"displayfb:LQ064V3DG01"
/*#elif !defined(CONFIG_UBOOT_CRT_VGA) && !defined(CONFIG_UBOOT_LQ057Q3DC12I_TFT_LCD) && \
    !defined(CONFIG_UBOOT_LQ064V3DG01_TFT_LCD) && defined(CONFIG_UBOOT_CUSTOM_DISPLAY)
#error "Custom display name should be defined here"*/
#endif

#if defined(CONFIG_DISPLAY1_ENABLE)
#define VIDEO_DISPLAY	"displayfb:LCD@LQ070Y3DG3B"
#endif
#if defined(CONFIG_DISPLAY2_ENABLE)
#define VIDEO_DISPLAY_2	"displayfb:disabled"
#endif

/* common kernel bootargs if not defined by the platform */
#ifndef CONFIG_BOOTARGS
# define CONFIG_BOOTARGS
#endif

#endif  /* __DIGI_COMMON_POST_H */
