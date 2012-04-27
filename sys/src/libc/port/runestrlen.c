#include <u.h>
#include <libc.h>

long
runestrlen(Rune *s)
{

	return runestrchr(s, 0) - s;
}
