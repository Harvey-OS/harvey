#include <u.h>
#include <libc.h>
#include <oventi.h>

int
vtErrFmt(Fmt *f)
{
	char *s;

	s = vtGetError();
	return fmtstrcpy(f, s);
}
