/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Linux and BSD
 */
#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"devaudio.h"

/* maybe this should return -1 instead of sysfatal */
void
audiodevopen(void)
{
	error("no audio support");
}

void
audiodevclose(void)
{
	error("no audio support");
}

int
audiodevread(void *a, int n)
{
	error("no audio support");
	return -1;
}

int
audiodevwrite(void *a, int n)
{
	error("no audio support");
	return -1;
}

void
audiodevsetvol(int what, int left, int right)
{
	error("no audio support");
}

void
audiodevgetvol(int what, int *left, int *right)
{
	error("no audio support");
}

