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

uintptr_t uart_platform_base(int idx)
{
	return (uintptr_t) 0x40001000;
}

void putchar(uint8_t c)
{
	uint8_t *cp = KADDR(uart_platform_base(0));
	*cp = c;
}

// Get a 7-bit char. < 0 means err.
int getchar(void)
{
	uint8_t *cp = KADDR(uart_platform_base(0));
	if (cp[5] & 1) {
		int c = cp[0];
		if (0) print("getchar: got 0x%x\n", c);
		return c;
	}
	return -1;
}
	

