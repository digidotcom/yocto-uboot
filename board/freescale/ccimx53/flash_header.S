/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <asm/arch/mx53.h>

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

#define MAKE_TAG(tag, len, v) CPU_2_BE_32(( ((tag) << 24) | ((len) << 8) | (v) ))
#define CPU_2_BE_32(l) \
       ((((l) & 0x000000FF) << 24) | \
	(((l) & 0x0000FF00) << 8)  | \
	(((l) & 0x00FF0000) >> 8)  | \
	(((l) & 0xFF000000) >> 24))

#define MXC_DCD_ITEM(i, addr, val)   \
dcd_node_##i:                        \
        .word CPU_2_BE_32(addr) ;     \
        .word CPU_2_BE_32(val)  ;     \

.section ".text.flasheader", "x"
	b	_start

#ifdef CONFIG_MXC_NAND
/*
 * NAND Flash Configuration Block
 */
fcb_fp2:          .word 0x20424346
fcb_fp3:          .word 0x00000001
	.org    0x68
fcb_img1:         .word 0
fcb_img2:         .word 0x400
                  .word 0xa9
                  .word 0xa9

#ifdef CONFIG_4KPAGE_NAND
	.org    0x78
fcb_dbbt:         .word 0x00000000
fcb_bboff:        .word 0x00000f4a
	.org    0xac
fcb_swapen:       .word 1
fcb_bbspare:      .word 0x000010b7
#else /* 2K page nand flash */
	.org    0x78
fcb_dbbt:         .word 0x00000000
fcb_bboff:        .word 0x000007d0
	.org    0xac
fcb_swapen:       .word 1
fcb_bbspare:      .word 0x00000831
#endif /* CONFIG_4KPAGE_NAND */
#endif /* CONFIG_MXC_NAND */

	.org	CONFIG_FLASH_HEADER_OFFSET
ivt_header:       .word 0x402000D1 /* Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:  .word _start
reserv1:          .word 0x0
dcd_ptr:          .word dcd_hdr
boot_data_ptr:	  .word boot_data
self_ptr:         .word ivt_header
app_code_csf:     .word 0x0
reserv2:          .word 0x0

boot_data:        .word TEXT_BASE
image_len:        .word __end - TEXT_BASE
plugin:           .word 0x0


#if CONFIG_NR_DRAM_BANKS > 1
dcd_hdr:          .word MAKE_TAG(0xD2, 64 * 8 + 4 + 4, 0x40)
write_dcd_cmd:    .word MAKE_TAG(0xCC, 64 * 8 + 4, 4)
#else
dcd_hdr:          .word MAKE_TAG(0xD2, 64 * 8 + 4 + 4 - 11 * 8, 0x40)
write_dcd_cmd:    .word MAKE_TAG(0xCC, 64 * 8 + 4 - 11 * 8, 4)
#endif
/*
0x400802D2 - Tag=0xD2, Len=64*8 + 4 + 4, Ver=0x40
0x040402CC  Tag=0xCC, Len=64*8 + 4, Param=4
*/

/*======================================
  DDR2 IOMUX configuration
  ======================================*/
MXC_DCD_ITEM(1, IOMUXC_BASE_ADDR + 0x554, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM3 */
MXC_DCD_ITEM(2, IOMUXC_BASE_ADDR + 0x558, 0x00200040)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3 */
MXC_DCD_ITEM(3, IOMUXC_BASE_ADDR + 0x560, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM2 */
MXC_DCD_ITEM(4, IOMUXC_BASE_ADDR + 0x564, 0x00200040)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT1 */
MXC_DCD_ITEM(5, IOMUXC_BASE_ADDR + 0x568, 0x00200040)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2 */
MXC_DCD_ITEM(6, IOMUXC_BASE_ADDR + 0x570, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_1 */
MXC_DCD_ITEM(7, IOMUXC_BASE_ADDR + 0x574, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS */
MXC_DCD_ITEM(8, IOMUXC_BASE_ADDR + 0x578, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK_0 */
MXC_DCD_ITEM(9, IOMUXC_BASE_ADDR + 0x57c, 0x00200040)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0 */
MXC_DCD_ITEM(10, IOMUXC_BASE_ADDR + 0x580, 0x00200040)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDODT0 */
MXC_DCD_ITEM(11, IOMUXC_BASE_ADDR + 0x584, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM0 */
MXC_DCD_ITEM(12, IOMUXC_BASE_ADDR + 0x588, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS */
MXC_DCD_ITEM(13, IOMUXC_BASE_ADDR + 0x590, 0x00200040)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1 */
MXC_DCD_ITEM(14, IOMUXC_BASE_ADDR + 0x594, 0x00200000)	/* IOMUXC_SW_PAD_CTL_PAD_DRAM_DQM1 */
MXC_DCD_ITEM(15, IOMUXC_BASE_ADDR + 0x6f0, 0x00200000)	/* IOMUXC_SW_PAD_CTL_GRP_ADDDS */
MXC_DCD_ITEM(16, IOMUXC_BASE_ADDR + 0x6f4, 0x00000200)	/* IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL */
MXC_DCD_ITEM(17, IOMUXC_BASE_ADDR + 0x6fc, 0x00000000)	/* IOMUXC_SW_PAD_CTL_GRP_DDRPKE */
MXC_DCD_ITEM(18, IOMUXC_BASE_ADDR + 0x714, 0x00000000)	/* IOMUXC_SW_PAD_CTL_GRP_DDRMODE - CMOS mode */
MXC_DCD_ITEM(19, IOMUXC_BASE_ADDR + 0x718, 0x00200000)	/* IOMUXC_SW_PAD_CTL_GRP_B0DS */
MXC_DCD_ITEM(20, IOMUXC_BASE_ADDR + 0x71c, 0x00200000)	/* IOMUXC_SW_PAD_CTL_GRP_B1DS */
MXC_DCD_ITEM(21, IOMUXC_BASE_ADDR + 0x720, 0x00200000)	/* IOMUXC_SW_PAD_CTL_GRP_CTLDS */
MXC_DCD_ITEM(22, IOMUXC_BASE_ADDR + 0x724, 0x06000000)	/* IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE - DDR_SEL0= */
MXC_DCD_ITEM(23, IOMUXC_BASE_ADDR + 0x728, 0x00200000)	/* IOMUXC_SW_PAD_CTL_GRP_B2DS */
MXC_DCD_ITEM(24, IOMUXC_BASE_ADDR + 0x72c, 0x00200000)	/* IOMUXC_SW_PAD_CTL_GRP_B3DS */

/* ======================================
   Initialize DDR2 memory
   ======================================*/
MXC_DCD_ITEM(25, ESDCTL_BASE_ADDR + 0x088, 0x2d313331)	/* ESDCTL.RDDLCTL */
MXC_DCD_ITEM(26, ESDCTL_BASE_ADDR + 0x090, 0x393b3836)	/* ESDCTL.WRDLCTL */
MXC_DCD_ITEM(27, ESDCTL_BASE_ADDR + 0x0f8, 0x00000800)	/* ESDCTL.MUR */
MXC_DCD_ITEM(28, ESDCTL_BASE_ADDR + 0x07c, 0x020c0211)	/* ESDCTL.DGCTRL0 */
MXC_DCD_ITEM(29, ESDCTL_BASE_ADDR + 0x080, 0x014c0155)	/* ESDCTL.DGCTRL1 */

/* ESDCTL Miscellaneous Register
   ONE_CS 	= 0b0	-- Two chip select are used, 1 GB per chip select.
   BI_ON 	= 0b1 	-- Banks are interleaved, and address is decoded as row-bank-column
   MIF3_MODE 	= 0b11
   RALAT 	= 0b011 -- 3 cycles additional latency
   DDR_4_BANK 	= 0b0	-- 8 banks device is being used
   DDR_TYPE 	= 0b10	-- DDR2 device is used. (Default) */
MXC_DCD_ITEM(30, ESDCTL_BASE_ADDR + 0x018, 0x000016d0)	/* ESDMISC */

/* ESDCTL Control Register
   SDE_0		= 0b1	-- Enhanced SDRAM Controller Enabled
   SDE_1		= 0b1	-- Enhanced SDRAM Controller Enabled
   ROW 		= 0b100	-- Row Address Width. 15 bits Row
   ROW 		= 0b010	-- Row Address Width. 13 bits Row (for 128MB variants)
   COL 		= 0b001	-- Column Address Width. 10 bits Column
   BL		= 0b0	-- Burst Length = 4
   DSIZ		= 0b1	-- SDRAM Memory Data Width = 32-bit memory width
		= 0b0	-- SDRAM Memory Data Width = 16-bit memory width (for 128MB variants) */
#if CONFIG_NR_DRAM_BANKS > 1
MXC_DCD_ITEM(31, ESDCTL_BASE_ADDR + 0x000, 0xc4110000)	/* ESDCTL */
#else
#ifdef CONFIG_CCIMX5_SDRAM_128MB
MXC_DCD_ITEM(31, ESDCTL_BASE_ADDR + 0x000, 0x82100000)	/* ESDCTL (16-bit memory) */
#else
MXC_DCD_ITEM(31, ESDCTL_BASE_ADDR + 0x000, 0x84110000)	/* ESDCTL */
#endif
#endif

/* ESDCTL Timing Configuration Register 0
   tRFC		= 0x4D	-- REF command to ACT.
   tXS		= 0x51	-- Exit self refresh to non READ command
   tXP		= 0x1	-- Exit precharge power down to any command
   tXPDLL	= 0x1	-- Exit active power down to read commands
   tFAW		= 0xD	-- Four Active Window (all banks).
   tCL		= 0x2	-- CAS Read Latency */
MXC_DCD_ITEM(32, ESDCTL_BASE_ADDR + 0x00c, 0x4d5122d2)	/* ESDCTL.ESDCFG0 */

/* ESDCTL Timing Configuration Register 1
   tRCD		= 0x4	-- ACT command to internal read or write delay time
   tRP		= 0x4 	-- PRE command period
   tRC		= 0x16	-- ACT to ACT or REF command period
   tRAS		= 0x11	-- ACT to PRE command period
   tRPA		= 0x1	-- Precharge-all command period
   tWR		= 0x5	-- WRITE recovery time
   tMRD		= 0x1	-- Mode Register Set command cycle
   tCWL		= 0x2	-- CAS Write Latency */
MXC_DCD_ITEM(33, ESDCTL_BASE_ADDR + 0x010, 0x92d18a22)	/* ESDCTL.ESDCFG1 */

/* ESDCTL Timing Configuration Register 2
   tDLLK/tXSRD	= 0xC7	-- Exit self refresh to read command
   tRTP		= 0x2	-- Internal READ command to PRECHARGE command delay
   tWTR		= 0x4	-- Internal WRITE to READ command delay
   tRRD		= 0x2	-- ACTIVE to ACTIVE command period */
MXC_DCD_ITEM(34, ESDCTL_BASE_ADDR + 0x014, 0x00c70092)	/* ESDCTL.ESDCFG2 */

/* Read / WRITE Command Delay
   tDAI		= 0x0	-- Field not relevant for DDR2
   RTW_SAME	= 0x2	-- Controls the cycles delay between Read to Write commands in same chip select
   WTR_DIFF	= 0x3	-- Controls the cycles delay between Write to Read commands in different chip select
   WTW_DIFF	= 0x6 	-- Controls the cycles delay between Write to Write commands in different chip select
   RTW_DIFF	= 0x2	-- Controls the cycles delay between Read to Write commands in different chip select
   RTR_DIFF	= 0x2 	-- Controls the cycles delay between Read to Read commands in different chip select */
MXC_DCD_ITEM(35, ESDCTL_BASE_ADDR + 0x02c, 0x000026d2)	/* ESDCTL.ESDRWD */

/* Out of Reset Delays
   tXPR		= 0x9F	-- DDR2: This value equals a fixed 400 ns by JEDEC
   SDE_to_RST	= 0x0	-- DDR2. Not relevant
   RST_to_CKE	= 0xE	-- DDR2: Time from SDE enable to CKE rise. (JEDEC value is 200 us) */
MXC_DCD_ITEM(36, ESDCTL_BASE_ADDR + 0x030, 0x009f000e)	/* ESDCTL.ESDOR */

/* ESDCTL ODT Timing Control Register
   tAOFPD	= 0x2	-- DDR2: ODT turn-off (power down mode) max value
   tAONPD	= 0x2	-- DDR2: ODT turn-on (power down mode) max value
   tANPD		= 0x2	-- DDR2: ODT to power down entry latency
   tAXPD		= 0x7	-- DDR2: ODT power down exit latency
   tODTLon/tAOND	= 0x2	-- ODT turn on latency
   tODT_off_idle	= 0x0	-- Idle period before turning memory ODT off */
MXC_DCD_ITEM(37, ESDCTL_BASE_ADDR + 0x008, 0x12272000)	/* ESDCTL.ESDOTC */

/* ESDCTL Power Down Control Register
   PRCT_1	= 0x0	-- Precharge Timer - Chip Select 1
   PRCT_0	= 0x0	-- Precharge Timer - Chip Select 0
   tCKE		= 0x3	-- CKE minimum pulse width
   PWDT_1	= 0x0	-- Power Down Timer - Chip Select 1
   PWDT_0	= 0x0	-- Power Down Timer - Chip Select 0
   SLOW_PD	= 0x0	-- DDR2: Slow active power-down.
   BOTH_CS_PD	= 0x0	--
   tCKSRX	= 0x2	-- Valid clock requirement before self-refresh exit
   tCKSRE	= 0x2	-- Valid clock requirement after self-refresh entry */
MXC_DCD_ITEM(38, ESDCTL_BASE_ADDR + 0x004, 0x00030012)	/* ESDCTL.ESDPDC */

/* SDRAM Special Command Register
   CMD_ADDR_MSB 	= 0x4	-- Address 8 LSB bits that match the issued command
   CMD_ADDR_LSB	= 0x0	-- Address 8 MSB bits that match the issued command
   CON_REQ	= 0x1 	-- Configuration request
   WL_EN		= 0x0	-- Write Level Enable
   CMD		= 0x1	-- Command - Precharge all command
   CMD_CS	= 0x0	-- Chip Select - to CS0
   CMD_BA	= 0x0	-- Bank Address - to bank 0 */
MXC_DCD_ITEM(39, ESDCTL_BASE_ADDR + 0x01c, 0x04008010)	/* ESDCTL.ESDSCR */

/* SDRAM Special Command Register
   CMD_ADDR_MSB 	= 0x0	-- Address 8 LSB bits that match the issued command
   CMD_ADDR_LSB	= 0x0	-- Address 8 MSB bits that match the issued command
   CON_REQ	= 0x1 	-- Configuration request
   WL_EN		= 0x0	-- Write Level Enable
   CMD		= 0x3	-- Command - Load Mode Register Command
   CMD_CS	= 0x0	-- Chip Select - to CS0
   CMD_BA	= 0x2	-- Bank Address - to bank 2 */
MXC_DCD_ITEM(40, ESDCTL_BASE_ADDR + 0x01c, 0x00008032)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(41, ESDCTL_BASE_ADDR + 0x01c, 0x00008033)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(42, ESDCTL_BASE_ADDR + 0x01c, 0x00008031)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(43, ESDCTL_BASE_ADDR + 0x01c, 0x0b5280b0)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(44, ESDCTL_BASE_ADDR + 0x01c, 0x04008010)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(45, ESDCTL_BASE_ADDR + 0x01c, 0x00008020)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(46, ESDCTL_BASE_ADDR + 0x01c, 0x00008020)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(47, ESDCTL_BASE_ADDR + 0x01c, 0x0a528030)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(48, ESDCTL_BASE_ADDR + 0x01c, 0x03c68031)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(49, ESDCTL_BASE_ADDR + 0x01c, 0x00448031)	/* ESDCTL.ESDSCR */

#if CONFIG_NR_DRAM_BANKS > 1
/* CS 1 Initialization */
MXC_DCD_ITEM(50, ESDCTL_BASE_ADDR + 0x01c, 0x04008018)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(51, ESDCTL_BASE_ADDR + 0x01c, 0x0000803a)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(52, ESDCTL_BASE_ADDR + 0x01c, 0x0000803b)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(53, ESDCTL_BASE_ADDR + 0x01c, 0x00008039)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(54, ESDCTL_BASE_ADDR + 0x01c, 0x0b528138)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(55, ESDCTL_BASE_ADDR + 0x01c, 0x04008018)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(56, ESDCTL_BASE_ADDR + 0x01c, 0x00008028)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(57, ESDCTL_BASE_ADDR + 0x01c, 0x00008028)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(58, ESDCTL_BASE_ADDR + 0x01c, 0x0a528038)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(59, ESDCTL_BASE_ADDR + 0x01c, 0x03c68039)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(60, ESDCTL_BASE_ADDR + 0x01c, 0x00448039)	/* ESDCTL.ESDSCR */
#endif
/* Refresh Control Register
   REF_CNT	= 0x0	-- Refresh Counter
   REF_SEL	= 0x1	-- Refresh Selector
   REFR		= 0x3	-- Refresh Rate
; START_REF	= 0x0	-- Start Refresh cycle */
MXC_DCD_ITEM(61, ESDCTL_BASE_ADDR + 0x020, 0x00005800)	/* ESDCTL.ESDREF */

/* ODT control register
   ODT3_INT_RES	= 0x3	-- On chip ODT byte3 resistor
   ODT2_INT_RES	= 0x3	-- On chip ODT byte2 resistor
   ODT1_INT_RES	= 0x3 	-- On chip ODT byte1 resistor
   ODT0_INT_RES	= 0x3	-- On chip ODT byte0 resistor
   ODT_RD_ACT_EN	= 0x0	-- Active read CS ODT enable
   ODT_RD_PAS_EN	= 0x1	-- Inactive read CS ODT enable.
   ODT_WR_ACT_EN	= 0x1	-- Active write CS ODT enable.
   ODT_WR_PAS_EN	= 0x1	-- Inactive write CS ODT enable */
MXC_DCD_ITEM(62, ESDCTL_BASE_ADDR + 0x058, 0x00022225)	/* ESDCTL.ODTCTRL */
MXC_DCD_ITEM(63, ESDCTL_BASE_ADDR + 0x01c, 0x00000000)	/* ESDCTL.ESDSCR */
MXC_DCD_ITEM(64, ESDCTL_BASE_ADDR + 0x040, 0x04b80003)

cmd_end1:
hdr_end:

#endif
