/*
 * (c) 2009 Digi International, Inc All rights reserved.
 *     Basic GPIO support for the i.mx51 processor
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <common.h>
#include <asm/arch/iomux.h>

#define PORT_BADDR(n)	(GPIO1_BASE_ADDR + (n) * 0x4000)

int imx_gpio_get_pin(unsigned int pin)
{
	u32 gpio_num = IOMUX_TO_GPIO(pin);
	u32 gpio_off = GPIO_TO_INDEX(gpio_num);
	u32 baddr = PORT_BADDR(GPIO_TO_PORT(gpio_num));

	return (__REG(baddr + GPIO_DR) & (1 << gpio_off)) ? 1 : 0;
}

void imx_gpio_set_pin(unsigned int pin, int val)
{
	u32 gpio_num = IOMUX_TO_GPIO(pin);
	u32 gpio_off = GPIO_TO_INDEX(gpio_num);
	u32 baddr = PORT_BADDR(GPIO_TO_PORT(gpio_num));

	if (val)
		__REG(baddr + GPIO_DR) |= (1 << gpio_off);
	else
		__REG(baddr + GPIO_DR) &= ~(1 << gpio_off);
}

void imx_gpio_pin_cfg_dir(unsigned int pin, int dir)
{
	u32 gpio_num = IOMUX_TO_GPIO(pin);
	u32 gpio_off = GPIO_TO_INDEX(gpio_num);
	u32 baddr = PORT_BADDR(GPIO_TO_PORT(gpio_num));

	if (dir)
		__REG(baddr + GPIO_GDIR) |= (1 << gpio_off);
	else
		__REG(baddr + GPIO_GDIR) &= ~(1 << gpio_off);
}
