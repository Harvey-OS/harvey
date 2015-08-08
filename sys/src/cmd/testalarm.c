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

static void catch(void* p, char* s)
{
	print("catch %p %s\n", p, s);
	alarm(5000);
	noted(NCONT);
}

void
main(int argc, char* argv[])
{
	notify(catch);

	alarm(5000);
	while(1)
		;
	exits(0);
}
