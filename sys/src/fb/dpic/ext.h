#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include <ctype.h>
#include <fb.h>
#include <stdarg.h>
#include "typeface.h"
#define ALPHABET	256		/* number of characters in basic alphabet */
#define MAXCH		512
#define MAXFONTS	99
#define POINTS	72.3			/* points per inch */
/*
 * State of a table entry in fonts[].
 */
#define NEWFONT		0
#define RELEASED	1
#define INMEMORY	2
typedef struct Chwid Chwid;
typedef struct Fonthd Fonthd;
typedef struct Edge Edge;
typedef struct Rgba Rgba;
typedef struct Span Span;
/*
 * Data about each character on a font. Omitted the ascender/descender field.
 */
struct Chwid{
	short		num;	/* 0 means not on this font */
	unsigned char	wid;	/* width */
	unsigned char	code;	/* code for actual device. */
};
/*
 * Font header - one for each available position.
 */
struct Fonthd{
	char	*path;		/* where it came from */
	char	*name;		/* as known to troff */
	char	*fontname;	/* real name (e.g. Times-Roman) */
	char	state;		/* NEWFONT, RELEASED, or INMEMORY */
	char	mounted;	/* mounted on this many positions */
	char	specfont;	/* 1 == special font */
	short	nchars;		/* number of width entries for this font */
	Chwid	*wp;		/* widths, etc., of the real characters */
};
struct Edge{
	double x0, y0;		/* upper endpoint */
	double x1, y1;		/* lower endpoint */
	double x;		/* current point on line */
	double dx;		/* change in x from one line to the next */
	int right;		/* is this edge to the right of the polygon? */
	Edge *next;		/* circular doubly linked w. dummy header */
	Edge *prev;
};
struct Rgba{
	unsigned char r, g, b, a;
};
struct Span{
	int min, max;
};
/*
 * Position adjusting macros.
 */
#define hgoto(n)	hpos=n
#define hmot(n)		hpos+=n
#define vgoto(n)	vpos=n
#define vmot(n)		vpos+=n
extern int hpos, vpos, size, curfont, fontheight, fontslant, pagenum;
#define skipline(f)	while(getc(f)!='\n')
extern int ignore;			/* what we do with FATAL errors */
extern long lineno;			/* line number */
extern char *prog_name;			/* and program name - for errors */
extern int devres;
extern int unitwidth;
extern int nfonts;
extern double res;
extern int xmin, ymin;			/* coords of UL corner, for picopen_w */
extern int pagewid, pagehgt;		/* size of page in pixels */
extern double dpi;			/* dots per inch */
extern Fonthd fonts[];			/* data about every font we see */
extern Fonthd *fmount[];		/* troff mounts fonts here */
extern Rgba **page;			/* page bitmap */
extern float **image;			/* tile bitmap */
extern Edge *enter;			/* y bucket list */
extern Rgba color;			/* color in which type is set */
extern Span *span;			/* scan-line spans */
#include "proto.h"
