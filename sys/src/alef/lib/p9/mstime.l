#include <alef.h>

intern int f = -1;

uint
mstime()
{
	uint t;
	byte b[20];

	memset(b, 0, sizeof(b));
	!{
		if(f < 0)
			f = open("/dev/msec", OREAD|OCEXEC);
		if(f >= 0) {
			seek(f, 0, 0);
			read(f, b, sizeof(b));
		}
	}
	t = strtoui(b, nil, 0);
	return t;
}
