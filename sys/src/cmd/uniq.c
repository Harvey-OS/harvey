/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Deal with duplicated lines in a file
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

#define	SIZE	8000

int	fields	= 0;
int	letters	= 0;
int	linec	= 0;
char	mode;
int	uniq;
char	*b1, *b2;
int32_t	bsize;
Biobuf	fin;
Biobuf	fout;

int	gline(char *buf);
void	pline(char *buf);
int	equal(char *b1, char *b2);
char*	skip(char *s);

void
main(int argc, char *argv[])
{
	int f;

	argv0 = argv[0];
	bsize = SIZE;
	b1 = malloc(bsize);
	b2 = malloc(bsize);
	f = 0;
	while(argc > 1) {
		if(*argv[1] == '-') {
			if(isdigit(argv[1][1]))
				fields = atoi(&argv[1][1]);
			else
				mode = argv[1][1];
			argc--;
			argv++;
			continue;
		}
		if(*argv[1] == '+') {
			letters = atoi(&argv[1][1]);
			argc--;
			argv++;
			continue;
		}
		f = open(argv[1], 0);
		if(f < 0)
			sysfatal("cannot open %s", argv[1]);
		break;
	}
	if(argc > 2)
		sysfatal("unexpected argument %s", argv[2]);
	Binit(&fin, f, OREAD);
	Binit(&fout, 1, OWRITE);

	if(gline(b1))
		exits(0);
	for(;;) {
		linec++;
		if(gline(b2)) {
			pline(b1);
			exits(0);
		}
		if(!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if(gline(b1)) {
					pline(b2);
					exits(0);
				}
			} while(equal(b2, b1));
			pline(b2);
			linec = 0;
		}
	}
}

int
gline(char *buf)
{
	int len;
	char *p;

	p = Brdline(&fin, '\n');
	if(p == 0)
		return 1;
	len = Blinelen(&fin);
	if(len >= bsize-1)
		sysfatal("line too int32_t");
	memmove(buf, p, len);
	buf[len-1] = 0;
	return 0;
}

void
pline(char *buf)
{
	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq)
			break;
		return;

	case 'c':
		Bprint(&fout, "%4d ", linec);
	}
	uniq = 0;
	Bprint(&fout, "%s\n", buf);
}

int
equal(char *b1, char *b2)
{
	char c;

	if(fields || letters) {
		b1 = skip(b1);
		b2 = skip(b2);
	}
	for(;;) {
		c = *b1++;
		if(c != *b2++) {
			if(c == 0 && mode == 's')
				return 1;
			return 0;
		}
		if(c == 0) {
			uniq++;
			return 1;
		}
	}
}

char*
skip(char *s)
{
	int nf, nl;

	nf = nl = 0;
	while(nf++ < fields) {
		while(*s == ' ' || *s == '\t')
			s++;
		while(!(*s == ' ' || *s == '\t' || *s == 0) )
			s++;
	}
	while(nl++ < letters && *s != 0)
			s++;
	return s;
}
