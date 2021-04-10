/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


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

int	inflateblock(u8 *dst, int dsize, u8 *src, int ssize);
int	deflateblock(u8 *dst, int dsize, u8 *src, int ssize,
			int level, int debug);

int	deflatezlib(void *wr, int (*w)(void*, void*, int), void *rr, int (*r)(void*, void*, int), int level, int debug);
int	inflatezlib(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*));

int	inflatezlibblock(u8 *dst, int dsize, u8 *src,
			    int ssize);
int	deflatezlibblock(u8 *dst, int dsize, u8 *src,
			    int ssize, int level, int debug);

char	*flateerr(int err);

u32	*mkcrctab(u32);
u32	blockcrc(u32 *tab, u32 crc, void *buf, int n);

u32	adler32(u32 adler, void *buf, int n);
