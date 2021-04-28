#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>

void
hname(char *buf)
{
	u8 d[SHA1dlen];
	u32 x;
	int n;

	n = strlen(buf);
	sha1((u8*)buf, n, d, nil);
	x = d[0] | d[1]<<8 | d[2]<<16;
	snprint(buf, n+1, "%.5ux", x & 0xfffff);
}
