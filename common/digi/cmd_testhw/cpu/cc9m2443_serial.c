/*
 *  common/digi/cmd_testhw/cpu/cc9m2443_serial.c
 *
 *  Copyright (C) 2007 by Digi International Inc.
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version2  as published by
 *  the Free Software Foundation.
*/
#include <common.h>
#if (CONFIG_COMMANDS & CFG_CMD_BSP && \
		defined(CONFIG_UBOOT_CMD_BSP_TESTHW) &&    \
		(defined(CONFIG_CC9M2443) || defined(CONFIG_CCW9M2443)))

DECLARE_GLOBAL_DATA_PTR;

#include <cmd_testhw/testhw.h>
#include <regs.h>
#define UART_BASE(x) (ELFIN_UART_BASE + (x * 0x4000)) 

static int test_putc(S3C24X0_UART *uart, const char c)
{
	int timeout = 1000;
	/* wait for room in the tx FIFO */
	while (!(uart->UTRSTAT & 0x2));

	while (!(uart->UMSTAT & 0x1)&& timeout) {
		udelay(10);
		timeout--;
	}

	if(!timeout) {
		printf("putc: timeout\n");
		return 0;
	}
	uart->UTXH = c;
	return 1;
}


static int test_getc(S3C24X0_UART *uart)
{
	int timeout = 1000;
	
	while (!(uart->UTRSTAT & 0x1) && timeout) {
		udelay(10);
		timeout --;
	}
	
	if(!timeout) {
		printf("getc: timeout\n");
		return -1;
	}

	return uart->URXH & 0xff;
}

static int init_uart(S3C24X0_UART *uart)
{
	unsigned int reg = 0;
	
	/* value is calculated so : (int)(PCLK/16./baudrate) -1 */
	reg = get_PCLK() / (16 * gd->baudrate) - 1;

	/* FIFO enable, Tx/Rx FIFO clear */
	uart->UFCON = 0x07;
	uart->UMCON = 0x0;
	/* Normal,No parity,1 stop,8 bit */
	uart->ULCON = 0x3;
	/*
	 * tx=level,rx=edge,disable timeout int.,enable rx error int.,
	 * normal,interrupt or polling
	 */
	uart->UCON = 0x245;
	uart->UBRDIV = reg;
	
	return 1;
}

static int do_testhw_serial(int argc, char* argv[])
{
	S3C24X0_UART* uart;
	int i, timeout;
	char b, a = 0x0;


	/* configure gpio RTS CTS Port B and C */
	GPHCON_REG &= ~(0xf << 20 | 0xfff0);
	GPHCON_REG |= 0x00a0faa0; 	
	
	for (i = 1; i < 3; i++) {
		uart = S3C24X0_GetBase_UART(i);

		a = 0x0;
		init_uart(uart);

		/* test RTS signal */
		uart->UMCON = 0x0;	/* RTS down */
		timeout = 1000;
		while (!(uart->UMSTAT & 0x1)&& timeout) {
			udelay(10);
			timeout--;
		}
		if(timeout) {
			printf("Error on serial port %d rts/cts does"
				" not work correct\n", i);
			return 0;
		}	
		uart->UMCON = 0x1;	/* RTS up */
		
		while(a != 0xFE) {
			test_putc(uart, a);
			b = (char)test_getc(uart);
			if(b != a) {
				printf("Error on serial port %d expected"
					" 0x%x, but getting 0x%x\n", i, a, b);
				return 0;
			}
			a++;
		}
	}
	
	return 1;
}

/* ********** Test command implemented ********** */

TESTHW_CMD( serial, "Tests serial ports B and C with loopback\n"
		"Jumper J4 and J5 has to be set to 1-2 to enable rts/cts port C.");

#endif

