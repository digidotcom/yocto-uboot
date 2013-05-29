/*
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
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
#include <serial.h>
#include <asm/arch/regs-uartdbg.h>
#include <asm/arch/regs-uartapp.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CCARDIMX28) || defined(CONFIG_CPX2) || defined(CONFIG_WR21)
extern void mxs_duart_gpios_init(void);
#endif
#ifdef CONFIG_SERIAL_MULTI
static int port_in_use = -1;   /* -1 indicates not initialized yet */
static uint32_t auart_baseaddr = 0; /* active uart base address */
extern void mxs_auart_gpios_init(int port);
/*
 * Set baud rate. The settings are always 8n1:
 * 8 data bits, no parity, 1 stop bit
 */
void auart_setbrg(void)
{
	u32 linectrl = 0;
	u32 quot;

	/* Calculate baudrate */
	quot = (CONFIG_UARTAPP_CLK * 32) / gd->baudrate;
	linectrl |= BF_UARTAPP_LINECTRL_BAUD_DIVFRAC(quot & 0x3f);
	linectrl |= BF_UARTAPP_LINECTRL_BAUD_DIVINT(quot >> 6);

	/* Set 8n1 mode, enable FIFOs */
	linectrl |= BF_UARTAPP_LINECTRL_WLEN(3) | BM_UARTAPP_LINECTRL_FEN;

	/* Write the new values */
	REG_WR(auart_baseaddr, HW_UARTAPP_LINECTRL, linectrl);
}

int auart_init(int port)
{
	/* Set base address for the selected port */
	switch (port) {
	case 0:         auart_baseaddr = REGS_UARTAPP0_BASE;    break;
	case 1:         auart_baseaddr = REGS_UARTAPP1_BASE;    break;
	case 2:         auart_baseaddr = REGS_UARTAPP2_BASE;    break;
	case 3:         auart_baseaddr = REGS_UARTAPP3_BASE;    break;
	case 4:         auart_baseaddr = REGS_UARTAPP4_BASE;    break;
	default:        return -1;
	}

	mxs_auart_gpios_init(port);

	/* Take the UART out of reset and enable clocks */
	REG_WR(auart_baseaddr, HW_UARTAPP_CTRL0, 0);

	/* Mask interrupts */
	REG_WR(auart_baseaddr, HW_UARTAPP_INTR, 0);

	/* Set default baudrate */
	auart_setbrg();

	/* Enable UART */
	REG_WR(auart_baseaddr, HW_UARTAPP_CTRL2,
		BM_UARTAPP_CTRL2_TXE | BM_UARTAPP_CTRL2_RXE | BM_UARTAPP_CTRL2_UARTEN);

	return 0;
}

int auart_deinit(int port)
{
	uint32_t addr;

	/* Set base address for the selected port */
	switch (port) {
	case 0:         addr = REGS_UARTAPP0_BASE;    break;
	case 1:         addr = REGS_UARTAPP1_BASE;    break;
	case 2:         addr = REGS_UARTAPP2_BASE;    break;
	case 3:         addr = REGS_UARTAPP3_BASE;    break;
	case 4:         addr = REGS_UARTAPP4_BASE;    break;
	default:        return -1;
	}

	/* Disable UART */
	REG_CLR(addr, HW_UARTAPP_CTRL2,
		BM_UARTAPP_CTRL2_TXE | BM_UARTAPP_CTRL2_RXE | BM_UARTAPP_CTRL2_UARTEN);

	return 0;
}

/* Send a character */
void auart_putc(const char c)
{
	/* Wait for room in TX FIFO */
	while (REG_RD(auart_baseaddr, HW_UARTAPP_STAT) & BM_UARTAPP_STAT_TXFF)
		;

	/* Write the data byte */
	REG_WR(auart_baseaddr, HW_UARTAPP_DATA, c);

	if (c == '\n')
		serial_putc('\r');
}

void auart_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

/* Test whether a character is in RX buffer */
int auart_tstc(void)
{
	/* Check if RX FIFO is not empty */
	return !(REG_RD(auart_baseaddr, HW_UARTAPP_STAT) & BM_UARTAPP_STAT_RXFE);
}

/* Receive character */
int auart_getc(void)
{
	/* Wait while RX FIFO is empty */
	while (REG_RD(auart_baseaddr, HW_UARTAPP_STAT) & BM_UARTAPP_STAT_RXFE)
		;

	/* Read data byte */
	return REG_RD(auart_baseaddr, HW_UARTAPP_DATA) & 0xff;
}

/* AUARTs */
/**
 * auart_start - starts the app uart
 */
static int auart_start(int port)
{
	int ret = -1;

	if ((port >= 0 ) && (port < MXS_MAX_AUARTS)) {
		/* switch to new console */
		if (port_in_use != -1 && port != port_in_use)
			auart_deinit(port_in_use);		/* not initialized yet */

		port_in_use = port;
		auart_init(port);
		ret = 0;
	} else
		printf("*** ERROR: Unsupported port %d\n", port);

	return ret;
}

#define AUART_PORT_INIT_FN(port) \
static int auart_start_##port(void) \
{ \
	auart_start(port); \
	return 0; \
}
AUART_PORT_INIT_FN(0)
AUART_PORT_INIT_FN(1)
AUART_PORT_INIT_FN(2)
AUART_PORT_INIT_FN(3)
AUART_PORT_INIT_FN(4)

#define DECLARE_AUART_PORT(port)                        \
       {                                                \
               .name           = "auart"#port,          \
               .init           = auart_start_##port,    \
               .setbrg         = auart_setbrg,          \
               .getc           = auart_getc,            \
               .tstc           = auart_tstc,            \
               .putc           = auart_putc,            \
               .puts           = auart_puts,            \
       }

struct serial_device serial_mxs_auart_devices[MXS_MAX_AUARTS] = {
	DECLARE_AUART_PORT(0),
	DECLARE_AUART_PORT(1),
	DECLARE_AUART_PORT(2),
	DECLARE_AUART_PORT(3),
	DECLARE_AUART_PORT(4)
};

#endif /* CONFIG_SERIAL_MULTI */

/*
 * Set baud rate. The settings are always 8n1:
 * 8 data bits, no parity, 1 stop bit
 */
void duart_setbrg(void)
{
	u32 cr;
	u32 quot;

	/* Disable everything */
	cr = REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGCR);
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, 0);

	/* Calculate and set baudrate */
	quot = (CONFIG_UARTDBG_CLK * 4)	/ gd->baudrate;
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGFBRD, quot & 0x3f);
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGIBRD, quot >> 6);

	/* Set 8n1 mode, enable FIFOs */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGLCR_H,
		BM_UARTDBGLCR_H_WLEN | BM_UARTDBGLCR_H_FEN);

	/* Enable Debug UART */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, cr);
}

int duart_init(void)
{
#if defined(CONFIG_CCARDIMX28) || defined(CONFIG_CPX2) || defined(CONFIG_WR21)
	mxs_duart_gpios_init();
#endif
	/* Disable UART */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR, 0);

	/* Mask interrupts */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGIMSC, 0);

	/* Set default baudrate */
	duart_setbrg();

	/* Enable UART */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGCR,
		BM_UARTDBGCR_TXE | BM_UARTDBGCR_RXE | BM_UARTDBGCR_UARTEN);

	return 0;
}

/* Send a character */
void duart_putc(const char c)
{
	/* Wait for room in TX FIFO */
	while (REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_TXFF)
		;

	/* Write the data byte */
	REG_WR(REGS_UARTDBG_BASE, HW_UARTDBGDR, c);

	if (c == '\n')
		serial_putc('\r');
}

void duart_puts(const char *s)
{
	while (*s)
		serial_putc(*s++);
}

/* Test whether a character is in TX buffer */
int duart_tstc(void)
{
	/* Check if RX FIFO is not empty */
	return !(REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_RXFE);
}

/* Receive character */
int duart_getc(void)
{
	/* Wait while TX FIFO is empty */
	while (REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGFR) & BM_UARTDBGFR_RXFE)
		;

	/* Read data byte */
	return REG_RD(REGS_UARTDBG_BASE, HW_UARTDBGDR) & 0xff;
}

/* DUART */
struct serial_device serial_mxs_duart_device = {
	.name           = "duart",
	.init           = duart_init,
	.setbrg         = duart_setbrg,
	.getc           = duart_getc,
	.tstc           = duart_tstc,
	.putc           = duart_putc,
	.puts           = duart_puts,
};

/* If CONFIG_SERIAL_MULTI not defined, use DUART by default */
#ifndef CONFIG_SERIAL_MULTI
int serial_init(void)		{return duart_init();}
void serial_setbrg(void)	{duart_setbrg();}
int serial_getc(void)		{return duart_getc();}
int serial_tstc(void)		{return duart_tstc();}
void serial_putc(const char c)	{duart_putc(c);}
void serial_puts(const char *s)	{duart_puts(s);}
#endif
