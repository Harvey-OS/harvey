#include <u.h>
#include <libc.h>

static
char*
skip(char *p)
{

	while(*p == ' ')
		p++;
	while(*p != ' ' && *p != 0)
		p++;
	return p;
}

long
times(long *t)
{
	char b[200], *p;
	static int f = -1;
	ulong r;

	memset(b, 0, sizeof(b));
	if(f < 0)
		f = open("/dev/cputime", OREAD|OCEXEC);
	if(f >= 0) {
		seek(f, 0, 0);
		read(f, b, sizeof(b));
	}
	p = b;
	if(t)
		t[0] = atol(p);
	p = skip(p);
	if(t)
		t[1] = atol(p);
	p = skip(p);
	r = atol(p);
	if(t){
		p = skip(p);
		t[2] = atol(p);
		p = skip(p);
		t[3] = atol(p);
	}
	return r;
}
