/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * tee-- pipe fitting
 */

#include <u.h>
#include <libc.h>

int	uflag;
int	aflag;
int	*openf;

char in[8192];

int	intignore(void *c, char*);

void
main(int argc, char **argv)
{
	int i;
	int r, n;

	ARGBEGIN {
	case 'a':
		aflag++;
		break;

	case 'i':
		atnotify(intignore, 1);
		break;

	default:
		fprint(2, "usage: tee [-ai] [file ...]\n");
		exits("usage");
	} ARGEND

	openf = malloc((1+argc)*sizeof(int));
	if(openf == nil)
		sysfatal("out of memory: %r");

	n = 0;
	while(*argv) {
		if(aflag) {
			openf[n] = open(argv[0], OWRITE);
			if(openf[n] < 0)
				openf[n] = create(argv[0], OWRITE, 0666);
			seek(openf[n], 0L, 2);
		} else
			openf[n] = create(argv[0], OWRITE, 0666);
		if(openf[n] < 0) {
			fprint(2, "tee: cannot open %s: %r\n", argv[0]);
		} else
			n++;
		argv++;
	}
	openf[n++] = 1;

	for(;;) {
		r = read(0, in, sizeof in);
		if(r <= 0)
			exits(nil);
		for(i=0; i<n; i++)
			write(openf[i], in, r);
	}
}

int
intignore(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "interrupt") == 0)
		return 1;
	return 0;
}
