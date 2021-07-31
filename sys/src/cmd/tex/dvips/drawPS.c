/*
 *                      D R A W P S . C
 *
 *  Changed to take magnification into account, 27 August 1990.
 *
 * (minor mods by don on 5 Jan 90 to accommodate highres PostScript)
 *
 * $Revision: 1.1 $
 *
 * $Log:	drawPS.c,v $
 * Revision 1.1  90/03/10  20:32:48  grunwald
 * Initial revision
 *
 *
 * 89/04/24: decouty@irisa.fr, priol@irisa.fr
 *              added three shading levels for fig,
 *              removed '\n' in cmdout() calls
 *
 * Revision 1.4  87/05/07  15:06:24  dorab
 * relinked back hh to h and vv to v undoing a previous change.
 * this was causing subtle quantization problems. the main change
 * is in the definition of hconvPS and vconvPS.
 *
 * changed the handling of the UC seal. the PS file should now be
 * sent via the -i option.
 *
 * Revision 1.3  86/04/29  23:20:55  dorab
 * first general release
 *
 * Revision 1.3  86/04/29  22:59:21  dorab
 * first general release
 *
 * Revision 1.2  86/04/29  13:23:40  dorab
 * Added distinctive RCS header
 *
 */
#ifndef lint
char RCSid[] =
  "@(#)$Header: /usr/local/src/TeX/Dvips-5.0.2/RCS/drawPS.c,v 1.1 90/03/10 20:32:48 grunwald Exp $ (UCLA)";
#endif

/*
 the driver for handling the \special commands put out by
 the tpic program as modified by Tim Morgan <morgan@uci.edu>
 the co-ordinate system is with the origin at the top left
 and the x-axis is to the right, and the y-axis is to the bottom.
 when these routines are called, the origin is set at the last dvi position,
 which has to be gotten from the dvi-driver (in this case, dvips) and will
 have to be converted to device co-ordinates (in this case, by [hv]convPS).
 all dimensions are given in milli-inches and must be converted to what
 dvips has set up (i.e. there are convRESOLUTION units per inch).

 it handles the following \special commands
    pn n                        set pen size to n
    pa x y                      add path segment to (x,y)
    fp                          flush path
    ip                          flush invisible path, i.e. do shading only
    da l                        flush dashed - each dash is l (inches)
    dt l                        flush dotted - one dot per l (inches)
    sp [l]                      flush spline - optional l is dot/dash length
    ar x y xr yr sa ea          arc centered at (x,y) with x-radius xr
                                and y-radius yr, start angle sa (in rads),
                                end angle ea (in rads)
    ia x y xr yr sa ea          invisible arc with same parameters as above
    sh [s]                      shade last path (box, circle, ellipse)
				s = gray level (0 = white, 1 = black)
    wh                          whiten last path (box, circle, ellipse)
    bk                          blacken last path (box,circle, ellipse)
    tx                          texture command - Added by T. Priol
				(priol@irisa.fr), enhanced by M. Jourdan:
				textures are translated into uniform gray levels

  this code is in the public domain

  written by dorab patel <dorab@cs.ucla.edu>
  december 1985
  released feb 1986
  changes for dvips july 1987

  */

#ifdef TPIC                     /* rest of the file !! */

#include "dvips.h"
#include <math.h>		/* used in function "arc" */

#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */

/*
 * external functions used here
 */
extern void cmdout();
extern void floatout();
extern void numout();
extern void error();

/*
 * external variables used here
 */
extern integer hh,vv;           /* the current x,y position in pixel units */
extern int actualdpi ;
extern int vactualdpi ;
extern integer mag ;

#define convRESOLUTION DPI
#define convVRESOLUTION VDPI
#define TRUE 1
#define FALSE 0
#define tpicRESOLUTION 1000     /* number of tpic units per inch */

/* convert from tpic units to PS units */
#define PixRound(a,b) zPixRound((integer)(a),(integer)(b))
#define convPS(x) PixRound((x),convRESOLUTION)
#define convVPS(x) PixRound((x),convVRESOLUTION)

/* convert from tpic locn to abs horiz PS locn */
#define hconvPS(x) (integer)(hh + convPS(x))
/* convert from tpic locn to abs vert PS locn */
#define vconvPS(x) (integer)(vv + convVPS(x))
/* convert to degrees */
#define convDeg(x) (360*(x)/(2*3.14159265358))

/* if PostScript had splines, i wouldnt need to store the path */
#define MAXPATHS 600            /* maximum number of path segments */
#define NONE 0                  /* for shading */
#define BLACK           1       /* for shading */
#define GRAY		2       /* MJ; for shading */
#define WHITE           3       /* for shading */

/* the following defines are used to make the PostScript shorter;
  the corresponding defines are in the PostScript prolog special.lpro */
#define MOVETO "a"
#define LINETO "li"
#define RCURVETO "rc"
#define RLINETO "rl"
#define STROKE "st"
#define FILL "fil"
#define NEWPATH "np"
#define CLOSEPATH "closepath"
/*
 * STROKE and FILL must restore the current point to that
 * saved by NEWPATH
 */

static integer xx[MAXPATHS], yy[MAXPATHS]; /* the current path in milli-inches */
static integer pathLen = 0;             /* the current path length */
static integer shading = NONE;  /* what to shade the last figure */
static integer penSize = 2;             /* pen size in PS units */

/* forward declarations */
/* static void doShading(); */
static integer zPixRound();             /* (int)(x/y)PixRound((int)x,(int)y) */
static double  shadetp = 0.5;
             /* shading level, initialized as requested by tpic 2.0 -- MJ */

void
setPenSize(cp)
     char *cp;
{
  long ps;

  if (sscanf(cp, " %ld ", &ps) != 1)
    {
      error("Illegal .ps command format");
      return;
    }

  penSize = convPS(ps);
  numout((integer)penSize);
  cmdout("setlinewidth");
}                               /* end setPenSize */

void
addPath(cp)
     char *cp;
{
  long x,y;

  if (++pathLen >= MAXPATHS) error("! Too many points");
  if (sscanf(cp, " %ld %ld ", &x, &y) != 2)
    error("! Malformed path expression");
  xx[pathLen] = x;
  yy[pathLen] = y;
}                               /* end of addPath */

void
arc(cp, invis)
     char *cp;
    int invis;
{
  long xc, yc, xrad, yrad;
  float startAngle, endAngle;

  if (sscanf(cp, " %ld %ld %ld %ld %f %f ", &xc, &yc, &xrad, &yrad,
             &startAngle, &endAngle) != 6)
    {
      error("Illegal arc specification");
      return;
    }

/* To have filled ellipses/circles also have borders, we duplicate the object
   specification; first time we emit a FILL command, second time we emit a
   STROKE command.
   There certainly exists a better way to do that in PostScript, by
   remembering the object specification (path) across the fill command.
   However I (MJ) am completely unproficient at PostScript...
*/

/* we need the newpath since STROKE doesnt do a newpath */

  if (shading) {
      /* first time for shading */
      cmdout(NEWPATH);
      numout((integer)hconvPS(xc));
      numout((integer)vconvPS(yc));
      numout((integer)convPS(xrad));
      if (xrad != yrad && VDPI == DPI)
	numout((integer)convPS(yrad));
      floatout(convDeg(startAngle));
      floatout(convDeg(endAngle));

      if (xrad == yrad && VDPI == DPI)		/* for arcs and circles */
	cmdout("arc");
      else
	cmdout("ellipse");

      cmdout(FILL);
      shading = NONE;
      cmdout("0 setgray");	/* default of black */
  }

  if (!invis) {
      /* There is a problem with tpic 2.0's handling of dotted ellipses.
	 In most (all?) cases, after conversion to degrees and rounding
	 to integers, startAngle and endAngle become the same, so nothing,
	 not even a dot (as requested), is printed in the output (at least
	 on my Apple LaserWriter).
	 So I (MJ) singled out this case.
       */
      float degStAng = convDeg(startAngle),
            degEnAng = convDeg(endAngle);

      cmdout(NEWPATH);		/* save current point */

      if (degStAng != degEnAng) { /* normal case */
	  numout((integer)hconvPS(xc));
	  numout((integer)vconvPS(yc));
	  numout((integer)convPS(xrad));
	  if (xrad != yrad)
	      numout((integer)convPS(yrad));
	  floatout(degStAng);
	  floatout(degEnAng);

	  if (xrad == yrad)	/* for arcs and circles */
	      cmdout("arc");
	  else
	      cmdout("ellipse");
      }
      else {			/* draw a single dot */
	  long xdot, ydot;
	  xdot = (long) ((double)xc + (double)xrad * cos ((startAngle + endAngle) / 2.0));
	  ydot = (long) ((double)yc + (double)yrad * sin ((startAngle + endAngle) / 2.0));
	  numout((integer)hconvPS(xdot));
	  numout((integer)vconvPS(ydot));
	  cmdout(MOVETO);
	  /* +1 to make sure the dot is printed */
	  numout((integer)hconvPS(xdot)+1);
	  numout((integer)vconvPS(ydot)+1);
	  cmdout(LINETO);
      }

      cmdout(STROKE);
  }
}                               /* end of arc */

/*
 * In pic dashed lines are created in a manner which makes boths ends
 * of a line solid.
 * This means that the corner of a dotted box is a dot and the corner
 * of a dashed box is solid.
 * The number of inches/dash must be adjusted accordingly.
 */
void
flushDashedPath(dotted, inchesPerDash)
    int dotted;
    float inchesPerDash;
{
  register int i;
  int nipd = (integer) convPS((int) inchesPerDash);

  if (nipd == 0)
    nipd = 1 ;
  if (pathLen < 2)
    {
      error("Path less than 2 points - ignored");
      pathLen = 0 ;
      return;
    }

  cmdout(NEWPATH);		/* to save the current point */
  for (i=2; i <= pathLen; i++) {
      integer dx = hconvPS(xx[i-1]) - hconvPS(xx[i]);
      integer dy = vconvPS(yy[i-1]) - vconvPS(yy[i]);
      float delta = sqrt((double) (dx * dx + dy * dy));
      float ipd;
      int ndashes;

      /*
       * If a line is dashed there must be an odd number of dashes.
       * This makes the first and last dash of the line solid.
       * If a line is dotted the length of a dot + space sequence must be
       * chosen such that (n+1) * dotlength + n space length = linelength
       * => (dot + space) = (line - dot) / n
       */
      if (dotted)
	  delta -= penSize;
      ndashes = (integer) (delta / nipd + 0.5); /* number of dashes on line */
      /* only odd when dashed, choose odd number with smallest deviation */
      if (ndashes > 2 && (ndashes & 0x1) == 0 && !dotted) {
	if (fabs(nipd - delta/(ndashes - 1)) <
	    (fabs(nipd - delta/(ndashes + 1))))
	    ndashes -= 1;
	else
	    ndashes += 1;
      } else if (ndashes < 1)
         ndashes = 1 ;
      ipd = delta / ndashes;
      if (ipd <= 0.0)
         ipd = 1.0 ;
      cmdout("[");
      if (dotted) {
	numout((integer)penSize);
	floatout(fabs(ipd - penSize));
      } else                               /* if dashed */
	floatout(ipd);

      cmdout("]");
      numout((integer)0);
      cmdout("setdash");
      numout((integer)hconvPS(xx[i-1]));
      numout((integer)vconvPS(yy[i-1]));
      cmdout(MOVETO);
      numout((integer)hconvPS(xx[i]));
      numout((integer)vconvPS(yy[i]));
      cmdout(LINETO);
      cmdout(STROKE);
  }
  cmdout("[] 0 setdash");
  pathLen = 0;
}

void
flushPath(invis)
    int invis;
{
  register int i;

  if (pathLen < 2)
    {
      error("Path less than 2 points - ignored");
      pathLen = 0 ;
      return;
    }

/* To have filled boxes also have borders, we duplicate the object
   specification; first time we emit a FILL command, second time we emit a
   STROKE command.
   There certainly exists a better way to do that in PostScript, by
   remembering the object specification (path) across the fill command.
   However I (MJ) am completely unproficient at PostScript...
*/

#ifdef DEBUG
    if (dd(D_SPECIAL))
        (void)fprintf(stderr,
#ifdef SHORTINT
            "flushpath(1): hh=%ld, vv=%ld, x=%ld, y=%ld, xPS=%ld, yPS=%ld\n",
#else   /* ~SHORTINT */
            "flushpath(1): hh=%d, vv=%d, x=%d, y=%d, xPS=%d, yPS=%d\n",
#endif  /* ~SHORTINT */
                    hh, vv, xx[1], yy[1], hconvPS(xx[1]), vconvPS(yy[1]));
#endif /* DEBUG */
  if (shading) {
      /* first time for shading */
      cmdout(NEWPATH); /* to save the current point */
      numout((integer)hconvPS(xx[1]));
      numout((integer)vconvPS(yy[1]));
      cmdout(MOVETO);
      for (i=2; i < pathLen; i++) {
#ifdef DEBUG
	if (dd(D_SPECIAL))
	    (void)fprintf(stderr,
#ifdef SHORTINT
		"flushpath(%ld): hh=%ld, vv=%ld, x=%ld, y=%ld, xPS=%ld, yPS=%ld\n",
#else   /* ~SHORTINT */
		"flushpath(%d): hh=%d, vv=%d, x=%d, y=%d, xPS=%d, yPS=%d\n",
#endif  /* ~SHORTINT */
			i, hh, vv, xx[i], yy[i], hconvPS(xx[i]), vconvPS(yy[i]));
#endif /* DEBUG */
	numout((integer)hconvPS(xx[i]));
	numout((integer)vconvPS(yy[i]));
	cmdout(LINETO);
	}
      /* Shading should always apply to a closed path
	 force it and complain if necessary */
      if (xx[1] == xx[pathLen] && yy[1] == yy[pathLen])
	cmdout(CLOSEPATH);
      else {
	numout((integer)hconvPS(xx[pathLen]));
	numout((integer)vconvPS(yy[pathLen]));
	cmdout(LINETO);
	cmdout(CLOSEPATH);
	error("Attempt to fill a non-closed path");
	fprintf(stderr,
#ifdef SHORTINT
		"\tfirst point: x=%ld, y=%ld; last point: x=%ld, y=%ld\n",
#else   /* ~SHORTINT */
		"\tfirst point: x=%d, y=%d; last point: x=%d, y=%d\n",
#endif  /* ~SHORTINT */
			xx[1], yy[1], xx[pathLen], yy[pathLen]);

      }
      cmdout(FILL);
      shading = NONE;
      cmdout("0 setgray");	/* default of black */
  }

  if (!invis) {
      cmdout(NEWPATH);		/* to save the current point */
      numout((integer)hconvPS(xx[1]));
      numout((integer)vconvPS(yy[1]));
      cmdout(MOVETO);
      for (i=2; i < pathLen; i++) {
	  numout((integer)hconvPS(xx[i]));
	  numout((integer)vconvPS(yy[i]));
	  cmdout(LINETO);
      }
      if (xx[1] == xx[pathLen] && yy[1] == yy[pathLen])
	  cmdout(CLOSEPATH);
      else {
	  numout((integer)hconvPS(xx[pathLen]));
	  numout((integer)vconvPS(yy[pathLen]));
	  cmdout(LINETO);
      }
      cmdout(STROKE);
  }

  pathLen = 0;
}                               /* end of flushPath */

void
flushDashed(cp, dotted)
     char *cp;
     int dotted;
{
  float inchesPerDash;
  int savelen = pathLen;

  if (sscanf(cp, " %f ", &inchesPerDash) != 1)
    {
      error ("Illegal format for dotted/dashed line");
      return;
    }

  if (inchesPerDash <= 0.0)
    {
      error ("Length of dash/dot cannot be negative");
      return;
    }

  inchesPerDash = 1000 * inchesPerDash; /* to get milli-inches */
  flushPath(1);/* print filled, always invisible bounds */
  pathLen = savelen;
  flushDashedPath(dotted,inchesPerDash);

  cmdout("[] 0 setdash");
}                               /* end of flushDashed */

void
flushSpline(cp)
    char *cp;
{                               /* as exact as psdit!!! */
  register long i, dxi, dyi, dxi1, dyi1;

  if (*cp) {
      float inchesPerDash;
      int ipd ;

      if (sscanf(cp, "%f ", &inchesPerDash) != 1) {
	  error ("Illegal format for dotted/dashed spline");
	  return;
      }

      ipd = (int)(1000.0 * inchesPerDash); /* to get milli-inches */

      if (ipd != 0) {
	  cmdout("[");
	  if (inchesPerDash < 0.0) /* dotted */ {
	      numout((integer)penSize);
	      numout((integer)fabs(convPS((int)-ipd) - penSize));
	  } else		/* dashed */
	      numout((integer)convPS((int)ipd));

	  cmdout("]");
	  numout((integer)0);
	  cmdout("setdash");
      }
  }

  if (pathLen < 2)
    {
      error("Spline less than two points - ignored");
      return;
    }

  cmdout(NEWPATH);      /* to save the current point */
  numout((integer)hconvPS(xx[1]));
  numout((integer)vconvPS(yy[1]));
  cmdout(MOVETO);
  numout((integer)convPS((xx[2]-xx[1])/2));
  numout((integer)convPS((yy[2]-yy[1])/2));
  cmdout(RLINETO);

  for (i=2; i < pathLen; i++)
    {
      dxi = convPS(xx[i] - xx[i-1]);
      dyi = convVPS(yy[i] - yy[i-1]);
      dxi1 = convPS(xx[i+1] - xx[i]);
      dyi1 = convVPS(yy[i+1] - yy[i]);

      numout((integer)dxi/3);
      numout((integer)dyi/3);
      numout((integer)(3*dxi+dxi1)/6);
      numout((integer)(3*dyi+dyi1)/6);
      numout((integer)(dxi+dxi1)/2);
      numout((integer)(dyi+dyi1)/2);
      cmdout(RCURVETO);
    }

  numout((integer)hconvPS(xx[pathLen]));
  numout((integer)vconvPS(yy[pathLen]));
  cmdout(LINETO);

/*  doShading(); */
  cmdout(STROKE);		/* assume no shaded splines */
  pathLen = 0;

  if (*cp)
      cmdout("[] 0 setdash");

}                               /* end of flushSpline */

/* set shading level (used with  Fig 1.4-TFX). priol@irisa.fr, 89/04 */
/* A better trial at setting shading level -- jourdan@minos.inria.fr */
/* Count number of black bits in the pattern together with total number, */
/* compute the average and use this as the PostScript gray level */
void
SetShade(cp)
register char *cp;
{
    int blackbits = 0, totalbits = 0;

    while (*cp) {
	switch (*cp) {
	case '0':
	    totalbits += 4;
	    break;
	case '1':
	case '2':
	case '4':
	case '8':
	    blackbits += 1;
	    totalbits += 4;
	    break;
	case '3':
	case '5':
	case '6':
	case '9':
	case 'a':
	case 'A':
	case 'c':
	case 'C':
	    blackbits += 2;
	    totalbits += 4;
	    break;
	case '7':
	case 'b':
	case 'B':
	case 'd':
	case 'D':
	case 'e':
	case 'E':
	    blackbits += 3;
	    totalbits += 4;
	    break;
	case 'f':
	case 'F':
	    blackbits += 4;
	    totalbits += 4;
	    break;
	case ' ':
	    break;
	default:
	    error("Invalid character in .tx pattern");
	    break;
	}
	cp++;
    }
    shadetp = 1.0 - ((double) blackbits / (double) totalbits);
    shading = GRAY;
}                               /* end of SetShade       */

void
shadeLast(cp)
    char *cp;
{
  char tpout[20];

  if (*cp) {
      float tempShadetp;

      if (sscanf(cp, "%f ", &tempShadetp) != 1)
	  error ("Illegal format for shade level");
      else if (tempShadetp < 0.0 || tempShadetp > 1.0)
	  error ("Invalid shade level");
      else
	  /* if "sh" has an argument we can safely assume that tpic 2.0 is used
	     so that all subsequent "sh" commands will come with an explicit
	     argument.  Hence we may overwrite shadetp's old value */
	  /* Also note the inversion of gray levels for tpic 2.0 (0 = white,
	     1 = black) w.r.t. PostScript (0 = black, 1 = white) */
	  shadetp = 1.0 - tempShadetp;
  }

  shading = GRAY;
  sprintf(tpout,"%1.3f setgray",shadetp); /* priol@irisa.fr, MJ */
  cmdout(tpout);
}                               /* end of shadeLast */

void
whitenLast()
{
  shading = WHITE;
  cmdout("1 setgray");
}                               /* end of whitenLast */

void
blackenLast()
{
  shading = BLACK;
  cmdout("0 setgray");          /* actually this aint needed */
}                               /* end of whitenLast */

/*
static void
doShading()
{
  if (shading)
    {
      cmdout(FILL);
      shading = NONE;
      cmdout("0 setgray");      !* default of black *!
    }
  else
    cmdout(STROKE);
}                               !* end of doShading *!
*/

/*
 *   We need to calculate (x * convDPI * mag) / (tpicResolution * 1000)
 *   So we use floating point.  (This should have a very small impact
 *   on the speed of the program.)
 */
static integer
zPixRound(x, convDPI)      /* return rounded number of pixels */
        register integer x;             /* in DVI units */
        register integer convDPI;       /* dots per inch */
{
   return ((integer)(x * (float)mag * (float)convDPI /
                    (1000.0 * tpicRESOLUTION))) ;
}

#endif /* TPIC */

