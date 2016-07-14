/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "flashfs.h"

char*	prog;
uint32_t	sectsize;
uint32_t	nsects;
uint8_t*	sectbuff;
int	readonly;
uint32_t	delta;
int	eparity;

uint8_t	magic[]	= { MAGIC0, MAGIC1, MAGIC2, FFSVERS };

int
putc3(uint8_t *buff, uint32_t v)
{
	if(v < (1 << 7)) {
		buff[0] = v;
		return 1;
	}

	if(v < (1 << 13)) {
		buff[0] = 0x80 | (v >> 7);
		buff[1] = v & ((1 << 7) - 1);
		return 2;
	}

	if(v < (1 << 21)) {
		buff[0] = 0x80 | (v >> 15);
		buff[1] = 0x80 | ((v >> 8) & ((1 << 7) - 1));
		buff[2] = v;
		return 3;
	}

	fprint(2, "%s: putc3 fail 0x%lx, called from %#p\n", prog, v, getcallerpc());
	abort();
	return -1;
}

int
getc3(uint8_t *buff, uint32_t *p)
{
	int c, d;

	c = buff[0];
	if((c & 0x80) == 0) {
		*p = c;
		return 1;
	}

	c &= ~0x80;
	d = buff[1];
	if((d & 0x80) == 0) {
		*p = (c << 7) | d;
		return 2;
	}

	d &= ~0x80;
	*p = (c << 15) | (d << 8) | buff[2];
	return 3;
}

uint32_t
get4(uint8_t *b)
{
	return	(b[0] <<  0) |
		(b[1] <<  8) |
		(b[2] << 16) |
		(b[3] << 24);
}

void
put4(uint8_t *b, uint32_t v)
{
	b[0] = v >>  0;
	b[1] = v >>  8;
	b[2] = v >> 16;
	b[3] = v >> 24;
}
