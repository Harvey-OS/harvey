#include <u.h>
#include <libc.h>

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
char*
utfutf(char *s1, char *s2)
{
	char *p;
	long n1, n2;
	Rune r;

	n1 = chartorune(&r, s2);
	if(r < Runeself)
		return strstr(s1, s2);

	n2 = strlen(s2);
	for(p = s1; p = utfrune(p, r); p += n1)
		if(strncmp(p, s2, n2) == 0)
			return p;
	return 0;
}
