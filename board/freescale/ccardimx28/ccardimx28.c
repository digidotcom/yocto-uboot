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
#include <asm/errno.h>
#include <nvram.h>
#include <mxs-persistent.h>
#include <asm/mxs_otp.h>

#include <mmc.h>
#include <imx_ssp_mmc.h>
#include <serial.h>
#include <linux/ctype.h>

#include "board-ccardimx28.h"

DECLARE_GLOBAL_DATA_PTR;

struct ccardimx28_hwid mod_hwid;
static u8 hwid[CONFIG_HWID_LENGTH];

#ifdef CONFIG_FEC1_ENABLED
int fec_get_mac_addr(unsigned int index, unsigned char *mac);
#else
int fec_get_mac_addr(unsigned char *mac);
#endif

#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
extern unsigned long wdt_enable(unsigned long value);
#endif

extern unsigned int get_total_ram(void);

/* DUART pins */
static struct pin_desc duart_pins_desc[] = {
	{ PINID_I2C0_SCL, PIN_FUN3, PAD_4MA, PAD_3V3, 0 },
	{ PINID_I2C0_SDA, PIN_FUN3, PAD_4MA, PAD_3V3, 0 },
};
static struct pin_group duart_pins = {
	.pins		= duart_pins_desc,
	.nr_pins	= ARRAY_SIZE(duart_pins_desc),
};

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
	/* SSP0 on ccardimx28 only has 4 data lines
	{ PINID_SSP0_DATA4, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA5, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA6, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	{ PINID_SSP0_DATA7, PIN_FUN1, PAD_8MA, PAD_3V3, 1 },
	*/
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
/* Gpmi pins at 1.8V */
static struct pin_desc gpmi_pins_desc[] = {
	{ PINID_GPMI_D00, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D01, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D02, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D03, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D04, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D05, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D06, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_D07, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_RDN, PIN_FUN1, PAD_8MA, PAD_1V8, 0 },
	{ PINID_GPMI_WRN, PIN_FUN1, PAD_8MA, PAD_1V8, 0 },
	{ PINID_GPMI_ALE, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_CLE, PIN_FUN1, PAD_4MA, PAD_1V8, 0 },
	{ PINID_GPMI_RDY0, PIN_FUN1, PAD_4MA, PAD_1V8, 1 },
//	{ PINID_GPMI_RDY1, PIN_FUN1, PAD_4MA, PAD_1V8, 1 },
	{ PINID_GPMI_CE0N, PIN_FUN1, PAD_4MA, PAD_1V8, 1 },
//	{ PINID_GPMI_CE1N, PIN_FUN1, PAD_4MA, PAD_1V8, 1 },
	{ PINID_GPMI_RESETN, PIN_FUN1, PAD_4MA, PAD_1V8, 0 }
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

#ifdef CONFIG_I2C_MXS
static struct pin_desc i2c1_pins_desc[] = {
	{ PINID_PWM0, PIN_FUN2, PAD_4MA, PAD_3V3, 0 },
	{ PINID_PWM1, PIN_FUN2, PAD_4MA, PAD_3V3, 0 },
};

static struct pin_group i2c1_pins = {
	.pins		= i2c1_pins_desc,
	.nr_pins	= ARRAY_SIZE(i2c1_pins_desc)
};
#endif /* CONFIG_I2C_MXS */

#ifdef CONFIG_USER_KEY
static struct pin_desc userkey_pins_desc[] = {
	{ USER_KEY1_GPIO, PIN_GPIO, PAD_4MA, PAD_3V3, 0 },
	{ USER_KEY2_GPIO, PIN_GPIO, PAD_4MA, PAD_3V3, 0 },
};

static struct pin_group userkey_pins = {
	.pins		= userkey_pins_desc,
	.nr_pins	= ARRAY_SIZE(userkey_pins_desc)
};
#endif /* CONFIG_USER_KEY */

/*
 * Functions
 */
u32 get_board_rev(void)
{
	return 0;
}

void get_board_serial(struct tag_serialnr *serialnr)
{
	u8 *p = (u8 *)serialnr;

	/* On linux, we pass the hwid information using serialnr TAG */
	memcpy(p, hwid, CONFIG_HWID_LENGTH);
}

static int is_valid_hw_id(u8 variant)
{
	int ret = 0;

	if (variant < ARRAY_SIZE(ccardimx28_id)) {
		if (ccardimx28_id[variant].sdram != 0)
			ret = 1;
	}
	return ret;
}

int variant_has_ethernet(void)
{
	/* Assume YES for any variant code except for those
	 * that don't have Ethernet. An empty HWID would return 0
	 * and we want to enable Ethernet for that case.
	 */
	if (mod_hwid.variant != 0x06)
		return 1;

	return 0;
}

int variant_machid(void)
{
	/* Return the MACHINE ID from the variant
	 */
	switch(mod_hwid.variant) {
	case 0x05:
	case 0x09:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
		/* Non-wireless variants */
		return MACH_TYPE_CCARDMX28JS;
	default:
		break;
	}
	/* Wireless variants (or unspecified) */
	return MACH_TYPE_CCARDWMX28JS;
}

int array_to_hwid(u8 *hwid)
{
	/*
	 *       | 31..           HWOTP_CUST1            ..0 | 31..          HWOTP_CUST0             ..0 |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * HWID: |    --    | TF (loc) | variant  | HV |Cert |   Year   | Mon |     Serial Number        |
	 *       +----------+----------+----------+----------+----------+----------+----------+----------+
	 * Byte: 0          1          2          3          4          5          6          7
	 */
	if (!is_valid_hw_id(hwid[2]))
		return -EINVAL;

	mod_hwid.tf = hwid[1];
	mod_hwid.variant = hwid[2];
	mod_hwid.hv = (hwid[3] & 0xf0) >> 4;
	mod_hwid.cert = hwid[3] & 0xf;
	mod_hwid.year = hwid[4];
	mod_hwid.month = (hwid[5] & 0xf0) >> 4;
	mod_hwid.sn = ((hwid[5] & 0xf) << 16) | (hwid[6] << 8) | hwid[7];

	return  0;
}

int get_module_hw_id(void)
{
	nv_critical_t *pNVRAM;

	memset(&mod_hwid, 0, sizeof(struct ccardimx28_hwid));
	memset(hwid, 0, ARRAY_SIZE(hwid));

	/* nvram settings override fuse hwid */
	if (NvCriticalGet(&pNVRAM)) {
		const nv_param_module_id_t *pModule = &pNVRAM->s.p.xID;

		if (!array_to_hwid((u8 *)&pModule->szProductType)) {
			/* Set global hwid with value in NVRAM */
			memcpy(hwid, (u8 *)&pModule->szProductType, CONFIG_HWID_LENGTH);
			return 0;
		}
	}

	/* Use fuses if no valid hwid was set in the nvram */
	if (!read_otp_hwid(hwid))
		array_to_hwid(hwid);

	/* returning something != 0 will halt the boot process */
	return 0;
}

#ifdef CONFIG_COMPUTE_LPJ
unsigned int get_preset_lpj(void)
{
	unsigned int mcu_clk, lpj = 0;

	mcu_clk = mxc_get_clock(MXC_ARM_CLK);

	switch (mcu_clk) {
	case 454000000:
		lpj = CONFIG_PRESET_LPJ_454MHZ;
		break;
	}
	return lpj;
}
#endif

void NvPrintHwID(void)
{
	printf("    TF (location): 0x%02x\n", mod_hwid.tf);
	printf("    Variant:       0x%02x\n", mod_hwid.variant);
	printf("    HW Version:    %d\n", mod_hwid.hv);
	printf("    Cert:          0x%x\n", mod_hwid.cert);
	printf("    Year:          20%02d\n", mod_hwid.year);
	printf("    Month:         %02d\n", mod_hwid.month);
	printf("    S/N:           %d\n", mod_hwid.sn);
}

#ifdef CONFIG_NAND_GPMI
void setup_gpmi_nand(void)
{
	/* Set up GPMI pins */
	pin_set_group(&gpmi_pins);
}
#endif

#ifdef CONFIG_USER_KEY
static void setup_user_keys(void)
{
	/* Configure iomux */
	pin_set_group(&userkey_pins);
	/* Configure direction as input */
	pin_gpio_direction(USER_KEY1_GPIO, 0);
	pin_gpio_direction(USER_KEY2_GPIO, 0);
	/* Configure pull-up */
	pin_set_pullup(USER_KEY1_GPIO, 1);
	pin_set_pullup(USER_KEY2_GPIO, 1);
}

static void run_user_keys(void)
{
	char cmd[60];

#ifdef CONFIG_USER_KEY12
	if ((pin_gpio_get(USER_KEY1_GPIO) == 0) &&
	    (pin_gpio_get(USER_KEY2_GPIO) == 0)) {
		printf("\nUser Key 1 and 2 pressed\n");
		if (getenv("key12") != NULL) {
			sprintf(cmd, "run key12");
			run_command(cmd, 0);
			return;
		}
	}
#endif /* CONFIG_USER_KEY12 */

	if (pin_gpio_get(USER_KEY1_GPIO) == 0) {
		printf("\nUser Key 1 pressed\n");
		if (getenv("key1") != NULL) {
			sprintf(cmd, "run key1");
			run_command(cmd, 0);
		}
	}

	if (pin_gpio_get(USER_KEY2_GPIO) == 0) {
		printf("\nUser Key 2 pressed\n");
		if (getenv("key2") != NULL) {
			sprintf(cmd, "run key2");
			run_command(cmd, 0);
		}
	}
}
#endif /* CONFIG_USER_KEY */

#if defined(CONFIG_SILENT_CONSOLE) && defined(ENABLE_CONSOLE_GPIO)
static void init_console_gpio(void)
{
	/* Configure iomux */
	pin_set_type(ENABLE_CONSOLE_GPIO, PIN_GPIO);
	/* Configure direction as input */
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

#ifdef CONFIG_I2C_MXS
void i2c1_gpios_init(void)
{
	pin_set_group(&i2c1_pins);
}
#endif

int board_init(void)
{
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#if defined(CONFIG_SILENT_CONSOLE) && defined(ENABLE_CONSOLE_GPIO)
	init_console_gpio();
#endif

#ifdef CONFIG_NAND_GPMI
	setup_gpmi_nand();
#endif

#ifdef CONFIG_I2C_MXS
	i2c1_gpios_init();
#endif

#ifdef CONFIG_USER_KEY
	setup_user_keys();
#endif

	return 0;
}

#ifdef BOARD_LATE_INIT
int board_late_init(void)
{
	int fet_active = 1;	/* default polarity */

	/* On modules newer than V1, a FET is blocking the 3V3 supply.
	 * To have this voltage enabled to the module LCD_RS
	 * must be driven low on modules V2..V5 and driven high on modules V6.
	 * On V1 modules this line is not used so the code is harmless.
	 * On modules with blank OTP, a V6 is considered by default.
	 */
	if (mod_hwid.hv > 0 && mod_hwid.hv < 6)
		fet_active = 0;	/* active low */
	/* First set values to avoid glitches */
	pin_gpio_set(PINID_LCD_RS, fet_active);
	pin_gpio_direction(PINID_LCD_RS, 1);
	pin_set_type(PINID_LCD_RS, PIN_GPIO);

#ifdef CONFIG_DISABLE_CONSOLE_RX
	{
		char *s;
		int bootdelay;

		if ((s=getenv("bootdelay")) != NULL) {
			bootdelay = (int)simple_strtol(s, NULL, 10);
			if (bootdelay == 0)
				gd->flags |= GD_FLG_DISABLE_CONSOLE_RX;
		}
	}
#endif

	return 0;
}
#endif

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	if (is_valid_hw_id(mod_hwid.variant)) {
		gd->bd->bi_dram[0].size = ccardimx28_id[mod_hwid.variant].sdram;
	} else {
		gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	}

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

/* The ccardimx28 doesn't have WP pin */
#if 0
#define PINID_SSP0_GPIO_WP PINID_SSP1_SCK
#endif
#define PINID_SSP1_GPIO_WP PINID_GPMI_RESETN

u32 ssp_mmc_is_wp(struct mmc *mmc)
{
	/* The ccardimx28 doesn't have WP pin */
#if 0
	return (mmc->block_dev.dev == 0) ?
		pin_gpio_get(PINID_SSP0_GPIO_WP) :
		pin_gpio_get(PINID_SSP1_GPIO_WP);
#else
	return (mmc->block_dev.dev == 0) ?
		0 :
		pin_gpio_get(PINID_SSP1_GPIO_WP);
#endif
}

int ssp_mmc_gpio_init(bd_t *bis, int index)
{
	s32 status = 0;

#ifdef CONFIG_CMD_MMC
	switch (index) {
	case 0:
		/* Set up MMC pins */
		pin_set_group(&mmc0_pins);

		/* The ccardimx28 doesn't have power on and WP pins */
#if 0
		/* Power on the card slot 0 */
		pin_set_type(PINID_PWM3, PIN_GPIO);
		pin_gpio_direction(PINID_PWM3, 1);
		pin_gpio_set(PINID_PWM3, 0);

		/* Wait 10 ms for card ramping up */
		udelay(10000);

		/* Set up SD0 WP pin */
		pin_set_type(PINID_SSP0_GPIO_WP, PIN_GPIO);
		pin_gpio_direction(PINID_SSP0_GPIO_WP, 0);
#endif

		break;
	case 1:
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
		break;
	default:
		printf("Warning: invalid ssp mmc controller. "
			"Max supported by the board is %d\n",
			CONFIG_SYS_SSP_MMC_NUM);
		return status;
	}
	status = imx_ssp_mmc_initialize(bis, &ssp_mmc_cfg[index]);
#endif /* CONFIG_CMD_MMC */

	return status;
}

int board_mmc_init(bd_t *bis)
{
	/* Init SSP0 */
	if (!ssp_mmc_gpio_init(bis, 0))
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

	/* 1st wired MAC (ENET0) */
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

	/* Reset the external phy */
	pin_set_type(CONFIG_PHY_RESET_GPIO, PIN_GPIO);
	pin_gpio_set(CONFIG_PHY_RESET_GPIO, 0);
	pin_gpio_direction(CONFIG_PHY_RESET_GPIO, 1);
	udelay(10300);  // from datasheet: tvr + tsr

	/* Bring the external phy out of reset */
	pin_gpio_set(CONFIG_PHY_RESET_GPIO, 1);

	/* Ensure recommended delay of 100us in PHY datasheet is met before
	 * accessing the MDIO interface */
	udelay(100);
}

#ifdef CONFIG_DUAL_BOOT

#ifdef CONFIG_DUAL_BOOT_WDT_ENABLE
/* Enables the watchdog timer with a given timeout value (in seconds) */
unsigned long board_wdt_enable(unsigned long seconds)
{
	/* The watchdog takes the value in miliseconds */
	/* We should return the value in seconds (to print it) */
	return (wdt_enable(seconds * 1000) / 1000);
}
#endif /* CONFIG_DUAL_BOOT_WDT_ENABLE */

#ifndef CONFIG_DUAL_BOOT_PERSISTENT_IN_NVRAM
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

void debug_led(int status)
{
       /* Debug LED */
       pin_set_type(PINID_SSP2_SS0, PIN_GPIO);
       pin_gpio_direction(PINID_SSP2_SS0, 1);
       pin_gpio_set(PINID_SSP2_SS0, !status); /* 0 is ON, 1 is OFF */
}

#ifdef CONFIG_PRINT_SYS_INFO
void print_sys_info(void)
{
	char *s;
	int boot_verb = 0;

	if ((s = getenv("bv")) != NULL)
		boot_verb = simple_strtoul(s, NULL, 10);

	if (boot_verb) {
		/* CPU info, version, voltage and frequency */
		printf("CPU:   Freescale i.MX28 family at %d MHz\n",
				mxc_get_clock(MXC_ARM_CLK)/1000000);

		if (is_valid_hw_id(mod_hwid.variant)) {
			puts("Module HW ID      : ");
			printf("%s\n", ccardimx28_id[mod_hwid.variant].id_string);
			printf(" Module variant   : %02x\n", mod_hwid.variant);
			printf(" Hardware version : %d\n", mod_hwid.hv);
			printf(" Module SN        : %d\n", mod_hwid.sn);
		}
	}
}
#endif

#ifdef BOARD_BEFORE_MLOOP_INIT
int board_before_mloop_init (void)
{
#ifdef CONFIG_PRINT_SYS_INFO
	print_sys_info();
#endif
#ifdef CONFIG_USER_KEY
	run_user_keys();
#endif
	return 0;
}
#endif

void serial_ports_deinit(void)
{
	int port;

#ifdef CONFIG_GET_CONS_FROM_CONS_VAR
	int conidx = CONFIG_CONS_INDEX;
	char *s, *k;

	if ((s = getenv("console")) != NULL) {
		if ((k = strstr(s, CONFIG_UARTDBG_CONSOLE)) != NULL) {
			/* console is DUART. Reconfigure all AUARTs GPIOs */
			for (port = 0; port < MXS_MAX_AUARTS; port++)
				pin_unset_group(&auart_pins[port]);
		}
		else if ((k = strstr(s, CONFIG_UARTAPP_CONSOLE)) != NULL) {
			/* console is AUART. Index is only one digit... */
			conidx = k[strlen(CONFIG_UARTAPP_CONSOLE)] - '0';
			if (conidx < 0 || conidx >= MXS_MAX_AUARTS)
				conidx = CONFIG_CONS_INDEX;
			/* Reconfigure all other AUARTs GPIOs */
			for (port = 0; port < MXS_MAX_AUARTS; port++) {
				if (port != conidx)
					pin_unset_group(&auart_pins[port]);
			}
			/* Reconfigure DUART GPIOs */
			pin_unset_group(&duart_pins);
		}
	}
#else
#ifdef CONFIG_DEFAULT_SERIAL_AUART
	/* Reconfigure all other AUARTs GPIOs */
	for (port = 0; port < MXS_MAX_AUARTS; port++) {
		if (port != CONFIG_CONS_INDEX)
			pin_unset_group(&auart_pins[port]);
	}
	/* Reconfigure DUART GPIOs */
	pin_unset_group(&duart_pins);
#else
	/* console is DUART. Reconfigure all AUARTs GPIOs */
	for (port = 0; port < MXS_MAX_AUARTS; port++)
		pin_unset_group(&auart_pins[port]);
#endif
#endif /* CONFIG_GET_CONS_FROM_CONS_VAR */
}


#ifdef CONFIG_BOARD_CLEANUP_BEFORE_LINUX
void board_cleanup_before_linux(void)
{
	/*
	 * Before giving control to Linux, make sure we deinit
	 * all the GPIOs (except those for the console).
	 * The reason is that U-Boot may have configured these,
	 * and the kernel will not reconfigure them as inputs.
	 */
	pin_unset_group(&enet_pins);
	pin_unset_group(&gpmi_pins);
#ifdef CONFIG_I2C_MXS
	pin_unset_group(&i2c1_pins);
#endif
#ifdef CONFIG_IMX_SSP_MMC
	pin_unset_group(&mmc0_pins);
	pin_unset_group(&mmc1_pins);
#endif
	pin_unset_group(&userkey_pins);
	serial_ports_deinit();
}
#endif

#define SIZE_128MB	0x8000000

/*
 * This function returns the 'loadaddr', the RAM address to use to transfer
 * files to, before booting or updating firmware.
 */
unsigned int get_platform_loadaddr(char *loadaddr)
{
	/* We distinguish two RAM memory variants: 256MB and 128MB
	 * U-Boot is loaded at 120MB offset.
	 */
	if (!strcmp(loadaddr, "loadaddr")) {
		if (gd->bd->bi_dram[0].size > SIZE_128MB)
			/*  For 256MB: 128MB */
			return (gd->bd->bi_dram[0].start + SIZE_128MB);
		else
			/*  For 128MB: CONFIG_LOADADDR */
			return CONFIG_LOADADDR;
	} else
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

	/* We distinguish two RAM memory variants: 256MB and 128MB
	 * U-Boot is loaded at 120MB offset.
	 */
	if (gd->bd->bi_dram[0].size > SIZE_128MB) {
		if (la_off >= SIZE_128MB)
			/*  total RAM - loadaddr offset */
			return (get_total_ram() - la_off);
		else
			/* 128M - UBOOT_RESERVED - loadaddr offset */
			return (SIZE_128MB - CONFIG_UBOOT_RESERVED_RAM -
				la_off);
	} else
		/*  For 128MB: total ram - UBOOT_RESERVED - loadaddr offset */
		return (get_total_ram() - CONFIG_UBOOT_RESERVED_RAM - la_off);
}
