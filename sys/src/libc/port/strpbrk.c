#include <u.h>
#include <libc.h>
#define	N	256

char*
strpbrk(char *cs, char *cb)
{
	char map[N];
	uchar *s=(uchar*)cs, *b=(uchar*)cb;

	memset(map, 0, N);
	for(;;) {
		map[*b] = 1;
		if(*b++ == 0)
			break;
	}
	while(map[*s++] == 0)
		;
	if(*--s)
		return (char*)s;
	return 0;
}
