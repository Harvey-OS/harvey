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
	fprint(2, "usage: %s -n nsect -z sectsize -f file\n", prog);
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
	char *file;

	prog = "testldfs";
	file = nil;

	ARGBEGIN {
	case 'n':
		nsects = argval(ARGF());
		break;
	case 'z':
		sectsize = argval(ARGF());
		break;
	case 'f':
		file = ARGF();
		break;
	default:
		usage();
	} ARGEND

	if(argc != 0 || nsects == 0 || sectsize == 0 || file == nil)
		usage();

	if(nsects < 8) {
		fprint(2, "%s: unreasonable value for nsects: %lu\n", prog, nsects);
		exits("nsects");
	}

	if(sectsize < 512) {
		fprint(2, "%s: unreasonable value for sectsize: %lu\n", prog, sectsize);
		exits("sectsize");
	}

	sectbuff = emalloc9p(sectsize);
	initdata(file, 0);
	einit();
	loadfs(1);
}
