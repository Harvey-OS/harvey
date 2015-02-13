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

static int spinning;
static int8_t* wheel[4] = { "\b|", "\b/", "\b-", "\b\\", };
static int spoke;

void
startwheel(void)
{
	spoke = 1;
	consputs("|", 1);
	spinning = 1;
}

void
turnwheel(int dir)
{
	if(!spinning)
		return;
	spoke += dir;
	consputs(wheel[spoke & 3], 2);
}

void
stopwheel(void)
{
	consputs("\b", 1);
	spinning = 0;
}

int
print(int8_t* fmt, ...)
{
	va_list arg;
	int8_t buf[PRINTSIZE], *e, *p;

	p = buf;
	if(spinning)
		*p++ = '\b';

	va_start(arg, fmt);
	e = vseprint(p, buf+sizeof(buf), fmt, arg);
	va_end(arg);

	if(spinning && e < &buf[PRINTSIZE-2]){
		*e++ = ' ';
		*e = 0;
	}
	consputs(buf, e-buf);

	return e-buf;
}

static Lock fmtl;

void
_fmtlock(void)
{
	lock(&fmtl);
}

void
_fmtunlock(void)
{
	unlock(&fmtl);
}

int
_efgfmt(Fmt*)
{
	return -1;
}
