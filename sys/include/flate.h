/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"libflate.a"
#pragma	src	"/sys/src/libflate"

/*
 * errors from deflate, deflateinit, deflateblock,
 * inflate, inflateinit, inflateblock.
 * convertable to a string by flateerr
 */
enum
{
	FlateOk			= 0,
	FlateNoMem		= -1,
	FlateInputFail		= -2,
	FlateOutputFail		= -3,
	FlateCorrupted		= -4,
	FlateInternal		= -5,
};

int	deflateinit(void);
int	deflate(void *wr, int (*w)(void*, void*, int), void *rr, int (*r)(void*, void*, int), int level, int debug);

int	inflateinit(void);
int	inflate(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*));

int	inflateblock(uchar *dst, int dsize, uchar *src, int ssize);
int	deflateblock(uchar *dst, int dsize, uchar *src, int ssize, int level, int debug);

int	deflatezlib(void *wr, int (*w)(void*, void*, int), void *rr, int (*r)(void*, void*, int), int level, int debug);
int	inflatezlib(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*));

int	inflatezlibblock(uchar *dst, int dsize, uchar *src, int ssize);
int	deflatezlibblock(uchar *dst, int dsize, uchar *src, int ssize, int level, int debug);

char	*flateerr(int err);

ulong	*mkcrctab(ulong);
ulong	blockcrc(ulong *tab, ulong crc, void *buf, int n);

ulong	adler32(ulong adler, void *buf, int n);
