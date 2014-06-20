/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2013 Digi International, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 * Author: Jason Liu <r64343@freescale.com>
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
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/gpio.h>
#include <netdev.h>
#if CONFIG_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif
#include "../ccimx6/ccimx6.h"

DECLARE_GLOBAL_DATA_PTR;

static int phy_addr;

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

iomux_v3_cfg_t const uart4_pads[] = {
	MX6_PAD_KEY_COL0__UART4_TXD | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_KEY_ROW0__UART4_RXD | MUX_PAD_CTRL(UART_PAD_CTRL),
};

iomux_v3_cfg_t const ksz9031_pads[] = {
	/* Micrel KSZ9031 PHY reset */
	MX6_PAD_ENET_CRS_DV__GPIO_1_25		| MUX_PAD_CTRL(NO_PAD_CTRL),
};

#ifdef CONFIG_I2C_MXC
int setup_pmic_voltages(void)
{
	unsigned char dev_id, var_id, conf_id, cust_id;
#ifdef CONFIG_I2C_MULTI_BUS
	int ret;

	ret = i2c_set_bus_num(0);
	if (ret)
                return -1;
#endif

	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	if (!i2c_probe(CONFIG_PMIC_I2C_ADDR)) {
		/* Read and print PMIC identification */
		if (pmic_read_reg(DA9063_DEVICE_ID_ADDR, &dev_id) ||
		    pmic_read_reg(DA9063_VARIANT_ID_ADDR, &var_id) ||
		    pmic_read_reg(DA9063_CUSTOMER_ID_ADDR, &cust_id) ||
		    pmic_read_reg(DA9063_CONFIG_ID_ADDR, &conf_id)) {
			printf("Could not read PMIC ID registers\n");
			return -1;
		}
		printf("PMIC:  DA9063, Device: 0x%02x, Variant: 0x%02x, "
			"Customer: 0x%02x, Config: 0x%02x\n", dev_id, var_id,
			cust_id, conf_id);

#if defined(CONFIG_FEC_MXC)
		/* Both NVCC_ENET and NVCC_RGMII come from LDO4 (2.5V) */
		/* Config LDO4 voltages A and B at 2.5V, then enable VLDO4 */
		if (pmic_write_reg(DA9063_VLDO4_A_ADDR, 0x50) ||
		    pmic_write_reg(DA9063_VLDO4_B_ADDR, 0x50) ||
		    pmic_write_bitfield(DA9063_VLDO4_CONT_ADDR, 0x1, 0, 0x1))
			printf("Could not configure VLDO4\n");
#endif
		/* PMIC GPIO11 is the LCD backlight which is low level
		 * enabled. If left with default configuration (input) or when
		 * coming from power-off, the backlight may be enabled and draw
		 * too much power from the 5V source when this source is
		 * enabled, which may cause a voltage drop on the 5V line and
		 * hang the I2C bus where the touch controller is attached.
		 * To prevent this, configure GPIO11 as output and set it
		 * high, to make sure the backlight is disabled when the 5V is
		 * enabled.
		 */
		if (pmic_write_bitfield(DA9063_GPIO10_11_ADDR, 0xf, 4, 0x3))
			printf("Could not configure GPIO11\n");
		if (pmic_write_bitfield(DA9063_GPIO_MODE8_15_ADDR, 0x1, 3, 0x1))
			printf("Could not set GPIO11 high\n");

		/* PWR_EN on the ccimx6sbc enables the +5V suppy and comes
		 * from PMIC_GPIO7. Set this GPIO high to enable +5V supply.
		 */
		if (pmic_write_bitfield(DA9063_GPIO6_7_ADDR, 0xf, 4, 0x3))
			printf("Could not configure GPIO7\n");
		if (pmic_write_bitfield(DA9063_GPIO_MODE0_7_ADDR, 0x1, 7, 0x1))
			printf("Could not enable PWR_EN\n");
	}
	return 0;
}
#endif /* CONFIG_I2C_MXC */

static void setup_board_enet(void)
{
	int phy_reset_gpio;

	/* Gigabit ENET (Micrel PHY) */
	phy_reset_gpio = IMX_GPIO_NR(1, 25);
	phy_addr = CONFIG_ENET_PHYADDR_MICREL;
	imx_iomux_v3_setup_multiple_pads(ksz9031_pads,
					 ARRAY_SIZE(ksz9031_pads));
	/* Assert PHY reset */
	gpio_direction_output(phy_reset_gpio , 0);
	/* Need 10ms to guarantee stable voltages */
	udelay(10 * 1000);
	/* Deassert PHY reset */
	gpio_set_value(phy_reset_gpio, 1);
	/* Need to wait 100us before accessing the MIIM (MDC/MDIO) */
	udelay(100);
}

int board_get_enet_phy_addr(void)
{
	return phy_addr;
}

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
}

int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_enet();
	setup_board_enet();

	ret = cpu_eth_init(bis);
	if (ret)
		printf("FEC MXC: %s:failed\n", __func__);

	return 0;
}

int board_early_init_f(void)
{
	setup_iomux_uart();

#ifdef CONFIG_CMD_SATA
	setup_sata();
#endif
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	return 0;
}

int board_late_init(void)
{
	return ccimx6_late_init();
}
