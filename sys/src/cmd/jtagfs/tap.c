#include <u.h>
#include <libc.h>
#include "debug.h"
#include "tap.h"


static stdebug = 1;

/*
	!8c -FVw tap.c
	!8l -o tap tap.8
	!tap > /tmp/l 
*/

static char *stnames[] = {
	[TapReset]		"TapReset",
	[TapIdle]			"TapIdle",
	[TapSelDR]		"TapSelDR",
	[TapCaptureDR]	"TapCaptureDR" ,
	[TapShiftDR]		"TapShiftDR",
	[TapExit1DR]		"TapExit1DR",
	[TapPauseDR]		"TapPauseDR",
	[TapExit2DR]		"TapExit2DR",
	[TapUpdateDR]		"TapUpdateDR",
	[TapSelIR]			"TapSelIR",
	[TapCaptureIR]		"TapCaptureIR",
	[TapShiftIR]		"TapShiftIR",
	[TapExit1IR]		"TapExit1IR",
	[TapPauseIR]		"TapPauseIR",
	[TapExit2IR]		"TapExit2IR",
	[TapUpdateIR]		"TapUpdateIR",
	[NStates]			"",
	[TapUnknown]		"TapUnknown",
};

typedef struct TapState TapState;
struct TapState {
	int next[2];
};


static TapState sm[]	= {
	[TapReset]		{TapIdle, TapReset},
	[TapIdle]			{TapIdle, TapSelDR},
	[TapSelDR]		{TapCaptureDR, TapSelIR},
	[TapCaptureDR]	{TapShiftDR, TapExit1DR},
	[TapShiftDR]		{TapShiftDR, TapExit1DR,},
	[TapExit1DR]		{TapPauseDR, TapUpdateDR},
	[TapPauseDR]		{TapPauseDR, TapExit2DR},
	[TapExit2DR]		{TapShiftDR, TapUpdateDR},
	[TapUpdateDR]		{TapIdle, TapSelDR},
	[TapSelIR]			{TapCaptureIR, TapReset},
	[TapCaptureIR]		{TapShiftIR, TapExit1IR},
	[TapShiftIR]		{TapShiftIR, TapExit1IR},
	[TapExit1IR]		{TapPauseIR, TapUpdateIR},
	[TapPauseIR]		{TapPauseIR, TapExit2IR},
	[TapExit2IR]		{TapShiftIR, TapUpdateIR},
	[TapUpdateIR]		{TapIdle, TapSelDR},
};

static int state=TapReset;

static int
isvalidst(int state)
{
	if(state < 0 || state >= NStates && state != TapUnknown){
		fprint(2, "invalid state: %d\n", state);
		return 0;
	}
	return 1;
}

static int
tapsmmove(int state, int tms)
{
	assert(tms == 0 || tms == 1);

	state = sm[state].next[tms];

       return state;
}

/*
 * To find the shortest path from here to a state
 * go in parallel through all neighbour states till I find it
 * with memoization of advancing passed states
 * (Breadth First Search)
 * (all paths weight the same, simple topology)
 * could probably use this to generate a lookup table
 * but all time is spent waiting for usb anyway
 */

static void
movepath(SmPath *path, int tms, int newst)
{
	path->st = newst;
	path->ptmslen++;
	path->ptms <<= 1;
	path->ptms |= tms;
}


static SmPath
findpath(int origin, int dest)
{
	int np, memost, tms, nc, i, j, ip, st;
	SmPath paths[NStates],  nextp, newp;

	memset(paths, 0, sizeof(SmPath));
	paths[0].st = origin;
	np = 1;
	memost = 0;
	if(origin == dest)
		return *paths;
	
	for(i = 0; i < NStates; i++){
		for(j = 0; j < np; j++){
			nc = 0;
			nextp = paths[j];
			for(tms = 0; tms < 2; tms++){
				newp = nextp;
				st = tapsmmove(newp.st, tms);
				if((1 << st) & memost){
					if(++nc == 2)		/*trapped state kill*/
						paths[j] = paths[--np];
					continue;
				}
				movepath(&newp, tms, st);
				memost |= 1 << newp.st;

				if(newp.st == dest)
					return newp;
				if(tms == 0)
					ip = j;
				else
					ip = np++;
				paths[ip] = newp;
	
			}
		}
	}
	fprint(2, "should not come through here\n");
	newp.st = -1;
	return newp;
}


static void
concatpath(SmPath *p1, SmPath *p2)
{
	ulong msk;
	assert(p1->ptmslen < 8*sizeof(p1->ptms));
		
	msk = (1 << p2->ptmslen)-1;
	p1->ptms = (p1->ptms << p2->ptmslen)|(p2->ptms & msk);
	p1->ptmslen += p2->ptmslen;
	p1->st = p2->st;
}

static void
dumppath(SmPath *pth)
{
	uchar frstbit;
	int i;

	fprint(2, "\n(");
	for(i = 0; i < pth->ptmslen; i++){
		frstbit = (pth->ptms >> (pth->ptmslen-i-1))&1;
		fprint(2, "tms[%d] = %ud ", i, frstbit);
	}
	fprint(2, ")\n");
}

SmPath
pathto(TapSm *sm, int dest)
{
	SmPath pth1, pth2;

	if(!isvalidst(sm->state) || !isvalidst(dest))
		sysfatal("invalid state");
	dprint(DState, "pathto: %s -> %s\n",
		stnames[sm->state], stnames[dest]);
	if(sm->state == TapUnknown){
		pth1.ptmslen = 5;	/* resynch */
		pth1.ptms = 0x1f;
		pth2 = findpath(TapReset, dest);

		concatpath(&pth1, &pth2);
		if(debug[DPath])
			dumppath(&pth1);
		return pth1;
	}
	pth1 = findpath(sm->state, dest);
	if(debug[DPath])
		dumppath(&pth1);
	return pth1;
}

void
moveto(TapSm *sm, int dest)
{
	dprint(DState, "moveto: %s -> %s\n",
		stnames[sm->state], stnames[dest]);
	sm->state = dest;
}

static ulong
pathpref(SmPath *pth, int nbits)
{	
	ulong msk;
	assert(pth->ptmslen >= nbits);

	msk = (1 << nbits)-1;
	return msk & (pth->ptms >> (pth->ptmslen - nbits));
}

SmPath
takepathpref(SmPath *pth, int nbits)
{
	SmPath pref;

	pref.ptmslen = 0;
	if(pth->ptmslen == 0)
		return pref;
	if(nbits > pth->ptmslen)
		nbits = pth->ptmslen;
	pref.ptmslen = nbits;
	pref.ptms = pathpref(pth, nbits);
	pth->ptmslen -= nbits;
	return pref;
}

/*
 * Testing

void
main(int, char *[])
{
	SmPath pth;
	TapSm sm;
	int orig, dest;

	orig = TapReset;
	dest = TapExit2DR;

	print("origin %s dest %s\n", stnames[orig], stnames[dest]);
	pth = findpath(orig, dest);
	print("\n");
	dumppath(&pth);
	orig = TapShiftIR;
	dest = TapExit2DR;

	print("origin %s dest %s\n", stnames[orig], stnames[dest]);
	pth = findpath(orig, dest);
	print("\n");
	dumppath(&pth);
	orig = TapIdle;
	dest = TapExit2IR;

	print("origin %s dest %s\n", stnames[orig], stnames[dest]);
	pth = findpath(orig, dest);
	print("\n");
	dumppath(&pth);
	sm.state = TapUnknown;
	pth = pathto(&sm, TapExit2DR);
	dumppath(&pth);
	
}
*/
