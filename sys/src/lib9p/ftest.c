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
#include "9p.h"

void
main(void)
{
	Tree *t;
	File *hello, *goodbye, *world;

	t = mktree();

	hello = fcreate(t->root, "hello", CHDIR|0777);
	assert(hello != nil);

	goodbye = fcreate(t->root, "goodbye", CHDIR|0777);
	assert(goodbye != nil);

	world = fcreate(hello, "world", 0666);
	assert(world != nil);
	world = fcreate(goodbye, "world", 0666);
	assert(world != nil);
	fdump(t->root, 0);

	fremove(world);
	fdump(t->root, 0);
}
