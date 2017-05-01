/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

// cmux will server a tty file system structured as
// /s/cmux
// /dev/cmuxctl
// /dev/cmuxsnarf
// /dev/pty*
// /dev/tty*
// pty0 and tty0 will be the session leaders for stuff like ^Z
// all half-baked theories.
void	keyboardsend(char*, int);
// whide/unhide will probably become console switching?
int	whide(Window*);
int	wunhide(int);
void	freescrtemps(void);
// /dev/ttyfs/ctl
int	parsewctl(char**, int*, int*, char**, char*, char*);
int	writewctl(Xfid*, char*);
Window *new(Console*, int, char*, char*, char**);

int	min(int, int);
int	max(int, int);
Rune*	strrune(Rune*, Rune);
int	isalnum(Rune);
void	timerstop(Timer*);
void	timercancel(Timer*);
Timer*	timerstart(int);
void	error(char*);
void	killprocs(void);
int	shutdown(void*, char*);
void	*erealloc(void*, uint);
void *emalloc(uint);
char *estrdup(char*);
// snarf buffer? Sure, why not?
// /dev/snarf can hold items to be pasted between consoles.
void	putsnarf(void);
void	getsnarf(void);
void	timerinit(void);

void	cvttorunes(char*, int, Rune*, int*, int*, int*);
#define	runemalloc(n)		malloc((n)*sizeof(Rune))
#define	runerealloc(a, n)	realloc(a, (n)*sizeof(Rune))
#define	runemove(a, b, n)	memmove(a, b, (n)*sizeof(Rune))

// ??
void
wsendctlmesg(Window *w, int m, Console *i);
char*
runetobyte(Rune *r, int n, int *ip);
