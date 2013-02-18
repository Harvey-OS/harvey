#include <u.h>
#include <libc.h>
void
abort(void)
{
	while(*(int*)0)
		;
}
