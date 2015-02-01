/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Cursor Cursor;
typedef struct Cursorinfo Cursorinfo;
struct Cursorinfo {
	Cursor;
	Lock;
};

extern Cursorinfo	cursor;
extern Cursor		arrow;
extern Memimage		*gscreen;
extern int		cursorver;
extern Point		cursorpos;

Point 		mousexy(void);
int		cursoron(int);
void		cursoroff(int);
void		setcursor(Cursor*);
void		flushmemscreen(Rectangle r);
Rectangle	cursorrect(void);
void		cursordraw(Memimage *dst, Rectangle r);

void		drawactive(int);
void		drawlock(void);
void		drawunlock(void);
int		candrawlock(void);
void		getcolor(ulong, ulong*, ulong*, ulong*);
int		setcolor(ulong, ulong, ulong, ulong);
#define		TK2SEC(x)	0
extern void	blankscreen(int);
void		screeninit(int x, int y, char *chanstr);
void		mousetrack(int x, int y, int b, int msec);
uchar		*attachscreen(Rectangle*, ulong*, int*, int*, int*);

void		fsinit(char *mntpt, int x, int y, char *chanstr);
