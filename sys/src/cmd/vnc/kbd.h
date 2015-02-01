/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct	Snarf	Snarf;

struct Snarf
{
	QLock;
	int		vers;
	int		n;
	char		*buf;
};

enum
{
	MAXSNARF	= 100*1024
};

extern	Snarf		snarf;

long			latin1(Rune *k, int n);
void			kbdputc(int c);
void			screenputs(char*, int);
void			vncputc(int, int);
void			setsnarf(char *buf, int n, int *vers);
