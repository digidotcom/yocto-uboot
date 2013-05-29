/*
 * cmd_mxsotp.c - interface to Freescale's iMX One-Time-Programmable memory
 *
 * Copyright (c) 2012 Digi International Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/mxs_otp.h>
#include <asm/arch/regs-ocotp.h>
#include <asm/arch/regs-clkctrl.h>
#include <asm/arch/regs-power.h>

#ifdef CONFIG_MXS_OTP

#define HW_DIGCTL_MICROSECONDS	0x0c0

/* Global vars */
unsigned int original_power_vddioctrl;

/* Functions */
int open_otp_bank(void)
{
	unsigned long retries = 100000;

	REG_SET(REGS_OCOTP_BASE, HW_OCOTP_CTRL, BM_OCOTP_CTRL_RD_BANK_OPEN);
	while((BM_OCOTP_CTRL_BUSY & REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CTRL)) &&
	      retries--)
		udelay(10);

	if (retries)
		return 0;
	else
		return 1;
}

void close_otp_bank(void)
{
	REG_CLR(REGS_OCOTP_BASE, HW_OCOTP_CTRL, BM_OCOTP_CTRL_RD_BANK_OPEN);
}

void dump_otp_regs(void)
{
	int i;

	printf("HW_OTP_CUSTn\n");
	for (i=0; i < 4; i++)
		printf("  HW_OTP_CUST%d: 0x%08x\n", i,
			REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CUSTn(i)));
	printf("HW_OTP_CRYPTOn\n");
	for (i=0; i < 4; i++)
		printf("  HW_OTP_CRYPTO%d: 0x%08x\n", i,
			REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CRYPTOn(i)));
	printf("HW_OCOTP_HWCAPn\n");
	for (i=0; i < 6; i++)
		printf("  HW_OCOTP_HWCAPn%d: 0x%08x\n", i,
			REG_RD(REGS_OCOTP_BASE, HW_OCOTP_HWCAPn(i)));
	printf("HW_OCOTP_SWCAP: 0x%08x\n",
		REG_RD(REGS_OCOTP_BASE, HW_OCOTP_SWCAP));
	printf("HW_OCOTP_CUSTCAP: 0x%08x\n",
		REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CUSTCAP));
	printf("HW_OCOTP_LOCK: 0x%08x\n",
		REG_RD(REGS_OCOTP_BASE, HW_OCOTP_LOCK));
	printf("HW_OCOTP_OPSn\n");
	for (i=0; i < 7; i++)
		printf("  HW_OCOTP_OPSn%d: 0x%08x\n", i,
			REG_RD(REGS_OCOTP_BASE, HW_OCOTP_OPSn(i)));
	printf("HW_OCOTP_ROMn\n");
	for (i=0; i < 8; i++)
		printf("  HW_OCOTP_ROMn%d: 0x%08x\n", i,
			REG_RD(REGS_OCOTP_BASE, HW_OCOTP_ROMn(i)));
	printf("HW_OCOTP_SRKn\n");
	for (i=0; i < 8; i++)
		printf("  HW_OCOTP_SRKn%d: 0x%08x\n", i,
			REG_RD(REGS_OCOTP_BASE, HW_OCOTP_SRKn(i)));
}

unsigned int read_otp_reg(unsigned int addr)
{
	/* Each register address is separated 16 bytes from the next one */
	return(REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CUSTn(0) + (16 * addr)));
}

static void exit_otp_blow(void)
{
	/* Restore VDDIO voltage */
	REG_WR(REGS_POWER_BASE, HW_POWER_VDDIOCTRL, original_power_vddioctrl);
	/* Restore HCLK frequency by clearing CPU bypass */
	REG_CLR(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ, BM_CLKCTRL_CLKSEQ_BYPASS_CPU);
}

static int prepare_otp_blow(void)
{
	unsigned int retries = 1000;
	unsigned int ocotp_ctrl;

	/* Lower HCLK frequency to 24MHz by activating
	 * CPU clock bypass */
	REG_SET(REGS_CLKCTRL_BASE, HW_CLKCTRL_CLKSEQ, BM_CLKCTRL_CLKSEQ_BYPASS_CPU);

	/* Set VDDIO voltage to 2.8V by clearing TRG */
	original_power_vddioctrl = REG_RD(REGS_POWER_BASE, HW_POWER_VDDIOCTRL);
	REG_WR(REGS_POWER_BASE, HW_POWER_VDDIOCTRL, original_power_vddioctrl & ~BM_POWER_VDDIOCTRL_TRG);

	/* Check that HW_OCOTP_CTRL_BUSY and HW_OCOTP_CTRL_ERROR are clear */
	while(((ocotp_ctrl = REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CTRL)) &
	      (BM_OCOTP_CTRL_ERROR | BM_OCOTP_CTRL_BUSY)) && retries--)
		udelay(100);
	if (!retries) {
		exit_otp_blow();
		return -1;
	}

	return 0;
}

static int write_otp_data(unsigned int addr, unsigned int data)
{
	unsigned int ocotp_ctrl;
	unsigned int retries = 1000;
	unsigned int start_usec, end_usec;

	/* Write requested address and unlock code */
	ocotp_ctrl = REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CTRL);
	ocotp_ctrl &= ~(BM_OCOTP_CTRL_WR_UNLOCK | BM_OCOTP_CTRL_ADDR);
	ocotp_ctrl |= BF_OCOTP_CTRL_WR_UNLOCK(BV_OCOTP_CTRL_WR_UNLOCK__KEY);
	ocotp_ctrl |= BF_OCOTP_CTRL_ADDR(addr);
	REG_WR(REGS_OCOTP_BASE, HW_OCOTP_CTRL, ocotp_ctrl);

	/* Program the blow OTP data */
#ifdef CONFIG_EMULATE_OTP_BLOW
	printf("OTP blow emulation activated\n");
	printf("Emulating OTP blow of data 0x%08x at addr 0x%02x\n", data, addr);
	data = 0;
#endif
	REG_WR(REGS_OCOTP_BASE, HW_OCOTP_DATA, data);

	/* Wait for BUSY to be cleared by controller */
	while(((ocotp_ctrl = REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CTRL)) &
	      BM_OCOTP_CTRL_BUSY) && retries--)
		udelay(100);
	if (!retries)
		return -1;

	/* Wait 2 usec write postamble before allowing any further OTP access */
	start_usec = REG_RD(REGS_DIGCTL_BASE, HW_DIGCTL_MICROSECONDS);
	do {
		end_usec = REG_RD(REGS_DIGCTL_BASE, HW_DIGCTL_MICROSECONDS);
	}while(end_usec - start_usec < 2);

	return 0;
}

int blow_otp_reg(unsigned int addr, unsigned int data)
{
	int ret = -1;

	if (!prepare_otp_blow()) {
		/* Write OTP data */
		ret = write_otp_data(addr, data);
		/* Exit otp blow process */
		exit_otp_blow();
	}

	return ret;
}

int lock_otp_reg(unsigned int addr)
{
	unsigned int mask;

	if (addr <= OTPREG_ADDR_CUST3)
		mask = BM_OCOTP_LOCK_CUST0 << addr;
	else if (addr >= OTPREG_ADDR_CRYPTO0 && addr <= OTPREG_ADDR_CRYPTO3)
		mask = BM_OCOTP_LOCK_CRYPTOKEY;
	else if (addr >= OTPREG_ADDR_HWCAP0 && addr <= OTPREG_ADDR_SWCAP)
		mask = BM_OCOTP_LOCK_HWSW;
	else if (addr == OTPREG_ADDR_CUSTCAP)
		mask = BM_OCOTP_LOCK_CUSTCAP;
	else if (addr >= OTPREG_ADDR_OPS0 && addr <= OTPREG_ADDR_OPS3)
		mask = BM_OCOTP_LOCK_OPS;
	else if (addr == OTPREG_ADDR_OPS4)
		mask = BM_OCOTP_LOCK_UN0;
	else if (addr == OTPREG_ADDR_OPS5)
		mask = BM_OCOTP_LOCK_UN1;
	else if (addr == OTPREG_ADDR_OPS6)
		mask = BM_OCOTP_LOCK_UN2;
	else if (addr >= OTPREG_ADDR_ROM0 && addr <= OTPREG_ADDR_ROM7)
		mask = BM_OCOTP_LOCK_ROM0 << (addr - OTPREG_ADDR_ROM0);
	else if (addr >= OTPREG_ADDR_SRK0 && addr <= OTPREG_ADDR_SRK7)
		mask = BM_OCOTP_LOCK_SRK;
	else
		return -1;

	return (blow_otp_reg(OTPREG_ADDR_LOCK, mask));
}

#ifdef CONFIG_PLATFORM_HAS_HWID
int blow_otp_hwid(unsigned char *hwid)
{
	/*
	 *       | 31..           HWOTP_CUST1            ..0 | 31..          HWOTP_CUST0             ..0 |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * HWID: |    --    | TF (loc) | variant  | HV |Cert |   Year   | Mon |     Serial Number        |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * Byte: 0          1          2          3          4          5          6          7
	 */
	u32 cust0, cust1;
	int ret = -1;

	cust1 = (hwid[0] << 24) | (hwid[1] << 16) | (hwid[2] << 8) | hwid[3];
	cust0 = (hwid[4] << 24) | (hwid[5] << 16) | (hwid[6] << 8) | hwid[7];

	if (!prepare_otp_blow()) {
		/* Write CUST0 */
		ret = write_otp_data(OTPREG_ADDR_CUST0, cust0);
		if (0 == ret) {
			/* Write CUST1 */
			ret = write_otp_data(OTPREG_ADDR_CUST1, cust1);
		}
		/* Exit otp blow process */
		exit_otp_blow();
	}

	return ret;
}

int read_otp_hwid(unsigned char *hwid)
{
	unsigned int cust0, cust1;

	if (open_otp_bank())
		return -1;

	cust0 = read_otp_reg(OTPREG_ADDR_CUST0);
	cust1 = read_otp_reg(OTPREG_ADDR_CUST1);
	close_otp_bank();

	/* Compose hwid */
	hwid[0] = (cust1 >> 24) & 0xff;
	hwid[1] = (cust1 >> 16) & 0xff;
	hwid[2] = (cust1 >> 8) & 0xff;
	hwid[3] = cust1 & 0xff;
	hwid[4] = (cust0 >> 24) & 0xff;
	hwid[5] = (cust0 >> 16) & 0xff;
	hwid[6] = (cust0 >> 8) & 0xff;
	hwid[7] = cust0 & 0xff;

	return 0;
}
#endif /* CONFIG_PLATFORM_HAS_HWID */

#endif /* CONFIG_MXS_OTP */
