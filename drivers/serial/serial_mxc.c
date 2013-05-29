/*
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2009 Digi International Inc.
 *       -Added multiport support (Pedro Perez de Heredia)
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

/* Register definitions */
#define URXD  0x0  /* Receiver Register */
#define UTXD  0x40 /* Transmitter Register */
#define UCR1  0x80 /* Control Register 1 */
#define UCR2  0x84 /* Control Register 2 */
#define UCR3  0x88 /* Control Register 3 */
#define UCR4  0x8c /* Control Register 4 */
#define UFCR  0x90 /* FIFO Control Register */
#define USR1  0x94 /* Status Register 1 */
#define USR2  0x98 /* Status Register 2 */
#define UESC  0x9c /* Escape Character Register */
#define UTIM  0xa0 /* Escape Timer Register */
#define UBIR  0xa4 /* BRM Incremental Register */
#define UBMR  0xa8 /* BRM Modulator Register */
#define UBRC  0xac /* Baud Rate Count Register */
#define UTS   0xb4 /* UART Test Register (mx31) */

/* UART Control Register Bit Fields.*/
#define  URXD_CHARRDY    (1<<15)
#define  URXD_ERR        (1<<14)
#define  URXD_OVRRUN     (1<<13)
#define  URXD_FRMERR     (1<<12)
#define  URXD_BRK        (1<<11)
#define  URXD_PRERR      (1<<10)
#define  URXD_RX_DATA    (0xFF)
#define  UCR1_ADEN       (1<<15) /* Auto dectect interrupt */
#define  UCR1_ADBR       (1<<14) /* Auto detect baud rate */
#define  UCR1_TRDYEN     (1<<13) /* Transmitter ready interrupt enable */
#define  UCR1_IDEN       (1<<12) /* Idle condition interrupt */
#define  UCR1_RRDYEN     (1<<9)	 /* Recv ready interrupt enable */
#define  UCR1_RDMAEN     (1<<8)	 /* Recv ready DMA enable */
#define  UCR1_IREN       (1<<7)	 /* Infrared interface enable */
#define  UCR1_TXMPTYEN   (1<<6)	 /* Transimitter empty interrupt enable */
#define  UCR1_RTSDEN     (1<<5)	 /* RTS delta interrupt enable */
#define  UCR1_SNDBRK     (1<<4)	 /* Send break */
#define  UCR1_TDMAEN     (1<<3)	 /* Transmitter ready DMA enable */
#define  UCR1_UARTCLKEN  (1<<2)	 /* UART clock enabled */
#define  UCR1_DOZE       (1<<1)	 /* Doze */
#define  UCR1_UARTEN     (1<<0)	 /* UART enabled */
#define  UCR2_ESCI	 (1<<15) /* Escape seq interrupt enable */
#define  UCR2_IRTS	 (1<<14) /* Ignore RTS pin */
#define  UCR2_CTSC	 (1<<13) /* CTS pin control */
#define  UCR2_CTS        (1<<12) /* Clear to send */
#define  UCR2_ESCEN      (1<<11) /* Escape enable */
#define  UCR2_PREN       (1<<8)  /* Parity enable */
#define  UCR2_PROE       (1<<7)  /* Parity odd/even */
#define  UCR2_STPB       (1<<6)	 /* Stop */
#define  UCR2_WS         (1<<5)	 /* Word size */
#define  UCR2_RTSEN      (1<<4)	 /* Request to send interrupt enable */
#define  UCR2_TXEN       (1<<2)	 /* Transmitter enabled */
#define  UCR2_RXEN       (1<<1)	 /* Receiver enabled */
#define  UCR2_SRST	 (1<<0)	 /* SW reset */
#define  UCR3_DTREN	 (1<<13) /* DTR interrupt enable */
#define  UCR3_PARERREN   (1<<12) /* Parity enable */
#define  UCR3_FRAERREN   (1<<11) /* Frame error interrupt enable */
#define  UCR3_DSR        (1<<10) /* Data set ready */
#define  UCR3_DCD        (1<<9)  /* Data carrier detect */
#define  UCR3_RI         (1<<8)  /* Ring indicator */
#define  UCR3_TIMEOUTEN  (1<<7)  /* Timeout interrupt enable */
#define  UCR3_RXDSEN	 (1<<6)  /* Receive status interrupt enable */
#define  UCR3_AIRINTEN   (1<<5)  /* Async IR wake interrupt enable */
#define  UCR3_AWAKEN	 (1<<4)  /* Async wake interrupt enable */
#define  UCR3_REF25	 (1<<3)  /* Ref freq 25 MHz */
#define  UCR3_REF30	 (1<<2)  /* Ref Freq 30 MHz */
#define  UCR3_INVT	 (1<<1)  /* Inverted Infrared transmission */
#define  UCR3_BPEN	 (1<<0)  /* Preset registers enable */
#define  UCR4_CTSTL_32   (32<<10) /* CTS trigger level (32 chars) */
#define  UCR4_INVR	 (1<<9)  /* Inverted infrared reception */
#define  UCR4_ENIRI	 (1<<8)  /* Serial infrared interrupt enable */
#define  UCR4_WKEN	 (1<<7)  /* Wake interrupt enable */
#define  UCR4_REF16	 (1<<6)  /* Ref freq 16 MHz */
#define  UCR4_IRSC	 (1<<5)  /* IR special case */
#define  UCR4_TCEN	 (1<<3)  /* Transmit complete interrupt enable */
#define  UCR4_BKEN	 (1<<2)  /* Break condition interrupt enable */
#define  UCR4_OREN	 (1<<1)  /* Receiver overrun interrupt enable */
#define  UCR4_DREN	 (1<<0)  /* Recv data ready interrupt enable */
#define  UFCR_RXTL_SHF   0       /* Receiver trigger level shift */
#define  UFCR_RFDIV      (7<<7)  /* Reference freq divider mask */
#define  UFCR_TXTL_SHF   10      /* Transmitter trigger level shift */
#define  USR1_PARITYERR  (1<<15) /* Parity error interrupt flag */
#define  USR1_RTSS	 (1<<14) /* RTS pin status */
#define  USR1_TRDY	 (1<<13) /* Transmitter ready interrupt/dma flag */
#define  USR1_RTSD	 (1<<12) /* RTS delta */
#define  USR1_ESCF	 (1<<11) /* Escape seq interrupt flag */
#define  USR1_FRAMERR    (1<<10) /* Frame error interrupt flag */
#define  USR1_RRDY       (1<<9)	 /* Receiver ready interrupt/dma flag */
#define  USR1_TIMEOUT    (1<<7)	 /* Receive timeout interrupt status */
#define  USR1_RXDS	 (1<<6)	 /* Receiver idle interrupt flag */
#define  USR1_AIRINT	 (1<<5)	 /* Async IR wake interrupt flag */
#define  USR1_AWAKE	 (1<<4)	 /* Aysnc wake interrupt flag */
#define  USR2_ADET	 (1<<15) /* Auto baud rate detect complete */
#define  USR2_TXFE	 (1<<14) /* Transmit buffer FIFO empty */
#define  USR2_DTRF	 (1<<13) /* DTR edge interrupt flag */
#define  USR2_IDLE	 (1<<12) /* Idle condition */
#define  USR2_IRINT	 (1<<8)	 /* Serial infrared interrupt flag */
#define  USR2_WAKE	 (1<<7)	 /* Wake */
#define  USR2_RTSF	 (1<<4)	 /* RTS edge interrupt flag */
#define  USR2_TXDC	 (1<<3)	 /* Transmitter complete */
#define  USR2_BRCD	 (1<<2)	 /* Break condition */
#define  USR2_ORE        (1<<1)	 /* Overrun error */
#define  USR2_RDR        (1<<0)	 /* Recv data ready */
#define  UTS_FRCPERR	 (1<<13) /* Force parity error */
#define  UTS_LOOP        (1<<12) /* Loop tx and rx */
#define  UTS_TXEMPTY	 (1<<6)	 /* TxFIFO empty */
#define  UTS_RXEMPTY	 (1<<5)	 /* RxFIFO empty */
#define  UTS_TXFULL	 (1<<4)	 /* TxFIFO full */
#define  UTS_RXFULL	 (1<<3)	 /* RxFIFO full */
#define  UTS_SOFTRST	 (1<<0)	 /* Software reset */

static int port_in_use = -1;   /* -1 indicates not initialized yet */
static uint32_t uart_addr = 0; /* active uart base address */

extern void mxc_serial_gpios_init(int port);


DECLARE_GLOBAL_DATA_PTR;

void mxc_serial_setbrg (void)
{
	u32 clk = mxc_get_clock(MXC_UART_CLK);

	if (!gd->baudrate)
		gd->baudrate = CONFIG_BAUDRATE;

	__REG(uart_addr + UFCR) = 4 << 7; /* divide input clock by 2 */
	__REG(uart_addr + UBIR) = 0xf;
	__REG(uart_addr + UBMR) = clk / (2 * gd->baudrate);

}

int mxc_serial_getc (void)
{
	while (__REG(uart_addr + UTS) & UTS_RXEMPTY);
	return (__REG(uart_addr + URXD) & URXD_RX_DATA); /* mask out status from upper word */
}

#ifdef DEBUG_SERIAL
void debug_serial_regs(void)
{
	printf("UCR1 = 0x%08x\n", __REG(uart_addr + UCR1));
	printf("UCR2 = 0x%08x\n", __REG(uart_addr + UCR2));
	printf("UCR3 = 0x%08x\n", __REG(uart_addr + UCR3));
	printf("UCR4 = 0x%08x\n", __REG(uart_addr + UCR4));
	printf("UFCR = 0x%08x\n", __REG(uart_addr + UFCR));
	printf("USR1 = 0x%08x\n", __REG(uart_addr + USR1));
	printf("USR2 = 0x%08x\n", __REG(uart_addr + USR2));
	printf("UESC = 0x%08x\n", __REG(uart_addr + UESC));
	printf("UTIM = 0x%08x\n", __REG(uart_addr + UTIM));
	printf("UBIR = 0x%08x\n", __REG(uart_addr + UBIR));
	printf("UBMR = 0x%08x\n", __REG(uart_addr + UBMR));
	printf("UBRC = 0x%08x\n", __REG(uart_addr + UBRC));
	printf("UTS  = 0x%08x\n", __REG(uart_addr + UTS));
	printf("\n");
}
#endif

void mxc_serial_putc (const char c)
{
	__REG(uart_addr + UTXD) = c;

	/* wait for transmitter to be ready */
	while(!(__REG(uart_addr + UTS) & UTS_TXEMPTY));

	/* If \n, also do \r */
	if (c == '\n')
		mxc_serial_putc ('\r');
}

/*
 * Test whether a character is in the RX buffer
 */
int mxc_serial_tstc (void)
{
	/* If receive fifo is empty, return false */
	if (__REG(uart_addr + UTS) & UTS_RXEMPTY)
		return 0;
	return 1;
}

void mxc_serial_puts (const char *s)
{
	while (*s) {
		mxc_serial_putc (*s++);
	}
}

void mxc_serial_gpios_deinit(void)
{
	/* Set gpios as inputs? */
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
int mxc_serial_init (void)
{
	/* Set base address for the selected port */
	switch (port_in_use) {
	case 0:         uart_addr = UART1_BASE_ADDR;    break;
	case 1:         uart_addr = UART2_BASE_ADDR;    break;
	case 2:         uart_addr = UART3_BASE_ADDR;    break;
#ifdef CONFIG_MX53
	case 3:         uart_addr = UART4_BASE_ADDR;    break;
	case 4:         uart_addr = UART5_BASE_ADDR;    break;
#endif
	default:        return -1;
	}

	mxc_serial_gpios_init(port_in_use);

	__REG(uart_addr + UCR1) = 0x0;
	__REG(uart_addr + UCR2) = 0x0;

	while (!(__REG(uart_addr + UCR2) & UCR2_SRST));

	__REG(uart_addr + UCR3) = 0x0704;
	__REG(uart_addr + UCR4) = 0x8000;
	__REG(uart_addr + UESC) = 0x002b;
	__REG(uart_addr + UTIM) = 0x0;

	__REG(uart_addr + UTS) = 0x0;

	mxc_serial_setbrg();

	__REG(uart_addr + UCR2) = UCR2_WS | UCR2_IRTS | UCR2_RXEN | UCR2_TXEN | UCR2_SRST;

	__REG(uart_addr + UCR1) = UCR1_UARTEN;

	return 0;
}

/*
 * Unconfig gpios as special function and config to defaults.
 */
int mxc_serial_deinit(void)
{
	__REG(uart_addr + UCR1) = 0x0;
	__REG(uart_addr + UCR2) = 0x0;

	mxc_serial_gpios_deinit();

	return 0;
}


/**
 * serial_start - starts the console.
 */
static int mxc_serial_start(int port)
{
	int ret = -1;

	if ((port >= 0 ) && (port < MXC_MAX_UARTS)) {
		/* switch to new console */
		if (port_in_use != -1)
			mxc_serial_deinit();		/* not initialized yet */

		port_in_use = port;
		mxc_serial_init();
		ret = 0;
	} else
		printf("*** ERROR: Unsupported port %d\n", port);

	return ret;
}


/* some stuff to provide serial0...serial3 */
#define SERIAL_PORT_START_FN(port) \
static int mxc_serial_start_##port(void) \
{ \
       mxc_serial_start(port); \
       return 0; \
}
SERIAL_PORT_START_FN(0)
SERIAL_PORT_START_FN(1)
SERIAL_PORT_START_FN(2)
#ifdef CONFIG_MX53
SERIAL_PORT_START_FN(3)
SERIAL_PORT_START_FN(4)
#endif

#define DECLARE_SERIAL_PORT(port)                              \
       {                                                       \
               .name           = "serial"#port,                \
               .init           = mxc_serial_start_##port,    \
               .setbrg         = mxc_serial_setbrg,          \
               .getc           = mxc_serial_getc,            \
               .tstc           = mxc_serial_tstc,            \
               .putc           = mxc_serial_putc,            \
               .puts           = mxc_serial_puts,            \
       }

struct serial_device serial_mxc_devices[MXC_MAX_UARTS] = {
       DECLARE_SERIAL_PORT(0),
       DECLARE_SERIAL_PORT(1),
       DECLARE_SERIAL_PORT(2),
#ifdef CONFIG_MX53
       DECLARE_SERIAL_PORT(3),
       DECLARE_SERIAL_PORT(4)
#endif
};
