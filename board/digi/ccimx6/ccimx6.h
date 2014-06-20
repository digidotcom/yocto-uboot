/*
 *  Copyright (C) 2014 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/

#ifndef __CCIMX6_H
#define __CCIMX6_H

/* DA9063 PMIC */
#define DA9063_PAGE_CON			0x0
#define DA9063_GPIO6_7_ADDR		0x18
#define DA9063_GPIO10_11_ADDR		0x1a
#define DA9063_GPIO_MODE0_7_ADDR	0x1d
#define DA9063_GPIO_MODE8_15_ADDR	0x1e
#define DA9063_VLDO4_CONT_ADDR		0x29
#define DA9063_VLDO4_A_ADDR		0xac
#define DA9063_VLDO4_B_ADDR		0xbd
#define DA9063_CONFIG_D_ADDR		0x109
#define DA9063_DEVICE_ID_ADDR		0x181
#define DA9063_VARIANT_ID_ADDR		0x182
#define DA9063_CUSTOMER_ID_ADDR		0x183
#define DA9063_CONFIG_ID_ADDR		0x184

/* Common ccimx6 functions */
int pmic_read_reg(int reg, unsigned char *value);
int pmic_write_reg(int reg, unsigned char value);
int pmic_write_bitfield(int reg, unsigned char mask, unsigned char off,
			       unsigned char bfval);
int setup_sata(void);
void setup_iomux_enet(void);
int ccimx6_late_init(void);

/* Board defined functions */
int setup_pmic_voltages(void);

#endif  /* __CCIMX6_H */
