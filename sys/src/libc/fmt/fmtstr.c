#include <u.h>
#include <libc.h>

char*
fmtstrflush(Fmt *f)
{
	*(char*)f->to = '\0';
	return f->start;
}
