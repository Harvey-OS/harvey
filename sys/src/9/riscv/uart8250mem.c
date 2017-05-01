/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2003 Eric Biederman
 * Copyright (C) 2006-2010 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

extern void *uart;

void udelay(int d)
{
	volatile int i;
	for(i = 0; i < 1*d; i++)
		;
}

// Note: this was a separate file in coreboot. The Plan 9 convention
// is to keep constants in the files that need them. This avoids the
// proliferation of files and their use seen in other projects.

/* Data */
#define UART8250_RBR 0x00
#define UART8250_TBR 0x00

/* Control */
#define UART8250_IER 0x01
#define   UART8250_IER_MSI	0x08 /* Enable Modem status interrupt */
#define   UART8250_IER_RLSI	0x04 /* Enable receiver line status interrupt */
#define   UART8250_IER_THRI	0x02 /* Enable Transmitter holding register int. */
#define   UART8250_IER_RDI	0x01 /* Enable receiver data interrupt */

#define UART8250_IIR 0x02
#define   UART8250_IIR_NO_INT	0x01 /* No interrupts pending */
#define   UART8250_IIR_ID	0x06 /* Mask for the interrupt ID */

#define   UART8250_IIR_MSI	0x00 /* Modem status interrupt */
#define   UART8250_IIR_THRI	0x02 /* Transmitter holding register empty */
#define   UART8250_IIR_RDI	0x04 /* Receiver data interrupt */
#define   UART8250_IIR_RLSI	0x06 /* Receiver line status interrupt */

#define UART8250_FCR 0x02
#define   UART8250_FCR_FIFO_EN		0x01 /* Fifo enable */
#define   UART8250_FCR_CLEAR_RCVR	0x02 /* Clear the RCVR FIFO */
#define   UART8250_FCR_CLEAR_XMIT	0x04 /* Clear the XMIT FIFO */
#define   UART8250_FCR_DMA_SELECT	0x08 /* For DMA applications */
#define   UART8250_FCR_TRIGGER_MASK	0xC0 /* Mask for the FIFO trigger range */
#define   UART8250_FCR_TRIGGER_1	0x00 /* Mask for trigger set at 1 */
#define   UART8250_FCR_TRIGGER_4	0x40 /* Mask for trigger set at 4 */
#define   UART8250_FCR_TRIGGER_8	0x80 /* Mask for trigger set at 8 */
#define   UART8250_FCR_TRIGGER_14	0xC0 /* Mask for trigger set at 14 */

#define   UART8250_FCR_RXSR		0x02 /* Receiver soft reset */
#define   UART8250_FCR_TXSR		0x04 /* Transmitter soft reset */

#define UART8250_LCR 0x03
#define   UART8250_LCR_WLS_MSK	0x03 /* character length select mask */
#define   UART8250_LCR_WLS_5	0x00 /* 5 bit character length */
#define   UART8250_LCR_WLS_6	0x01 /* 6 bit character length */
#define   UART8250_LCR_WLS_7	0x02 /* 7 bit character length */
#define   UART8250_LCR_WLS_8	0x03 /* 8 bit character length */
#define   UART8250_LCR_STB	0x04 /* Number of stop Bits, off = 1, on = 1.5 or 2) */
#define   UART8250_LCR_PEN	0x08 /* Parity enable */
#define   UART8250_LCR_EPS	0x10 /* Even Parity Select */
#define   UART8250_LCR_STKP	0x20 /* Stick Parity */
#define   UART8250_LCR_SBRK	0x40 /* Set Break */
#define   UART8250_LCR_BKSE	0x80 /* Bank select enable */
#define   UART8250_LCR_DLAB	0x80 /* Divisor latch access bit */

#define UART8250_MCR 0x04
#define   UART8250_MCR_DTR	0x01 /* DTR   */
#define   UART8250_MCR_RTS	0x02 /* RTS   */
#define   UART8250_MCR_OUT1	0x04 /* Out 1 */
#define   UART8250_MCR_OUT2	0x08 /* Out 2 */
#define   UART8250_MCR_LOOP	0x10 /* Enable loopback test mode */

#define UART8250_MCR_DMA_EN	0x04
#define UART8250_MCR_TX_DFR	0x08

#define UART8250_DLL 0x00
#define UART8250_DLM 0x01

/* Status */
#define UART8250_LSR 0x05
#define   UART8250_LSR_DR	0x01 /* Data ready */
#define   UART8250_LSR_OE	0x02 /* Overrun */
#define   UART8250_LSR_PE	0x04 /* Parity error */
#define   UART8250_LSR_FE	0x08 /* Framing error */
#define   UART8250_LSR_BI	0x10 /* Break */
#define   UART8250_LSR_THRE	0x20 /* Xmit holding register empty */
#define   UART8250_LSR_TEMT	0x40 /* Xmitter empty */
#define   UART8250_LSR_ERR	0x80 /* Error */

#define UART8250_MSR 0x06
#define   UART8250_MSR_DCD	0x80 /* Data Carrier Detect */
#define   UART8250_MSR_RI	0x40 /* Ring Indicator */
#define   UART8250_MSR_DSR	0x20 /* Data Set Ready */
#define   UART8250_MSR_CTS	0x10 /* Clear to Send */
#define   UART8250_MSR_DDCD	0x08 /* Delta DCD */
#define   UART8250_MSR_TERI	0x04 /* Trailing edge ring indicator */
#define   UART8250_MSR_DDSR	0x02 /* Delta DSR */
#define   UART8250_MSR_DCTS	0x01 /* Delta CTS */

#define UART8250_SCR 0x07
#define UART8250_SPR 0x07

/* Should support 8250, 16450, 16550, 16550A type UARTs */

/* Expected character delay at 1200bps is 9ms for a working UART
 * and no flow-control. Assume UART as stuck if shift register
 * or FIFO takes more than 50ms per character to appear empty.
 */
#define SINGLE_CHAR_TIMEOUT	(50 * 1000)
#define FIFO_TIMEOUT		(16 * SINGLE_CHAR_TIMEOUT)

/* Other essential constants. TODO: make these all plan 9 style */
#define CONFIG_TTYS0_LCS 0x3

// These functions are here as we may need to get tricky
// for riscv with memory barriers and such.
uint8_t read8(uint32_t *p)
{
	return *p;
}

void write8(uint32_t *p, uint8_t val)
{
	*p = val;
}

static uint8_t uart8250_read(uint32_t *base, uint8_t reg)
{
	return read8(base + reg);
}

static void uart8250_write(uint32_t *base, uint8_t reg, uint8_t data)
{
	write8(base + reg, data);
}

static int uart8250_mem_can_tx_byte(void *base)
{
	return uart8250_read(base, UART8250_LSR) & UART8250_LSR_THRE;
}

static void uart8250_mem_tx_byte(void *base, unsigned char data)
{
	unsigned long int i = SINGLE_CHAR_TIMEOUT;
	while (i-- && !uart8250_mem_can_tx_byte(base))
		udelay(1);
	uart8250_write(base, UART8250_TBR, data);
}

static void uart8250_mem_tx_flush(void *base)
{
	unsigned long int i = FIFO_TIMEOUT;
	while (i-- && !(uart8250_read(base, UART8250_LSR) & UART8250_LSR_TEMT))
		udelay(1);
}

static int uart8250_mem_can_rx_byte(void *base)
{
	return uart8250_read(base, UART8250_LSR) & UART8250_LSR_DR;
}

static int uart8250_mem_rx_byte(void *base)
{
	unsigned long int i = SINGLE_CHAR_TIMEOUT;
	uint8_t c;
	while (i-- && !uart8250_mem_can_rx_byte(base))
		udelay(1);
	if (0) print("rx_byte, register is 0x%x LSR 0x%x\n", uart8250_read(base, UART8250_LSR) , UART8250_LSR_DR);
	if (i) {
		c = uart8250_read(base, UART8250_RBR);
		if (0) print("c is 0x%x\n", c);
		return  c;
	}
	else
		return -1;
}

static void uart8250_mem_init(void *base, unsigned divisor)
{
	/* Disable interrupts */
	uart8250_write(base, UART8250_IER, 0x0);
	/* Enable FIFOs */
	uart8250_write(base, UART8250_FCR, UART8250_FCR_FIFO_EN);

	/* Assert DTR and RTS so the other end is happy */
	uart8250_write(base, UART8250_MCR, UART8250_MCR_DTR | UART8250_MCR_RTS);

	/* Divisor Latch Access Bit -- DLAB -- on */
	uart8250_write(base, UART8250_LCR, UART8250_LCR_DLAB | CONFIG_TTYS0_LCS);

	uart8250_write(base, UART8250_DLL, divisor & 0xFF);
	uart8250_write(base, UART8250_DLM, (divisor >> 8) & 0xFF);

	/* Set to 3 for 8N1 */
	uart8250_write(base, UART8250_LCR, CONFIG_TTYS0_LCS);
}

// We don't use idx at present but it's a reasonable API to preserve
// should someone ever add more than one uart.

static void *uart_platform_baseptr(int idx)
{
	return uart;
}

/* Calculate divisor. Do not floor but round to nearest integer. */
static unsigned int uart_baudrate_divisor(unsigned int baudrate,
	unsigned int refclk, unsigned int oversample)
{
	return (1 + (2 * refclk) / (baudrate * oversample)) / 2;
}

static unsigned int uart_platform_refclk(void)
{
	return 25 * 1000 * 1000;
}

static unsigned int uart_input_clock_divider(void)
{
	/* Specify the default oversample rate for the UART.
	 *
	 * UARTs oversample the receive data.  The UART's input clock first
	 * enters the baud-rate divider to generate the oversample clock.  Then
	 * the UART typically divides the result by 16.  The asynchronous
	 * receive data is synchronized with the oversample clock and when a
	 * start bit is detected the UART delays half a bit time using the
	 * oversample clock.  Samples are then taken to verify the start bit and
	 * if present, samples are taken for the rest of the frame.
	 */
	return 16;
}

void lowrisc_uart_init(int idx)
{
	void *base = uart_platform_baseptr(idx);
	if (!base)
		return;

	unsigned int div;
	div = uart_baudrate_divisor(115200,
				    uart_platform_refclk(), uart_input_clock_divider());
	uart8250_mem_init(base, div);
}

void lowrisc_putchar(int data)
{
	void *base = uart_platform_baseptr(0);
	if (!base)
		return;
	uart8250_mem_tx_byte(base, data);
}

int lowrisc_getchar(int idx)
{
	void *base = uart_platform_baseptr(idx);
	if (!base)
		return -1;
	return uart8250_mem_rx_byte(base);
}

void lowrisc_flush(int idx)
{
	void *base = uart_platform_baseptr(idx);
	if (!base)
		return;
	uart8250_mem_tx_flush(base);
}
