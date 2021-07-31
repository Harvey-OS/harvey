#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

/*
 * Allocate memory for use in kernel bitblts.
 *
 * This code will have to be interlocked if we ever get
 * a multiprocessor with a bitmapped display.
 */

/* a 0->3 bitblt can take 800 longs */
enum {
	narena=2,	/* put interrupt time stuff in separate arena */
	nbbarena=4096	/* number of words in an arena */
};

static ulong	*bbarena[narena];
static ulong	*bbcur[narena];
static ulong	*bblast[narena];

#define INTENABLED(v)	((v)&(1<<9))
void *
bbmalloc(int nbytes)
{
	int nw, a;
	int s;
	ulong *ans;

	nw = nbytes/sizeof(long);
	s = splhi();
	a = INTENABLED(s) ? 0 : 1;
	if(bbcur[a] + nw > bbarena[a] + nbbarena)
		ans = bbarena[a];
	else
		ans = bbcur[a];
	bbcur[a] = ans + nw;
	bblast[a] = ans;
	splx(s);
	return ans;
}

void
bbinit(void)
{
	int i;

	if(bbarena[0])
		return;
	for(i = 0; i < narena; i++){
		bbarena[i] = xalloc(nbbarena * sizeof(long));
		bbcur[i] = bbarena[i];
		bblast[i] = 0;
	}
}


void
bbfree(void *p, int n)
{
	int a, s;

	s = splhi();
	a = INTENABLED(s) ? 0 : 1;
	if(p == bblast[a])
		bbcur[a] = (ulong *)(((char *)bblast[a]) + n);
	splx(s);
}

void
bbdflush(void *p, int n)
{
	USED(p, n);
}

int
bbonstack(void)
{
	return 0;
}

void
bbexec(void (*memstart)(void), int len, int onstack)
{
	USED(len, onstack);
	memstart();
}
