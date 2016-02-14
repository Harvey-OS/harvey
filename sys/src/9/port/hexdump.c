/*
 * Copyright 2013 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

static int isprint(int c)
{
	return (c >= 32 && c <= 126);
}

void hexdump(void *v, int length)
{
	int i;
	uint8_t *m = v;
	uintptr_t memory = (uintptr_t) v;
	int all_zero = 0;

	for (i = 0; i < length; i += 16) {
		int j;

		all_zero++;
		for (j = 0; (j < 16) && (i + j < length); j++) {
			if (m[i + j] != 0) {
				all_zero = 0;
				break;
			}
		}

		if (all_zero < 2) {
			iprint("%p:", (void *)(memory + i));
			for (j = 0; j < 16; j++)
				iprint(" %02x", m[i + j]);
			iprint("  ");
			for (j = 0; j < 16; j++)
				iprint("%c", isprint(m[i + j]) ? m[i + j] : '.');
			iprint("\n");
		} else if (all_zero == 2) {
			iprint("...\n");
		}
	}
}

void pahexdump(uintptr_t pa, int len)
{
	void *v = KADDR(pa);
	hexdump(v, len);
}

/* Print a string, with printables preserved, and \xxx where not possible. */
int printdump(char *buf, int buflen, uint8_t *data)
{
	int ret = 0;
	int ix = 0;
	while (ret < buflen) {
		if (isprint(data[ix])) {
			buf[ret++] = data[ix];
		} else if (ret < buflen - 4) {
			/* guarantee there is room for a \xxx sequence */
			ret += snprint(&buf[ret], buflen-ret, "\\%03o", data[ix]);
		} else {
			break;
		}
		ix++;
	}
	return ret;
}
