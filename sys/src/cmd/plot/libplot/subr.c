/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "mplot.h"
#define pSMALL    0.5
struct penvir  E[9] = {
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite},
{ 0., 1024., 0., 0., 1., -1.,1024., -1024., 0., 0., pSMALL, 1., 1, 0.,1, DBlack, DWhite}
};
struct penvir *e0 = E, *e1 = &E[1], *esave;
bcolor(char *s){
	int c;
	while (*s != 0) {
		switch (*s) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			c=0;
			while('0'<=*s && *s<='9')
				c=c*10+*s++-'0';
			if(c < 0)
				return DBlack;
			return cmap2rgba(c);
		case 'k':  case 'z':	/* zero was old name for kblack */
			return(DBlack);
		case 'r':
			return(DRed);
		case 'g':
			return(DGreen);
		case 'b':
			return(DBlue);
		case 'm':
			return(DMagenta);
		case 'y':
			return(DYellow);
		case 'c':
			return(DCyan);
		case 'w':
			return(DWhite);
		case 'R':
			return(atoi(s + 1));
		case 'G':
			e1->pgap = atof(s + 1);
			return(-1);
		case 'A':
			e1->pslant = (180. - atof(s + 1)) / RADIAN;
			return(-1);
		}
		while (*++s != 0)
			if (*s == '/') {
				s++;
				break;
			}
	}
	return DBlack;
}
void sscpy(struct penvir *a, struct penvir *b){ /* copy 'a' onto 'b' */
	b->left = a->left;
	b->bottom = a->bottom;
	b->xmin = a->xmin;
	b->ymin = a->ymin;
	b->scalex = a->scalex;
	b->scaley = a->scaley;
	b->sidex = a->sidex;
	b->sidey = a->sidey;
	b->copyx = a->copyx;
	b->copyy = a->copyy;
	b->quantum = a->quantum;
	b->grade = a->grade;
	b->pmode = a->pmode;
	b->foregr = a->foregr;
	b->backgr = a->backgr;
}
void idle(void){}

void ptype(char *s){USED(s);}
