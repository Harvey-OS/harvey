#include <u.h>
#include <libc.h>

#define	N	256

char*
strtok(char *s, char *b)
{
	static char *under_rock;
	char map[N], *os;

	memset(map, 0, N);
	while(*b)
		map[*(uchar*)b++] = 1;
	if(s == 0)
		s = under_rock;
	while(map[*(uchar*)s++])
		;
	if(*--s == 0)
		return 0;
	os = s;
	while(map[*(uchar*)s] == 0)
		if(*s++ == 0) {
			under_rock = s-1;
			return os;
		}
	*s++ = 0;
	under_rock = s;
	return os;
}
