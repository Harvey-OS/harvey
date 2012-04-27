#include <u.h>
#include <libc.h>
#include <tos.h>

/*
 * getcore() conflicts with tbl source.
 */
int
getcoreno(int *type)
{
	if (type != nil)
		*type = _tos->nixtype;
	return _tos->core;
}
