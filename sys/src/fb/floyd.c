/*
 *	Floyd-Steinberg halftoning with Ulichney's improvements
 *	D. P. Mtichell  86/07/12.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
main(int argc, char *argv[]){
	long *error0, *error1;
	long *temp;
	unsigned char *inbuffer, *outbuffer;
	register long quantization_error;
	PICFILE *in, *out;
	int i, x, dx, inpixel, outpixel, random;
	short da, db;

	if((argc=getflags(argc, argv, ""))!=1 && argc!=2) usage("[picture]");
	srand(time(0) + getpid());
	in = picopen_r(argc==2?argv[1]:"IN");
	if (in == 0) {
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	if(PIC_NCHAN(in)!=1){
		fprint(2, "%s: input must be 1 channel\n", argv[0]);
		exits(">1 chan");
	}
	out = picopen_w("OUT", "bitmap",
		PIC_XOFFS(in), PIC_YOFFS(in), PIC_WIDTH(in), PIC_HEIGHT(in), "m", argv, 0);
	if (out == 0) {
		perror(argv[0]);
		exits("create output");
	}
	inbuffer = (unsigned char *)malloc(PIC_WIDTH(in));
	outbuffer = (unsigned char *)malloc(PIC_WIDTH(in));
	error0 = (long *)malloc(sizeof(long)*(PIC_WIDTH(in) + 2)) + 1;
	error1 = (long *)malloc(sizeof(long)*(PIC_WIDTH(in) + 2)) + 1;
	for (i = 0; i < PIC_WIDTH(in); i++)
		error0[i] = error1[i] = 0;
	dx = 1;
	x = 0;
	while (picread(in, (char *)inbuffer)) {
		for (i = 0; i < PIC_WIDTH(in); i++) {
			inpixel = inbuffer[x]+error0[x]/128;
			outpixel = (inpixel >= 128) ? 255 : 0;
			quantization_error = inpixel - outpixel;
			random = rand();
			da = (random & 31) - 15;
			db = ((random >> 6) & 7) - 3;
			error0[x + dx] += (8*7+da)*quantization_error;
			error1[x + dx]  = (8*1+db)*quantization_error;
			error1[x +  0] += (8*5-da)*quantization_error;
			error1[x - dx] += (8*3-db)*quantization_error;
			outbuffer[x] = outpixel;
			x += dx;
		}
		error1[x - dx] += error1[x];
		dx = -dx;
		x += dx;
		temp = error0;
		error0 = error1;
		error1 = temp;
		error1[x] = 0;
		picwrite(out, (char *)outbuffer);
	}
	exits("");
}
