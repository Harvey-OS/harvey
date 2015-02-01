/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
usage(void)
{
	fprint(2, "usage: readifile file\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	IFile ifile;

	ARGBEGIN{
	default:
		usage();
	}ARGEND
	
	if(argc != 1)
		usage();
	
	if(readifile(&ifile, argv[0]) < 0)
		sysfatal("readifile %s: %r", argv[0]);
	write(1, ifile.b->data, ifile.b->len);
	threadexitsall(nil);
}
