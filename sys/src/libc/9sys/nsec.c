#include <u.h>
#include <libc.h>
#include <tos.h>

static uvlong order = 0x0001020304050607ULL;

static void
be2vlong(vlong *to, uchar *f)
{
	uchar *t, *o;
	int i;

	t = (uchar*)to;
	o = (uchar*)&order;
	for(i = 0; i < sizeof order; i++)
		t[o[i]] = f[i];
}

vlong
nsec(void)
{
	uchar b[8];
	vlong t;
	int f;

	f = open("/dev/bintime", OREAD);
	if(f < 0)
		return 0;
	t = 0;
	if(pread(f, b, sizeof b, 0) == sizeof b)
		be2vlong(&t, b);
	close(f);
	return t;
}
