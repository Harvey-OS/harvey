/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

DSApriv*getdsakey(int argc, char **argv, int needprivate, Attr **pa);
RSApriv*getkey(int argc, char **argv, int needprivate, Attr **pa);
uchar*	put4(uchar *p, uint n);
uchar*	putmp2(uchar *p, mpint *b);
uchar*	putn(uchar *p, void *v, uint n);
uchar*	putstr(uchar *p, char *s);
