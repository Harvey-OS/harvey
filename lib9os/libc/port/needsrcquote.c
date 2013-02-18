#include <u.h>
#include <libc.h>

int
needsrcquote(int c)
{
	if(c <= ' ')
		return 1;
	if(utfrune("`^#*[]=|\\?${}()'<>&;", c))
		return 1;
	return 0;
}
