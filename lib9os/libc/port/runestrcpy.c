#include <u.h>
#include <libc.h>

Rune*
runestrcpy(Rune *s1, Rune *s2)
{
	Rune *os1;

	os1 = s1;
	while(*s1++ = *s2++)
		;
	return os1;
}
