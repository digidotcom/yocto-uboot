/*
 * Copyright 2014 Digi International Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __HWID_H_
#define __HWID_H_

struct ccimx6_hwid {
	unsigned char	location;	/* location */
	u8		variant;	/* module variant */
	unsigned char	hv;		/* hardware version */
	unsigned char	cert;		/* type of wifi certification */
	u8		year;		/* manufacturing year */
	u8		week;		/* manufacturing week */
	u8		genid;		/* generator id */
	u32		sn;		/* serial number */
};

enum imx6_cpu {
	IMX6_NONE = 0,	/* Reserved */
	IMX6Q,		/* Quad */
	IMX6D,		/* Dual */
	IMX6DL,		/* Dual-Lite */
	IMX6S,		/* Solo */
};

struct ccimx6_variant {
	enum imx6_cpu	cpu;
	const int	sdram;
	u8		capabilities;
	const char	*id_string;
};

/* RAM size */
#define MEM_1GB		(1024 * 1024 * 1024)
#define MEM_512MB	(512 * 1024 * 1024)
#define MEM_256MB	(256 * 1024 * 1024)

/* Capabilities */
#define	CCIMX6_HAS_WIRELESS	(1 << 0)
#define	CCIMX6_HAS_BLUETOOTH	(1 << 1)
#define	CCIMX6_HAS_KINETIS	(1 << 2)
#define	CCIMX6_HAS_EMMC		(1 << 3)

extern struct ccimx6_variant ccimx6_variants[];

int get_hwid(void);

#endif	/* __HWID_H_ */
