#include "cgksincl.h"
#include <stdio.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/*
 * hershey.c   Display all of the Hershey font data in a font file
 *
 * This sample program is intended more so the user can see how the
 * font data is read and used in an (admittedly minimal) application,
 * than as a useable program.
 *
 * Its function is to display all of the hershey font characters on-screen,
 * in a format 8 characters across and 8 vertical.
 *
 * usage:
 *    hershey <device> <fontfile>
 *
 *       where <device> is a supported device ('c' for pc-cga is all
 *                      in this version)
 *             <fontfile> is the name of a hershey font file, e.g.
 *                      hersh.oc1
 *
 * Translated from Fortran to C and GKS (that's why it looks wierd!)
 *    (PRIOR Data Sciences non-ANSI GKS binding used here. Sorry!
 *     Somebody else is welcome to translate to the ANSI binding)
 *
 */

/*
     Translated by Pete Holzmann
         Octopus Enterprises
         19611 La Mar Court
         Cupertino, CA 95014

      Original...  
     .. By James Hurt when with
     ..    Deere and Company
     ..    John Deere Road
     ..    Moline, IL 61265

     .. now with Cognition, Inc.
     ..          900 Technology Park Drive
     ..          Billerica, MA 01821
*/

/* local variables */
   /* the next variables set the percent-of-screen used for each
      character. 'colmax' should be left at 100. */

      float deltac = 12.5, deltar = 12.5, colmax = 100.0;

   	FILE	*INfile,*OUTfile,*fopen(); /* some files */

      /* some variables to record the largest bounding rectangle of
         the displayed characters. Printed when all finished. */

      int minx = 999,miny = 999,maxx=-999,maxy=-999;

/*    .. font data file name */
      char name[80];

/* a forward referenced function */
      void jnumbr();

   float aspect = 1.2;  /* PC graphics screen aspect ratio */

/* GKS local variables */

   int cga();           /* device initializer function */
	int	(*wsinitf)();  /* pointer to the initializer function */
	Ws_id   wss_xxxx;    /* workstation id (an int) */
	Int 	wstyp;         /* ws type, and some vars */
	Drect	maxsurf;       /* max device surface rectangle */
	Int	rastunit[2];   /* device surface in raster units */

	Wc	p1[2];            /* two world coordinates */

/* world coordinate windows */
	Wrect	wrect  = {-17,-17,17,17};
/* NDC coordinate viewport and windows */
	Nrect	nrect = {0.0, 0.0, 1.0, 1.0};
/* DC coordinate viewport */
	Drect	drect = {0.0, 0.0, 319.0, 199.0}; /* work area */


/*
 * scanint: a function to scan an integer, using n characters of
 *          the input file, ignoring newlines. (scanf won't work
 *          because it also ignores blanks)
 */
int scanint(file,n)
FILE *file;
int n;
{
char buf[20];
int i,c;

   for (i=0;i<n;i++){
      while ((c = fgetc(file)) == '\n') ; /* discard spare newlines */
      if (c == EOF) return(-1);
       buf[i] = c;
   }
   
   buf[i] = 0;
   return(atoi(buf));
}

/*
 * Convert desired viewport in percentages into Normalized Device Coords
 * (NDC) for GKS
 */

setview(minx,miny,maxx,maxy)
float minx,miny,maxx,maxy;
{
   Nrect newview;

   newview.n_ll.n_x = minx*nrect.n_ur.n_x/100.0;
   newview.n_ll.n_y = miny*nrect.n_ur.n_y/100.0;
   newview.n_ur.n_x = maxx*nrect.n_ur.n_x/100.0;
   newview.n_ur.n_y = maxy*nrect.n_ur.n_y/100.0;
   /* use normalization transformation number 1 */
   s_viewport(1,&newview);
}

/*
 * GKS uses a polyline function instead of skip, draw and move
 *    functions. The following routines translate from skip/draw/move
 *    to polyline:
 */

int skipflag = 1; /* 1 if next draw is 'pen up' */
int oldx,oldy;

static void
skip()
{
skipflag = TRUE;
}

static void
draw(newx,newy)
int newx,newy;
{
   if (!skipflag) {
      p1[0].w_x = oldx;
      p1[0].w_y = oldy;
      p1[1].w_x = newx;
      p1[1].w_y = newy;
      polyline(2,p1);
   }
   skipflag = FALSE;
   oldx = newx;
   oldy = newy;
}

/*
 * The main program...
 */

main(argc,argv)
int argc;
char **argv;
{
/*    .. file unit number */
      FILE *kfile,*f1,*f2;
/*    .. font data   */
      char line[2][256];
      int x,y;
      float col,row;
/*    .. which data point and which character */
      int     ipnt,ich,nch,i,ichar;

    	if (argc != 3) {
         printf("usage: hershey [c,<other devices?>] file\n");
         exit(1);
      }

    	switch (*argv[1]) {
         case 'c':  wsinitf = cga; break;
     		default:
                    printf("usage: hershey [c,<other devices?>] file\n");
                    exit(1);
    	}

   /* get GKS started */

	open_gks((Ercode (*)())NULL,(Erarea *)NULL,(Size)NULL,(String) "");
	      OUTfile=stdout;
         INfile = NULL;

	wss_xxxx = open_ws(INfile, OUTfile, wsinitf);

	activate(wss_xxxx);

   nrect.n_ur.n_y /= aspect;  /* correct NDC square for pixel aspect ratio */

	/* adjust drect to be the biggest square possible, and adjust for
      aspect ratio */

   drect.d_ur.d_x = drect.d_ur.d_y * aspect;

/* set up the normalization transformation (number 1) from WC to NDC */
	s_window  (1, &wrect);
	s_viewport(1, &nrect);
   s_clip(FALSE);
	sel_cntran(1);
/* set up the workstation transformation from NDC to DC */
	s_w_wind(wss_xxxx, &nrect);
	s_w_view(wss_xxxx, &drect);
	update(wss_xxxx,1);

/*
 * GKS is all set up now, so let's get started...
 */

/*    .. get hershey file name */
      if (!(kfile = fopen(argv[2],"r"))) {
         fprintf(stderr,"Can't open font file '%s'\n",argv[1]);
         exit(1);
         }

/*    .. loop per screen */
label5:
/*		     .. start with a clean sheet */
      clear(wss_xxxx);

/*    .. where to display this character */
      col = 0.0;
      row = 100.0;

/*		     .. loop per character */
      while (TRUE) {

/*		     .. read character number and data */
      if ((ich = scanint(kfile,5)) < 1) {
	         deactivate(wss_xxxx);
            getchar();
         	close_ws(wss_xxxx);
         	close_gks();
            printf("\nDone\n");
            printf("min,max = (%d,%d) (%d,%d)\n",minx,miny,maxx,maxy);
            exit(0);
      }
      nch = scanint(kfile,3);

      for (i=0; i<nch;i++) {
         if ((i==32) ||(i==68) ||(i==104) ||(i==140)) fgetc(kfile); /* skip newlines */
         line[0][i] = fgetc(kfile);
         line[1][i] = fgetc(kfile);
      }
      fgetc(kfile);

/*		     .. select view port (place character on screen) */
      setview(col,row-deltar,col+deltac,row);

/*		     .. identify character */

		jnumbr(ich,4,-15.0,-16.0,6.0);

/*		     .. draw left and right lines */
/*		     .. Note: this data can be used for proportional spacing */

      x=(int)line[0][0] - (int)'R';
      y=(int)line[1][0] - (int)'R';

      skip();
      draw(x,-10);draw(x,10);
      skip();
      draw(y,-10);draw(y,10);


/*		     .. first data point is a move */
      skip();
/*		     .. loop per line of data */
    for (ipnt=1;ipnt<nch;ipnt++) {

/*		     .. process vector number ipnt */
      if (line[0][ipnt] == ' ') {
/*		        .. next data point is a move */
         skip();
      } else {
/*		        .. draw (or move) to this data point */
         x=(int)line[0][ipnt] -(int) 'R';
         y=(int)line[1][ipnt] -(int) 'R';
         if (x < minx) minx = x;
         if (x >maxx) maxx = x;
         if (-y < miny) miny = -y;
         if (-y >maxy) maxy = -y;
/*		        .. Note that Hershey Font data is in TV coordinate system */
		   draw(x,-y);
      }
    } /* for loop */
/*		     .. end of this character */

      if( (col += deltac) < colmax )
         continue;
      col = 0.0;
      if( (row -= deltar) >= deltar ) 
         continue;

      getchar();     /* wait for user to hit a newline */
      goto label5;
   } /* while true */
/*		     .. all done */
      exit();
}


      long power[] ={ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
          100000000, 1000000000 };
      int  start[] ={0,11,14,22,36,42,55,68,73,91,104};
/*		 0:poly(4 9,2 8,1 6,1 3,2 1,4 0,6 1,7 3,7 6,6 8,4 9) */
/*		 1:poly(2 7,4 9,4 0) */
/*		 2:poly(1 8,3 9,5 9,7 8,7 6,6 4,1 0,7 0) */
/*		 3:poly(1 8,3 9,5 9,7 8,7 6,5 5) */
/*		   poly(4 5,5 5,7 4,7 1,5 0,3 0,1 1) */
/*		 4:poly(5 9,5 0) */
/*		   poly(5 9,0 3,8 3) */
/*		 5:poly(2 9,1 5,3 6,4 6,6 5,7 3,6 1,4 0,3 0,1 1) */
/*		   poly(2 9,6 9) */
/*		 6:poly(6 9,4 9,2 8,1 6,1 3,2 1,4 0,6 1,7 3,6 5,4 6,2 5,1 3) */
/*		 7:poly(7 9,3 0) */
/*		   poly(1 9,7 9) */
/*		 8:poly(3 9,1 8,1 6,3 5,5 5,7 6,7 8,5 9,3 9) */
/*		   poly(3 5,1 4,1 1,3 0,5 0,7 1,7 4,5 5) */
/*		 9:poly(7 6,6 4,4 3,2 4,1 6,2 8,4 9,6 8,7 6,7 3,6 1,4 0,2 0) */
/*		 */
    char linedat[]={'R','M','P','N','O','P','O','S','P','U','R','V','T','U',
      'U','S','U','P','T','N','R','M','P','O','R','M','R',
        'V','O','N','Q','M','S','M','U','N','U','P','T','R','O',
      'V','U','V','O','N','Q','M','S','M','U','N','U','P','S','Q',
        ' ','R','R','Q','S','Q','U','R','U','U','S','V','Q','V','O','U',
      'S','M','S','V',' ','R','S','M','N','S','V','S','P',
        'M','O','Q','Q','P','R','P','T','Q','U','S','T','U','R','V','Q',
      'V','O','U',' ','R','P','M','T','M','T','M','R','M','P','N',
        'O','P','O','S','P','U','R','V','T','U','U','S','T','Q','R','P',
      'P','Q','O','S','U','M','Q','V',' ','R','O','M','U','M',
      'Q','M','O','N','O','P','Q','Q','S','Q','U','P','U','N','S',
      'M','Q','M',' ','R','Q','Q','O','R','O','U','Q','V','S','V','U','U',
        'U','R','S','Q','U','P','T','R','R','S','P','R','O','P',
      'P','N','R','M','T','N','U','P','U','S','T','U','R','V','P','V'};
 
#define line(a,b) linedat[(b*2+a)]
void
jnumbr( number, iwidth, x0, y0, height )
      int number, iwidth;
      float x0, y0, height;

{
/*		     .. draw one of the decimal digits */
/*		     .. number = the integer to be displayed */
/*		     .. iwidth = the number of characters */
/*		     .. (x0, y0) = the lower left corner */
/*		     .. height = height of the characters */
/*		 */
/*		 */
/*		     .. By James Hurt when with */
/*		     ..    Deere and Company */
/*		     ..    John Deere Road */
/*		     ..    Moline, IL 61265 */
/*		 */
/*		     .. Author now with Cognition, Inc. */
/*		     ..                 900 Technology Park Drive */
/*		     ..                 Billerica, MA 01821 */
/*		 */

/*		     .. local variables used */
      int ipnt, ipos, ival, idigit;
      float x, y, scale;
      float xleft, ylower;

/*		     .. character data for the ten decimal digit characters */
/*		     .. data extracted from one of the Hershey fonts */

/*	        .. compute scale factor and lower left of first digit */
      scale = height/10.0;
      xleft = x0;
      ylower = y0;
      ival = number;

/*		     .. loop for each character */

      for (ipos = iwidth;ipos>=1;ipos--) {
         idigit = (ival/power[ipos-1])% 10;

/*		        .. first data point is a move */
         skip();

/*		        .. loop over data for this digit */
         for ( ipnt=start[idigit]; ipnt < start[idigit+1];ipnt++) {
            if(((char)line(0,ipnt)) == ' ') {
               skip();   /* next data point is a move */
            } else {
/*		              .. draw (or move) to this data point */
               x=(int)line(0,ipnt) -(int) 'N';
               y=(int)line(1,ipnt) -(int) 'V';

         		draw((int)(xleft+scale*x),(int)(ylower-scale*y));
            }
          } /* data for this digit */
/*		        .. move for next digit */
         xleft += height;
       } /* whole string */
}
