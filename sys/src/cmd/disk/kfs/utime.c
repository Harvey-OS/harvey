#include "all.h"

long
time(void)
{
	char b[20];
	static int f = -1;

	memset(b, 0, sizeof(b));
	if(f < 0)
		f = open("#c/time", OREAD);
	if(f >= 0) {
		seek(f, 0, 0);
		read(f, b, sizeof(b));
	}
	return atol(b);
}
