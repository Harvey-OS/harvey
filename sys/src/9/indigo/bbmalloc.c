#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#define UNCACHEBIT	((~UNMAPPED)&UNCACHED)
#define LINEWORDS	(64/sizeof(ulong))

/*
 * Allocate memory for use in kernel bitblts.
 * The allocated memory must have a flushed instruction
 * cache, and the data cache must be flushed by bbdflush().
 * To avoid the need for frequent cache flushes, the memory
 * is allocated out of an arena, and the i-cache is only
 * flushed when it has to be reused
 *
 * This code will have to be interlocked if we ever get
 * a multiprocessor with a bitmapped display.
 */

/* a 0->3 bitblt can take 900 words */
enum {
	narena=2,	/* put interrupt time stuff in separate arena */
	nbbarena=4096	/* number of words in an arena */
};

static ulong	bbarena[narena][nbbarena+LINEWORDS];
static ulong	*bbcur[narena] = {&bbarena[0][0], &bbarena[1][0]};
static ulong	*bblast[narena] = {0, 0};

#define INTLEVEL(v)	(((v)&IEC) == 0)
void *
bbmalloc(int nbytes)
{
	int nw, a;
	int s;
	ulong *ans;

	nw = nbytes/sizeof(long);
	s = splhi();
	a = INTLEVEL(s)? 1 : 0;
	if(bbcur[a] + nw > &bbarena[a][nbbarena])
		ans = bbarena[a];
	else
		ans = bbcur[a];
	bbcur[a] = ans + nw + (LINEWORDS);
	splx(s);
	if(ans == bbarena[a])
		icflush(ans, nbbarena*sizeof(ulong));
	bblast[a] = ans;
	return ans;
}

void
bbfree(void *p, int n)
{
	int a, s;

	s = splhi();
	a = INTLEVEL(s)? 1 : 0;
	if(p == bblast[a])
		bbcur[a] = (ulong *)(((char *)bblast[a]) + n + LINEWORDS*sizeof(long));
	splx(s);
}

int
bbonstack(void)
{
	/*if(u)
		return 1;*/
	return 0;
}
