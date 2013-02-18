#include <u.h>
#include <libc.h>

Rune*
runestrcat(Rune *s1, Rune *s2)
{

	runestrcpy(runestrchr(s1, 0), s2);
	return s1;
}
