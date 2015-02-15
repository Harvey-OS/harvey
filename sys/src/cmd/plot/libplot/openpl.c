/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "mplot.h"
void openpl(char *s){
	m_initialize(s);
	e0->left=mapminx;
	e0->bottom=mapmaxy;
	e0->sidex=mapmaxx-mapminx;
	e0->sidey=mapminy-mapmaxy;
	e0->scalex=e0->sidex;
	e0->scaley=e0->sidey;
	sscpy(e0, e1);
	move(0., 0.);
}
