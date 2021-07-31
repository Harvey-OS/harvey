#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include "comments.h"
#include "gen.h"
#include "path.h"

int debug = 0;

/* predefine
int cat(char *);
void error(int, char *, ...);
 */

#define HDLEN	60
char header[HDLEN];
char *screenbits;
char hex[] = { 'f', 'e', 'd', 'c', 'b', 'a', '9', '8',
		'7', '6', '5', '4', '3', '2', '1', '0' };	/* black on gnot is not
								 * black on printer.
								 */
#define LETTERWIDTH	8.5
#define	LETTERHEIGHT	11.0
int landscape = 0;

#define XOFF	(int)((landscape)?((LETTERWIDTH/2 + (float)height * yscale / (2 * 100.)) * 72.):((LETTERWIDTH/2 - (float)width * pixperbyte * xscale / (2 * 100.)) * 72.))
#define YOFF	(int)((landscape)?((LETTERHEIGHT/2 - (float)width * pixperbyte * xscale / (2 * 100.)) * 72.):((LETTERHEIGHT/2 - (float)height * yscale / (2 * 100.)) * 72.))

char *prologue = POSTDMD;

void
preamble(void) {
	fprintf(stdout, "%s", CONFORMING);
	fprintf(stdout, "%s %s\n", VERSION, PROGRAMVERSION);
	fprintf(stdout, "%s %s\n", DOCUMENTFONTS, ATEND);
	fprintf(stdout, "%s %s\n", PAGES, ATEND);
	fprintf(stdout, "%s", ENDCOMMENTS);

/*	if (cat(prologue) == FALSE) {
		fprintf(2, "can't read %s", prologue);
		exits("copying prologue");
	}
 */

	fprintf(stdout, "%s", ENDPROLOG);
	fprintf(stdout, "%s", BEGINSETUP);
	fprintf(stdout, "mark\n");
}

void
main(int argc, char *argv[])
{
	FILE *screenfd = stdin;
	int i, j, bcnt;
	int width, height, ldepth, bitsperpix, pixperbyte;
	float xscale = 1.0, yscale = 1.0;
	Rectangle r;
	int ;
	char *bp, *optstr, *patch = 0;

	for (i=1; i<argc; i++) {
		if (*argv[i] == '-') {
			switch(argv[i][1]) {
			case 'L':
				landscape = 1;
				break;
			case 'P':
				if (argv[i][2] == '\0')
					patch = argv[++i];
				else
					patch = &(argv[i][2]);
				break;
			case 'd':
				debug = 1;
				break;
			case 'm':
				if (argv[i][2] == '\0')
					optstr = argv[++i];
				else
					optstr = &(argv[i][2]);
				if ((optstr=strtok(optstr, " ,")) != 0)
					xscale = yscale = atof(optstr);
				 if ((optstr=strtok(0, " ,")) != 0)
					yscale = atof(optstr);
				break;
			default:
				fprintf(stderr, "usage: %s [-m mag] [file]\n");
				exits("incorrect usage");
			}
		} else if (argv[i][0] != '\0')
			screenfd = fopen(argv[i], "r");
	}
	if (screenfd == NULL) {
		fprintf(stderr, "cannot open /dev/screen.\n");
		fprintf(stderr, "try: bind -a /mnt/term/dev /dev\n");
		exits("open failed");
	}
	if (fread(header, 1, HDLEN, screenfd)!=HDLEN) {
		fprintf(stderr, "cannot read bitmap header.\n");
		exits("read failed");
	}
	if (sscanf(header, " %d %d %d %d %d ", &ldepth, &r.min.x, &r.min.y, &r.max.x, &r.max.y)!=5) {
		fprintf(stderr, "bad header format.\n");
		exits("bad format");
	}
	bitsperpix  = 1<<ldepth;
	pixperbyte = 8 / bitsperpix;
	width = ((r.max.x - r.min.x + (r.min.x % pixperbyte)) * (1<<ldepth) + 7) / 8;
	height = r.max.y - r.min.y;
	bcnt = height * width;
	if (debug) fprintf(stderr, "width=%d height=%d bcnt=%d\n", width, height, bcnt);
	screenbits = malloc(bcnt);
	if (screenbits == 0) {
		fprintf(stderr, "cannot allocate bitmap.\n");
		exits("malloc failed");
	}
	if ((i=fread(screenbits, 1, bcnt, screenfd)) != bcnt) {
		fprintf(stderr, "read failed: read %d bytes out of %d.\n", i, bcnt);
		exits("read failed");
	}
	preamble();
	if (patch) fprintf(stdout, "%s\n", patch);
	fprintf(stdout, "/picstr %d string def\n", width);
	fprintf(stdout, "%d %d translate\n", XOFF, YOFF);
	if (landscape) fprintf(stdout, "90 rotate\n");
	fprintf(stdout, "%6.2f %6.2f scale\n\n", (float)width * pixperbyte * .72 * xscale,
						(float)height * .72 * yscale);
	fprintf(stdout, "%d %d %d [%d 0 0 %d 0 %d]\n", width*pixperbyte, height,
		1<<ldepth, width*pixperbyte, -height, height);
	fprintf(stdout, "{currentfile picstr readhexstring pop} image\n\n");
	bp = screenbits;
	for (i=0; i<height; i++) {
		for(j=0;j<width;j++) {
			/* (8 - (2        )) - ((2        ) * (128       % 4         )) */
			/* (8 - bitsperpix - (bitsperpix * (r.min.x+j % pixperbyte)) */
			fprintf(stdout, "%c%c", hex[(*bp&0xf0)>>4], hex[(*bp++&0xf)]);
			if (j%32 == 31) fprintf(stdout, "\n");
		}
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "showpage\n");
	fprintf(stdout, "%%%%BoundingBox: %d %d %6.2f %6.2f\n", XOFF, YOFF,
			(float)width * pixperbyte * .72 * xscale + XOFF,
			(float)height * .72 * yscale + YOFF);
	exits("");
}
