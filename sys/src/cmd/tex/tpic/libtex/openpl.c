#include "tex.h"

extern	double	xmin, ymin, xmax, ymax;

/* tpic TeX coord system uses millinches, printer's points for pensize */
/* positive y downward, origin at upper left */

#define pHEIGHT 5000.
#define pWIDTH  5000.
#define pPENSIZE 9
#define pPSIZE 10
#define pDLEN .05
struct penvir E[2] = {
{0.,pHEIGHT,0.,0.,1.,-1.,pWIDTH,pHEIGHT,0.,0.,0,pPSIZE,SOLIDPEN,pPENSIZE,pDLEN},
{0.,pHEIGHT,0.,0.,1.,-1.,pWIDTH,pHEIGHT,0.,0.,0,pPSIZE,SOLIDPEN,pPENSIZE,pDLEN}
};
struct penvir *e0 = E, *e1 = &E[1];
FILE *TEXFILE = stdout;

openpl()
{ 
	space(xmin, ymin, xmax, ymax);
	fprintf(TEXFILE,"\\catcode`@=11\n");
	fprintf(TEXFILE, "\\expandafter\\ifx\\csname graph\\endcsname\\relax");
	fprintf(TEXFILE, " \\alloc@4\\box\\chardef\\insc@unt\\graph\\fi\n");
	fprintf(TEXFILE, "\\catcode`@=12\n");
	fprintf(TEXFILE, "\\setbox\\graph=\\vtop{%%\n");
	fprintf(TEXFILE, "  \\baselineskip=0pt \\lineskip=0pt ");
	fprintf(TEXFILE, "\\lineskiplimit=0pt\n");
	fprintf(TEXFILE, "  \\vbox to0pt{\\hbox{%%\n");
	fprintf(TEXFILE, "    \\special{pn %d}%%\n", e1->pdiam);
}
