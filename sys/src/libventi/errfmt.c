#include <u.h>
#include <libc.h>
#include <venti.h>

int
vtErrFmt(Fmt *f)
{
	char *s;

	s = vtGetError();
	return fmtstrcpy(f, s);
}
