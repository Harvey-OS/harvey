/*
 *   emspecial.c
 *   This routine handles the emTeX special commands.
 */
#include "dvips.h" /* The copyright notice in that file is included too!*/

#include <ctype.h>
extern int atoi();
extern FILE *search();
extern char *getenv();
/*
 *   These are the external routines called:
 */
/**/
extern void hvpos() ;
extern void cmdout() ;
extern void mhexout() ;
extern void nlcmdout() ;
extern void newline() ;
extern void floatout() ;
extern void numout() ;
extern void error() ;
extern void specerror() ;
extern char errbuf[] ;
extern shalfword linepos;
extern FILE *bitfile;
extern int actualdpi ;
extern int vactualdpi ;
extern integer hh, vv;
extern char *figpath ;
extern int prettycolumn ;
extern int quiet;
extern Boolean disablecomments ;

#ifdef DEBUG
extern integer debug_flag;
#endif


#ifdef EMTEX
/* emtex specials, added by rjl */

#define TRUE 1
#define FALSE 0

static long emmax = 161 ;

/*
 *   We define these seek constants if they don't have their
 *   values already defined.
 */
#ifndef SEEK_SET
#define SEEK_SET (0)
#endif
#ifndef SEEK_END
#define SEEK_END (2)
#endif

struct empt {
   struct empt *next ;
   shalfword point;
   integer x, y;
};

static struct empt **empoints = NULL ;
boolean emused = FALSE;  /* true if em points used on this page */
integer emx, emy;

struct emunit {
   char *unit;
   float factor;
};
struct emunit emtable[] = {
  {"pt",72.27},
  {"pc",72.27/12},
  {"in",1.0},
  {"bp",72.0},
  {"cm",2.54},
  {"mm",25.4},
  {"dd",72.27/(1238.0/1157)},
  {"cc",72.27/12/(1238.0/1157)},
  {"sp",72.27*65536},
  {0,0.0}
};


/* clear the empoints array if necessary */
void
emclear()
{
   int i;
   if (emused && empoints)
      for (i=0; i<emmax; i++)
         empoints[i] = 0 ;
   emused = FALSE ;
}

/* put an empoint into the empoints array */
struct empt *emptput(point, x, y)
shalfword point;
integer x, y;
{
   struct empt *p ;
   int start ;

   emused = TRUE;
   start = point % emmax ;
   p = empoints[start] ;
   while ( p ) {
      if ( p->point == point )
         break;
      p = p->next ;
   }
   if (p == 0) {
      p = (struct empt *)mymalloc(sizeof(struct empt)) ;
      p->next = empoints[start] ;
      empoints[start] = p ;
   }
   p->point = point;
   p->x = x;
   p->y = y;
   return(p) ;
}

/* get an empoint from the empoints array */
struct empt *emptget(point)
shalfword point;
{
   struct empt *p ;
   int start;

   start = point % emmax;
   if (emused == TRUE) {
      p = empoints[start] ;
      while (p) {
	 if (p->point == point)
	    return p ;
	 p = p->next ;
      }
   }
   sprintf(errbuf,"!em: point %d not defined",point);
   specerror(errbuf);
   return(NULL); /* never returns due to error */
}


/* convert width into dpi units */
float emunits(width,unit)
float width;
char *unit;
{
struct emunit *p;
	for (p=emtable; p->unit; p++) {
	   if (strcmp(p->unit,unit)==0)
		return( width * actualdpi / p->factor );
	}
	return (-1.0); /* invalid unit */
}

/* The main routine for \special{em:graph ...} called from dospecial.c */
/* the line cut parameter is not supported (and is ignored) */

void emspecial(p)
char *p ;
{
float emwidth, emheight;
shalfword empoint1, empoint2;
struct empt *empoint;
char emunit[3];
char emstr[80];
char *emp;
void emgraph();

        hvpos() ;
	for (emp = p+3; *emp && isspace(*emp); emp++); /* skip blanks */
	if (strncmp(emp, "linewidth", 9) == 0) {
	   /* code for linewidth */
	   for (emp = emp+9; *emp && isspace(*emp); emp++); /* skip blanks */
	   sscanf(emp, "%f%2s", &emwidth, emunit);
	   emwidth = emunits(emwidth,emunit);
	   if (emwidth!=-1.0) {
	      sprintf(emstr,"%.1f setlinewidth", emwidth);
	      cmdout(emstr);
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "em special: Linewidth set to %.1f dots\n", 
		emwidth) ;
#endif
	   } else {
	      sprintf(errbuf,"Unknown em: special width");
	      specerror(errbuf);
	   }
	}
        else if (strncmp(emp, "moveto", 6) == 0) {
#ifdef DEBUG
   if (dd(D_SPECIAL))
#ifdef SHORTINT
      (void)fprintf(stderr, "em special: moveto %ld,%ld\n", hh, vv);
#else
      (void)fprintf(stderr, "em special: moveto %d,%d\n", hh, vv);
#endif
#endif
           emx = hh;
           emy = vv;
        }
        else if (strncmp(emp, "lineto", 6) == 0) {
#ifdef DEBUG
   if (dd(D_SPECIAL))
#ifdef SHORTINT
      (void)fprintf(stderr, "em special: lineto %ld,%ld\n", hh, vv);
#else
      (void)fprintf(stderr, "em special: lineto %d,%d\n", hh, vv);
#endif
#endif
	   cmdout("np");
	   numout(emx);
	   numout(emy);
	   cmdout("a");
	   numout(hh);
	   numout(vv);
	   cmdout("li");
	   cmdout("st");
           emx = hh;
           emy = vv;
        }
	else if (strncmp(emp, "point", 5) == 0) {
           if (empoints == NULL) {
              empoints = 
              (struct empt **)mymalloc((integer)emmax * sizeof(struct empt *)) ;
              emused = TRUE;
              emclear();
           }
	   for (emp = emp+5; *emp && isspace(*emp); emp++); /* skip blanks */
           empoint1 = (shalfword)atoi(emp);
           empoint = emptput(empoint1,hh,vv);
#ifdef DEBUG
   if (dd(D_SPECIAL))
#ifdef SHORTINT
      (void)fprintf(stderr, "em special: Point %d is %ld,%ld\n",
#else
      (void)fprintf(stderr, "em special: Point %d is %d,%d\n",
#endif
		empoint->point, empoint->x, empoint->y) ;
#endif
	}
	else if (strncmp(emp, "line", 4) == 0) {
	   for (emp = emp+4; *emp && isspace(*emp); emp++); /* skip blanks */
           empoint1 = (shalfword)atoi(emp);
	   for (; *emp && isdigit(*emp); emp++); /* skip point 1 */
	   if ( *emp && strchr("hvp",*emp)!=0 )
	      emp++;  /* skip line cut */
	   for (; *emp && isspace(*emp); emp++); /* skip blanks */
	   if ( *emp && (*emp==',') )
	      emp++; /*  skip comma separator */
	   for (; *emp && isspace(*emp); emp++); /* skip blanks */
           empoint2 = (shalfword)atoi(emp);
	   for (; *emp && isdigit(*emp); emp++); /* skip point 2 */
	   if ( *emp && strchr("hvp",*emp)!=0 )
	      emp++;  /* skip line cut */
	   for (; *emp && isspace(*emp); emp++); /* skip blanks */
	   if ( *emp && (*emp==',') )
	      emp++; /*  skip comma separator */
	   emwidth = -1.0;
	   emunit[0]='\0';
	   sscanf(emp, "%f%2s", &emwidth, emunit);
	   emwidth = emunits(emwidth,emunit);
#ifdef DEBUG
   if (dd(D_SPECIAL))
      (void)fprintf(stderr, "em special: Line from point %d to point %d\n",
		empoint1, empoint2) ;
#endif
	   cmdout("np");
	   if (emwidth!=-1.0) {
#ifdef DEBUG
   if (dd(D_SPECIAL))
   (void)fprintf(stderr,"em special: Linewidth temporarily set to %.1f dots\n", 
		emwidth) ;
#endif
	   	strcpy(emstr,"currentlinewidth");
	   	cmdout(emstr);
	        sprintf(emstr,"%.1f setlinewidth", emwidth);
	        cmdout(emstr);
	   }
           empoint = emptget(empoint1);
	   numout(empoint->x);
	   numout(empoint->y);
	   cmdout("a");
           empoint = emptget(empoint2);
	   numout(empoint->x);
	   numout(empoint->y);
	   cmdout("li");
	   cmdout("st");
	   if (emwidth!=-1.0) {
	   	strcpy(emstr,"setlinewidth");
	   	cmdout(emstr);
	   }
	}
	else if (strncmp(emp, "message", 7) == 0) {
           (void)fprintf(stderr, "em message: %s\n", emp+7) ;
	}
	else if (strncmp(emp, "graph", 5) == 0) {
	   int i;
	   for (emp = emp+5; *emp && isspace(*emp); emp++); /* skip blanks */
	   for (i=0; *emp && !isspace(*emp) && !(*emp==',') ; emp++)
	      emstr[i++] = *emp; /* copy filename */
	   emstr[i] = '\0';
	   /* now get optional width and height */
	   emwidth = emheight = -1.0;	/* no dimension is <= 0 */
	   for (; *emp && ( isspace(*emp) || (*emp==',') ); emp++)
	      ;  /* skip blanks and comma */
	   if (*emp) {
	      sscanf(emp, "%f%2s", &emwidth, emunit); /* read width */
	      emwidth = emunits(emwidth,emunit); /* convert to pixels */
	      for (; *emp && (*emp=='.'||isdigit(*emp)||isalpha(*emp)); emp++)
	         ; /* skip width dimension */
	      for (; *emp && ( isspace(*emp) || (*emp==',') ); emp++)
	         ;  /* skip blanks and comma */
	      if (*emp) {
	         sscanf(emp, "%f%2s", &emheight, emunit); /* read height */
	         emheight = emunits(emheight,emunit)*vactualdpi/actualdpi;
	      }
	   }
	   if (emstr[0]) {
	      emgraph(emstr,emwidth,emheight);
	   }
	   else {
              (void)fprintf(stderr, "em:graph: no file given\n") ;
	   }
	}
	else {
           sprintf(errbuf, 
	      "Unknown em: command (%s) in \\special will be ignored", p);
           specerror(errbuf) ;
	}
	return;
   }


/* em:graph routines */

/* The graphics routines currently decode 3 types of IBM-PC based graphics   */
/* files: .pcx, .bmp, .msp. The information on the formats has occasionally  */
/* been sketchy, and subject to interpretation. I have attempted to implement*/
/* these routines to correctly decode the graphics file types mentioned.     */
/* The compressed .bmp file type has not been tested fully due to a lack of  */
/* test files.                                                               */
/*                                                                           */
/* The method of reading in the headers of the binary files is ungainly, but */
/* portable. Failure to use a byte by byte read and convert method will      */
/* result in portability problems. Specifically there is no requirement that */
/* elements of a C structure be contiguous, only that they have an ascending */
/* order of addresses.                                                       */
/*                                                                           */
/* The current implementations of the graphics format to postscript          */
/* conversion are not the most efficient possible. Specifically: color       */
/* formats are converted to RGB ratios prior to using a simple thresholding  */
/* method to determine if the postscript bit is to be set. The thresholding  */
/* method is compabible with that used in emtex's drivers.                   */
/*                                                                           */
/* Please send bug reports relating to the em:graph routines to:             */
/*                maurice@bruce.cs.monash.edu.au                             */
/*                                                                           */
/* My thanks to Russell Lang for his assistance with the implementation of   */
/* these routines in a manner compatible with dvips and for the optimization */
/* of some routines.                                                         */
/*                                                                           */
/*                                                   Maurice Castro          */
/*                                                   8 Oct 92                */

/* Routines to read binary numbers from IBM PC file types */
integer readinteger(f)
FILE *f;
{
   integer i;
   int r;

   i = 0;
   for (r = 0; r < 4; r++)
   {
      i = i |  ( ((integer) fgetc(f)) << (8*r) );
   }
   return(i);
}

halfword readhalfword(f)
FILE *f;
{
   halfword i;
   int r;
 
   i = 0;
   for (r = 0; r < 2; r++)
   {
     i = i |  ( ((halfword) fgetc(f)) << (8*r) );
   }
   return(i);
}

#define readquarterword(f) ((unsigned char)fgetc(f))
#define tobyte(x) ((x/8) + (x%8 ? 1 : 0))

/* These routines will decode PCX files produced by Microsoft 
 * Windows Paint.  Other programs may not produce files which
 * will be successfully decoded, most notably version 1.xx of
 * PC Paint. */

/*
 *   Type declarations.  integer must be a 32-bit signed; shalfword must
 *   be a sixteen-bit signed; halfword must be a sixteen-bit unsigned;
 *   quarterword must be an eight-bit unsigned.
 */
typedef struct 
{
	quarterword man;
	quarterword ver;
	quarterword enc;
	quarterword bitperpix;
	halfword xmin;
	halfword ymin;
	halfword xmax;
	halfword ymax;
	halfword hres;
	halfword vres;
	quarterword pal[48];
	quarterword reserved;
	quarterword colorplanes;
	halfword byteperline;
	halfword paltype;
	quarterword fill[58];
} PCXHEAD; 

int PCXreadhead(pcxf, pcxh)
FILE *pcxf;
PCXHEAD *pcxh;
{
	pcxh->man = readquarterword(pcxf);
	pcxh->ver = readquarterword(pcxf);
	pcxh->enc = readquarterword(pcxf);
	pcxh->bitperpix = readquarterword(pcxf);
	pcxh->xmin = readhalfword(pcxf);
	pcxh->ymin = readhalfword(pcxf);
	pcxh->xmax = readhalfword(pcxf);
	pcxh->ymax = readhalfword(pcxf);
	pcxh->hres = readhalfword(pcxf);
	pcxh->vres = readhalfword(pcxf);
	fread(pcxh->pal, 1, 48, pcxf);
	pcxh->reserved = readquarterword(pcxf);
	pcxh->colorplanes = readquarterword(pcxf);
	pcxh->byteperline = readhalfword(pcxf);
	pcxh->paltype = readhalfword(pcxf);
	fread(pcxh->fill, 1, 58, pcxf);

	if (feof(pcxf))
		return(0);				/* fail */
	if (pcxh->man != 0x0a)
		return(0);				/* fail */
	if (pcxh->enc != 0x1)
		return(0);				/* fail */
	return(1);					/* success */
	}

int PCXreadline(pcxf, pcxbuf, byteperline)
FILE *pcxf;
unsigned char *pcxbuf;
unsigned int byteperline;
{
	int n;
	int c;
	int i;

	n = 0;
	memset(pcxbuf,0,byteperline);
	do {
		c = fgetc(pcxf);
		if ((c & 0xc0) == 0xc0) {
			i = c & 0x3f;
			c = fgetc(pcxf) & 0xff;
			while ((i--) && (n < byteperline)) pcxbuf[n++] = c;
		}
		else {
			pcxbuf[n++] = c & 0xff;
		}
	} while (n < byteperline);
	if (c==EOF) n=0;
	return(n);
}

void PCXgetpalette(pcxf, pcxh, r, g, b)
FILE *pcxf;
PCXHEAD *pcxh;
unsigned char r[256];
unsigned char g[256];
unsigned char b[256];
{
	int i;

	/* clear palette */
	for (i=0; i < 256; i++) {
		r[i] = 0;
		g[i] = 0;
		b[i] = 0;
	}

	switch (pcxh->ver) {
		case 0:
			/* version 2.5 of PC Paint Brush */
			for (i=0; i < 16; i++) {
				r[i] = pcxh->pal[i*3];
				g[i] = pcxh->pal[i*3+1];
				b[i] = pcxh->pal[i*3+2];
			}
			break;
		case 2:
			if (pcxh->colorplanes != 1) {
			    /* version 2.8 of PC Paint Brush with valid Palette */
			for (i=0; i < 16; i++) {
				r[i] = pcxh->pal[i*3];
				g[i] = pcxh->pal[i*3+1];
				b[i] = pcxh->pal[i*3+2];
			    }
			}
			else {	/* not sure if this is correct - rjl */
				/* mono palette */
				r[0] = 0x00; g[0] = 0x00; b[0] = 0x00;
				r[1] = 0xff; g[1] = 0xff; b[1] = 0xff;
			}
			break;
		case 3:
			/* version 2.8 of PC Paint Brush with no valid Palette */
			/* either mono or default */

			if (pcxh->colorplanes != 1) {
				/* Color default palette - should be checked */
				r[0] = 0x00;		g[0] = 0x00;		b[0] = 0x00;
				r[1] = 0x80;		g[1] = 0x00; 		b[1] = 0x00;
				r[2] = 0x00; 		g[2] = 0x80; 		b[2] = 0x00;
				r[3] = 0x80;		g[3] = 0x80;	 	b[3] = 0x00;
				r[4] = 0x00;		g[4] = 0x00; 		b[4] = 0x80;
				r[5] = 0x80;		g[5] = 0x00; 		b[5] = 0x80;
				r[6] = 0x00;		g[6] = 0x80;	 	b[6] = 0x80;
				r[7] = 0x80;		g[7] = 0x80; 		b[7] = 0x80;
				r[8] = 0xc0;		g[8] = 0xc0;		b[8] = 0xc0;
				r[9] = 0xff;		g[9] = 0x00; 		b[9] = 0x00;
				r[10] = 0x00; 		g[10] = 0xff; 		b[10] = 0x00;
				r[11] = 0xff; 		g[11] = 0xff; 		b[11] = 0x00;
				r[12] = 0x00; 		g[12] = 0x00; 		b[12] = 0xff;
				r[13] = 0xff; 		g[13] = 0x00; 		b[13] = 0xff;
				r[14] = 0x00; 		g[14] = 0xff; 		b[14] = 0xff;
				r[15] = 0xff; 		g[15] = 0xff; 		b[15] = 0xff;
			}
			else {
				/* mono palette */
				r[0] = 0x00;		g[0] = 0x00;		b[0] = 0x00;
				r[1] = 0xff;		g[1] = 0xff; 		b[1] = 0xff;
			}
			break;
		case 5:
		default:
			/* version 3.0 of PC Paint Brush or Better */
			fseek(pcxf, -769L, SEEK_END);
			/* if signature byte is correct then read the palette */
			/* otherwise copy the existing palette */
			if (fgetc(pcxf) == 12) {
				for (i=0; i < 256; i++) {
					r[i] = fgetc(pcxf);
					g[i] = fgetc(pcxf);
					b[i] = fgetc(pcxf);
				}
			}
			else {
				for (i=0; i < 16; i++) {
					r[i] = pcxh->pal[i*3];
					g[i] = pcxh->pal[i*3+1];
					b[i] = pcxh->pal[i*3+2];
				}
			}
			break;
	}
}

extern void mhexout() ;

void PCXshowpicture(pcxf, wide, high, bytes, cp, bp, r, g, b)
FILE *pcxf;
int wide;
int high;
int bytes;
int cp;
int bp;
unsigned char r[256];
unsigned char g[256];
unsigned char b[256];
{
	int x;
	int y;
	int c;
	unsigned char *rowa[4];				/* row array */
	unsigned char *row;
	int p;
	unsigned char *pshexa;
	int xdiv, xmod, xbit;
	int width;
	int bytewidth;

	bytewidth = tobyte(wide);
	width = bytewidth * 8;

	/* output the postscript image size preamble */
	cmdout("/picstr ");
	numout((integer)tobyte(wide));
	cmdout("string def");

	numout((integer)width);
	numout((integer)high);
	numout((integer)1);
	newline();

	cmdout("[");
	numout((integer)width);
	numout((integer)0);
	numout((integer)0);
	numout((integer)-high);
	numout((integer)0);
	numout((integer)0);
	cmdout("]");

	nlcmdout("{currentfile picstr readhexstring pop} image");

	/* allocate the postscript hex output array */
	pshexa = (unsigned char *) mymalloc((integer)bytewidth);

	/* make sure that you start at the right point in the file */
	fseek(pcxf, (long) sizeof(PCXHEAD), SEEK_SET);

	/* malloc the lines */
	row = (unsigned char *) mymalloc((integer)bytes * cp);
	for (c = 0; c < cp; c++)
		rowa[c] = row + bytes*c;

	for (y = 0; y < high; y++) {
		/* clear the postscript hex array */
		memset(pshexa,0xff,bytewidth);

		/* read in all the color planes for a row of pixels */
		PCXreadline(pcxf, rowa[0], bytes*cp);

		/* process each pixel */
		for (x = 0; x < wide; x++) {
			/* build up the pixel value from color plane entries */
			p = 0;
			xdiv = x>>3;
			xmod = 7 - (x&7);
			xbit = 1 << xmod;
			switch(bp) {
				case 1: 
					for (c = 0; c < cp; c++) {
						row = rowa[c];
						p |= ( (unsigned)(row[xdiv] & xbit) >> xmod ) << c;
					}
					break;
				case 4:  /* untested - most programs use 1 bit/pixel, 4 planes */
					row = rowa[0]; /* assume single plane */
					p = ( x & 1 ? row[xdiv] : row[xdiv]>>4 ) & 0x0f;
					break;
				case 8:
					row = rowa[0]; /* assume single plane */
					p = (unsigned) (row[x]);
					break;
				default: 
					fprintf(stderr, "em:graph: Unable to Decode this PCX file\n");
					return;
			}
			if ((r[p] < 0xff) || (g[p] < 0xff) || (b[p] < 0xff))
				pshexa[xdiv] &= (~xbit);
			else
				pshexa[xdiv] |= xbit;
		}
		newline();
		mhexout(pshexa,(long)bytewidth);
	}
	free(pshexa);
	free(rowa[0]);
}

void imagehead(filename,wide,high,emwidth,emheight)
char filename[];
int wide, high;
float emwidth, emheight;	/* dimension in pixels */
{
	if (!quiet) {
	    if (strlen(filename) + prettycolumn > STDOUTSIZE) {
		fprintf(stderr,"\n");
		prettycolumn = 0;
	    }
	    (void)fprintf(stderr,"<%s",filename);
	    (void)fflush(stderr);
	    prettycolumn += 2+strlen(filename);
	}
	hvpos();
	nlcmdout("@beginspecial @setspecial") ;
	if (!disablecomments) {
		cmdout("%%BeginDocument: em:graph");
		cmdout(filename);
		newline();
	}
	/* set the image size */
	if (emwidth <= 0.0)  emwidth = (float)wide;
	if (emheight <= 0.0)  emheight = (float)high;
	floatout(emwidth*72/actualdpi);
	floatout(emheight*72/vactualdpi);
	newline();
	cmdout("scale");
#ifdef DEBUG
	if (dd(D_SPECIAL)) {
	  (void)fprintf(stderr, 
	    "\nem:graph: %s width  %d pixels scaled to %.1f pixels\n",
	    filename, wide, emwidth);
	  (void)fprintf(stderr, 
	    "em:graph: %s height %d pixels scaled to %.1f pixels\n",
	    filename, high, emheight);
   	}
#endif
}

void imagetail()
{
	if (!disablecomments) {
	    (void)fprintf(bitfile, "\n%%%%EndDocument\n") ;
	    linepos = 0;
	}
	nlcmdout("@endspecial") ;
	if (!quiet) {
	    (void)fprintf(stderr,">");
	    (void)fflush(stderr);
	}
}

void pcxgraph(pcxf,filename,emwidth,emheight)
FILE *pcxf;
char filename[];
float emwidth, emheight;	/* dimension in pixels */
{
	PCXHEAD pcxh;
	unsigned char red[256];
	unsigned char green[256];
	unsigned char blue[256];
	int wide;
	int high;
	int bytes;
	int cp;
	int bpp;

	if (!PCXreadhead(pcxf, &pcxh)) {
		sprintf(errbuf,"em:graph: Unable to Read Valid PCX Header");
		specerror(errbuf);
	}
	PCXgetpalette(pcxf, &pcxh, red, green, blue);

	/* picture size calculation */
	wide = (pcxh.xmax - pcxh.xmin) + 1;
	high = (pcxh.ymax - pcxh.ymin) + 1;
	bytes = pcxh.byteperline;
	cp = pcxh.colorplanes;
	bpp = pcxh.bitperpix;

	imagehead(filename,wide,high,emwidth,emheight);
	PCXshowpicture(pcxf, wide, high, bytes, cp, bpp, red, green, blue);
	imagetail();
}

/* Microsoft Paint routines */
struct wpnt_1 {
	quarterword id[4];
	halfword width;
	halfword high;
	halfword x_asp;
	halfword y_asp;
	halfword x_asp_prn;
	halfword y_asp_prn;
	halfword width_prn;
	halfword high_prn;
	integer chk_sum;
	halfword chk_head;
};

#define WPAINT_1 1
#define WPAINT_2 2

void MSP_2_ps(f, wide, high)
FILE *f;
int wide;
int high;
{
	char *line;
	char *l;
	int i;
	int j;
	unsigned char a, b, c;
	int d;
	int width;
	halfword *linelen;

	/* an undocumented format - based on a type of run length encoding    */
	/* the format is made up of a list of line lengths, followed by a set */
	/* of lines. Each line is made up of 2 types of entries:              */
	/* 	1) A 3 term entry (0 a b): the byte b is repeated a times     */
	/*	2) A variable length entry (a xxxx....xxxx): a bytes are read */
	/*	   from the file.                                             */
	/* These entries are combined to build up a line                      */

	width = tobyte(wide)*8;

	/* output the postscript image size preamble */
	cmdout("/picstr");
	numout((integer)tobyte(wide));
	cmdout("string def");

	numout((integer)width);
	numout((integer)high);
	numout((integer)1);

	cmdout("[");
	numout((integer)width);
	numout((integer)0);
	numout((integer)0);
	numout((integer)-high);
	numout((integer)0);
	numout((integer)0);
	cmdout("]");

	nlcmdout("{currentfile picstr readhexstring pop} image");

	fseek(f, 32, SEEK_SET);

	/* read in the table of line lengths */	
	linelen = (halfword *) mymalloc((integer)sizeof(halfword) * high);
	for (i = 0; i < high; i++) {
		linelen[i] = readhalfword(f);
		if (feof(f))
			return;
	}

	line = (char *) mymalloc((integer)tobyte(wide));
	for (i = 0; i < high; i++) {
		memset(line, 0xff, tobyte(wide));
		l = line;
		if (linelen[i] != 0) {
			d = linelen[i];
			while (d) {
				a = fgetc(f);
				d--;
				if (a == 0) {
					b = fgetc(f);
					c = fgetc(f);
					d -= 2;
					for (j = 0; j < b; j++)
						*l++ = c;
				}
				else {
					for (j = 0; j < a; j++)
						*l++ = fgetc(f);
					d -= j;
				}
			}
		}
		newline();
		mhexout(line,(long)tobyte(wide));
	}
	free(linelen);
	free(line);
}

void MSP_1_ps(f, wide, high)
FILE *f;
int wide;
int high;
{
	char *line;
	int i;
	int width;

	width = tobyte(wide)*8;
	/* an partly documented format - see The PC Sourcebook                */
	/* the format is made up of a simple bitmap.                          */

	/* output the postscript image size preamble */
	cmdout("/picstr");
	numout((integer)tobyte(wide));
	cmdout("string def");

	numout((integer)width);
	numout((integer)high);
	numout((integer)1);

	cmdout("[");
	numout((integer)width);
	numout((integer)0);
	numout((integer)0);
	numout((integer)-high);
	numout((integer)0);
	numout((integer)0);
	cmdout("]");

	nlcmdout("{currentfile picstr readhexstring pop} image");

	fseek(f, 32, SEEK_SET);

	line = (char *) mymalloc((integer)tobyte(wide));
	for (i = 0; i < high; i++) {
		fread(line, 1, tobyte(wide), f);
		newline();
		mhexout(line,(long)tobyte(wide));
	}
	free(line);
}


void mspgraph(f,filename,emwidth,emheight)
FILE *f;
char filename[];
float emwidth, emheight;	/* dimension in pixels */
{
	struct wpnt_1 head;
	int paint_type = 0;

        /* read the header of the file and figure out what it is */
	fread(head.id, 1, 4, f);
	head.width = readhalfword(f); 
	head.high = readhalfword(f); 
	head.x_asp = readhalfword(f); 
	head.y_asp = readhalfword(f); 
	head.x_asp_prn = readhalfword(f); 
	head.y_asp_prn = readhalfword(f); 
	head.width_prn = readhalfword(f); 
	head.high_prn = readhalfword(f); 
	head.chk_sum = readinteger(f); 
	head.chk_head = readhalfword(f); 

	if (feof(f)) {
		fprintf(stderr, "em:graph: Unable to Read Valid MSP Header\n");
		return;
	}
		
        /* check the id bytes */
	if (!memcmp(head.id, "DanM", 4))
        	paint_type = WPAINT_1;
	if (!memcmp(head.id, "LinS", 4))
		paint_type = WPAINT_2;


	imagehead(filename,head.width,head.high,emwidth,emheight);
	switch (paint_type) {
		case WPAINT_1:
                	MSP_1_ps(f, head.width, head.high);
			break;
                case WPAINT_2:
                	MSP_2_ps(f, head.width, head.high);
			break;
		default:
			sprintf(errbuf, "em:graph: Unknown MSP File Type");
			specerror(errbuf);
	}
	imagetail() ;
}

/* ------------------------------------------------------------------------ */
/* .BMP file structures */
struct rgbquad {
	char blue;
	char green;
	char red;
	char res;
};

struct bitmapinfoheader {
	integer size;
	integer width;
	integer height;
	halfword planes;			/* must be set to 1 */
	halfword bitcount;			/* 1, 4, 8 or 24 */
	integer compression;
	integer sizeimage;
	integer xpelspermeter;
	integer ypelspermeter;
	integer clrused;
	integer clrimportant;
};

/* constants for the compression field */
#define RGB 0L
#define RLE8 1L
#define RLE4 2L

struct bitmapfileheader {
	char type[2];
	integer size;
	halfword reserved1;
	halfword reserved2;
	integer offbits;
};

void rgbread(f, w, b, s)
FILE *f;
int b;
int w;
char *s;
{
	int i;

	/* set the line to white */
	memset(s, 0xff, ((w*b)/8)+1); 

	/* read in all the full bytes */
	for (i = 0; i < (w * b) / 8; i++)
		*s++ = fgetc(f);

	/* read in a partly filled byte */
	if ((w * b) % 8) {
		i++;
		*s++ = fgetc(f);
	}

	/* check that we are on a 32 bit boundary; otherwise align */
	while (i % 4 != 0) {
		fgetc(f);
		i++;
	}
}

unsigned rle_dx = 0;	/* delta command horizontal offset */
unsigned rle_dy = 0;	/* delta command vertical offset */

/* checked against output from Borland Resource Workshop */
void rle4read(f, w, b, s)
FILE *f;
int b;
int w;
char *s;
{
	int i;
	int limit;
	int ch;
	unsigned cnt;
	int hi;

	limit = (w*b)/8;
	i = 0;
	hi = TRUE;
	/* set the line to white */
	memset(s, 0xff, limit+1); 

	if (rle_dy) {
	    rle_dy--;
	    return;
	}

	if (rle_dx) {
	    for ( ; rle_dx>1; rle_dx-=2) {
		s++;
		i++;
	    }
	    if (rle_dx)
	    	hi = FALSE;
	}

	while (i<=limit) {
	    cnt = fgetc(f);
	    ch  = fgetc(f);
	    if (cnt == 0) { /* special operation */
		switch(ch) {
		    case 0:	/* EOL */
			return;	/* this is our way out */
		    case 1:	/* End of Bitmap */
		    	return;
		    case 2:	/* Delta */  /* untested */
			rle_dx = fgetc(f) + i*2 + (hi ? 1 : 0);
			rle_dy = fgetc(f);
			return;
		    default:	/* next cnt bytes are absolute */
		    	/* must be aligned on word boundary */
			if (!hi)
				fprintf(stderr,"em:graph: RLE4 absolute is not byte aligned\n");
			for (cnt = ch; cnt>0 && i<=limit; cnt-=2) {
		    	    i++;
			    *s++ = fgetc(f);
			}
			if (ch % 4)	/* word align file */
			    (void)fgetc(f);
  		}
	    }
	    else {   /* cnt is repeat count */
		if (!hi) {   /* we are about to place the low 4 bits */
		    ch = ((ch>>4)&0x0f) | ((ch<<4)&0xf0); /* swap order */
		    i++;
		    *s++ = (*s & 0xf0) | (ch & 0x0f);
		    hi = TRUE;
		    cnt--;
		}
		/* we are about to place the high 4 bits */
		for ( ; cnt>1 && i<=limit ; cnt-=2) { /* place the whole bytes */
		    i++;
		    *s++ = ch;
	        }
		if (cnt) { /* place the partial byte */
		    *s = (*s & 0x0f) | (ch & 0xf0);
		    hi = FALSE;
		}
	    }
  	}
}
  
/* untested */
void rle8read(f, w, b, s)
FILE *f;
int b;
int w;
char *s;
{
	int i;
	int limit;
	int ch;
	unsigned cnt;

	limit = (w*b)/8;
	i = 0;
	/* set the line to white */
	memset(s, 0xff, limit+1); 

	if (rle_dy) {
	    rle_dy--;
	    return;
	}

	if (rle_dx) {
	    for ( ; rle_dx > 0; rle_dx--) {
		s++;
		i++;
	    }
	}

	while (i<=limit) {
	    cnt = fgetc(f);
	    ch  = fgetc(f);
	    if (cnt == 0) { /* special operation */
		switch(ch) {
		    case 0:	/* EOL */
			return;	/* this is our way out */
		    case 1:	/* End of Bitmap */
			return;
		    case 2:	/* Delta */  /* untested */
			rle_dx = fgetc(f) + i;
			rle_dy = fgetc(f);
			return;
		    default:	/* next cnt bytes are absolute */
			for (cnt = ch; cnt>0 && i<=limit; cnt--) {
		    	    i++;
			    *s++ = fgetc(f);
		        }
			if (ch % 2)	/* word align file */
			    (void)fgetc(f);
		}
	    }
	    else { /* cnt is repeat count */
		for ( ; cnt>0 && i<=limit; cnt--) {
		    i++;
		    *s++ = ch;
		}
	    }
	}
}

void bmpgraph(f,filename,emwidth,emheight)
FILE *f;
char filename[];
float emwidth, emheight;	/* dimension in pixels */
{
	struct bitmapfileheader bmfh;
	struct bitmapinfoheader bmih;

	unsigned char isblack[256];
	unsigned char rr;
	unsigned char gg;
	unsigned char bb;
	unsigned char c = 0;

	char *line;
	char *pshexa;

	int clrtablesize;
	int i;
	int j;

	unsigned char omask;
	int oroll;

	unsigned char emask = 0;
	integer ewidth = 0;
	int isOS2;

        /* read the header of the file */
	fread(bmfh.type, 1, 2, f);
	bmfh.size = readinteger(f);
	bmfh.reserved1 = readhalfword(f);
	bmfh.reserved2 = readhalfword(f);
	bmfh.offbits = readinteger(f);
	if (feof(f)) {
		sprintf(errbuf, "em:graph: Unable to Read Valid BMP Header\n");
		specerror(errbuf);
		return;
	}

	bmih.size = readinteger(f);
	if (bmih.size == 12) { /* OS2 bitmap */
		isOS2 = TRUE;	
		bmih.width = readhalfword(f);
		bmih.height = readhalfword(f);
		bmih.planes = readhalfword(f);
		bmih.bitcount = readhalfword(f);
		/* the following don't exist in OS2 format so fill with 0's */
		bmih.compression = RGB;
		bmih.sizeimage = 0;
		bmih.xpelspermeter = 0;
		bmih.ypelspermeter = 0;
		bmih.clrused = 0;
		bmih.clrimportant = 0;
	}
	else { /* Windows bitmap */
		isOS2 = FALSE;	
		bmih.width = readinteger(f);
		bmih.height = readinteger(f);
		bmih.planes = readhalfword(f);
		bmih.bitcount = readhalfword(f);
		bmih.compression = readinteger(f);
		bmih.sizeimage = readinteger(f);
		bmih.xpelspermeter = readinteger(f);
		bmih.ypelspermeter = readinteger(f);
		bmih.clrused = readinteger(f);
		bmih.clrimportant = readinteger(f);
	}

	if (feof(f)) {
		sprintf(errbuf, "em:graph: Unable to Read Valid BMP Info");
		specerror(errbuf);
		return;
	}

	if (memcmp(bmfh.type, "BM", 2)) {
		sprintf(errbuf, "em:graph: Unknown BMP File Type");
		specerror(errbuf);
		return;
	}

	if ((bmih.compression == RLE4) && (bmih.bitcount != 4)) {
		sprintf(errbuf, "em:graph: Can't do BMP RLE4 with %d bits per pixel",
			bmih.bitcount);
		specerror(errbuf);
		return;
	}

	if ((bmih.compression == RLE8) && (bmih.bitcount != 8)) {
		sprintf(errbuf, "em:graph: Can't do BMP RLE8 with %d bits per pixel\n",
			bmih.bitcount);
		specerror(errbuf);
		return;
	}

	imagehead(filename,(int)bmih.width,(int)bmih.height,emwidth,emheight);

	/* determine the size of the color table to read */
	clrtablesize = 0;
	if (bmih.clrused == 0) {
		switch (bmih.bitcount) {
			case 1:
				clrtablesize = 2;
				break;
			case 4:
				clrtablesize = 16;
				break;
			case 8:
				clrtablesize = 256;
				break;
			case 24:
				break;
		}
	}
        else
		clrtablesize = bmih.clrused;

	/* read in the color table */
	for (i = 0; i < clrtablesize; i++) {
		bb = fgetc(f);
		gg = fgetc(f);
		rr = fgetc(f);
		isblack[i] = (rr < 0xff) || (gg < 0xff) || (bb < 0xff);
		if (!isOS2)
			(void) fgetc(f);
	}

	line = (char *) mymalloc((integer)((bmih.width * bmih.bitcount) / 8) + 1);
	pshexa = (char *) mymalloc((integer)tobyte(bmih.width));

	/* output the postscript image size preamble */
	cmdout("/picstr");
	numout((integer)tobyte(bmih.width));
	cmdout("string def");

	numout((integer)bmih.width);
	numout((integer)bmih.height);
	numout((integer)1);

	cmdout("[");
	numout((integer)bmih.width);
	numout((integer)0);
	numout((integer)0);
	numout((integer)bmih.height);
	numout((integer)0);
	numout((integer)bmih.height);
	cmdout("]");

	nlcmdout("{currentfile picstr readhexstring pop} image");

	if (bmih.bitcount == 1) {
		if (bmih.width%8)
			emask = (1<<(8-(bmih.width%8)))-1;	/* mask for edge of bitmap */
		else
			emask = 0;
		ewidth = tobyte(bmih.width);
	}

	/* read in all the lines of the file */
	for (i = 0; i < bmih.height; i++) {
		memset(pshexa,0xff,tobyte(bmih.width));
		switch (bmih.compression) {
		    case RGB:
			rgbread(f, (int) bmih.width, (int) bmih.bitcount, line);
			break;
		    case RLE4:
			rle4read(f, (int) bmih.width, (int) bmih.bitcount, line);
			break;
		    case RLE8:
			rle8read(f, (int) bmih.width, (int) bmih.bitcount, line);
			break;
		    default:
			sprintf(errbuf,"em:graph: Unknown BMP compression\n");
			specerror(errbuf);
			return;
		}

		omask = 0x80;
		oroll = 7;

		if (bmih.bitcount == 1) {
		    if (isblack[0])
			for (j = 0; j < ewidth ; j++)
				pshexa[j] = line[j];
		    else
			for (j = 0; j < ewidth ; j++)
				pshexa[j] = ~line[j];
		    pshexa[ewidth-1] |= emask;
		}
		else {
		    for (j = 0; j < bmih.width; j++) {
			switch (bmih.bitcount) {
				case 4:
					c = line[j>>1];
					if (!(j&1))
						c >>= 4;
					c = isblack[ c & 0x0f ];
					break;
				case 8:
					c = isblack[ (int)(line[j]) ];
					break;
				case 24:
					rr = line[j*3];
					gg = line[j*3+1];
					bb = line[j*3+2];
					c = (rr < 0xff) || (gg < 0xff) || (bb < 0xff);
					break;
			}
			if (c) 
			    pshexa[j/8] &= ~omask;
			else
			    pshexa[j/8] |= omask;
			oroll--;
			omask >>= 1;
			if (oroll < 0) {
			    omask = 0x80;
			    oroll = 7;
			}
		    }
		}
		newline();
		mhexout(pshexa,(long)tobyte(bmih.width));
	}
	imagetail() ;
	free(pshexa);
	free(line);
}
/* ------------------------------------------------------------------------ */

#define PCX 0
#define MSP 1
#define BMP 2
char *extarr[]=
{ ".pcx", ".msp", ".bmp", NULL };

void emgraph(filename,emwidth,emheight)
char filename[];
float emwidth, emheight;	/* dimension in pixels */
{
	char fname[80];
	int filetype;
	FILE *f;
	char *env;
	char id[4];
	int i;

	strcpy(fname, filename);

	/* find the file */
	f = search(figpath, fname, READBIN);
	if (f == (FILE *)NULL) {
   	    if ( (env = getenv("DVIDRVGRAPH")) != NULL )
		f = search(env,filename,READBIN);
	}
	/* if still haven't found it try adding extensions */
	if (f == (FILE *)NULL) {
	    i = 0;
	    while (extarr[i] != NULL) {
		strcpy(fname, filename);
		strcat(fname, extarr[i]);
		f = search(figpath, fname, READBIN);
		if (f == (FILE *)NULL) {
	    	    if ( (env = getenv("DVIDRVGRAPH")) != NULL )
			f = search(env,filename,READBIN);
		}
		if (f != (FILE *)NULL)
		    break;
		i++;
	    }
	}

	filetype = -1;
	if (f != (FILE *)NULL) {
	    for (i=0; i<4; i++) {
		id[i] = readquarterword(f);
	    }
	    if ( (id[0] == 0x0a) && (id[2] == 0x01) )
		filetype = PCX;
	    if (!memcmp(id, "DanM", 4))
		filetype = MSP;
	    if (!memcmp(id, "LinS", 4))
		filetype = MSP;
	    if (!memcmp(id, "BM", 2))
		filetype = BMP;
	    fseek(f, 0L, SEEK_SET);
	}

	switch (filetype) {
		case PCX:
			pcxgraph(f, fname, emwidth, emheight);
			break;
		case MSP:
			mspgraph(f, fname, emwidth, emheight);
			break;
		case BMP:
			bmpgraph(f, fname, emwidth, emheight);
			break;
		default:
			sprintf(fname,"em:graph: %s: File not found", filename);
			error(fname);
	}
	if (f != (FILE *)NULL)
	    fclose(f);
}

#else
void emspecial(p)
char *p ;
{
	sprintf(errbuf,"emTeX specials not compiled in this version");
	specerror(errbuf);
}
#endif /* EMTEX */
