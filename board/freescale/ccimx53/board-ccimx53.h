/*
 * Copyright 2011 Digi International Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __BOARD_CCIMX53_H_
#define __BOARD_CCIMX53_H_

struct ccimx53_hwid {
	u8		variant;
	u8		version;
	u32		sn;
	char		mloc;
};

struct ccimx53_ident {
	const char	*id_string;
	const int	mem[2];
	const int	cpu_freq;
};

/**
 * To add new valid variant ID, append new lines in this array with its configuration
 */
struct ccimx53_ident ccimx53_id[] = {
/* 0x00 - 5500xxxx-xx */	{ "Unknown",						{         0,     0},          0},
/* 0x01 - 5500xxxx-xx */	{ "Not supported",					{         0,     0},          0},
/* 0x02 - 55001604-01 */	{ "i.MX535@1000MHz, Wireless, PHY, Ext. Eth, Accel",	{0x20000000,   512}, 1000000000},
/* 0x03 - 55001605-01 */	{ "i.MX535@1000MHz, PHY, Accel",			{0x20000000,   512}, 1000000000},
/* 0x04 - 55001604-02 */	{ "i.MX535@1000MHz, Wireless, PHY, Ext. Eth, Accel",	{0x40000000,   512}, 1000000000},
/* 0x05 - 5500xxxx-xx */	{ "i.MX535@1000MHz, PHY, Ext. Eth, Accel",		{0x40000000,   512}, 1000000000},
/* 0x06 - 55001604-03 */	{ "i.MX535@1000MHz, Wireless, PHY, Accel",		{0x20000000,   512}, 1000000000},
/* 0x07 - 5500xxxx-xx */	{ "i.MX535@1000MHz, PHY, Accel",			{0x20000000,   512}, 1000000000},
/* 0x08 - 55001604-04 */	{ "i.MX537@800MHz, Wireless, PHY, Accel",		{0x20000000,   512},  800000000},
/* 0x09 - 55001605-02 */	{ "i.MX537@800MHz, PHY, Accel",				{0x20000000,   512},  800000000},
/* 0x0a - 5500xxxx-xx */	{ "i.MX537@800MHz, Wireless, PHY, Ext. Eth, Accel",	{0x40000000,   512},  800000000},
/* 0x0b - 55001605-03 */	{ "i.MX537@800MHz, PHY, Ext. Eth, Accel",		{0x40000000,   512},  800000000},
/* 0x0c - 5500xxxx-xx */	{ "Reserved for future use",				{         0,     0},          0},
/* 0x0d - 55001605-05 */	{ "i.MX537@800MHz, PHY, Accel",				{0x20000000,  1024},  800000000},
/* 0x0e - 55001604-05 */	{ "i.MX535@800MHz, Wireless, PHY, Ext. Eth, Accel",	{0x20000000,  1024},  800000000},
/* 0x0f - 5500xxxx-xx */	{ "Reserved for future use",				{         0,     0},          0},
/* 0x10 - 55001604-06 */	{ "i.MX535@1000MHz, Wireless, PHY, Accel",		{ 0x8000000,   128}, 1000000000},
/* 0x11 - 55001605-07 */	{ "i.MX535@1000MHz, PHY, Accel",			{ 0x8000000,   128}, 1000000000},
/* 0x12 - 5500xxxx-xx */	{ "Reserved for future use",				{         0,     0},          0},
/* 0x13 - 5500xxxx-xx */	{ "Reserved for future use",				{         0,     0},          0},
};

#ifdef CONFIG_MXC_NAND_SWAP_BI
#define SWAP_BI_TAG			0x01
#else
#define SWAP_BI_TAG			0x00
#endif

#endif	/* __BOARD_CCIMX53_H_ */
