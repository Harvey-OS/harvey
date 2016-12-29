/*
 * This file is part of the coreboot project.
 *
 * Copyright 2014 Google Inc.
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

void lowrisc_uart_init(int idx);
void lowrisc_putchar(int idx, int data);
unsigned char lowrisc_getchar(int idx);
void lowrisc_flush(int idx);

// riscv has no standard uart. Further, if we use a bad address,
// the hardware explodes. So, the uart, if 0, is ignored.
// At some point it will be ready.

extern uint8_t *uart;
extern uintptr_t uartpa;

static void spike_putchar(uint8_t c)
{
	*uart = c;
}

// Get a 7-bit char. < 0 means err.
static int spike_getchar(void)
{
	if (uart[5] & 1) {
		int c = uart[0];
		if (0) print("getchar: got 0x%x\n", c);
		return c;
	}
	return -1;
}

void uart_init(void)
{
	switch(uartpa){
	case 0x42000000:
		lowrisc_uart_init(0);
	default:
		break;
	}
}

void putchar(uint8_t c)
{
	switch(uartpa){
	case 0x40001000:
		spike_putchar(c);
		break;
	case 0x42000000:
		//lowrisc_flush(0);
		lowrisc_putchar(0, c);
		*uart = c;
		break;
	default:
		break;
	}
}

// Get a 7-bit char. < 0 means err.
int getchar(void)
{
	switch(uartpa){
	case 0x40001000:
		return spike_getchar();
		break;
	case 0x42000000:
		return lowrisc_getchar(0);
		break;
	default:
		break;
	}
	return -1;
}


