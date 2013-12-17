/*
 * Digi ConnectCard for i.MX28 IOMUX setup
 *
 * Copyright (C) 2013 Digi International Inc.
 *
 * Based on mx28evk.c:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>

struct ccardxmx28_ident {
        const int       sdram;
        const int       flash;
        const char      *id_string;
};

/**
 * To add new valid variant ID, append new lines in this array with its configuration
 */
struct ccardxmx28_ident ccardxmx28_id[] = {
/* 0x00 */	        	{         0,     0, "Unknown"},
/* 0x01 */	        	{         0,     0, "Not supported"},
/* 0x02 - 55001667-01 */	{0x10000000,   256, "i.MX287, 2 Eth, 1 USB, Wireless, BT, LCD, JTAG, 1-wire"},
/* 0x03 - 55001668-01 */	{0x10000000,   256, "i.MX287, 2 Eth, 1 USB, Wireless, BT, LCD, JTAG"},
/* 0x04 - 55001669-01 */	{ 0x8000000,   128, "i.MX287, 1 Eth, 2 USB, Wireless, BT, LCD, JTAG"},
/* 0x05 - 55001674-01 */	{ 0x8000000,   128, "i.MX287, 1 Eth, 2 USB, LCD, JTAG"},
/* 0x06 - 55001670-01 */	{ 0x8000000,   128, "i.MX280, 2 USB, Wireless"},
/* 0x07 - 55001671-01 */	{ 0x8000000,   128, "i.MX280, 1 Eth, 2 USB, Wireless, JTAG"},
/* 0x08 - 55001672-01 */	{ 0x8000000,   128, "i.MX280, 1 Eth, 2 USB, Wireless"},
/* 0x09 - 55001673-01 */	{ 0x8000000,   128, "i.MX280, 1 Eth, 2 USB"},
/* SPR variants */
/* 0x0a - 55001671-02 */	{ 0x8000000,   128, "i.MX283, 1 Eth, 2 USB, Wireless, LCD, JTAG"},
/* 0x0b - 55001671-03 */	{ 0x8000000,   128, "i.MX283, 1 Eth, 2 USB, LCD, JTAG"},
/* 0x0c - 55001674-02 */	{ 0x8000000,   128, "i.MX287, 2 Eth, 1 USB, LCD, JTAG"},
/* 0x0d - 55001674-03 */	{ 0x8000000,   128, "i.MX287, 1 Eth, 2 USB, LCD, JTAG"},
/* 0x0a */			{         0,     0, "Reserved for future use"},
/* 0x0b */			{         0,     0, "Reserved for future use"},
};

#define	MUX_CONFIG_SSP0	(MXS_PAD_3V3 | MXS_PAD_8MA | MXS_PAD_PULLUP)
#define	MUX_CONFIG_GPMI	(MXS_PAD_1V8 | MXS_PAD_4MA | MXS_PAD_NOPULL)
#define	MUX_CONFIG_ENET	(MXS_PAD_3V3 | MXS_PAD_8MA | MXS_PAD_PULLUP)
#define	MUX_CONFIG_EMI	(MXS_PAD_1V8 | MXS_PAD_12MA | MXS_PAD_NOPULL)

const iomux_cfg_t iomux_setup[] = {
	/* DUART */
	/* Unconfigure BOOT ROM default DUART */
	MX28_PAD_PWM0__GPIO_3_16,
	MX28_PAD_PWM1__GPIO_3_17,
	/* Configure ccardimx28 DUART */
	MX28_PAD_I2C0_SCL__DUART_RX,
	MX28_PAD_I2C0_SDA__DUART_TX,

	/* MMC0 */
	MX28_PAD_SSP0_DATA0__SSP0_D0 | MUX_CONFIG_SSP0,
	MX28_PAD_SSP0_DATA1__SSP0_D1 | MUX_CONFIG_SSP0,
	MX28_PAD_SSP0_DATA2__SSP0_D2 | MUX_CONFIG_SSP0,
	MX28_PAD_SSP0_DATA3__SSP0_D3 | MUX_CONFIG_SSP0,
	MX28_PAD_SSP0_CMD__SSP0_CMD | MUX_CONFIG_SSP0,
	MX28_PAD_SSP0_DETECT__SSP0_CARD_DETECT |
		(MXS_PAD_8MA | MXS_PAD_3V3 | MXS_PAD_NOPULL),
	MX28_PAD_SSP0_SCK__SSP0_SCK |
		(MXS_PAD_12MA | MXS_PAD_3V3 | MXS_PAD_NOPULL),

#ifdef CONFIG_NAND_MXS
	/* GPMI NAND */
	MX28_PAD_GPMI_D00__GPMI_D0 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D01__GPMI_D1 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D02__GPMI_D2 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D03__GPMI_D3 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D04__GPMI_D4 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D05__GPMI_D5 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D06__GPMI_D6 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_D07__GPMI_D7 | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_CE0N__GPMI_CE0N |
		(MXS_PAD_1V8 | MXS_PAD_4MA | MXS_PAD_PULLUP),
	MX28_PAD_GPMI_RDY0__GPMI_READY0 |
		(MXS_PAD_1V8 | MXS_PAD_4MA | MXS_PAD_PULLUP),
	MX28_PAD_GPMI_RDN__GPMI_RDN |
		(MXS_PAD_1V8 | MXS_PAD_8MA | MXS_PAD_NOPULL),
	MX28_PAD_GPMI_WRN__GPMI_WRN |
		(MXS_PAD_1V8 | MXS_PAD_8MA | MXS_PAD_NOPULL),
	MX28_PAD_GPMI_ALE__GPMI_ALE | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_CLE__GPMI_CLE | MUX_CONFIG_GPMI,
	MX28_PAD_GPMI_RESETN__GPMI_RESETN | MUX_CONFIG_GPMI,
#endif

	/* FEC0 */
	MX28_PAD_ENET0_MDC__ENET0_MDC | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_MDIO__ENET0_MDIO | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_RX_EN__ENET0_RX_EN | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_TX_EN__ENET0_TX_EN | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_RXD0__ENET0_RXD0 | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_RXD1__ENET0_RXD1 | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_TXD0__ENET0_TXD0 | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_TXD1__ENET0_TXD1 | MUX_CONFIG_ENET,
	MX28_PAD_ENET_CLK__CLKCTRL_ENET | MUX_CONFIG_ENET,
	/* FEC0 Reset */
	MX28_PAD_PWM4__GPIO_3_29 |
		(MXS_PAD_12MA | MXS_PAD_3V3 | MXS_PAD_PULLUP),

	/* FEC1 */
	MX28_PAD_ENET0_COL__ENET1_TX_EN | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_CRS__ENET1_RX_EN | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_RXD2__ENET1_RXD0 | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_RXD3__ENET1_RXD1 | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_TXD2__ENET1_TXD0 | MUX_CONFIG_ENET,
	MX28_PAD_ENET0_TXD3__ENET1_TXD1 | MUX_CONFIG_ENET,

	/* EMI */
	MX28_PAD_EMI_D00__EMI_DATA0 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D01__EMI_DATA1 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D02__EMI_DATA2 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D03__EMI_DATA3 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D04__EMI_DATA4 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D05__EMI_DATA5 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D06__EMI_DATA6 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D07__EMI_DATA7 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D08__EMI_DATA8 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D09__EMI_DATA9 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D10__EMI_DATA10 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D11__EMI_DATA11 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D12__EMI_DATA12 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D13__EMI_DATA13 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D14__EMI_DATA14 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_D15__EMI_DATA15 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_ODT0__EMI_ODT0 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_DQM0__EMI_DQM0 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_ODT1__EMI_ODT1 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_DQM1__EMI_DQM1 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_DDR_OPEN_FB__EMI_DDR_OPEN_FEEDBACK | MUX_CONFIG_EMI,
	MX28_PAD_EMI_CLK__EMI_CLK | MUX_CONFIG_EMI,
	MX28_PAD_EMI_DQS0__EMI_DQS0 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_DQS1__EMI_DQS1 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_DDR_OPEN__EMI_DDR_OPEN | MUX_CONFIG_EMI,

	MX28_PAD_EMI_A00__EMI_ADDR0 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A01__EMI_ADDR1 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A02__EMI_ADDR2 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A03__EMI_ADDR3 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A04__EMI_ADDR4 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A05__EMI_ADDR5 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A06__EMI_ADDR6 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A07__EMI_ADDR7 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A08__EMI_ADDR8 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A09__EMI_ADDR9 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A10__EMI_ADDR10 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A11__EMI_ADDR11 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A12__EMI_ADDR12 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A13__EMI_ADDR13 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_A14__EMI_ADDR14 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_BA0__EMI_BA0 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_BA1__EMI_BA1 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_BA2__EMI_BA2 | MUX_CONFIG_EMI,
	MX28_PAD_EMI_CASN__EMI_CASN | MUX_CONFIG_EMI,
	MX28_PAD_EMI_RASN__EMI_RASN | MUX_CONFIG_EMI,
	MX28_PAD_EMI_WEN__EMI_WEN | MUX_CONFIG_EMI,
	MX28_PAD_EMI_CE0N__EMI_CE0N | MUX_CONFIG_EMI,
	MX28_PAD_EMI_CE1N__EMI_CE1N | MUX_CONFIG_EMI,
	MX28_PAD_EMI_CKE__EMI_CKE | MUX_CONFIG_EMI,
};

uint32_t mxs_mem_get_size(void)
{
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;
	unsigned char variant;
	unsigned long retries = 100000;

	/* Open OTP banks */
	writel(OCOTP_CTRL_RD_BANK_OPEN, &ocotp_regs->hw_ocotp_ctrl_set);
	while((OCOTP_CTRL_BUSY & readl(&ocotp_regs->hw_ocotp_ctrl)) &&
		retries++);

	/* Read variant from HW_OCOTP_CUST1[15..8] */
	variant = (readl(&ocotp_regs->hw_ocotp_cust1) >> 8) & 0xff;

	/* Close OTP banks */
	writel(OCOTP_CTRL_RD_BANK_OPEN, &ocotp_regs->hw_ocotp_ctrl_clr);

	/*
	 * Return DRAM size of variant (default to 256MiB if invalid
	 * variant detected on OTP bits)
	 */
	if (variant > ARRAY_SIZE(ccardxmx28_id) ||
	    ccardxmx28_id[variant].sdram == 0)
		return 0x10000000;

        return (ccardxmx28_id[variant].sdram);
}

void mxs_adjust_memory_params(uint32_t *dram_vals)
{
	uint32_t dram_size = mxs_mem_get_size();

	if (0x8000000 == dram_size) {
		/* 128 SDRAM (MT47H64M16-25E) */
		dram_vals[29] = 0x0102020a;	// Enable CS0; 10 bit col addr, 13 addr pins, auto precharge=A10
	}
	else {
		/* 256 SDRAM (MT47H128M16-25E, assume default) */
		dram_vals[29] = 0x0102010a;	// Enable CS0; 10 bit col addr, 14 addr pins, auto precharge=A10
	}
	//dram_vals[37] = 0x07080403;	// CASLAT_LIN_GATE=7 CASLAT_LIN=8 CASLAT=4 WRLAT=3 (could potentially use: CASLAT_LIN_GATE=6, 6, 3, 2)

	/* EMI freq = 205.71 MHz, cycle=4.861ns */
	dram_vals[38] = 0x06005303;	// tDAL=tWR+tRP=15ns+12.5ns=27.5ns/4.86ns=6, CPD=400ns/4.86ns=83 (0x53), TCKE=3
	dram_vals[39] = 0x0a0000c8;	// tFAW=45ns/4.86ns=10, DLL reset recovery (lock) time = 200 cycles
	dram_vals[40] = 0x0200a0c1;	// TMRD=2, TINIT=200us/4.86ns=41153=0xa0c1 - see init timing diagram (note 3)
	//dram_vals[41] = 0x0002030c;	// TPDEX=tXP=2, tRCD=12.5ns/4.86ns=3, tRC=55/4.86ns=12
	dram_vals[42] = 0x00384309;	// TRAS_max=floor(70000ns/4.86ns)=14403=0x3843, TRAS_min=40ns/4.86ns=9
	dram_vals[43] = 0x03160322;	// tRP=12.5ns/4.86ns=3, tRFC(512Mb)=105ns/4.86ns=22=0x16, tREFIit=floor(3900ns/4.86ns)=802=0x322 (32ms refresh)
	dram_vals[44] = 0x02040203;	// tWTR=7.5ns/4.86ns=2, tWR=15ns/4.86ns=4 tRTP=7.5ns/4.86ns=2 tRRD(x16)=10ns/4.86ns=3
	dram_vals[45] = 0x00c80018;	// TSXR=tXSRDmin=200, TXSNR=tXSNR=tRFC(512Mb)+10ns=115ns/4.86ns=24

	dram_vals[67] = 0x01000102;	// Enable CS0 clock only
	dram_vals[73] = 0x00000000;
	dram_vals[74] = 0x00000000;
	dram_vals[75] = 0x07400300;	// EVK   value - bit 22 is usually set by FSL, but not in...
	dram_vals[76] = 0x07400300;	//   ...200Mhz case; assume a typo and correct it
	dram_vals[77] = 0x00000000;
	dram_vals[78] = 0x00000000;
	dram_vals[83] = 0x00000000;	// Disable CS0 ODT during reads
	dram_vals[84] = 0x00000001;	// Enable  CS0 ODT during writes to CS0
	dram_vals[89] = 0x00000000;
	dram_vals[90] = 0x00000000;
	dram_vals[93] = 0x00000000;
	dram_vals[94] = 0x00000000;
	dram_vals[163] = 0x00030404;
	dram_vals[164] = 0x00000002;	// TMOD=tMRD=2 cycles

	dram_vals[177] = 0x02030101;	// TCCD=2, TRPA=tRPA(<1Gb)=12.5ns/4.86ns=3, CKSRX/CKSRE=1 (see pg 115, note 1)
	dram_vals[181] = 0x00000442;	// MR0 settings for CS0: WR=3, CASLat=4, Sequential, BurstLength=4

	dram_vals[182] = 0x00000000;
	dram_vals[183] = 0x00000004;	// MR1 settings for CS0: 75ohm ODT nominal, Full drive strength
	dram_vals[184] = 0x00000000;	// MR2 settings for CS0: 2x self-refresh timing (Tcase > 85C) (to give us more temp margin)
	dram_vals[185] = 0x00000080;	// MR3 settings for CS0:
}

void board_init_ll(void)
{
	mxs_common_spl_init(iomux_setup, ARRAY_SIZE(iomux_setup));
}
