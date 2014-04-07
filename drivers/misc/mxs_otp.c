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
#include <asm/arch/regs-clkctrl-mx28.h>
#include <asm/arch/regs-digctl.h>
#include <asm/arch/regs-ocotp.h>
#include <asm/arch/regs-power-mx28.h>

#ifdef CONFIG_MXS_OTP

#define MXS_OCOTP_MAX_TIMEOUT	1000000

/* Global vars */
unsigned int original_power_vddioctrl;

/* Functions */
extern int mxs_wait_mask_clr(struct mxs_register_32 *reg,
			     uint32_t mask, unsigned int timeout);

int open_otp_bank(void)
{
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;

	writel(OCOTP_CTRL_RD_BANK_OPEN, &ocotp_regs->hw_ocotp_ctrl_set);
	if (mxs_wait_mask_clr(&ocotp_regs->hw_ocotp_ctrl_reg, OCOTP_CTRL_BUSY,
				MXS_OCOTP_MAX_TIMEOUT)) {
		printf("MXS OTP: Can't open OCOTP bank\n");
		return 1;
	}

	return 0;
}

void close_otp_bank(void)
{
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;

	writel(OCOTP_CTRL_RD_BANK_OPEN, &ocotp_regs->hw_ocotp_ctrl_clr);
}

void dump_otp_regs(void)
{
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;
	int i;

	printf("HW_OTP_CUSTn\n");
	for (i=0; i < 4; i++)
		printf("  HW_OTP_CUST%d: 0x%08x\n", i,
			readl(&ocotp_regs->hw_ocotp_cust0_reg + i));
	printf("HW_OTP_CRYPTOn\n");
	for (i=0; i < 4; i++)
		printf("  HW_OTP_CRYPTO%d: 0x%08x\n", i,
			readl(&ocotp_regs->hw_ocotp_crypto0_reg + i));
	printf("HW_OCOTP_HWCAPn\n");
	for (i=0; i < 6; i++)
		printf("  HW_OCOTP_HWCAPn%d: 0x%08x\n", i,
			readl(&ocotp_regs->hw_ocotp_hwcap0_reg + i));
	printf("HW_OCOTP_SWCAP: 0x%08x\n",
		readl(&ocotp_regs->hw_ocotp_swcap_reg));
	printf("HW_OCOTP_CUSTCAP: 0x%08x\n",
		readl(&ocotp_regs->hw_ocotp_custcap_reg));
	printf("HW_OCOTP_LOCK: 0x%08x\n",
		readl(&ocotp_regs->hw_ocotp_lock_reg));
	printf("HW_OCOTP_OPSn\n");
	for (i=0; i < 7; i++)
		printf("  HW_OCOTP_OPSn%d: 0x%08x\n", i,
			readl(&ocotp_regs->hw_ocotp_ops0_reg + i));
	printf("HW_OCOTP_ROMn\n");
	for (i=0; i < 8; i++)
		printf("  HW_OCOTP_ROMn%d: 0x%08x\n", i,
			readl(&ocotp_regs->hw_ocotp_rom0_reg + i));
	printf("HW_OCOTP_SRKn\n");
	for (i=0; i < 8; i++)
		printf("  HW_OCOTP_SRKn%d: 0x%08x\n", i,
			readl(&ocotp_regs->hw_ocotp_srk0_reg + i));
}

unsigned int read_otp_reg(unsigned int addr)
{
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;

	return(readl(&ocotp_regs->hw_ocotp_cust0_reg + addr));
}

static void exit_otp_blow(void)
{
	struct mxs_power_regs *power_regs =
		(struct mxs_power_regs *)MXS_POWER_BASE;
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;

	/* Restore VDDIO voltage */
	writel(original_power_vddioctrl, &power_regs->hw_power_vddioctrl);
	/* Restore HCLK frequency by clearing CPU bypass */
	writel(CLKCTRL_CLKSEQ_BYPASS_CPU, &clkctrl_regs->hw_clkctrl_clkseq_clr);
}

static int prepare_otp_blow(void)
{
	struct mxs_power_regs *power_regs =
		(struct mxs_power_regs *)MXS_POWER_BASE;
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;

	/* Lower HCLK frequency to 24MHz by activating
	 * CPU clock bypass */
	writel(CLKCTRL_CLKSEQ_BYPASS_CPU, &clkctrl_regs->hw_clkctrl_clkseq_set);

	/* Set VDDIO voltage to 2.8V by clearing TRG */
	original_power_vddioctrl = readl(&power_regs->hw_power_vddioctrl);
	writel(original_power_vddioctrl & ~POWER_VDDIOCTRL_TRG_MASK,
	       &power_regs->hw_power_vddioctrl);

	/* Check that HW_OCOTP_CTRL_BUSY and HW_OCOTP_CTRL_ERROR are clear */
	if (mxs_wait_mask_clr(&ocotp_regs->hw_ocotp_ctrl_reg,
			      OCOTP_CTRL_ERROR | OCOTP_CTRL_BUSY,
			      MXS_OCOTP_MAX_TIMEOUT)) {
		printf("MXS OTP: failed to prepare OTP bank\n");
		exit_otp_blow();
		return 1;
	}

	return 0;
}

static int write_otp_data(unsigned int addr, unsigned int data)
{
	struct mxs_ocotp_regs *ocotp_regs =
		(struct mxs_ocotp_regs *)MXS_OCOTP_BASE;
	struct mxs_digctl_regs *digctl_regs =
		(struct mxs_digctl_regs *)MXS_DIGCTL_BASE;
	unsigned int start_usec, end_usec;

	/* Write requested address and unlock code */
	writel(OCOTP_CTRL_WR_UNLOCK_MASK | OCOTP_CTRL_ADDR_MASK,
	       &ocotp_regs->hw_ocotp_ctrl_clr);
	writel(OCOTP_CTRL_WR_UNLOCK_KEY | (addr << OCOTP_CTRL_ADDR_OFFSET),
	       &ocotp_regs->hw_ocotp_ctrl_set);

	/* Program the blow OTP data */
#ifdef CONFIG_EMULATE_OTP_BLOW
	printf("OTP blow emulation activated\n");
	printf("Emulating OTP blow of data 0x%08x at addr 0x%02x\n", data, addr);
	data = 0;
#endif
	writel(data, &ocotp_regs->hw_ocotp_data);

	/* Wait for BUSY to be cleared by controller */
	if (mxs_wait_mask_clr(&ocotp_regs->hw_ocotp_ctrl_reg, OCOTP_CTRL_BUSY,
				MXS_OCOTP_MAX_TIMEOUT)) {
		printf("MXS OTP: Failed to blow OCOTP bank\n");
		return 1;
	}

	/* Wait 2 usec write postamble before allowing any further OTP access */
	start_usec = readl(&digctl_regs->hw_digctl_microseconds_reg);
	do {
		end_usec = readl(&digctl_regs->hw_digctl_microseconds_reg);
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

enum {
	OTPREG_ADDR_CUST0,
	OTPREG_ADDR_CUST1,
	OTPREG_ADDR_CUST2,
	OTPREG_ADDR_CUST3,
	OTPREG_ADDR_CRYPTO0,
	OTPREG_ADDR_CRYPTO1,
	OTPREG_ADDR_CRYPTO2,
	OTPREG_ADDR_CRYPTO3,
	OTPREG_ADDR_HWCAP0,
	OTPREG_ADDR_HWCAP1,
	OTPREG_ADDR_HWCAP2,
	OTPREG_ADDR_HWCAP3,
	OTPREG_ADDR_HWCAP4,
	OTPREG_ADDR_HWCAP5,
	OTPREG_ADDR_SWCAP,
	OTPREG_ADDR_CUSTCAP,
	OTPREG_ADDR_LOCK,
	OTPREG_ADDR_OPS0,
	OTPREG_ADDR_OPS1,
	OTPREG_ADDR_OPS2,
	OTPREG_ADDR_OPS3,
	OTPREG_ADDR_OPS4,
	OTPREG_ADDR_OPS5,
	OTPREG_ADDR_OPS6,
	OTPREG_ADDR_ROM0,
	OTPREG_ADDR_ROM1,
	OTPREG_ADDR_ROM2,
	OTPREG_ADDR_ROM3,
	OTPREG_ADDR_ROM4,
	OTPREG_ADDR_ROM5,
	OTPREG_ADDR_ROM6,
	OTPREG_ADDR_ROM7,
	OTPREG_ADDR_SRK0,
	OTPREG_ADDR_SRK1,
	OTPREG_ADDR_SRK2,
	OTPREG_ADDR_SRK3,
	OTPREG_ADDR_SRK4,
	OTPREG_ADDR_SRK5,
	OTPREG_ADDR_SRK6,
	OTPREG_ADDR_SRK7,
};

int lock_otp_reg(unsigned int addr)
{
	unsigned int mask;

	if (addr <= OTPREG_ADDR_CUST3)
		mask = OCOTP_LOCK_CUST0 << addr;
	else if (addr >= OTPREG_ADDR_CRYPTO0 && addr <= OTPREG_ADDR_CRYPTO3)
		mask = OCOTP_LOCK_CRYPTOKEY;
	else if (addr >= OTPREG_ADDR_HWCAP0 && addr <= OTPREG_ADDR_SWCAP)
		mask = OCOTP_LOCK_HWSW;
	else if (addr == OTPREG_ADDR_CUSTCAP)
		mask = OCOTP_LOCK_CUSTCAP;
	else if (addr >= OTPREG_ADDR_OPS0 && addr <= OTPREG_ADDR_OPS3)
		mask = OCOTP_LOCK_OPS;
	else if (addr == OTPREG_ADDR_OPS4)
		mask = OCOTP_LOCK_UN0;
	else if (addr == OTPREG_ADDR_OPS5)
		mask = OCOTP_LOCK_UN1;
	else if (addr == OTPREG_ADDR_OPS6)
		mask = OCOTP_LOCK_UN2;
	else if (addr >= OTPREG_ADDR_ROM0 && addr <= OTPREG_ADDR_ROM7)
		mask = OCOTP_LOCK_ROM0 << (addr - OTPREG_ADDR_ROM0);
	else if (addr >= OTPREG_ADDR_SRK0 && addr <= OTPREG_ADDR_SRK7)
		mask = OCOTP_LOCK_SRK;
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
