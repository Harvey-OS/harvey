#include <u.h>
#include <libc.h>

static	long	sqtab[64] =
{
	0x6cdb2, 0x726d4, 0x77ea3, 0x7d52f, 0x82a85, 0x87eb1, 0x8d1c0, 0x923bd,
	0x974b2, 0x9c4a8, 0xa13a9, 0xa61be, 0xaaeee, 0xafb41, 0xb46bf, 0xb916e,
	0xbdb55, 0xc247a, 0xc6ce3, 0xcb495, 0xcfb95, 0xd41ea, 0xd8796, 0xdcca0,
	0xe110c, 0xe54dd, 0xe9818, 0xedac0, 0xf1cd9, 0xf5e67, 0xf9f6e, 0xfdfef,
	0x01fe0, 0x05ee6, 0x09cfd, 0x0da30, 0x11687, 0x1520c, 0x18cc8, 0x1c6c1,
	0x20000, 0x2388a, 0x27068, 0x2a79e, 0x2de32, 0x3142b, 0x3498c, 0x37e5b,
	0x3b29d, 0x3e655, 0x41989, 0x44c3b, 0x47e70, 0x4b02b, 0x4e16f, 0x51241,
	0x542a2, 0x57296, 0x5a220, 0x5d142, 0x60000, 0x62e5a, 0x65c55, 0x689f2,
};

double
sqrt(double arg)
{
	int e, ms;
	double a, t;
	union
	{
		double	d;
		struct
		{
			long	ms;
			long	ls;
		};
	} u;

	u.d = arg;
	ms = u.ms;

	/*
	 * sign extend the mantissa with
	 * exponent. result should be > 0 for
	 * normal case.
	 */
	e = ms >> 20;
	if(e <= 0) {
		if(e == 0)
			return 0;
		return NaN();
	}

	/*
	 * pick up arg/4 by adjusting exponent
	 */
	u.ms = ms - (2 << 20);
	a = u.d;

	/*
	 * use 5 bits of mantissa and 1 bit
	 * of exponent to form table index.
	 * insert exponent/2 - 1.
	 */
	e = (((e - 1023) >> 1) + 1022) << 20;
	u.ms = *(long*)((char*)sqtab + ((ms >> 13) & 0xfc)) | e;
	u.ls = 0;

	/*
	 * three laps of newton
	 */
	e = 1 << 20;
	t = u.d;
	u.d = t + a/t;
	u.ms -= e;		/* u.d /= 2; */
	t = u.d;
	u.d = t + a/t;
	u.ms -= e;		/* u.d /= 2; */
	t = u.d;

	return t + a/t;
}

/*
 * this is the program that generated the table.
 * it calls sqrt by some other means.
 *
 * void
 * main(void)
 * {
 * 	int i;
 * 	union	U
 * 	{
 * 		double	d;
 * 		struct
 * 		{
 * 			long	ms;
 * 			long	ls;
 * 		};
 * 	} u;
 *
 * 	for(i=0; i<64; i++) {
 * 		u.ms = (i<<15) | 0x3fe04000;
 * 		u.ls = 0;
 * 		u.d = sqrt(u.d);
 * 		print(" 0x%.5lux,", u.ms & 0xfffff);
 * 	}
 * 	print("\n");
 * 	exits(0);
 * }
 */
