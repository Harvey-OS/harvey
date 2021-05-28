#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * There's no pager process here.
 * One process waiting for memory becomes the pager,
 * during the call to kickpager()
 */

static QLock	pagerlck;
static struct
{
	ulong ntext;
	ulong nbig;
	ulong nall;
} pstats;

static void
freepages(uint lg2size, int once)
{
	Pallocpg *pg;
	Page *p;
	int i;

	for(i = lg2size; i < nelem(palloc.avail); i++){
		pg = &palloc.avail[i];
		if(pg->freecount > 0){
			DBG("kickpager() up %#p: releasing %udK pages\n",
				up, (1<<lg2size)/KiB);
			lock(&palloc);
			if(pg->freecount == 0){
				unlock(&palloc);
				continue;
			}
			p = pg->head;
			pageunchain(p);
			unlock(&palloc);
			if(p->ref != 0)
				panic("freepages pa %#ullx", p->pa);
			pgfree(p);
			if(once)
				break;
		}
	}
}

static int
tryalloc(int lg2size, int color)
{
	Page *p;

	p = pgalloc(lg2size, color);
	if(p != nil){
		lock(&palloc);
		pagechainhead(p);
		unlock(&palloc);
		return 0;
	}
	return -1;
}

static int
hascolor(Page *pl, int color)
{
	Page *p;

	lock(&palloc);
	for(p = pl; p != nil; p = p->next)
		if(color == NOCOLOR || p->color == color){
			unlock(&palloc);
			return 1;
		}
	unlock(&palloc);
	return 0;
}

/*
 * Someone couldn't find pages of the given size index and color.
 * (color may be NOCOLOR if the caller is trying to get any page
 * and is desperate).
 * Many processes may be calling this at the same time,
 * The first one operates as a pager and does what it can.
 */
void
kickpager(uint lg2size, int color)
{
	Pallocpg *pg;

	print("kickpager page size %dK color %d\n", (1<<lg2size)/KiB, color);

	if(DBGFLG>1)
		DBG("kickpager() %#p\n", up);
	if(waserror())
		panic("error in kickpager");
	qlock(&pagerlck);
	pg = &palloc.avail[lg2size];

	/*
	 * 1. did anyone else release one for us in the mean time?
	 */
	if(hascolor(pg->head, color))
		goto Done;

	/*
	 * 2. try allocating from physical memory
	 */
	tryalloc(lg2size, color);
	if(hascolor(pg->head, color))
		goto Done;

	/*
	 * Try releasing memory from bigger pages.
	 */
	pstats.nbig++;
	freepages(1<<(lg2size+1), 1);
	tryalloc(lg2size, color);
	if(hascolor(pg->head, color)){
		DBG("kickpager() found %uld free\n", pg->freecount);
		goto Done;
	}

	/*
	 * Really the last resort. Try releasing memory from all pages.
	 */
	pstats.nall++;
	DBG("kickpager() up %#p: releasing all pages\n", up);
	freepages(0, 0);
	tryalloc(lg2size, color);
	if(pg->freecount > 0){
		DBG("kickpager() found %uld free\n", pg->freecount);
		goto Done;
	}

	/*
	 * What else can we do?
	 * But don't panic if we are still trying to get memory of
	 * a particular color and there's none. We'll retry asking
	 * for any color.
	 */
	if(color == NOCOLOR){
		print("out of physical memory\n");
		killbig("out of physical memory");
		freebroken();
	}

Done:
	poperror();
	qunlock(&pagerlck);
	if(DBGFLG>1)
		DBG("kickpager() done %#p\n", up);
}

void
pagersummary(void)
{
	print("ntext %uld nbig %uld nall %uld\n",
		pstats.ntext, pstats.nbig, pstats.nall);
	print("no swap\n");
}
