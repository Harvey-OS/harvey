/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Cursor Cursor;
typedef struct Cursorinfo	Cursorinfo;
typedef struct CorebootScreen CorebootScreen;
typedef struct Corebootfb Corebootfb;
typedef struct Settings Settings;

extern Cursor	arrow;
extern Cursorinfo cursor;

/* devmouse.c */
extern void mousetrack(int, int, int, int);
extern Point mousexy(void);

extern void	mouseaccelerate(int);
extern void	mouseresize(void);

/* screen.c */
extern uint8_t* attachscreen(Rectangle*, uint32_t*, int*, int*, int*);
extern void	flushmemscreen(Rectangle);
extern int	cursoron(int);
extern void	cursoroff(int);
extern void	setcursor(Cursor*);
extern int	screensize(int, int, int, uint32_t);
extern int	screenaperture(int, int);
extern Rectangle physgscreenr;	/* actual monitor size */
extern void	blankscreen(int);

extern void swcursorinit(void);
extern void swcursorhide(void);
extern void swcursoravoid(Rectangle);
extern void swcursorunhide(void);

/* devdraw.c */
extern void	deletescreenimage(void);
extern void	resetscreenimage(void);
extern int		drawhasclients(void);
extern uint32_t	blanktime;
extern void	setscreenimageclipr(Rectangle);
extern void	drawflush(void);
extern int drawidletime(void);
extern QLock	drawlock;

/* for communication between devdss.c and screen.c */

enum {
	/* maxima */
	Wid		= 1280,
	Ht		= 1024,
	Depth		= 24,		/* bits per pixel */
};

struct CorebootScreen {
	Lock l;
	Cursor Cursor;
	uint	wid;		/* width in pixels */
	uint	ht;		/* height in pixels */
	int	open;
};

struct Corebootfb {		/* frame buffer for 24-bit active color */
	uint8_t *pixel;
};


