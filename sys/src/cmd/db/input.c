/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

Rune	line[LINSIZ];
extern	int	infile;
Rune	*lp;
int	peekc,lastc = EOR;
int	eof;

/* input routines */

eol(int c)
{
	return(c==EOR || c==';');
}

int
rdc(void)
{
	do {
		readchar();
	} while (lastc==SPC || lastc==TB);
	return(lastc);
}

void
reread(void)
{
	peekc = lastc;
}

void
clrinp(void)
{
	flush();
	lp = 0;
	peekc = 0;
}

int
readrune(int fd, Rune *r)
{
	char buf[UTFmax+1];
	int i;

	for(i=0; i<UTFmax && !fullrune(buf, i); i++)
		if(read(fd, buf+i, 1) <= 0)
			return -1;
	buf[i] = 0;
	chartorune(r, buf);
	return 1;
}

int
readchar(void)
{
	Rune *p;

	if (eof)
		lastc=0;
	else if (peekc) {
		lastc = peekc;
		peekc = 0;
	}
	else {
		if (lp==0) {
			for (p = line; p < &line[LINSIZ-1]; p++) {
				eof = readrune(infile, p) <= 0;
				if (mkfault) {
					eof = 0;
					error(0);
				}
				if (eof) {
					p--;
					break;
				}
				if (*p == EOR) {
					if (p <= line)
						break;
					if (p[-1] != '\\')
						break;
					p -= 2;
				}
			}
			p[1] = 0;
			lp = line;
		}
		if ((lastc = *lp) != 0)
			lp++;
	}
	return(lastc);
}

nextchar(void)
{
	if (eol(rdc())) {
		reread();
		return(0);
	}
	return(lastc);
}

quotchar(void)
{
	if (readchar()=='\\')
		return(readchar());
	else if (lastc=='\'')
		return(0);
	else
		return(lastc);
}

void
getformat(char *deformat)
{
	char *fptr;
	BOOL	quote;
	Rune r;

	fptr=deformat;
	quote=FALSE;
	while ((quote ? readchar()!=EOR : !eol(readchar()))){
		r = lastc;
		fptr += runetochar(fptr, &r);
		if (lastc == '"')
			quote = ~quote;
	}
	lp--;
	if (fptr!=deformat)
		*fptr = '\0';
}

/*
 *	check if the input line if of the form:
 *		<filename>:<digits><verb> ...
 *
 *	we handle this case specially because we have to look ahead
 *	at the token after the colon to decide if it is a file reference
 *	or a colon-command with a symbol name prefix.
 */

int
isfileref(void)
{
	Rune *cp;

	for (cp = lp-1; *cp && !strchr(CMD_VERBS, *cp); cp++)
		if (*cp == '\\' && cp[1])	/* escape next char */
			cp++;
	if (*cp && cp > lp-1) {
		while (*cp == ' ' || *cp == '\t')
			cp++;
		if (*cp++ == ':') {
			while (*cp == ' ' || *cp == '\t')
				cp++;
			if (isdigit(*cp))
				return 1;
		}
	}
	return 0;
}
