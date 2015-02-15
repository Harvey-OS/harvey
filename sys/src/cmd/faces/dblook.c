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
#include <draw.h>
#include <plumb.h>
#include <regexp.h>
#include <bio.h>
#include "faces.h"

void
main(int argc, char **argv)
{
	Face f;
	char *q;

	if(argc != 3){
		fprint(2, "usage: dblook name domain\n");
		exits("usage");
	}

	q = findfile(&f, argv[2], argv[1]);
	print("%s\n", q);
}

void
killall(char *s)
{
	USED(s);
}
