#include <u.h>
#include <libc.h>
#include "fmtdef.h"

char*
fmtstrflush(Fmt *f)
{
	if(f->start == nil)
		return nil;
	*(char*)f->to = '\0';
	return (char*)f->start;
}
