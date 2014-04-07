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

#ifndef __BOARD_CCARDIMX28_H_
#define __BOARD_CCARDIMX28_H_

struct ccardimx28_hwid {
	u8		tf;		/* location */
	u8		variant;	/* module variant */
	unsigned char	hv;		/* hardware version */
	unsigned char	cert;		/* type of wifi certification */
	u8		year;		/* manufacturing year */
	unsigned char	month;		/* manufacturing month */
	u32		sn;		/* serial number */
};

struct ccardimx28_ident {
	const int	sdram;
	const int	flash;
	u8		has_wireless;
	const char	*id_string;
};

/**
 * To add new valid variant ID, append new lines in this array with its configuration
 */
struct ccardimx28_ident ccardimx28_id[] = {
/* 0x00 */	        	{         0,     0, 0, "Unknown"},
/* 0x01 */	        	{         0,     0, 0, "Not supported"},
/* 0x02 - 55001667-01/03 */	{0x10000000,   256, 1, "i.MX287, 2 Eth, 1 USB, Wireless, BT, LCD, JTAG, 1-wire"},
/* 0x03 - 55001668-01 */	{0x10000000,   256, 1, "i.MX287, 2 Eth, 1 USB, Wireless, BT, LCD, JTAG"},
/* 0x04 - 55001669-01 */	{ 0x8000000,   128, 1, "i.MX287, 1 Eth, 2 USB, Wireless, LCD, JTAG"},
/* 0x05 - 55001674-01 */	{ 0x8000000,   128, 0, "i.MX287, 1 Eth, 2 USB, LCD, JTAG"},
/* 0x06 - 55001670-01 */	{ 0x8000000,   128, 1, "i.MX280, 2 USB, Wireless"},
/* 0x07 - 55001671-01 */	{ 0x8000000,   128, 1, "i.MX280, 1 Eth, 2 USB, Wireless, JTAG"},
/* 0x08 - 55001672-01 */	{ 0x8000000,   128, 1, "i.MX280, 1 Eth, 2 USB, Wireless"},
/* 0x09 - 55001673-01 */	{ 0x8000000,   128, 0, "i.MX280, 1 Eth, 2 USB"},
/* 0x0a - 55001671-02 (SPR) */	{ 0x8000000,   128, 1, "i.MX283, 1 Eth, 2 USB, Wireless, LCD, JTAG"},
/* 0x0b - 55001671-03 (SPR) */	{ 0x8000000,   128, 0, "i.MX283, 1 Eth, 2 USB, LCD, JTAG"},
/* 0x0c - 55001674-02 (SPR) */	{ 0x8000000,   128, 0, "i.MX287, 2 Eth, 1 USB, LCD, JTAG"},
/* 0x0d - 55001674-03 (SPR) */	{ 0x8000000,   128, 0, "i.MX287, 1 Eth, 2 USB, LCD, JTAG"},
/* 0x0e - 55001674-04 */	{0x10000000,   256, 0, "i.MX287, 2 Eth, 1 USB, LCD, JTAG"},
/* 0x0f - 55001669-02 */	{0x10000000,   256, 1, "i.MX287, 1 Eth, 2 USB, Wireless, LCD, JTAG"},
/* 0x10 - 55001667-02 */	{ 0x8000000,   128, 1, "i.MX287, 1 Eth, 2 USB, Wireless, BT, LCD, JTAG"},
/* 0x11 - 55001668-02 */	{ 0x8000000,   128, 1, "i.MX287, 2 Eth, 1 USB, Wireless, BT, LCD, JTAG"},
				{         0,     0, 0, "Reserved for future use"},
};

#endif	/* __BOARD_CCARDIMX28_H_ */
