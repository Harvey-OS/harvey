#include <u.h>
#include <libc.h>
#include <draw.h>

Point
stringbg(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, s, nil, 1<<24, dst->clipr, bg, bgp);
}

Point
stringnbg(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, int len, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, s, nil, len, dst->clipr, bg, bgp);
}

Point
runestringbg(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, nil, r, 1<<24, dst->clipr, bg, bgp);
}

Point
runestringnbg(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, int len, Image *bg, Point bgp)
{
	return _string(dst, pt, src, sp, f, nil, r, len, dst->clipr, bg, bgp);
}

