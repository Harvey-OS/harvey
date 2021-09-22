#define D	1	/* double precision */
#define DYN	7	/* rounding mode: from fcsr */

#define FSQRT(src, dst) \
	WORD $(013<<27 | D<<25 | (src)<<15 | DYN<<12 | (dst)<<7 | 0123)

TEXT	sqrt(SB), $0
	MOVD	arg+0(FP), F0
	FSQRT(0, 0)
	RET
