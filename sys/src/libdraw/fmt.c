#include <u.h>
#include <libc.h>
#include <draw.h>

int
Rfmt(Fmt *f)
{
	Rectangle r;

	r = va_arg(f->args, Rectangle);
	return fmtprint(f, "%P %P", r.min, r.max);
}

int
Pfmt(Fmt *f)
{
	Point p;

	p = va_arg(f->args, Point);
	return fmtprint(f, "[%d %d]", p.x, p.y);
}

