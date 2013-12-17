/*
 *  common/digi/cmd_bsp.h
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
*/

#ifndef __DIGI_CMD_BSP_H
#define __DIGI_CMD_BSP_H

#ifdef CONFIG_TFTP_UPDATE_ONTHEFLY
# define B_WRITE_IMG_TO_FLASH	1
# define B_PARTITION_IS_JFFS2	2
# define B_ERROR_DURING_FLASH	4
# define B_PARTITION_IS_UBIFS	8
#endif /* CONFIG_TFTP_UPDATE_ONTHEFLY */

typedef enum {
        IS_TFTP,
        IS_NFS,
        IS_FLASH,
        IS_USB,
        IS_MMC,
        IS_HSMMC,
        IS_RAM,
	IS_SATA,
} image_source_e;

const char* GetEnvVar( const char* szVar, char bSilent );

#endif  /* __DIGI_CMD_BSP_H */
