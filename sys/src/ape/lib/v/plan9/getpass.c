/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define _POSIX_SOURCE
#define _RESEARCH_SOURCE
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <libv.h>

char *
getpass(char *prompt)
{
	int c;
	char *p;
	FILE *fi;
	static char pbuf[PASS_MAX];
	void (*sig)(int);

	if ((fi = fopen("/dev/cons", "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, NULL);
	sig = signal(SIGINT, SIG_IGN);
	tty_echooff(fileno(fi));
	fprintf(stderr, "%s", prompt);
	fflush(stderr);

	for (p = pbuf; (c = getc(fi)) != '\n' && c != EOF; )
		if (c == ('u' & 037))
			p = pbuf;
		else if (c == '\b') {
			if (p > pbuf)
				p--;
		} else if (p < &pbuf[sizeof(pbuf)-1])
			*p++ = c;
	*p = '\0';

	fprintf(stderr, "\n");
	fflush(stderr);
	tty_echoon(fileno(fi));
	signal(SIGINT, sig);
	if (fi != stdin)
		fclose(fi);
	return(pbuf);
}
