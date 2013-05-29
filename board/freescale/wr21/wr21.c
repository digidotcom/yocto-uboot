/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/regs-pinctrl.h>
#include <asm/arch/pinctrl.h>
#include <asm/arch/regs-clkctrl.h>
#include <asm/arch/regs-enet.h>
#include <asm/arch/regs-ocotp.h>
#include <asm/arch/regs-uartdbg.h>
#include <asm/arch/mx28.h>
#include <asm/arch/regs-rtc.h>
#include <asm/io.h>
#include <nvram.h>
#include <mxs-persistent.h>

#include <mmc.h>
#include <imx_ssp_mmc.h>
#include <serial.h>

/* This should be removed after it's added into mach-types.h */
#ifndef MACH_TYPE_WR21
#define MACH_TYPE_WR21	3737
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_FEC1_ENABLED
int fec_get_mac_addr(unsigned int index, unsigned char *mac);
#else
int fec_get_mac_addr(unsigned char *mac);
#endif

#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
extern unsigned long wdt_enable(unsigned long value);
#endif

extern unsigned int get_total_ram(void);

/* App UART pins */
/* AUART0 pins */
static struct pin_desc auart_pins_desc_0[] = {
	{ PINID_AUART0_RX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_AUART0_TX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
};
/* AUART1 pins */
static struct pin_desc auart_pins_desc_1[] = {
	{ PINID_AUART1_RX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_AUART1_TX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
};
/* AUART2 pins */
static struct pin_desc auart_pins_desc_2[] = {
	{ PINID_AUART2_RX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_AUART2_TX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
};
/* AUART3 pins */
static struct pin_desc auart_pins_desc_3[] = {
	{ PINID_AUART3_RX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_AUART3_TX, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
};
/* AUART4 pins */
static struct pin_desc auart_pins_desc_4[] = {
	{ PINID_SAIF0_BITCLK, PIN_FUN3, PAD_4MA, PAD_3V3, 0 },
	{ PINID_SAIF0_SDATA0, PIN_FUN3, PAD_4MA, PAD_3V3, 0 },
};

#ifdef CONFIG_IMX_SSP_MMC
/* MMC pins */
static struct pin_desc mmc0_pins_desc[] = {
	{ PINID_SSP0_DATA0, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA1, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA2, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA3, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA4, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA5, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA6, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA7, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_CMD, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DETECT, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_SCK, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
};

static struct pin_desc mmc1_pins_desc[] = {
	{ PINID_GPMI_D00, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D01, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D02, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D03, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D04, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D05, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D06, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_D07, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_RDY1, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_RDY0, PIN_FUN2, PAD_8MA, PAD_3V3, 1 },
	{ PINID_GPMI_WRN, PIN_FUN2, PAD_8MA, PAD_3V3, 1 }
};

static struct pin_group mmc0_pins = {
	.pins		= mmc0_pins_desc,
	.nr_pins	= ARRAY_SIZE(mmc0_pins_desc)
};

static struct pin_group mmc1_pins = {
	.pins		= mmc1_pins_desc,
	.nr_pins	= ARRAY_SIZE(mmc1_pins_desc)
};

struct imx_ssp_mmc_cfg ssp_mmc_cfg[2] = {
	{REGS_SSP0_BASE, HW_CLKCTRL_SSP0, BM_CLKCTRL_CLKSEQ_BYPASS_SSP0},
	{REGS_SSP1_BASE, HW_CLKCTRL_SSP1, BM_CLKCTRL_CLKSEQ_BYPASS_SSP1},
};
#endif

/* ENET pins */
static struct pin_desc enet_pins_desc[] = {
	{ PINID_ENET0_MDC, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_MDIO, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_RX_EN, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_RXD0, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_RXD1, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_TX_EN, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_TXD0, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET0_TXD1, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_ENET_CLK, PIN_FUN1, PAD_8MA, PAD_3V3, 1 }
};

#ifdef CONFIG_NAND_GPMI
/* Gpmi pins */
static struct pin_desc gpmi_pins_desc[] = {
	{ PINID_GPMI_D00, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D01, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D02, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D03, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D04, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D05, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D06, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_D07, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_RDN, PIN_FUN1, PAD_8MA, PAD_3V3, 0 },
	{ PINID_GPMI_WRN, PIN_FUN1, PAD_8MA, PAD_3V3, 0 },
	{ PINID_GPMI_ALE, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_CLE, PIN_FUN1, PAD_4MA, PAD_3V3, 0 },
	{ PINID_GPMI_RDY0, PIN_FUN1, PAD_4MA, PAD_3V3, 1 },
	{ PINID_GPMI_RDY1, PIN_FUN1, PAD_4MA, PAD_3V3, 1 },
	{ PINID_GPMI_CE0N, PIN_FUN1, PAD_4MA, PAD_3V3, 1 },
	{ PINID_GPMI_CE1N, PIN_FUN1, PAD_4MA, PAD_3V3, 1 },
	{ PINID_GPMI_RESETN, PIN_FUN1, PAD_4MA, PAD_3V3, 0 }
};
#endif

static struct pin_group enet_pins = {
	.pins		= enet_pins_desc,
	.nr_pins	= ARRAY_SIZE(enet_pins_desc)
};

#ifdef CONFIG_NAND_GPMI
static struct pin_group gpmi_pins = {
	.pins		= gpmi_pins_desc,
	.nr_pins	= ARRAY_SIZE(gpmi_pins_desc)
};
#endif

/*
 * Functions
 */
#ifdef CONFIG_NAND_GPMI
void setup_gpmi_nand(void)
{
	/* Set up GPMI pins */
	pin_set_group(&gpmi_pins);
}
#endif

#if defined(CONFIG_SILENT_CONSOLE) && defined(ENABLE_CONSOLE_GPIO)
static void init_console_gpio(void)
{
	/* set ENABLE_CONSOLE_GPIO as GPIO output, high, keeper enabled
	 * to simulate pull-up */
	pin_set_type(ENABLE_CONSOLE_GPIO, PIN_GPIO);
	pin_gpio_direction(ENABLE_CONSOLE_GPIO, 1);
	pin_set_pullup(ENABLE_CONSOLE_GPIO, 0);
	pin_gpio_set(ENABLE_CONSOLE_GPIO, 1);

	/* now reconfigure as input to act as enable console */
	pin_gpio_direction(ENABLE_CONSOLE_GPIO, 0);
}

int gpio_enable_console(void)
{
	return (pin_gpio_get(ENABLE_CONSOLE_GPIO) == CONSOLE_ENABLE_GPIO_STATE);
}
#else
int gpio_enable_console(void) { return 0; }
#endif /* CONFIG_SILENT_CONSOLE && ENABLE_CONSOLE_GPIO */

#ifdef CONFIG_GET_CONS_FROM_CONS_VAR
struct serial_device *default_serial_console(void)
{
	/* WARNING: Do not try to print debug messages in this
	 * function or in functions called from this one, or
	 * you will enter an endless loop */
	int conidx = CONFIG_CONS_INDEX;
	char *s, *k;
	static int previous_auart = 0;	/* flag set if previous default console was AUART */

	if ((s = getenv("console")) != NULL) {
		if ((k = strstr(s, CONFIG_UARTDBG_CONSOLE)) != NULL) {
			if (previous_auart) {
				serial_mxs_duart_device.init();
				previous_auart = 0;
			}
			/* There is only one DUART */
			return &serial_mxs_duart_device;
		}
		else if ((k = strstr(s, CONFIG_UARTAPP_CONSOLE)) != NULL) {
			/* Index is only one digit... */
			conidx = k[strlen(CONFIG_UARTAPP_CONSOLE)] - '0';
			if (conidx < 0 || conidx >= MXS_MAX_AUARTS)
				conidx = CONFIG_CONS_INDEX;

			if (!previous_auart || conidx != CONFIG_CONS_INDEX) {
				serial_mxs_auart_devices[conidx].init();
			}

			previous_auart = 1;
			return &serial_mxs_auart_devices[conidx];
		}
	}

	/* If no console init yet, use default port */
#if defined(CONFIG_DEFAULT_SERIAL_AUART)
	previous_auart = 1;
	return &serial_mxs_auart_devices[conidx];
#else
	return &serial_mxs_duart_device;
#endif

	return NULL;
}
#endif

void mxs_duart_gpios_init(void)
{
	/* DUART0 RXD line is LOW level by default. For some reason this
	 * makes the UART shift data into the RX FIFO and clear the RX FIFO
	 * EMPTY flag, thus stopping U-Boot auto-boot if there is no serial
	 * cable connected that pulls the line up.
	 * To prevent this effect we force the level of DUART RXD gpio to be high
	 * by setting it as GPIO output high, with keeper enabled, and then
	 * configuring it back as DUART RXD.
	 */

	/* set port(3,24) as GPIO, output, high, keeper enabled */
	pin_set_type(PINID_I2C0_SCL, PIN_GPIO);
	pin_gpio_direction(PINID_I2C0_SCL, 1);
	pin_gpio_set(PINID_I2C0_SCL, 1);
	pin_set_pullup(PINID_I2C0_SCL, 0);

	/* now set it back as DUART0 RXD function */
	pin_set_type(PINID_I2C0_SCL, PIN_FUN3);
}

#define AUART_GPIO_PIN_GROUP(port)					\
	{								\
		.pins		= auart_pins_desc_##port,		\
		.nr_pins	= ARRAY_SIZE(auart_pins_desc_##port),	\
	}
static struct pin_group auart_pins[MXS_MAX_AUARTS] = {
	AUART_GPIO_PIN_GROUP(0),
	AUART_GPIO_PIN_GROUP(1),
	AUART_GPIO_PIN_GROUP(2),
	AUART_GPIO_PIN_GROUP(3),
	AUART_GPIO_PIN_GROUP(4),
};

void mxs_auart_gpios_init(int port)
{
	if (port >= 0 && port < MXS_MAX_AUARTS)
		pin_set_group(&auart_pins[port]);
}

int board_init(void)
{
	/* Will change it for MX28 EVK later */
	gd->bd->bi_arch_number = MACH_TYPE_WR21;
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#if defined(CONFIG_SILENT_CONSOLE) && defined(ENABLE_CONSOLE_GPIO)
	init_console_gpio();
#endif

#ifdef CONFIG_NAND_GPMI
	setup_gpmi_nand();
#endif
	return 0;
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

#ifdef CONFIG_IMX_SSP_MMC

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	unsigned long global_boot_mode;

	global_boot_mode = REG_RD_ADDR(GLOBAL_BOOT_MODE_ADDR);
	return ((global_boot_mode & 0xf) == BOOT_MODE_SD1) ? 1 : 0;
}
#endif

#define PINID_SSP0_GPIO_WP PINID_SSP1_SCK
#define PINID_SSP1_GPIO_WP PINID_GPMI_RESETN

u32 ssp_mmc_is_wp(struct mmc *mmc)
{
	return (mmc->block_dev.dev == 0) ?
		pin_gpio_get(PINID_SSP0_GPIO_WP) :
		pin_gpio_get(PINID_SSP1_GPIO_WP);
}

int ssp_mmc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

	for (index = 0; index < CONFIG_SYS_SSP_MMC_NUM;
		++index) {
		switch (index) {
		case 0:
			/* Set up MMC pins */
			pin_set_group(&mmc0_pins);

			/* Power on the card slot 0 */
			pin_set_type(PINID_PWM3, PIN_GPIO);
			pin_gpio_direction(PINID_PWM3, 1);
			pin_gpio_set(PINID_PWM3, 0);

			/* Wait 10 ms for card ramping up */
			udelay(10000);

			/* Set up SD0 WP pin */
			pin_set_type(PINID_SSP0_GPIO_WP, PIN_GPIO);
			pin_gpio_direction(PINID_SSP0_GPIO_WP, 0);

			break;
		case 1:
#ifdef CONFIG_CMD_MMC
			/* Set up MMC pins */
			pin_set_group(&mmc1_pins);

			/* Power on the card slot 1 */
			pin_set_type(PINID_PWM4, PIN_GPIO);
			pin_gpio_direction(PINID_PWM4, 1);
			pin_gpio_set(PINID_PWM4, 0);

			/* Wait 10 ms for card ramping up */
			udelay(10000);

			/* Set up SD1 WP pin */
			pin_set_type(PINID_SSP1_GPIO_WP, PIN_GPIO);
			pin_gpio_direction(PINID_SSP1_GPIO_WP, 0);
#endif
			break;
		default:
			printf("Warning: you configured more ssp mmc controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_SSP_MMC_NUM);
			return status;
		}
		status |= imx_ssp_mmc_initialize(bis, &ssp_mmc_cfg[index]);
	}

	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!ssp_mmc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

#ifdef CONFIG_MXC_FEC
#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#ifdef CONFIG_FEC1_ENABLED
int fec_get_mac_addr(unsigned int, unsigned char *mac)
#else
int fec_get_mac_addr(unsigned char *mac)
#endif
{
	u32 val;

	/*set this bit to open the OTP banks for reading*/
	REG_WR(REGS_OCOTP_BASE, HW_OCOTP_CTRL_SET,
		BM_OCOTP_CTRL_RD_BANK_OPEN);

	/*wait until OTP contents are readable*/
	while (BM_OCOTP_CTRL_BUSY & REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CTRL))
		udelay(100);

	mac[0] = 0x00;
	mac[1] = 0x04;
	val = REG_RD(REGS_OCOTP_BASE, HW_OCOTP_CUSTn(0));
	mac[2] = (val >> 24) & 0xFF;
	mac[3] = (val >> 16) & 0xFF;
	mac[4] = (val >> 8) & 0xFF;
	mac[5] = (val >> 0) & 0xFF;
	return 0;
}
#else
#ifdef CONFIG_FEC1_ENABLED
int fec_get_mac_addr(unsigned int index, unsigned char *mac)
{
	nv_critical_t *pNvram;

	if (!NvCriticalGet(&pNvram))
		return -1;

	if (0 == index) {
		/* 1st wired MAC (ENET0) */
		memcpy(mac, &pNvram->s.p.xID.axMAC[0], 6);
	}
	else if (1 == index) {
		/* 2nd wired MAC (ENET1) corresponds to ethaddr3
		 * because ethaddr2 is for the wireless.
		 * ethaddr3 is a field out of xID which originally
		 * only had space for two interfaces (wired and wireless)
		 */
		memcpy(mac, &pNvram->s.p.eth1addr, 6);
	}

	return 0;
}
#else
int fec_get_mac_addr(unsigned char *mac)
{
	nv_critical_t *pNvram;

	if (!NvCriticalGet(&pNvram))
		return -1;

	memcpy(mac, &pNvram->s.p.xID.axMAC[0], 6);

	return 0;
}
#endif /* CONFIG_FEC1_ENABLED */
#endif /* CONFIG_GET_FEC_MAC_ADDR_FROM_IIM */
#endif

void enet_board_init(void)
{
	/* Set up ENET pins */
	pin_set_group(&enet_pins);

    /* Hold the external phy in reset */
	pin_set_type(CONFIG_PHY_RESET_GPIO, PIN_GPIO);
	pin_gpio_set(CONFIG_PHY_RESET_GPIO, 0);
	pin_gpio_direction(CONFIG_PHY_RESET_GPIO, 1);

	/* Turn on the power to the external phy */
	pin_set_type(CONFIG_PHY_POWER_GPIO, PIN_GPIO);
	pin_gpio_set(CONFIG_PHY_POWER_GPIO, 0);
	pin_gpio_direction(CONFIG_PHY_POWER_GPIO, 1);
	udelay(10300);  // from datasheet: tvr + tsr

	/* Bring the external phy out of reset */
	pin_gpio_set(CONFIG_PHY_RESET_GPIO, 1);

	/* Ensure recommended delay is met before accessing the MDIO interface */
	udelay(100);
}

#ifdef CONFIG_DUAL_BOOT

#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
/* Enables the watchdog timer with a given timeout value (in seconds) */
unsigned long board_wdt_enable(unsigned long seconds)
{
	/* The watchdog takes the value in miliseconds */
	return wdt_enable(seconds * 1000);
}
#endif /* CONFIG_DUAL_BOOT_WDT_ENABLE */

#ifdef CONFIG_DUAL_BOOT_PERSISTENT_IN_NVRAM
/* Increments the number of boot attempts in persistent memory */
void dualb_increment_bootattempts(void)
{
	/* Boot attempts is a two-bit field [1:0]
	 * in HW_RTC_PERSISTENT2 register
	 */
	unsigned long reg;
	int count;

	/* Read the current count value */
	reg = persistent_reg_read(2);
	count = reg & 0x3;

	/* Increment counter up to the limit */
	if (count < 0x3)
		persistent_reg_write(2, (reg & ~0x3) | (count + 1));
}

/* Increments the number of boot attempts in persistent memory */
int dualb_get_bootattempts(void)
{
	/* Boot attempts is a two-bit field [1:0]
	 * in HW_RTC_PERSISTENT2 register
	 */
	return((int)persistent_reg_read(2) & 0x3);
}

/* Resets the number of boot attempts in persistent memory */
void dualb_reset_bootattempts(void)
{
	/* Boot attempts is a two-bit field [1:0]
	 * in HW_RTC_PERSISTENT2 register
	 */
	persistent_reg_clr(2, 0x3);
}
#endif /* CONFIG_DUAL_BOOT_PERSISTENT_IN_NVRAM */

#endif /* CONFIG_DUAL_BOOT */

unsigned int get_platform_loadaddr(char *loadaddr)
{
	/* WR21 has 128MB
	 * U-Boot is loaded at 120MB offset.
	 */
	return CONFIG_LOADADDR;
}

/*
 * This function returns the size of available RAM for doing a TFTP transfer.
 * This size depends on:
 *   - The total RAM available
 *   - The loadaddr
 *   - The RAM occupied by U-Boot and its location
 */
unsigned int get_tftp_available_ram(unsigned int loadaddr)
{
	unsigned int la_off = loadaddr - gd->bd->bi_dram[0].start;

	/* WR21 has 128MB
	 * U-Boot is loaded at 120MB offset.
	 */
	return (get_total_ram() - CONFIG_UBOOT_RESERVED_RAM - la_off);
}
