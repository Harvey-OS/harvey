/*
 *	halftone - perform electronic screening on 8-bit B/W picture
 *	D. P. Mitchell  85/10/09.
 *
 *	Halftone reads an 8-bit deep B/W picture file and write one
 *	out that is bilevel (contains only 0 and 255).  A
 *	variety of halftoning screens are available in
 *
 *		/usr/don/lib/screens
 *
 *	both dithering, and classic screen patterns.
 *
 *	usage:	halftone SCREENNAME [infile]
 */

#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <libg.h>
#include <fb.h>
#define DITHDIR "/lib/fb/screens/"
#define DSIZE 64

int thresh[DSIZE][DSIZE];
int wide, high;

void loaddither(char *name){
	FILE *fid;
	int i, j;
	int maximum;
	char buffer[128];
	int th;

	strcpy(buffer, DITHDIR);
	strcat(buffer, name);
	fid = fopen(buffer, "r");
	maximum = 0;
	fscanf(fid, "%d%d", &wide, &high);
	if(wide>DSIZE || high>DSIZE){
		fprint(2, "Dither matrix too big\n");
		exits("bad dither matrix");
	}
	for (j = 0; j < high; j++)
		for (i = 0; i < wide; i++) {
			fscanf(fid, "%d", &thresh[i][j]);
			if (thresh[i][j] > maximum)
				maximum = thresh[i][j];
		}
	if (high == 1 && wide == 1)
		goto noscale;
	for (j = 0; j < high; j++)
		for (i = 0; i < wide; i++) {
			th = 4096 * (thresh[i][j] + 1) / (maximum + 2);
			if (th > 4095) th = 4095;
			thresh[i][j] = th;
		}
noscale:
	fprint(2, "%d %d\n", wide, high);
/*
	for (j = 0; j < high; j++) {
		for (i = 0; i < wide; i++)
			fprint(2, "%3d ", thresh[i][j]);
		fprint(2, "\n");
	}
*/
	fclose(fid);
}

main(int argc, char *argv[]){
	int i, j;
	unsigned char *cp;
	int imodwide, jmodhigh;
	unsigned char *raster;
	PICFILE *in, *out;

	if((argc=getflags(argc, argv, ""))!=2 && argc!=3) usage("screentype [infile]");
	loaddither(argv[1]);
	in = picopen_r(argc==3?argv[2]:"IN");
	if (in == 0) {
		perror(argc==3?argv[2]:"IN");
		exits("open input");
	}
	if(PIC_NCHAN(in)!=1){
		fprint(2, "%s: input must be 1 channel\n", argv[0]);
		exits(">1 chan");
	}
	raster = (unsigned char *)malloc(in->width);
	out = picopen_w("OUT", "bitmap",
		PIC_XOFFS(in), PIC_YOFFS(in), PIC_WIDTH(in), PIC_HEIGHT(in), "m", argv, 0);
	if (out == 0) {
		perror(argv[0]);
		exits("create output");
	}
	for (j = 0; picread(in, (char *)raster); j++) {
		jmodhigh = j % high;
		cp = raster;
		for (imodwide = 0, i = 0; i < in->width; i++) {
			if ((*cp&255)*4095/255 >= thresh[imodwide][jmodhigh])
				*cp++ = 255;
			else
				*cp++ = 0;
			imodwide++;
			if (imodwide >= wide)
				imodwide = 0;
		}
		picwrite(out, (char *)raster);
	}
	exits("");
}
