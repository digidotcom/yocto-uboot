/***********************************************************************
 *
 * Copyright (C) 2009 by Digi International Inc.
 * All rights reserved.
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
 *
 */

#ifndef __ASM_STATUS_LED_H__
#define __ASM_STATUS_LED_H__

#ifndef __ASSEMBLY__

/* if not overriden */
#ifndef CONFIG_BOARD_SPECIFIC_LED

#if defined(CONFIG_CCIMX51)
#include <asm/arch/gpio.h>
#include <asm/arch/mx51_pins.h>
#include <asm/arch/iomux.h>

/* led_id_t is unsigned int mask */
typedef unsigned int led_id_t;

static inline void __led_toggle(led_id_t mask)
{
	imx_gpio_set_pin(mask, !imx_gpio_get_pin(mask));
}

static inline void __led_set( led_id_t mask, int state )
{
#if (STATUS_LED_ACTIVE == 0)
	imx_gpio_set_pin(mask, (STATUS_LED_ON == state) ? 0 : 1);
#else
        imx_gpio_set_pin(mask, (STATUS_LED_ON == state) ? 0 : 1);
#endif
}

static inline void __led_init( led_id_t mask, int state )
{
	/* GPIOs are initialized in the board file */
        imx_gpio_set_pin(mask, 0);
}

#endif	/* defined(CONFIG_CCIMX51) */

#endif	/* CONFIG_BOARD_SPECIFIC_LED */
#endif  /* __ASSEMBLY__ */
#endif	/* __ASM_STATUS_LED_H__ */
