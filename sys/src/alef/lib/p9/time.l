#include <alef.h>

intern int f = -1;

int
time(int *tp)
{
	int t;
	char b[20];

	memset(b, 0, sizeof(b));
	if(f < 0)
		f = open("/dev/time", OREAD|OCEXEC);
	if(f >= 0) {
		seek(f, 0, 0);
		read(f, b, sizeof(b));
	}
	t = atoi(b);
	if(tp)
		*tp = t;
	return t;
}
