/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

int unbzip(int);
void _unbzip(int, int);
int unbflz(int);
int xexpand(int);
void *emalloc(ulong);
void *erealloc(void*, ulong);
char *estrdup(char*);

void ramfsmain(int, char**);
extern int chatty;
void error(char*, ...);
