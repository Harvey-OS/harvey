#include <u.h>
#include <libc.h>
#include "fmtdef.h"

Rune*
runefmtstrflush(Fmt *f)
{
	if(f->start == nil)
		return nil;
	*(Rune*)f->to = '\0';
	return f->start;
}
