/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>

Point
stringbg(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, s, nil, 1<<24, dst->clipr, bg, bgp, SoverD);
}

Point
stringbgop(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, Image *bg, Point bgp, int op)
{
	return _string(dst, pt, src, sp, f, s, nil, 1<<24, dst->clipr, bg, bgp, op);
}

Point
stringnbg(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, int len, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, s, nil, len, dst->clipr, bg, bgp, SoverD);
}

Point
stringnbgop(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, int len, Image *bg, Point bgp, int op)
{
	return _string(dst, pt, src, sp, f, s, nil, len, dst->clipr, bg, bgp, op);
}

Point
runestringbg(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, nil, r, 1<<24, dst->clipr, bg, bgp, SoverD);
}

Point
runestringbgop(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, Image *bg, Point bgp, int op)
{
	return _string(dst, pt, src, sp, f, nil, r, 1<<24, dst->clipr, bg, bgp, op);
}

Point
runestringnbg(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, int len, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, nil, r, len, dst->clipr, bg, bgp, SoverD);
}

Point
runestringnbgop(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, int len, Image *bg, Point bgp, int op)
{
	return _string(dst, pt, src, sp, f, nil, r, len, dst->clipr, bg, bgp, op);
}
