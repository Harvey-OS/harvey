/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

int
qgetc(IOQ* q)
{
	int c;

	if(q->in == q->out)
		return -1;
	c = *q->out;
	if(q->out == q->buf + sizeof(q->buf) - 1)
		q->out = q->buf;
	else
		q->out++;
	return c;
}

static int
qputc(IOQ* q, int c)
{
	uint8_t* nextin;
	if(q->in >= &q->buf[sizeof(q->buf) - 1])
		nextin = q->buf;
	else
		nextin = q->in + 1;
	if(nextin == q->out)
		return -1;
	*q->in = c;
	q->in = nextin;
	return 0;
}

void
qinit(IOQ* q)
{
	q->in = q->out = q->buf;
	q->getc = qgetc;
	q->putc = qputc;
}
