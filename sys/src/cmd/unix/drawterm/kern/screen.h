/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Mouseinfo Mouseinfo;
typedef struct Mousestate Mousestate;
typedef struct Cursorinfo Cursorinfo;
typedef struct Screeninfo Screeninfo;

#define Mousequeue 16   /* queue can only have Mousequeue-1 elements */
#define Mousewindow 500 /* mouse event window in millisec */

struct Mousestate {
	int buttons;
	Point xy;
	uint32_t msec;
};

struct Mouseinfo {
	Lock lk;
	Mousestate queue[Mousequeue];
	int ri, wi;
	int lastb;
	int trans;
	int open;
	Rendez r;
};

struct Cursorinfo {
	Lock lk;
	Point offset;
	uchar clr[2 * 16];
	uchar set[2 * 16];
};

struct Screeninfo {
	Lock lk;
	Memimage *newsoft;
	int reshaped;
	int depth;
	int dibtype;
};

extern Memimage *gscreen;
extern Mouseinfo mouse;
extern Cursorinfo cursor;
extern Screeninfo screen;

void screeninit(void);
void screenload(Rectangle, int, uchar *, Point, int);

void getcolor(uint32_t, uint32_t *, uint32_t *, uint32_t *);
void setcolor(uint32_t, uint32_t, uint32_t, uint32_t);

void refreshrect(Rectangle);

void cursorarrow(void);
void setcursor(void);
void mouseset(Point);
void drawflushr(Rectangle);
void flushmemscreen(Rectangle);
uchar *attachscreen(Rectangle *, uint32_t *, int *, int *, int *, void **);

void drawqlock(void);
void drawqunlock(void);
int drawcanqlock(void);
void terminit(void);
