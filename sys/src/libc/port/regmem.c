/*
 * verify memset on large allocations
 */

#include <u.h>
#include <libc.h>
#include <pool.h>

#define malign(p) (void *)(((uintptr)(p) + CLSIZE-1) & ~(uintptr)(CLSIZE-1))

enum {
	NLOOPS = 10,
	CLSIZE = 64,
};

static char buf1g[1ull<<30];

void
main(int argc, char **argv)
{
	int i;
	char pat;
	char pats[5];
	char *p;
	uintptr len;

	if (argc < 2 || argc > 3)
		sysfatal("usage: regmem bytes [pat]");

	len = atoll(argv[1]);
	if (len > CLSIZE)
		len -= CLSIZE;		/* leave room for CL alignment */

	if (argc == 3)
		pat = argv[2][0];
	else
		pat = 0;
	for (i = 0; i < 4; i++)
		pats[i] = pat;
	pats[4] = '\0';

	if (len < sizeof buf1g)
		p = buf1g;
	else {
//		mainmem->flags |= POOL_ANTAGONISM | POOL_PARANOIA |
//			POOL_VERBOSITY | POOL_DEBUGGING | POOL_LOGGING;
		print("alloc %lld...", (vlong)len);
		p = mallocz(len, 0);
	}

	p = malign(p);
	print("memset(%#p, %#o, %lld)...", p, pat, (vlong)len);
	for (i = NLOOPS; i-- > 0; )
		memset(p, pat, len);
	print("\n");

	if (memcmp(p, pats, 4) != 0)
		sysfatal("memset broken start");
	if (len >= 1024*1024  && memcmp(p+1024*1024-4, pats, 4) != 0)
		sysfatal("memset broken middle");
	if (memcmp(p+len-4, pats, 4) != 0)
		sysfatal("memset broken end");
	exits(0);
}
