#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

/*
 * Allocate memory for use in kernel bitblts.
 */

/* a 0->3 bitblt can take 900 words */
enum {
	nbbarena=8192	/* number of words in an arena */
};

static ulong	bbarena[nbbarena];
static ulong	*bbcur = bbarena;
static ulong	*bblast = 0;

void *
bbmalloc(int nbytes)
{
	int nw;
	int s;
	ulong *ans;

	nw = nbytes/sizeof(long);
	s = splhi();
	if(bbcur + nw > &bbarena[nbbarena])
		ans = bbarena;
	else
		ans = bbcur;
	bbcur = ans + nw;
	splx(s);
	bblast = ans;
	ans = (void *)ans;
	return ans;
}

void
bbfree(void *p, int n)
{
	if(p == bblast)
		bbcur = (ulong *)(((char *)bblast) + n);
}

int
bbonstack(void)
{
	if(u)
		return 1;
	return 0;
}
