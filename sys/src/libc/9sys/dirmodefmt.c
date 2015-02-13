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
#include <fcall.h>

static int8_t *modes[] =
{
	"---",
	"--x",
	"-w-",
	"-wx",
	"r--",
	"r-x",
	"rw-",
	"rwx",
};

static void
rwx(int32_t m, int8_t *s)
{
	strncpy(s, modes[m], 3);
}

int
dirmodefmt(Fmt *f)
{
	static int8_t buf[16];
	uint32_t m;

	m = va_arg(f->args, uint32_t);

	if(m & DMDIR)
		buf[0]='d';
	else if(m & DMAPPEND)
		buf[0]='a';
	else if(m & DMAUTH)
		buf[0]='A';
	else
		buf[0]='-';
	if(m & DMEXCL)
		buf[1]='l';
	else
		buf[1]='-';
	rwx((m>>6)&7, buf+2);
	rwx((m>>3)&7, buf+5);
	rwx((m>>0)&7, buf+8);
	buf[11] = 0;
	return fmtstrcpy(f, buf);
}
