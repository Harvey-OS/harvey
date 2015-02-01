/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef Bitmap *readbitsfn(char *, int, long *, int, uchar *, int **);
typedef void mapfn(int, int, long *);

extern Bitmap *kreadbits(char *file, int nc, long *chars, int size, uchar *bits, int **);
extern void kmap(int from, int to, long *chars);
extern Bitmap *breadbits(char *file, int nc, long *chars, int size, uchar *bits, int **);
extern void bmap(int from, int to, long *chars);
extern Bitmap *greadbits(char *file, int nc, long *chars, int size, uchar *bits, int **);
extern void gmap(int from, int to, long *chars);
extern Bitmap *qreadbits(char *file, int nc, long *chars, int size, uchar *bits, int **);
extern Subfont *bf(int n, int size, Bitmap *b, int *);
