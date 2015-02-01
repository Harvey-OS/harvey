/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <event.h>
#define SCX(A) ((((A) - e1->xmin)*e1->scalex  + e1->left)+.5)
#define SCY(A) ((((A) - e1->ymin)*e1->scaley + e1->bottom)+.5)
#define	SCR(A) ((A)*e1->scalex+.5)
#define unorm(y)	(double)(e1->sidey - y)
#define	BIGINT	0x3FFFFFFF	/* a large, but valid, int */
extern struct penvir {
	double left, bottom;
	double xmin, ymin;
	double scalex, scaley;
	double sidex, sidey;
	double copyx, copyy;
	double quantum;
	double grade;
	int pgap;
	double pslant;
	int pmode, foregr, backgr;
} *e0, *e1, *esave;
#define RADIAN 57.3	/* radians per degree */
struct seg {
	int x, y, X, Y;
	char stat;
};

/*
 * display parameters
 */
int clipminx, clipminy, clipmaxx, clipmaxy;	/* clipping rectangle */
int mapminx, mapminy, mapmaxx, mapmaxy;		/* centered square */
/*
 * Prototypes
 */
#include "../plot.h"
void m_clrwin(int, int, int, int, int);
void m_finish(void);
void m_initialize(char *);
int m_text(int, int, char *, char *, int, int, int);
void m_vector(int, int, int, int, int);
void m_swapbuf(void);
void m_dblbuf(void);
int bcolor(char *);
void sscpy(struct penvir *, struct penvir *);
Image *getcolor(int);
