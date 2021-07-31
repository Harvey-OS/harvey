#include <u.h>
#include <libc.h>

char*
fmtstrflush(Fmt *f)
{
	if(f->start == nil)
		return nil;
	*(char*)f->to = '\0';
	return f->start;
}
