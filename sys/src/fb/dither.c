/*
 *	dither - fast color quantization by dithering
 *	D. P. Mitchell  85/09/23.
 *
 *	Dither quickly reduces a full-color picture to 8 bits per
 *	pixel (as opposed to quantize, which does a slow careful job).
 *	The dither pattern could be improved.
 *
 *	usage:	dither [fullcolorfile]
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>

#define SIZE 34
#define SHIFT 13	/* 3 over 5 tilt prevents flicker */
#define COLORWIDTH 640
#define NRED 7		/* Psychometric division of color space */
#define NGRN 9
#define NBLU 4

static dither[SIZE] = {
			12,
		29, 13, 30,  1,
	    18,  5, 22, 15, 32,
	 8, 25, 16, 33,  3, 20,  4,
	    21, 14, 31,  6, 23,  2, 19,
	     9, 26, 11, 28, 10, 27,
		 7, 24,  0,
		    17,
};
static unsigned char lookup[SIZE][3][256];
unsigned char map[256][3];

void precompute_lookup(void)
{
	short i, j;

	for (i = 0; i < SIZE; i++)
		dither[i] = (dither[i] * 256) / SIZE;
	for (i = 0; i < NRED*NGRN*NBLU; i++) {
		map[i][2] = ((i % NBLU) * 255) / (NBLU - 1);
		map[i][1] = (((i / NBLU) % NGRN) * 255) / (NGRN - 1);
		map[i][0] = (((i / NBLU / NGRN) % NRED) * 255) / (NRED - 1);
	}
	for (; i < 256; i++)
		map[i][2] = map[i][1] = map[i][0] = 255;
	for (i = 0; i < 256; i++)
		for (j = 0; j < SIZE; j++) {
			lookup[j][0][i] = NBLU * NGRN *
				((i * (NRED - 1) + dither[j]) >> 8);
			/*
			 *	Anti correlate green to smooth luminance
			 */
			lookup[j][1][i] = NBLU *
				((i * (NGRN - 1) + (248 - dither[j])) >> 8);
			lookup[j][2][i] =
				((i * (NBLU - 1) + dither[j]) >> 8);
		}
}

main(int argc, char *argv[]){
	unsigned char *input, *output;
	unsigned char *src, *tab;
	short byte, i, j;
	PICFILE *in, *out;

	precompute_lookup();
	if((argc=getflags(argc, argv, ""))!=2 && argc!=1) usage("[picture]");
	in = picopen_r(argc==2?argv[1]:"IN");
	if (in == 0) {
		picerror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	if (in->nchan < 3) {
		perror("input must be full color");
		exits("not color");
	}
	out = picopen_w("OUT", in->type, PIC_XOFFS(in), PIC_YOFFS(in),
		PIC_WIDTH(in), PIC_HEIGHT(in), "m", argv, (char *)map);
	if (out == 0) {
		perror(argv[0]);
		exits("create output");
	}
	input = (unsigned char *)malloc(in->nchan * in->width);
	output = (unsigned char *)malloc(out->width);
	for (j = 0; picread(in, (char *)input) > 0; j++) {
		src = input;
		tab = lookup[(j * SHIFT) % SIZE][0];
		for (i = 0; i < in->width; i++) {
			if (tab == lookup[SIZE][0])
				tab = lookup[0][0];
			byte  = tab[*src++]; tab += 256;
			byte += tab[*src++]; tab += 256;
			byte += tab[*src++]; tab += 256;
			src += in->nchan - 3;
			output[i] = byte;
		}
		picwrite(out, (char *)output);
	}
	exits("");
}
