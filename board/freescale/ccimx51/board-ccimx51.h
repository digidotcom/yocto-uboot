/*
 * Copyright 2010 Digi International Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __BOARD_CCIMX51_H_
#define __BOARD_CCIMX51_H_

struct ccimx51_hwid {
	u8		variant;
	u8		version;
	u32		sn;
	char		mloc;
};

struct ccimx51_ident {
	const char	*id_string;
	const int	mem[2];
	const int	cpu_freq;
};

/**
 * To add new valid variant ID, append new lines in this array and edit the
 * is_valid_hw_id() function, accordingly
 */
struct ccimx51_ident ccimx51_id[] = {
/* 0x00 */	{ "Unknown",						{         0,     0},         0},
/* 0x01 */	{ "Not supported",					{         0,     0},         0},
/* 0x02 */	{ "i.MX515@800MHz, Wireless, PHY, Ext. Eth, Accel",	{0x20000000,   512}, 800000000},
/* 0x03 */	{ "i.MX515@800MHz, PHY, Ext. Eth, Accel",		{0x20000000,   512}, 800000000},
/* 0x04 */	{ "i.MX515@600MHz, Wireless, PHY, Ext. Eth, Accel",	{0x20000000,   512}, 600000000},
/* 0x05 */	{ "i.MX515@600MHz, PHY, Ext. Eth, Accel",		{0x20000000,   512}, 600000000},
/* 0x06 */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		{0x20000000,  2048}, 800000000},
/* 0x07 */	{ "i.MX515@800MHz, PHY, Acceleromter",			{0x20000000,  2048}, 800000000},
/* 0x08 */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		{0x10000000,   256}, 800000000},
/* 0x09 */	{ "i.MX515@800MHz, PHY, Acceleromter",			{0x10000000,   256}, 800000000},
/* 0x0a */	{ "i.MX515@600MHz, Wireless, PHY, Accel",		{0x10000000,   256}, 600000000},
/* 0x0b */	{ "i.MX515@600MHz, PHY, Acceleromter",			{0x10000000,   256}, 600000000},
/* 0x0c */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		{0x08000000,   128}, 800000000},
/* 0x0d */	{ "i.MX512@800MHz",					{0x08000000,   128}, 800000000},
/* 0x0e */	{ "i.MX515@800MHz, Wireless, PHY, Accel",		{0x20000000,   512}, 800000000},
/* 0x0f */	{ "i.MX515@600MHz, PHY, Accel",				{0x08000000,   128}, 600000000},
/* 0x10 */	{ "i.MX515@600MHz, Wireless, PHY, Accel",		{0x08000000,   128}, 600000000},
/* 0x11 */	{ "i.MX515@800MHz, PHY, Accel",				{0x08000000,   128}, 800000000},
/* 0x12 */	{ "i.MX515@600MHz, Wireless, PHY, Accel",		{0x20000000,   512}, 600000000},
/* 0x13 */	{ "i.MX515@800MHz, PHY, Accel",				{0x20000000,   512}, 800000000},
/* 0x14 */	{ "i.MX515@800MHz, Wireless, PHY, Ext. Eth, Accel",	{0x20000000,   512}, 800000000},
/* 0x15 */	{ "i.MX515@600MHz, PHY, Acccel",			{0x20000000,   512}, 600000000},
/* 0x16 */	{ "i.MX515@800MHz, Wireless, PHY",			{0x20000000,   512}, 800000000},
/* 0x17 */	{ "i.MX515@600MHz, PHY, Ext. Eth, Accel",		{0x20000000,  2048}, 600000000},
/* 0x18 */	{ "Reserved for future use",				{         0,     0},         0},
/* 0x19 */	{ "i.MX512@600MHz",					{0x08000000,   128}, 600000000},
/* (55001445-13) 0x1a */ { "i.MX512@800MHz, Wireless, PHY",		{0x20000000,   512}, 800000000},
/* (55001445-14) 0x1b */ { "i.MX512@800MHz, Wireless, PHY, Accel",	{0x08000000,   128}, 800000000},
/* (55001585-13) 0x1c */ { "i.MX512@800MHz, PHY, Accel",		{0x08000000,   128}, 800000000},
};

#ifdef CONFIG_MXC_NAND_SWAP_BI
#define SWAP_BI_TAG			0x01
#else
#define SWAP_BI_TAG			0x00
#endif

#endif	/* __BOARD_CCIMX51_H_ */
