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
/* 0x01 - 55001818-01 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_KINETIS,
		"Consumer quad-core 1.2GHz, 4GB eMMC, 1GB DDR3, 0/+70C, Wireless, Bluetooth, Kinetis",
	},
/* 0x02 - 55001818-02 */
	{
		IMX6Q,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH | CCIMX6_HAS_KINETIS,
		"Consumer quad-core 1.2GHz, 4GB eMMC, 1GB DDR3, -20/+85C, Wireless, Bluetooth, Kinetis",
	},
/* 0x03 - 55001818-03 */
	{
		IMX6Q,
		MEM_512MB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"Industrial quad-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C, Wireless, Bluetooth",
	},
/* 0x04 - 55001818-04 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"Industrial dual-core 800MHz, 4GB eMMC, 1GB DDR3, -40/+85C, Wireless, Bluetooth",
	},
/* 0x05 - 55001818-05 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS,
		"Consumer dual-core 1GHz, 4GB eMMC, 1GB DDR3, 0/+70C, Wireless",
	},
/* 0x06 - 55001818-06 */
	{
		IMX6D,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"Consumer dual-core 1GHz, 4GB eMMC, 512MB DDR3, 0/+70C, Wireless, Bluetooth",
	},
/* 0x07 - 55001818-07 */
	{
		IMX6S,
		MEM_256MB,
		CCIMX6_HAS_WIRELESS,
		"Consumer mono-core 1GHz, no eMMC, 256MB DDR3, 0/+70C, Wireless",
	},
/* 0x08 - 55001818-08 */
	{
		IMX6D,
		MEM_512MB,
		0,
		"Consumer dual-core 1GHz, 4GB eMMC, 512MB DDR3, 0/+70C",
	},
/* 0x09 - 55001818-09 */
	{
		IMX6S,
		MEM_256MB,
		0,
		"Consumer mono-core 1GHz, no eMMC, 256MB DDR3, 0/+70C",
	},
/* 0x0A - 55001818-10 */
	{
		IMX6DL,
		MEM_512MB,
		CCIMX6_HAS_WIRELESS,
		"Industrial DualLite-core 800MHz, 4GB eMMC, 512MB DDR3, -40/+85C, Wireless",
	},
/* 0x0B - 55001818-11 */
	{
		IMX6DL,
		MEM_1GB,
		CCIMX6_HAS_WIRELESS | CCIMX6_HAS_BLUETOOTH,
		"Consumer DualLite-core 1GHz, 4GB eMMC, 1GB DDR3, 0/+70C, Wireless, Bluetooth",
	},
};

const char *cert_regions[] = {
	"U.S.A.",
	"International",
	"Japan",
};

#endif	/* __HWID_H_ */
