#include <alef.h>

intern int f = -1;

intern
byte*
skip(byte *p)
{
	while(*p == ' ')
		p++;
	while(*p != ' ' && *p != 0)
		p++;
	return p;
}

int
times(int *t)
{
	byte b[200], *p;
	uint r;

	memset(b, 0, sizeof(b));
	!{
		if(f < 0)
			f = open("/dev/cputime", OREAD|OCEXEC);
		if(f >= 0) !{
			seek(f, 0, 0);
			read(f, b, sizeof(b));
		}
	}
	p = b;
	if(t)
		t[0] = atoi(p);
	p = skip(p);
	if(t)
		t[1] = atoi(p);
	p = skip(p);
	r = atoi(p);
	if(t){
		p = skip(p);
		t[2] = atoi(p);
		p = skip(p);
		t[3] = atoi(p);
	}
	return r;
}
