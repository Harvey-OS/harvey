#include <string.h>

#define	N	256

char*
strtok(char *s, const char *b)
{
	static char *under_rock;
	char map[N], *os;

	memset(map, 0, N);
	while(*b)
		map[*(unsigned char*)b++] = 1;
	if(s == 0)
		s = under_rock;
	while(map[*(unsigned char*)s++])
		;
	if(*--s == 0)
		return 0;
	os = s;
	while(map[*(unsigned char*)s] == 0)
		if(*s++ == 0) {
			under_rock = s-1;
			return os;
		}
	*s++ = 0;
	under_rock = s;
	return os;
}
