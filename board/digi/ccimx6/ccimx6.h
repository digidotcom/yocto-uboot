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

/* DA9063 PMIC registers */
#define DA9063_PAGE_CON			0x0
#define DA9063_FAULT_LOG_ADDR		0x05
#define DA9063_EVENT_A_ADDR		0x06
#define DA9063_EVENT_B_ADDR		0x07
#define DA9063_EVENT_C_ADDR		0x08
#define DA9063_EVENT_D_ADDR		0x09
#define DA9063_IRQ_MASK_B_ADDR		0x0b
#define DA9063_CONTROL_A_ADDR		0x0e
#define DA9063_CONTROL_B_ADDR		0x0f
#define DA9063_GPIO4_5_ADDR		0x17
#define DA9063_GPIO6_7_ADDR		0x18
#define DA9063_GPIO8_9_ADDR		0x19
#define DA9063_GPIO10_11_ADDR		0x1a
#define DA9063_GPIO_MODE0_7_ADDR	0x1d
#define DA9063_GPIO_MODE8_15_ADDR	0x1e
#define DA9063_VLDO4_CONT_ADDR		0x29
#define DA9063_BCORE2_CONF_ADDR		0x9d
#define DA9063_BCORE1_CONF_ADDR		0x9e
#define DA9063_BPRO_CONF_ADDR		0x9f
#define DA9063_BIO_CONF_ADDR		0xa0
#define DA9063_BMEM_CONF_ADDR		0xa1
#define DA9063_BPERI_CONF_ADDR		0xa2
#define DA9063_VLDO4_A_ADDR		0xac
#define DA9063_VLDO4_B_ADDR		0xbd
#define DA9063_CONFIG_D_ADDR		0x109
#define DA9063_DEVICE_ID_ADDR		0x181
#define DA9063_VARIANT_ID_ADDR		0x182
#define DA9063_CUSTOMER_ID_ADDR		0x183
#define DA9063_CONFIG_ID_ADDR		0x184

/* DA9063 FAULT_LOG bitfields */
#define DA9063_E_nSHUT_DOWN		0x40
#define DA9063_E_nKEY_RESET		0x20

/* DA9063 EVENT_A bitfields */
#define DA9063_E_ADC_RDY		0x08
#define DA9063_E_TICK			0x04
#define DA9063_E_ALARM			0x02
#define DA9063_E_nONKEY			0x01

/* DA9063 EVENT_B bitfields */
#define DA9063_E_VDD_WARN		0x80
#define DA9063_E_VDD_MON		0x40
#define DA9063_E_DVC_RDY		0x20
#define DA9063_E_UVOV			0x10
#define DA9063_E_LDO_LIM		0x08
#define DA9063_E_COMP1V2		0x04
#define DA9063_E_TEMP			0x02
#define DA9063_E_WAKE			0x01

/* DA9063 EVENT_C bitfields */
#define DA9063_E_GPIO5			0x20
#define DA9063_E_GPIO6			0x40
#define DA9063_E_GPIO7			0x80

/* DA9063 EVENT_D bitfields */
#define DA9063_E_GPIO8			0x01
#define DA9063_E_GPIO9			0x02

/* Common ccimx6 functions */
int pmic_read_reg(int reg, unsigned char *value);
int pmic_write_reg(int reg, unsigned char value);
int pmic_write_bitfield(int reg, unsigned char mask, unsigned char off,
			       unsigned char bfval);
int setup_sata(void);
void setup_iomux_enet(void);
int ccimx6_early_init(void);
int ccimx6_late_init(void);
int get_carrierboard_version(void);

/* Board defined functions */
int setup_pmic_voltages(void);

#endif  /* __CCIMX6_H */
