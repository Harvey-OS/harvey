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

static char x[1024];
static char s[64] = "  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static void
fill(void)
{
	int i;

	for(i = 0; i < sizeof(x); i += sizeof(s)){
		memmove(&x[i], s, sizeof(s));
		x[i] = i>>8;
		x[i+1] = i;
	}
}

void
main(int argc, char *argv[])
{
	int i = 2560;

	if(argc > 1){
		argc--; argv++;
		i = atoi(*argv);
	}
	USED(argc);

	fill();

	while(i--)
		write(1, x, sizeof(x));
	exits(0);
}
