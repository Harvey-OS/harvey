#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <libg.h>
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
 * Color values -- these only work with ldepth=3 & standard plan 9 color map
 */
#define	RGB(r,g,b) (255&~(((r)<<5)|((g)<<2)|((b)>>1)))
#define ZERO	RGB(0,0,0)	/* really should be ~0, but bcolor uses neg for error */
#define RED	RGB(7,0,0)
#define GREEN	RGB(0,7,0)
#define YELLOW	RGB(7,7,0)
#define BLUE	RGB(0,0,7)
#define MAGENTA	RGB(7,0,7)
#define CYAN	RGB(0,7,7)
#define WHITE	RGB(7,7,7)
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
