/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

extern void startboot(char *, char **);

void
main(char *argv0)
{
	int write(int, void *, int);
	//do it this way to make sure it doesn't end up in .data
	char a[1];
	a[1] = '0';
	write(1, a, 1);
	startboot(argv0, &argv0);
	//	while(1) write(1, "hi\n", 3);
}
