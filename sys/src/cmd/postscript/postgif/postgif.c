
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#ifdef plan9
#define	isascii(c)	((unsigned char)(c)<=0177)
#endif
#include <sys/types.h>
#include <fcntl.h>

#include "comments.h"
#include "gen.h"
#include "path.h"
#include "ext.h"

#define dbprt	if (debug) fprintf

char	*optnames = "a:c:fglm:n:o:p:x:y:C:E:DG:IL:P:";
char    *prologue = POSTGIF;		/* default PostScript prologue */
char    *formfile = FORMFILE;           /* stuff for multiple pages per sheet */
int     formsperpage = 1;               /* page images on each piece of paper */
int	copies = 1;                     /* and this many copies of each sheet */
int     page = 0;                       /* last page we worked on */
int     printed = 0;                    /* and the number of pages printed */

extern char *malloc();
extern void free();
extern double atof(), pow();

unsigned char ibuf[BUFSIZ];
unsigned char *cmap, *gcmap, *lcmap;
unsigned char *gmap, *ggmap, *lgmap;
unsigned char *pmap;
double gamma;
float cr = 0.3, cg = 0.59, cb = 0.11;
int maplength, gmaplength, lmaplength;
int scrwidth, scrheight;
int gcolormap, lcolormap;
int bitperpixel, background;
int imageleft, imagetop;
int imagewidth, imageheight;
int interlaced, lbitperpixel;
int gray = 0;
int gammaflag = 0;
int negative = 0;
int terminate = 0;
int codesize, clearcode, endcode, curstblsize, pmindex, byteinibuf, bitsleft;
int prefix[4096], suffix[4096], cstbl[4096];
int bburx = -32767, bbury = -32767;
FILE *fp_in = NULL;
FILE *fp_out = stdout;

char *
allocate(size)
    int size;
{
    char *p;

    if ((p = malloc(size)) == NULL) error(FATAL, "not enough memory");
    return(p);
}

void
puthex(c, fp)
    unsigned char c;
    FILE *fp;
{
    static char hextbl[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    };

    putc(hextbl[(c >> 4) & 017], fp);
    putc(hextbl[c & 017], fp);
}

void
setcolormap(bp)
    int bp;
{
    int i, entries = 1, scale = 1;
    unsigned char *p, *q;

    for (i = 0; i < bp; i++) entries *= 2;
    for (i = 0; i < 8 - bp; i++) scale *= 2;
    gcmap = (unsigned char *) allocate(entries*3);
    ggmap = (unsigned char *) allocate(entries);
    gmaplength = entries;
    for (i = 0, p = gcmap, q = ggmap; i < 256; i += scale, p += 3, q++) {
	if (negative) {
	    *p = 255 - i; p[1] = *p; p[2] = *p;
	    *q = *p;
	}
	else {
	    *p = i; p[1] = i; p[2] = i;
	    *q = i;
	}
    }
    if (gammaflag)
    	for (i = 0, p = gcmap; i < 256; i += scale, p += 3) {
	    *p = (unsigned char) (pow((double) *p/256.0, gamma)*256);
	    p[1] = *p; p[2] = *p;
	}
dbprt(stderr,"default color map:\n");
for (i = 0; i < entries*3; i += 3)
dbprt(stderr, "%d, %d, %d\n", gcmap[i], gcmap[i+1], gcmap[i+2]);
}

void
readgcolormap(bp)
    int bp;
{
    int i, entries = 1;
    unsigned char *p, *q;

    for (i = 0; i < bp; i++) entries *= 2;
    gcmap = (unsigned char *) allocate(entries*3);
    ggmap = (unsigned char *) allocate(entries);
    gmaplength = entries;
    fread(gcmap, sizeof(*gcmap), entries*3, fp_in);
    if (negative)
	for (i = 0, p = gcmap; i < entries*3; i++, p++) *p = 255 - *p;
    for (i = 0, p = gcmap, q = ggmap; i < entries; i++, p += 3, q++)
	*q = cr*(int)p[0] + cg*(int)p[1] + cb*(int)p[2] + 0.5;
    if (gammaflag)
    	for (i = 0, p = gcmap; i < entries*3; i++, p++)
	    *p = (unsigned char) (pow((double) *p/256.0, gamma)*256);
dbprt(stderr,"global color map:\n");
for (i = 0; i < entries*3; i += 3)
dbprt(stderr, "%d, %d, %d\n", gcmap[i], gcmap[i+1], gcmap[i+2]);
}

void
readlcolormap(bp)
    int bp;
{
    int i, entries = 1;
    unsigned char *p, *q;

    for (i = 0; i < bp; i++) entries *= 2;
    lcmap = (unsigned char *) allocate(entries*3);
    lgmap = (unsigned char *) allocate(entries);
    lmaplength = entries;
    fread(lcmap, sizeof(*lcmap), entries*3, fp_in);
    if (negative)
	for (i = 0, p = lcmap; i < entries*3; i++, p++) *p = 255 - *p;
    for (i = 0, p = lcmap, q = lgmap; i < entries; i++, p += 3, q++)
	*q = cr*(int)p[0] + cg*(int)p[1] + cb*(int)p[2] + 0.5;
    if (gammaflag)
    	for (i = 0, p = lcmap; i < entries*3; i++, p++)
	    *p = (unsigned char) (pow((double) *p/256.0, gamma)*256);
dbprt(stderr,"local color map:\n");
for (i = 0; i < entries*3; i += 3)
dbprt(stderr, "%d, %d, %d\n", lcmap[i], lcmap[i+1], lcmap[i+2]);
}

void
initstbl()
{
    int i, entries = 1, *p, *s;

    for (i = 0; i < codesize; i++) entries *= 2;
    clearcode = entries;
    endcode = clearcode + 1;
    for (i = 0, p = prefix, s = suffix; i <= endcode; i++, p++, s++) {
	*p = endcode;
	*s = i;
    }
    curstblsize = endcode + 1;
    pmindex = 0;
    byteinibuf = 0;
    bitsleft = 0;
}

int
nextbyte()
{
    static ibufindex;

    if (byteinibuf) {
	byteinibuf--;
	ibufindex++;
    }
    else {
    	fread(ibuf, sizeof(*ibuf), 1, fp_in);
    	byteinibuf = ibuf[0];
dbprt(stderr, "byte count: %d\n", byteinibuf);
	if (byteinibuf) fread(ibuf, sizeof(*ibuf), byteinibuf, fp_in);
	else error(FATAL, "encounter zero byte count block before end code");
	ibufindex = 0;
	byteinibuf--;
	ibufindex++;
    }
    return(ibuf[ibufindex-1]);
}

int masktbl[25] = {
    0, 01, 03, 07, 017, 037, 077, 0177, 0377, 0777, 01777, 03777, 07777,
    017777, 037777, 077777, 0177777, 0377777, 0777777, 01777777, 03777777,
    07777777, 017777777, 037777777, 077777777
};

int
getcode()
{
    int cs, c;
    static int oldc;

    if (curstblsize < 4096) cs = cstbl[curstblsize];
    else cs = 12;
    while (bitsleft < cs) {
	oldc = (oldc & masktbl[bitsleft]) | ((nextbyte() & 0377) << bitsleft);
	bitsleft += 8;
    }
    c = oldc & masktbl[cs];
    oldc = oldc >> cs;
    bitsleft -= cs;
/* dbprt(stderr, "code: %d %d %d\n", curstblsize, cs, c); */
    return(c);
}

void
putcode(c)
    int c;
{
    if (prefix[c] != endcode) {
	putcode(prefix[c]);
	pmap[pmindex] = suffix[c];
	pmindex++;
    }
    else {
   	pmap[pmindex] = suffix[c];
	pmindex++;
    }
}

int
firstof(c)
    int c;
{
    while (prefix[c] != endcode) c = prefix[c];
    return(suffix[c]);
}

void
writeimage()
{
    int i, j, k;

dbprt(stderr, "pmindex: %d\n", pmindex);
    fputs("save\n", fp_out);
    fprintf(fp_out, "/codestr %d string def\n", imagewidth);
    if (!gray) {
    	fprintf(fp_out, "/colortbl currentfile %d string readhexstring\n",
	    maplength*3);
        for (i = 0; i < maplength; i++) puthex(cmap[i], fp_out);
        fputs("\n", fp_out);
        for (i = maplength ; i < maplength*2; i++) puthex(cmap[i], fp_out);
        fputs("\n", fp_out);
        for (i = maplength*2 ; i < maplength*3; i++) puthex(cmap[i], fp_out);
        fputs("\npop def\n", fp_out);
    	fprintf(fp_out, "/graytbl currentfile %d string readhexstring\n",
	    maplength);
        for (i = 0; i < maplength; i++) puthex(gmap[i], fp_out);
        fputs("\npop def\n", fp_out);
    }
    fprintf(fp_out, "%s %d %d %d %d gifimage\n",
	gray ? "true" : "false", imagewidth, imageheight,
	scrwidth - imageleft - imagewidth, scrheight - imagetop - imageheight);
    if (gray) {
	if (interlaced) {
	    int *iltbl;

	    iltbl = (int *) allocate(imageheight*sizeof(int));
	    j = 0;
	    for (i = 0; i < imageheight; i += 8) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass1: %d\n", j);
	    for (i = 4; i < imageheight; i += 8) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass2: %d\n", j);
	    for (i = 2; i < imageheight; i += 4) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass3: %d\n", j);
	    for (i = 1; i < imageheight; i += 2) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass4: %d\n", j);

    	    for (i = 0; i < imageheight; i++) {
		k = iltbl[i];
	        for (j = 0; j < imagewidth; j++, k++)
		    puthex(gmap[pmap[k]], fp_out);
	        fputs("\n", fp_out);
	    }
	}
	else {
    	    for (i = 0, k = 0; i < imageheight; i++) {
	        for (j = 0; j < imagewidth; j++, k++)
		    puthex(gmap[pmap[k]], fp_out);
	        fputs("\n", fp_out);
	    }
    	}
    }
    else {
	if (interlaced) {
	    int *iltbl;

	    iltbl = (int *) allocate(imageheight*sizeof(int));
	    j = 0;
	    for (i = 0; i < imageheight; i += 8) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass1: %d\n", j);
	    for (i = 4; i < imageheight; i += 8) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass2: %d\n", j);
	    for (i = 2; i < imageheight; i += 4) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass3: %d\n", j);
	    for (i = 1; i < imageheight; i += 2) {
		iltbl[i] = j;
		j += imagewidth;
	    }
dbprt(stderr, "pass4: %d\n", j);

    	    for (i = 0; i < imageheight; i++) {
		k = iltbl[i];
	        for (j = 0; j < imagewidth; j++, k++) puthex(pmap[k], fp_out);
	        fputs("\n", fp_out);
    	    }
	}
	else {
    	    for (i = 0, k = 0; i < imageheight; i++) {
	        for (j = 0; j < imagewidth; j++, k++) puthex(pmap[k], fp_out);
	        fputs("\n", fp_out);
    	    }
	}
    }
    fputs("restore\n", fp_out);
}

void
readimage()
{
    int bytecount, zerobytecount = 0;
    int code, oldcode;

    fread(ibuf, sizeof(*ibuf), 9, fp_in);
    imageleft = ibuf[0] + 256*ibuf[1];
    imagetop = ibuf[2] + 256*ibuf[3];
    imagewidth = ibuf[4] + 256*ibuf[5];
    imageheight = ibuf[6] + 256*ibuf[7];
    lcolormap = ibuf[8] & 0200;
    interlaced = ibuf[8] & 0100;
    lbitperpixel = (ibuf[8] & 07) + 1;
dbprt(stderr, "imageleft: %d\n", imageleft);
dbprt(stderr, "imagetop: %d\n", imagetop);
dbprt(stderr, "imagewidth: %d\n", imagewidth);
dbprt(stderr, "imgaeheight: %d\n", imageheight);
dbprt(stderr, "lcolormap: %d\n", lcolormap ? 1 : 0);
dbprt(stderr, "interlaced: %d\n", interlaced ? 1 : 0);
dbprt(stderr, "lbitperpixel: %d\n", lbitperpixel);
    if (lcolormap) {
	readlcolormap(lbitperpixel);
	cmap = lcmap;
	gmap = lgmap;
	maplength = lmaplength;
    }

dbprt(stderr, "start reading raster data\n");
    fread(ibuf, sizeof(*ibuf), 1, fp_in);
    codesize = ibuf[0];
dbprt(stderr, "codesize: %d\n", codesize);
    pmap = (unsigned char *) allocate(imagewidth*imageheight);
    initstbl();
    while ((code = getcode()) != endcode) {
	if (code == clearcode) {
	    curstblsize = endcode + 1;
    	    code = getcode();
    	    putcode(code);
    	    oldcode = code;
	}
	else if (code < curstblsize) {
	    putcode(code);
	    prefix[curstblsize] = oldcode;
	    suffix[curstblsize] = firstof(code);
	    curstblsize++;
	    oldcode = code;
	}
	else {
	   if (code != curstblsize) error(FATAL, "code out of order");
	   prefix[curstblsize] = oldcode;
	   suffix[curstblsize] = firstof(oldcode);
	   curstblsize++;
	   putcode(curstblsize-1);
	   oldcode = code;
	}
    }
dbprt(stderr, "finish reading raster data\n");

    /* read the rest of the raster data */
    do {
    	fread(ibuf, sizeof(*ibuf), 1, fp_in);
    	bytecount = ibuf[0];
dbprt(stderr, "byte count: %d\n", bytecount);
	if (bytecount) fread(ibuf, sizeof(*ibuf), bytecount, fp_in);
	else zerobytecount = 1;
    } while (!zerobytecount);

    writeimage();

    if (lcolormap) {
	cmap = gcmap;
	gmap = ggmap;
	maplength = gmaplength;
	free(lcmap);
	free(lgmap);
    }
}

void
readextensionblock()
{
    int functioncode, bytecount, zerobytecount = 0;

    fread(ibuf, sizeof(*ibuf), 1, fp_in);
    functioncode = ibuf[0];
dbprt(stderr, "function code: %d\n", functioncode);
    do {
    	fread(ibuf, sizeof(*ibuf), 1, fp_in);
    	bytecount = ibuf[0];
dbprt(stderr, "byte count: %d\n", bytecount);
	if (bytecount) fread(ibuf, sizeof(*ibuf), bytecount, fp_in);
	else zerobytecount = 1;
    } while (!zerobytecount);
}

void
writebgscr()
{
    fprintf(fp_out, "%s %d %d\n", PAGE, page, printed+1);
    fputs("/saveobj save def\n", fp_out);
    fprintf(fp_out, "%s: %d %d %d %d\n",
	"%%PageBoundingBox", 0, 0, scrwidth, scrheight);
    if (scrwidth > bburx) bburx = scrwidth;
    if (scrheight > bbury) bbury = scrheight;
    fprintf(fp_out, "%d %d gifscreen\n", scrwidth, scrheight);
}

void
writeendscr()
{
    if ( fp_out == stdout ) printed++;
    fputs("showpage\n", fp_out);
    fputs("saveobj restore\n", fp_out);
    fprintf(fp_out, "%s %d %d\n", ENDPAGE, page, printed);
}

void
redirect(pg)
    int		pg;			/* next page we're printing */
{
    static FILE	*fp_null = NULL;	/* if output is turned off */

    if ( pg >= 0 && in_olist(pg) == ON )
	fp_out = stdout;
    else if ( (fp_out = fp_null) == NULL )
	fp_out = fp_null = fopen("/dev/null", "w");

}

void
readgif()
{
    int i, j, k;

    for (i = 0, j = 1, k = 0; i < 13; i++) {
	for (; k < j; k++) cstbl[k] = i;
	j *= 2;
    }

    fread(ibuf, sizeof(*ibuf), 6, fp_in);
dbprt(stderr, "%.6s\n", ibuf);
    if (strncmp((char *)ibuf, "GIF87a", 6) != 0) {
    	fread(ibuf, sizeof(*ibuf), 122, fp_in);
    	fread(ibuf, sizeof(*ibuf), 6, fp_in);
dbprt(stderr, "%.6s\n", ibuf);
    	if (strncmp((char *)ibuf, "GIF87a", 6) != 0)
		 error(FATAL, "wrong GIF signature");
    }
    fread(ibuf, sizeof(*ibuf), 7, fp_in);
    scrwidth = ibuf[0] + 256*ibuf[1];
    scrheight = ibuf[2] + 256*ibuf[3];
    gcolormap = ibuf[4] & 0200;
    bitperpixel = (ibuf[4] & 07) + 1;
    background = ibuf[5];
dbprt(stderr, "scrwidth: %d\n", scrwidth);
dbprt(stderr, "scrheight: %d\n", scrheight);
dbprt(stderr, "gcolormap: %d\n", gcolormap ? 1 : 0);
dbprt(stderr, "bitperpixel: %d\n", bitperpixel);
dbprt(stderr, "background: %d\n", background);
    if (ibuf[6] != 0) error(FATAL, "wrong screen descriptor");
    if (gcolormap) readgcolormap(bitperpixel);
    else setcolormap(bitperpixel);

    redirect(++page);
    writebgscr();

    cmap = gcmap;
    gmap = ggmap;
    maplength = gmaplength;

    do {
	fread(ibuf, sizeof(*ibuf), 1, fp_in);
	if (ibuf[0] == ',') readimage();
	else if (ibuf[0] == ';') terminate = 1;
	else if (ibuf[0] == '!') readextensionblock();
	else
	error(FATAL, "wrong image separator character or wrong GIF terminator");
    } while (!terminate);

    writeendscr();

    free(gcmap);
    free(ggmap);
}

void
init_signals()
{

    if ( signal(SIGINT, interrupt) == SIG_IGN )  {
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGHUP, SIG_IGN);
    }
    else {
        signal(SIGHUP, interrupt);
        signal(SIGQUIT, interrupt);
    }

    signal(SIGTERM, interrupt);
}

void
header()
{
    int         ch;                     /* return value from getopt() */
    int         old_optind = optind;    /* for restoring optind - should be 1 */

    while ( (ch = getopt(argc, argv, optnames)) != EOF )
        if ( ch == 'L' )
            prologue = optarg;
        else if ( ch == '?' )
            error(FATAL, "");

    optind = old_optind;                /* get ready for option scanning */

    fprintf(stdout, "%s", CONFORMING);
    fprintf(stdout, "%s %s\n", VERSION, PROGRAMVERSION);
    fprintf(stdout, "%s %s\n", BOUNDINGBOX, ATEND);
    fprintf(stdout, "%s %s\n", PAGES, ATEND);
    fprintf(stdout, "%s", ENDCOMMENTS);

    if ( cat(prologue) == FALSE )
        error(FATAL, "can't read %s", prologue);

    fprintf(stdout, "%s", ENDPROLOG);
    fprintf(stdout, "%s", BEGINSETUP);
    fprintf(stdout, "mark\n");

}

void
options()
{
    int		ch;			/* return value from getopt() */

    while ( (ch = getopt(argc, argv, optnames)) != EOF )  {
	switch ( ch )  {

	    case 'a':			/* aspect ratio */
		    fprintf(stdout, "/aspectratio %s def\n", optarg);
		    break;

	    case 'c':			/* copies */
		    copies = atoi(optarg);
		    fprintf(stdout, "/#copies %s store\n", optarg);
		    break;

	    case 'f':
		    negative = TRUE;
		    break;

	    case 'g':
		    gray = TRUE;
		    break;

	    case 'l':
		    fprintf(stdout, "/alignment true def\n");
		    break;

	    case 'm':			/* magnification */
		    fprintf(stdout, "/magnification %s def\n", optarg);
		    break;

	    case 'n':			/* forms per page */
		    formsperpage = atoi(optarg);
		    fprintf(stdout, "%s %s\n", FORMSPERPAGE, optarg);
		    fprintf(stdout, "/formsperpage %s def\n", optarg);
		    break;

	    case 'o':			/* output page list */
		    out_list(optarg);
		    break;

	    case 'p':			/* landscape or portrait mode */
		    if ( *optarg == 'l' )
			fprintf(stdout, "/landscape true def\n");
		    else fprintf(stdout, "/landscape false def\n");
		    break;

	    case 'x':			/* shift things horizontally */
		    fprintf(stdout, "/xoffset %s def\n", optarg);
		    break;

	    case 'y':			/* and vertically on the page */
		    fprintf(stdout, "/yoffset %s def\n", optarg);
		    break;

	    case 'C':			/* copy file straight to output */
		    if ( cat(optarg) == FALSE )
			error(FATAL, "can't read %s", optarg);
		    break;

	    case 'E':			/* text font encoding - unnecessary */
		    fontencoding = optarg;
		    break;

	    case 'D':			/* debug flag */
		    debug = ON;
		    break;

	    case 'G':
		    gammaflag = ON;
		    gamma = atof(optarg);
		    break;

	    case 'I':			/* ignore FATAL errors */
		    ignore = ON;
		    break;

	    case 'L':			/* PostScript prologue file */
		    prologue = optarg;
		    break;

	    case 'P':			/* PostScript pass through */
		    fprintf(stdout, "%s\n", optarg);
		    break;

	    case '?':			/* don't understand the option */
		    error(FATAL, "");
		    break;

	    default:			/* don't know what to do for ch */
		    error(FATAL, "missing case for option %c\n", ch);
		    break;

	}
    }

    argc -= optind;			/* get ready for non-option args */
    argv += optind;
}

void
setup()
{
    /*setencoding(fontencoding);*/
    fprintf(stdout, "setup\n");

    if ( formsperpage > 1 )  {          /* followed by stuff for multiple pages
*/
        if ( cat(formfile) == FALSE )
            error(FATAL, "can't read %s", formfile);
        fprintf(stdout, "%d setupforms\n", formsperpage);
    }   /* End if */

    fprintf(stdout, "%s", ENDSETUP);

}

void
arguments()
{
    if ( argc < 1 ) {
	fp_in = stdin;
	readgif();
    }
    else  {				/* at least one argument is left */
	while ( argc > 0 )  {
	    if ( strcmp(*argv, "-") == 0 )
		fp_in = stdin;
	    else if ( (fp_in = fopen(*argv, "r")) == NULL )
		error(FATAL, "can't open %s", *argv);
	    readgif();
	    if ( fp_in != stdin )
		fclose(fp_in);
	    argc--;
	    argv++;
	}
    }
}

void
done()
{
    fprintf(stdout, "%s", TRAILER);
    fprintf(stdout, "done\n");
    fprintf(stdout, "%s 0 0 %d %d\n", BOUNDINGBOX, bburx, bbury);
    fprintf(stdout, "%s %d\n", PAGES, printed); 
}

main(agc, agv)
    int agc;
    char *agv[];
{
    argc = agc;
    argv = agv;
    prog_name = argv[0];

    init_signals();
    header();
    options();
    setup();
    arguments();
    done();

    exit(0);
}

