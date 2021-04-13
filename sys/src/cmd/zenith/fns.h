/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

//#pragma	varargck	argpos	warning	2

void	warning(Mntdir*, char*, ...);

#define	fbufalloc()	emalloc(BUFSIZE)
#define	fbuffree(x)	free(x)

void	plumblook(Plumbmsg*m);
void	plumbshow(Plumbmsg*m);
void	putsnarf(void);
void	getsnarf(void);
int	tempfile(void);
void	scrlresize(void);
Font*	getfont(int, int, char*);
char*	getarg(Text*, int, int, Rune**, int*);
char*	getbytearg(Text*, int, int, char**);
void	new(Text*, Text*, Text*, int, int, Rune*, int);
void	undo(Text*, Text*, Text*, int, int, Rune*, int);
char*	getname(Text*, Text*, Rune*, int, int);
void	scrsleep(u32);
void	savemouse(Window*);
int	restoremouse(Window*);
void	clearmouse(void);
void	allwindows(void(*)(Window*, void*), void*);
u32 loadfile(int, u32, int*, int(*)(void*, u32, Rune*, int), void*);
void	movetodel(Window*);

Window*	errorwin(Mntdir*, int);
Window*	errorwinforwin(Window*);
Runestr cleanrname(Runestr);
void	run(Window*, char*, Rune*, int, int, char*, char*, int);
void fsysclose(void);
void	setcurtext(Text*, int);
int	isfilec(Rune);
void	rxinit(void);
int rxnull(void);
Runestr	dirname(Text*, Rune*, int);
void	error(char*);
void	cvttorunes(char*, int, Rune*, int*, int*, int*);
void*	tmalloc(u32);
void	tfree(void);
void	killprocs(void);
void	killtasks(void);
int	runeeq(Rune*, u32, Rune*, u32);
int	ALEF_tid(void);
void	iconinit(void);
Timer*	timerstart(int);
void	timerstop(Timer*);
void	timercancel(Timer*);
void	timerinit(void);
void	cut(Text*, Text*, Text*, int, int, Rune*, int);
void	paste(Text*, Text*, Text*, int, int, Rune*, int);
void	get(Text*, Text*, Text*, int, int, Rune*, int);
void	put(Text*, Text*, Text*, int, int, Rune*, int);
void	putfile(File*, int, int, Rune*, int);
void	fontx(Text*, Text*, Text*, int, int, Rune*, int);
int	isalnum(Rune);
void	execute(Text*, u32, u32, int, Text*);
int	search(Text*, Rune*, u32);
void	look3(Text*, u32, u32, int);
void	editcmd(Text*, Rune*, u32);
u32	min(u32, u32);
u32	max(u32, u32);
Window*	lookfile(Rune*, int);
Window*	lookid(int, int);
char*	runetobyte(Rune*, int);
Rune*	bytetorune(char*, int*);
void	fsysinit();
Mntdir*	fsysmount(Rune*, int, Rune**, int);
void		fsysincid(Mntdir*);
void		fsysdelid(Mntdir*);
Xfid*		respond(Xfid*, Fcall*, char*);
int		rxcompile(Rune*);
int		rgetc(void*, u32);
int		tgetc(void*, u32);
int		isaddrc(int);
int		isregexc(int);
void *emalloc(u32);
void *erealloc(void*, u32);
char	*estrdup(char*);
Range		address(Mntdir*, Text*, Range, Range, void*, u32, u32,
			     int (*)(void*, u32),  int*, u32*);
int		rxexecute(Text*, Rune*, u32, u32, Rangeset*);
int		rxbexecute(Text*, u32, Rangeset*);
Window*	makenewwindow(Text *t);
int	expand(Text*, u32, u32, Expand*);
Rune*	skipbl(Rune*, int, int*);
Rune*	findbl(Rune*, int, int*);
char*	edittext(Window*, int, Rune*, int);
void		flushwarnings(void);
void	loadinitscript(const char* initscript);
int	keymap(char* key, char* mapping);
int	extendedkeymap(char* key, char* mapping);

#define	runemalloc(a)		(Rune*)emalloc((a)*sizeof(Rune))
#define	runerealloc(a, b)	(Rune*)erealloc((a), (b)*sizeof(Rune))
#define	runemove(a, b, c)	memmove((a), (b), (c)*sizeof(Rune))
