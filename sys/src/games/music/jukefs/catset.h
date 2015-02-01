/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void catsetrealloc(Catset*, int);
void catsetinit(Catset*, int);
void catsetfree(Catset*t);
void catsetcopy(Catset*, Catset*);
void catsetset(Catset*, int);
int catsetisset(Catset*);
void catsetorset(Catset*, Catset*);
int catseteq(Catset*, Catset*);
