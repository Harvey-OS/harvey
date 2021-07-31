#include <u.h>
#include <libc.h>

/* Routines for letting gbitblt run at user level */

enum {
	nbbarena = 20000,
	linewords = (64/sizeof(ulong)),
};

static ulong bbarena[nbbarena];
static ulong *bbcur = bbarena;
static ulong *bblast = bbarena;

void *
bbmalloc(int nbytes)
{
	int nw;
	ulong *ans;

	nw = nbytes/sizeof(long);
	if(bbcur + nw > &bbarena[nbbarena])
		ans = bblast = bbarena;
	else
		ans = bbcur;
	bbcur = ans + nw;
	if(ans == bbarena)
		segflush(ans, nbbarena*sizeof(ulong));
	bblast = ans;
	return ans;
}

void
bbfree(void *a, int n)
{
	USED(a);
	bbcur = (ulong *)(((char *)bblast) + n + linewords*sizeof(long));
}

int
bbonstack(void)
{
#ifdef Tmips
	return 0;
#else
	return 1;
#endif
}

#ifdef T68020
void
flushvirtpage(void *p)
{
	segflush(p, 200);	/* 200 > bytes in biggest non-converting bitblt */
}

void
bbdflush(void *p, int n)
{
	segflush(p, n);
}
#endif

#ifdef Tmips
void
wbflush(void)
{
	/* mips has write-through data cache */
	/* assume this call is enough to trickle data cache out */
}

void
icflush(void *p, int n)
{
	segflush(p, n);
}
#endif
