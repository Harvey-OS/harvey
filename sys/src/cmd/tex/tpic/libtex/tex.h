#ifndef BUFSIZE
#include <stdio.h>
#endif
#define SCX(A) (int)((A)*e1->scalex+0.5)
#define SCY(A) (int)((A)*e1->scaley+0.5)
#define TRX(A) (int)(((A) - e1->xmin)*e1->scalex  + e1->left)
#define TRY(A) (int)(((A) - e1->ymin)*e1->scaley + e1->bottom)
#define DTRX(A) (((A) - e1->xmin)*e1->scalex  + e1->left)
#define DTRY(A) (((A) - e1->ymin)*e1->scaley + e1->bottom)
#define INCHES(A) ((A)/1000.)
extern struct penvir {
	double left, bottom;
	double xmin, ymin;
	double scalex, scaley;
	double sidex, sidey;
	double copyx, copyy;
	char *font;
	int psize;
	int pen;
	int pdiam;
	double dashlen;
	} *e0, *e1, *e2, *esave;
enum {
	SOLIDPEN, DASHPEN, DOTPEN
};
extern FILE *TEXFILE;
extern int round();
