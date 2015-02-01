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
#include <bio.h>

void
main(int argc, char **argv)
{
	Biobuf b;
	char *s;
	int n;


	n = atoi(argv[1]);
	s = malloc(n+1);
	memset(s, 'a', n);
	s[n] = '\0';
	Binit(&b, 1, OWRITE);
	Bprint(&b, "%s\n", s);
	Bflush(&b);
}
