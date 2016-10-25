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

#include <u.h>

uintptr_t uart_platform_base(int idx)
{
	return (uintptr_t) 0x40001000;
}

void testPrint(uint8_t c)
{
	uint8_t *cp = (void *)uart_platform_base(0);
	*cp = c;
}
