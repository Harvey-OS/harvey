#include <u.h>
#include <libc.h>

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
Rune*
runestrstr(Rune *s1, Rune *s2)
{
	Rune *p, *pa, *pb;
	int c0, c;

	c0 = *s2;
	if(c0 == 0)
		return s1;
	s2++;
	for(p=runestrchr(s1, c0); p; p=runestrchr(p+1, c0)) {
		pa = p;
		for(pb=s2;; pb++) {
			c = *pb;
			if(c == 0)
				return p;
			if(c != *++pa)
				break;
		}
	}
	return 0;
}
