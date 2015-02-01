/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* t9.c: write lines for tables over 200 lines */
# include "t.h"
static useln;

void
yetmore(void)
{
	for (useln = 0; useln < MAXLIN && table[useln] == 0; useln++)
		;
	if (useln >= MAXLIN)
		error("Wierd.  No data in table.");
	table[0] = table[useln];
	for (useln = nlin - 1; useln >= 0 && (fullbot[useln] || instead[useln]); useln--)
		;
	if (useln < 0)
		error("Wierd.  No real lines in table.");
	domore(leftover);
	while (gets1(cstore = cspace, MAXCHS) && domore(cstore))
		;
	last = cstore;
}


int
domore(char *dataln)
{
	int	icol, ch;

	if (prefix(".TE", dataln))
		return(0);
	if (dataln[0] == '.' && !isdigit(dataln[1])) {
		Bprint(&tabout, "%s\n", dataln);
		return(1);
	}
	fullbot[0] = 0;
	instead[0] = (char *)0;
	if (dataln[1] == 0)
		switch (dataln[0]) {
		case '_': 
			fullbot[0] = '-'; 
			putline(useln, 0);  
			return(1);
		case '=': 
			fullbot[0] = '='; 
			putline(useln, 0); 
			return(1);
		}
	for (icol = 0; icol < ncol; icol++) {
		table[0][icol].col = dataln;
		table[0][icol].rcol = 0;
		for (; (ch = *dataln) != '\0' && ch != tab; dataln++)
			;
		*dataln++ = '\0';
		switch (ctype(useln, icol)) {
		case 'n':
			table[0][icol].rcol = maknew(table[0][icol].col);
			break;
		case 'a':
			table[0][icol].rcol = table[0][icol].col;
			table[0][icol].col = "";
			break;
		}
		while (ctype(useln, icol + 1) == 's') /* spanning */
			table[0][++icol].col = "";
		if (ch == '\0') 
			break;
	}
	while (++icol < ncol)
		table[0][icol].col = "";
	putline(useln, 0);
	exstore = exspace;		 /* reuse space for numerical items */
	return(1);
}


