/*
 *	histograph equalization
 *	D. P. Mitchell  87/12/03.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>

long h[256];
long H[256];

main(int argc, char *argv[]){
	register i, jw;
	register unsigned char *screen;
	PICFILE *in, *out;
	int npix, low, high;
	unsigned char map[256];

	if((argc=getflags(argc, argv, ""))!=1 && argc!=2) usage("[picture]");
	in = picopen_r(argc==2?argv[1]:"IN");
	if (in == 0) {
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	out = picopen_w("OUT", PIC_SAMEARGS(in));
	if (out == 0) {
		perror(argv[2]);
		exits("create output");
	}
	screen = (unsigned char *)malloc(in->width*in->height*in->nchan);
	if (screen == 0) {
		perror("malloc");
		exits("no space");
	}
	npix = 0;
	for (jw = 0; picread(in,(char *)(screen+jw)); jw += in->width*in->nchan)
		;
	for (i = 0; i < jw; i++) {
		h[screen[i]]++;
		npix++;
	}
	for (low = 0; low < 256; low++)
		if (h[low])
			break;
	for (high = 255; high >= 0; --high)
		if (h[high])
			break;
	H[0] = h[0];
	for (i = 1; i < 256; i++)
		H[i] = H[i - 1] + h[i];
	/*
	 *	O'Gorman's Optimum Intensity Range stuff
	 */
		npix = npix - (h[high] + h[low])/2;
		for (i = 0; i < 256; i++)
			H[i] = H[i] - (h[i] + h[low])/2;
	for (i = 0; i < 256; i++)
		map[i] = (255 * H[i]) / npix;
	for (i = 0; i < jw; i++)
		screen[i] = map[screen[i]];
	for (i = 0; i < jw; i += in->width*in->nchan)
		picwrite(out, (char *)(screen + i));
	exits("");
}
