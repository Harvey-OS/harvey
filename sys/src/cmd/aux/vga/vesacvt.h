typedef struct Cvt Cvt;
struct Cvt {
	/* these are passed in to the cvt calculators */
	ulong	wid;
	ulong	ht;
	ulong	hz;
	ulong	vfporch;
	ulong	vsync;
	ulong	activehpix;
	ulong	reducedblank;	/* flag: crt timings if false */

	/* these are returned */
	ulong	vbporch;
	ulong	hsync;
	ulong	hblank;
	ulong	totvlines;
	ulong	totpix;
	ulong	pixfreq;
};

Cvt	*vesacvt(ulong, ulong, ulong, int);
