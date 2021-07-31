#define _POSIX_SOURCE
#define _RESEARCH_SOURCE
#include <stdio.h>
#include <signal.h>
#include <libv.h>

char *
getpass(char *prompt)
{
	register char *p;
	register c;
	FILE *fi;
	static char pbuf[9];
	void (*sig)(int);

	if ((fi = fopen("/dev/cons", "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	sig = signal(SIGINT, SIG_IGN);
	tty_echooff(fileno(fi));
	fprintf(stderr, "%s", prompt); fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[8])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); fflush(stderr);
	tty_echoon(fileno(fi));
	signal(SIGINT, sig);
	if (fi != stdin)
		fclose(fi);
	return(pbuf);
}
