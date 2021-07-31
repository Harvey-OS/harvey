#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "sys9.h"

typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned char uchar;

static uvlong order = 0x0001020304050607ULL;

static void
be2vlong(vlong *to, uchar *f)
{
	uchar *t, *o;
	int i;

	t = (uchar*)to;
	o = (uchar*)&order;
	for(i = 0; i < 8; i++)
		t[o[i]] = f[i];
}

int
gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	int f;
	uchar b[8];
	vlong t;

	memset(b, 0, sizeof b);
	f = _OPEN("/dev/bintime", 0);
	if(f >= 0) {
		_PREAD(f, b, sizeof(b), 0);
		_CLOSE(f);
	}
	be2vlong(&t, b);

	tp->tv_sec = t/1000000000;
	tp->tv_usec = (t/1000)%1000000;

	if(tzp) {
		tzp->tz_minuteswest = 240;
		tzp->tz_dsttime = 1;
	}

	return 0;
}
