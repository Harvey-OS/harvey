#include <u.h>
#include <libc.h>

Rune*
runefmtstrflush(Fmt *f)
{
	*(Rune*)f->to = '\0';
	return f->start;
}
