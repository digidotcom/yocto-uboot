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
	unsigned char	tf;		/* location */
	u8		variant;	/* module variant */
	unsigned char	hv;		/* hardware version */
	unsigned char	cert;		/* type of wifi certification */
	u8		year;		/* manufacturing year */
	unsigned char	month;		/* manufacturing month */
	u32		sn;		/* serial number */
};

enum imx6_cpu {
	IMX6_NONE = 0,	/* Reserved */
	IMX6Q,		/* Quad */
	IMX6D,		/* Dual */
	IMX6DL,		/* Dual-Lite */
	IMX6S,		/* Solo */
};

/* Capabilities */
#define	CCIMX6_HAS_WIRELESS	(1 << 0)
#define	CCIMX6_HAS_BLUETOOTH	(1 << 1)
#define	CCIMX6_HAS_KINETIS	(1 << 2)

struct ccimx6_variant {
	enum imx6_cpu	cpu;
	const int	sdram;
	u8		capabilities;
	const char	*id_string;
};

#define MEM_1GB		(1024 * 1024 * 1024)
#define MEM_512MB	(512 * 1024 * 1024)
#define MEM_256MB	(256 * 1024 * 1024)

/**
 * To add new valid variant ID, append new lines in this array with its configuration
 */
struct ccimx6_variant ccimx6_variants[] = {
/* 0x00 */ { IMX6_NONE,	0, 0, "Unknown"},
/* 0x01 */ { IMX6_NONE, 0, 0, "Not supported"},
/* 0x02 - 55001766-02 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_KINETIS,
		"i.MX6 Quad, Consumer, Wireless, Bluetooth, Kinetis",
	},
/* 0x03 - 55001766-03 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"i.MX6 Quad, Industrial, Wireless, Bluetooth",
	},
/* 0x04 - 55001766-04 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"i.MX6 Dual, Consumer, Wireless, Bluetooth",
	},
/* 0x05 - 55001766-05 - SPR: Furuno Japan */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS,
		"i.MX6 Dual, Consumer, Wireless",
	},
/* 0x06 - 55001766-06 */
	{
		IMX6DL,
		MEM_512MB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"i.MX6 DualLite, Industrial, Wireless, Bluetooth",
	},
/* 0x07 - 55001766-07 */
	{
		IMX6S,
		MEM_256MB,
		CCIMX6_HAS_WIRELESS,
		"i.MX6 Solo, Consumer, Wireless",
	},
/* 0x08 - 55001766-08 - SPR: Furuno Japan */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS,
		"i.MX6 Dual, Automotive, Wireless",
	},
};

#endif	/* __HWID_H_ */
