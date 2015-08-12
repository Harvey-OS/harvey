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
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

static void
usage(void)
{
	fprint(2, "usage: %s [-n nsect] [-z sectsize] file\n", argv0);
	exits("usage");
}

static uint32_t
argval(char *arg)
{
	int32_t v;
	char *extra;

	if(arg == nil)
		usage();
	v = strtol(arg, &extra, 0);
	if(*extra || v <= 0)
		usage();
	return v;
}

void
main(int argc, char **argv)
{
	uint32_t i;
	int m, n;
	char *file;
	unsigned char hdr[MAXHDR];

	ARGBEGIN
	{
	case 'n':
		nsects = argval(ARGF());
		break;
	case 'z':
		sectsize = argval(ARGF());
		break;
	default:
		usage();
	}
	ARGEND

	if(argc != 1)
		usage();
	file = argv[0];

	sectbuff = emalloc9p(sectsize);
	initdata(file, 1);

	memmove(hdr, magic, MAGSIZE);
	m = putc3(&hdr[MAGSIZE], 0);
	n = putc3(&hdr[MAGSIZE + m], 0);
	clearsect(0);
	writedata(0, 0, hdr, MAGSIZE + m + n, 0);

	for(i = 1; i < nsects - 1; i++)
		clearsect(i);

	m = putc3(&hdr[MAGSIZE], 1);
	n = putc3(&hdr[MAGSIZE + m], 0);
	clearsect(nsects - 1);
	writedata(0, nsects - 1, hdr, MAGSIZE + m + n, 0);
}
