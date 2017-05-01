/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* t3.c: interpret commands affecting whole table */
# include "t.h"

struct optstr {
	char	*optnam;
	int	*optadd;
} options [] = {
	"expand", &expflg,
	"EXPAND", &expflg,
	"center", &ctrflg,
	"CENTER", &ctrflg,
	"box", &boxflg,
	"BOX", &boxflg,
	"allbox", &allflg,
	"ALLBOX", &allflg,
	"doublebox", &dboxflg,
	"DOUBLEBOX", &dboxflg,
	"frame", &boxflg,
	"FRAME", &boxflg,
	"doubleframe", &dboxflg,
	"DOUBLEFRAME", &dboxflg,
	"tab", &tab,
	"TAB", &tab,
	"linesize", &linsize,
	"LINESIZE", &linsize,
	"delim", &delim1,
	"DELIM", &delim1,
	0, 0
};

void
getcomm(void)
{
	char	line[200], *cp, nb[25], *t;
	struct optstr *lp;
	int	c, ci, found;

	for (lp = options; lp->optnam; lp++)
		*(lp->optadd) = 0;
	texname = texstr[texct=0];
	tab = '\t';
	Bprint(&tabout, ".nr %d \\n(.s\n", LSIZE);
	gets1(line, sizeof(line));
	/* see if this is a command line */
	if (strchr(line, ';') == 0) {
		backrest(line);
		return;
	}
	for (cp = line; (c = *cp) != ';'; cp++) {
		if (!letter(c))
			continue;
		found = 0;
		for (lp = options; lp->optadd; lp++) {
			if (prefix(lp->optnam, cp)) {
				*(lp->optadd) = 1;
				cp += strlen(lp->optnam);
				if (letter(*cp))
					error("Misspelled global option");
				while (*cp == ' ')
					cp++;
				t = nb;
				if ( *cp == '(')
					while ((ci = *++cp) != ')')
						*t++ = ci;
				else
					cp--;
				*t++ = 0;
				*t = 0;
				if (lp->optadd == &tab) {
					if (nb[0])
						*(lp->optadd) = nb[0];
				}
				if (lp->optadd == &linsize)
					Bprint(&tabout, ".nr %d %s\n", LSIZE, nb);
				if (lp->optadd == &delim1) {
					delim1 = nb[0];
					delim2 = nb[1];
				}
				found = 1;
				break;
			}
		}
		if (!found)
			error("Illegal option");
	}
	cp++;
	backrest(cp);
}

void
backrest(char *cp)
{
	char	*s;

	for (s = cp; *s; s++)
		;
	un1getc('\n');
	while (s > cp)
		un1getc(*--s);
}
