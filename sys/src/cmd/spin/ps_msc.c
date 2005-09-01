/***** spin: ps_msc.c *****/

/* Copyright (c) 1997-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

/* The Postscript generation code below was written by Gerard J. Holzmann */
/* in June 1997. Parts of the prolog template are based on similar boiler */
/* plate in the Tcl/Tk distribution. This code is used to support Spin's  */
/* option M for generating a Postscript file from a simulation run.       */

#include "spin.h"
#include "version.h"

#ifdef PC
extern void free(void *);
#endif

static char *PsPre[] = {
	"%%%%Pages: (atend)",
	"%%%%PageOrder: Ascend",
	"%%%%DocumentData: Clean7Bit",
	"%%%%Orientation: Portrait",
	"%%%%DocumentNeededResources: font Courier-Bold",
	"%%%%EndComments",
	"",
	"%%%%BeginProlog",
	"50 dict begin",
	"",
	"/baseline 0 def",
	"/height 0 def",
	"/justify 0 def",
	"/lineLength 0 def",
	"/spacing 0 def",
	"/stipple 0 def",
	"/strings 0 def",
	"/xoffset 0 def",
	"/yoffset 0 def",
	"",
	"/ISOEncode {",
	"    dup length dict begin",
	"	{1 index /FID ne {def} {pop pop} ifelse} forall",
	"	/Encoding ISOLatin1Encoding def",
	"	currentdict",
	"    end",
	"    /Temporary exch definefont",
	"} bind def",
	"",
	"/AdjustColor {",
	"    CL 2 lt {",
	"	currentgray",
	"	CL 0 eq {",
	"	    .5 lt {0} {1} ifelse",
	"	} if",
	"	setgray",
	"    } if",
	"} bind def",
	"",
	"/DrawText {",
	"    /stipple exch def",
	"    /justify exch def",
	"    /yoffset exch def",
	"    /xoffset exch def",
	"    /spacing exch def",
	"    /strings exch def",
	"    /lineLength 0 def",
	"    strings {",
	"	stringwidth pop",
	"	dup lineLength gt {/lineLength exch def} {pop} ifelse",
	"	newpath",
	"    } forall",
	"    0 0 moveto (TXygqPZ) false charpath",
	"    pathbbox dup /baseline exch def",
	"    exch pop exch sub /height exch def pop",
	"    newpath",
	"    translate",
	"    lineLength xoffset mul",
	"    strings length 1 sub spacing mul height add yoffset mul translate",
	"    justify lineLength mul baseline neg translate",
	"    strings {",
	"	dup stringwidth pop",
	"	justify neg mul 0 moveto",
	"	stipple {",
	"	    gsave",
	"	    /char (X) def",
	"	    {",
	"		char 0 3 -1 roll put",
	"		currentpoint",
	"		gsave",
	"		char true charpath clip StippleText",
	"		grestore",
	"		char stringwidth translate",
	"		moveto",
	"	    } forall",
	"	    grestore",
	"	} {show} ifelse",
	"	0 spacing neg translate",
	"    } forall",
	"} bind def",
	"%%%%EndProlog",
	"%%%%BeginSetup",
	"/CL 2 def",
	"%%%%IncludeResource: font Courier-Bold",
	"%%%%EndSetup",
	0,
};

int MH  = 600;		/* page height - can be scaled */
int oMH = 600;		/* page height - not scaled */
#define MW	500	/* page width */
#define LH	100	/* bottom margin */
#define RH	100	/* right margin */
#define WW	 50	/* distance between process lines */
#define HH	  8	/* vertical distance between steps */
#define PH	 14	/* height of process-tag headers */

static FILE	*pfd;
static char	**I;		/* initial procs */
static int	*D,*R;		/* maps between depth and ldepth */
static short	*M;		/* x location of each box at index y */
static short	*T;		/* y index of match for each box at index y */
static char	**L;		/* text labels */
static char	*ProcLine;	/* active processes */
static int	pspno = 0;	/* postscript page */
static int	ldepth = 1;
static int	maxx, TotSteps = 2*4096; /* max nr of steps, about 40 pages */
static float	Scaler = (float) 1.0;

extern int	ntrail, s_trail, pno, depth;
extern Symbol	*oFname;
extern void	exit(int);
void putpages(void);
void spitbox(int, int, int, char *);

void
putlegend(void)
{
	fprintf(pfd, "gsave\n");
	fprintf(pfd, "/Courier-Bold findfont 8 scalefont ");
	fprintf(pfd, "ISOEncode setfont\n");
	fprintf(pfd, "0.000 0.000 0.000 setrgbcolor AdjustColor\n");
	fprintf(pfd, "%d %d [\n", MW/2, LH+oMH+ 5*HH);
	fprintf(pfd, "    (%s -- %s -- MSC -- %d)\n] 10 -0.5 0.5 0 ",
		Version, oFname?oFname->name:"", pspno);
	fprintf(pfd, "false DrawText\ngrestore\n");
}

void
startpage(void)
{	int i;

	pspno++;
	fprintf(pfd, "%%%%Page: %d %d\n", pspno, pspno);
	putlegend();

	for (i = TotSteps-1; i >= 0; i--)
	{	if (!I[i]) continue;
		spitbox(i, RH, -PH, I[i]);
	}

	fprintf(pfd, "save\n");
	fprintf(pfd, "10 %d moveto\n", LH+oMH+5);
	fprintf(pfd, "%d %d lineto\n", RH+MW, LH+oMH+5);
	fprintf(pfd, "%d %d lineto\n", RH+MW, LH);
	fprintf(pfd, "10 %d lineto\n", LH);
	fprintf(pfd, "closepath clip newpath\n");
	fprintf(pfd, "%f %f translate\n",
		(float) RH, (float) LH);
	memset(ProcLine, 0, 256*sizeof(char));
	if (Scaler != 1.0)
		fprintf(pfd, "%f %f scale\n", Scaler, Scaler);
}

void
putprelude(void)
{	char snap[256]; FILE *fd;

	sprintf(snap, "%s.ps", oFname?oFname->name:"msc");
	if (!(pfd = fopen(snap, "w")))
		fatal("cannot create file '%s'", snap);

	fprintf(pfd, "%%!PS-Adobe-2.0\n");
	fprintf(pfd, "%%%%Creator: %s\n", Version);
	fprintf(pfd, "%%%%Title: MSC %s\n", oFname?oFname->name:"--");
	fprintf(pfd, "%%%%BoundingBox: 119 154 494 638\n");
	ntimes(pfd, 0, 1, PsPre);

	if (s_trail)
	{	if (ntrail)
		sprintf(snap, "%s%d.trail", oFname?oFname->name:"msc", ntrail);
		else
		sprintf(snap, "%s.trail", oFname?oFname->name:"msc");
		if (!(fd = fopen(snap, "r")))
		{	snap[strlen(snap)-2] = '\0';
			if (!(fd = fopen(snap, "r")))
				fatal("cannot open trail file", (char *) 0);
		}
		TotSteps = 1;
		while (fgets(snap, 256, fd)) TotSteps++;
		fclose(fd);
	}
	R = (int   *) emalloc(TotSteps * sizeof(int));
	D = (int   *) emalloc(TotSteps * sizeof(int));
	M = (short *) emalloc(TotSteps * sizeof(short));
	T = (short *) emalloc(TotSteps * sizeof(short));
	L = (char **) emalloc(TotSteps * sizeof(char *));
	I = (char **) emalloc(TotSteps * sizeof(char *));
	ProcLine = (char *) emalloc(1024 * sizeof(char));
	startpage();
}

void
putpostlude(void)
{	putpages();
	fprintf(pfd, "%%%%Trailer\n");
	fprintf(pfd, "end\n");
	fprintf(pfd, "%%%%Pages: %d\n", pspno);
	fprintf(pfd, "%%%%EOF\n");
	fclose(pfd);
	/* stderr, in case user redirected output */
	fprintf(stderr, "spin: wrote %d pages into '%s.ps'\n",
		pspno, oFname?oFname->name:"msc");
	exit(0);
}
void
psline(int x0, int iy0, int x1, int iy1, float r, float g, float b, int w)
{	int y0 = MH-iy0;
	int y1 = MH-iy1;

	if (y1 > y0) y1 -= MH;

	fprintf(pfd, "gsave\n");
	fprintf(pfd, "%d %d moveto\n", x0*WW, y0);
	fprintf(pfd, "%d %d lineto\n", x1*WW, y1);
	fprintf(pfd, "%d setlinewidth\n", w);
	fprintf(pfd, "0 setlinecap\n");
	fprintf(pfd, "1 setlinejoin\n");
	fprintf(pfd, "%f %f %f setrgbcolor AdjustColor\n", r,g,b);
	fprintf(pfd, "stroke\ngrestore\n");
}

void
colbox(int x, int y, int w, int h, float r, float g, float b)
{	fprintf(pfd, "%d %d moveto\n", x - w, y-h);
	fprintf(pfd, "%d %d lineto\n", x + w, y-h);
	fprintf(pfd, "%d %d lineto\n", x + w, y+h);
	fprintf(pfd, "%d %d lineto\n", x - w, y+h);
	fprintf(pfd, "%d %d lineto\n", x - w, y-h);
	fprintf(pfd, "%f %f %f setrgbcolor AdjustColor\n", r,g,b);
	fprintf(pfd, "closepath fill\n");
}

void
putgrid(int p)
{	int i;

	for (i = p ; i >= 0; i--)
		if (!ProcLine[i])
		{	psline(i,0, i,MH-1,
				(float) 0.4, (float) 0.4, (float) 1.0,  1);
			ProcLine[i] = 1;
		}
}

void
putarrow(int from, int to)
{
	T[D[from]] = D[to];
}

void
stepnumber(int i)
{	int y = MH-(i*HH)%MH;

	fprintf(pfd, "gsave\n");
	fprintf(pfd, "/Courier-Bold findfont 6 scalefont ");
	fprintf(pfd, "ISOEncode setfont\n");
	fprintf(pfd, "0.000 0.000 0.000 setrgbcolor AdjustColor\n");
	fprintf(pfd, "%d %d [\n", -40, y);
	fprintf(pfd, "    (%d)\n] 10 -0.5 0.5 0 ", R[i]);
	fprintf(pfd, "false DrawText\ngrestore\n");
	fprintf(pfd, "%d %d moveto\n", -20, y);
	fprintf(pfd, "%d %d lineto\n", M[i]*WW, y);
	fprintf(pfd, "1 setlinewidth\n0 setlinecap\n1 setlinejoin\n");
	fprintf(pfd, "0.92 0.92 0.92 setrgbcolor AdjustColor\n");
	fprintf(pfd, "stroke\n");
}

void
spitbox(int x, int dx, int y, char *s)
{	float r,g,b, bw; int a; char d[256];

	if (!dx)
	{	stepnumber(y);
		putgrid(x);
	}
	bw = (float)2.7*(float)strlen(s);
	colbox(x*WW+dx, MH-(y*HH)%MH, (int) (bw+1.0),
		5, (float) 0.,(float) 0.,(float) 0.);
	if (s[0] == '~')
	{	switch (s[1]) {
		case 'B': r = (float) 0.2; g = (float) 0.2; b = (float) 1.;
			  break;
		case 'G': r = (float) 0.2; g = (float) 1.; b = (float) 0.2;
			  break;
		case 'R':
		default : r = (float) 1.; g = (float) 0.2; b = (float) 0.2;
			  break;
		}
		s += 2;
	} else if (strchr(s, '!'))
	{	r = (float) 1.; g = (float) 1.; b = (float) 1.;
	} else if (strchr(s, '?'))
	{	r = (float) 0.; g = (float) 1.; b = (float) 1.;
	} else
	{	r = (float) 1.; g = (float) 1.; b = (float) 0.;
		if (!dx
		&&  sscanf(s, "%d:%s", &a, d) == 2	/* was &d */
		&&  a >= 0 && a < TotSteps)
		{	if (!I[a]
			||  strlen(I[a]) <= strlen(s))
				I[a] = emalloc((int) strlen(s)+1);
			strcpy(I[a], s);
	}	}
	colbox(x*WW+dx, MH-(y*HH)%MH, (int) bw, 4, r,g,b);
	fprintf(pfd, "gsave\n");
	fprintf(pfd, "/Courier-Bold findfont 8 scalefont ");
	fprintf(pfd, "ISOEncode setfont\n");
	fprintf(pfd, "0.000 0.000 0.000 setrgbcolor AdjustColor\n");
	fprintf(pfd, "%d %d [\n", x*WW+dx, MH-(y*HH)%MH);
	fprintf(pfd, "    (%s)\n] 10 -0.5 0.5 0 ", s);
	fprintf(pfd, "false DrawText\ngrestore\n");
}

void
putpages(void)
{	int i, lasti=0; float nmh;

	if (maxx*WW > MW-RH/2)
	{	Scaler = (float) (MW-RH/2) / (float) (maxx*WW);
		fprintf(pfd, "%f %f scale\n", Scaler, Scaler);
		nmh = (float) MH; nmh /= Scaler; MH = (int) nmh;
	}

	for (i = TotSteps-1; i >= 0; i--)
	{	if (!I[i]) continue;
		spitbox(i, 0, 0, I[i]);
	}
	if (ldepth >= TotSteps) ldepth = TotSteps-1;
	for (i = 0; i <= ldepth; i++)
	{	if (!M[i] && !L[i]) continue;	/* no box here */
		if (6+i*HH >= MH*pspno)
		{ fprintf(pfd, "showpage\nrestore\n"); startpage(); }
		if (T[i] > 0)	/* red arrow */
		{	int reali = i*HH;
			int realt = T[i]*HH;
			int topop = (reali)/MH; topop *= MH;
			reali -= topop;  realt -= topop;

			if (M[i] == M[T[i]] && reali == realt)
				/* an rv handshake */
				psline( M[lasti], reali+2-3*HH/2,
					M[i], reali,
					(float) 1.,(float) 0.,(float) 0., 2);
			else
				psline(	M[i],    reali,
					M[T[i]], realt,
					(float) 1.,(float) 0.,(float) 0., 2);

			if (realt >= MH) T[T[i]] = -i;

		} else if (T[i] < 0)	/* arrow from prev page */
		{	int reali = (-T[i])*HH;
			int realt = i*HH;
			int topop = (realt)/MH; topop *= MH;
			reali -= topop;  realt -= topop;

			psline(	M[-T[i]], reali,
				M[i],     realt,
				(float) 1., (float) 0., (float) 0., 2);
		}
		if (L[i])
		{	spitbox(M[i], 0, i, L[i]);
			free(L[i]);
			lasti = i;
		}
	}
	fprintf(pfd, "showpage\nrestore\n");
}

void
putbox(int x)
{
	if (ldepth >= TotSteps)
	{	putpostlude();
		fprintf(stderr, "max length of %d steps exceeded\n",
			TotSteps);
		fatal("postscript file truncated", (char *) 0);
	}
	M[ldepth] = x;
	if (x > maxx) maxx = x;
}

void
pstext(int x, char *s)
{	char *tmp = emalloc((int) strlen(s)+1);

	strcpy(tmp, s);
	if (depth == 0)
		I[x] = tmp;
	else
	{	putbox(x);
		if (depth >= TotSteps || ldepth >= TotSteps)
		{	fprintf(stderr, "max nr of %d steps exceeded\n",
				TotSteps);
			fatal("aborting", (char *) 0);
		}

		D[depth] = ldepth;
		R[ldepth] = depth;
		L[ldepth] = tmp;
		ldepth += 2;
	}
}

void
dotag(FILE *fd, char *s)
{	extern int columns, notabs; extern RunList *X;
	int i = (!strncmp(s, "MSC: ", 5))?5:0;
	int pid = s_trail ? pno : (X?X->pid:0);

	if (columns == 2)
		pstext(pid, &s[i]);
	else
	{	if (!notabs)
		{	printf("  ");
			for (i = 0; i <= pid; i++)
				printf("    ");
		}
		fprintf(fd, "%s", s);
		fflush(fd);
	}
}
