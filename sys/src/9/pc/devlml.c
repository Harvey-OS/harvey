/*
 * Lml 22 driver
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"io.h"

#include	"devlml.h"

#define DBGREAD	0x01
#define DBGWRIT	0x02
#define DBGINTR	0x04
#define DBGINTS	0x08
#define DBGFS	0x10

int debug = DBGREAD|DBGWRIT|DBGFS;

enum{
	Qdir,
	Qctl0,
	Qjpg0,
	Qraw0,
	Qctl1,
	Qjpg1,
	Qraw1,
};

static Dirtab lmldir[] = {
	".",		{Qdir, 0, QTDIR},	0,	0555,
	"lml0ctl",	{Qctl0},		0,	0666,
	"lml0jpg",	{Qjpg0},		0,	0444,
	"lml0raw",	{Qraw0},		0,	0444,
	"lml1ctl",	{Qctl1},		0,	0666,
	"lml1jpg",	{Qjpg1},		0,	0444,
	"lml1raw",	{Qraw1},		0,	0444,
};

typedef struct LML LML;

struct LML {
	/* Hardware */
	Pcidev	*pcidev;
	ulong	pciBaseAddr;

	/* Allocated memory */
	CodeData *codedata;

	/* Software state */
	ulong	jpgframeno;
	int	frameNo;
	Rendez	sleepjpg;
	int	jpgopens;
} lmls[NLML];

int nlml;

static FrameHeader jpgheader = {
	MRK_SOI, MRK_APP3, (sizeof(FrameHeader)-4) << 8,
	{ 'L', 'M', 'L', '\0'},
	-1, 0, 0,  0
};

#define writel(v, a) *(ulong *)(a) = (v)
#define readl(a) *(ulong*)(a)

static int
getbuffer(void *x)
{
	static last = NBUF-1;
	int l = last;
	LML *lml;

	lml = x;
	for(;;){
		last = (last+1) % NBUF;
		if(lml->codedata->statCom[last] & STAT_BIT)
			return last + 1;
		if(last == l)
			return 0;
	}
}

static long
jpgread(LML *lml, void *va, long nbytes, vlong, int dosleep)
{
	int bufno;
	FrameHeader *jpgheader;

	/*
	 * reads should be of size 1 or sizeof(FrameHeader).
	 * Frameno is the number of the buffer containing the data.
	 */
	while((bufno = getbuffer(lml)) == 0 && dosleep)
		sleep(&lml->sleepjpg, getbuffer, lml);
	if(--bufno < 0)
		return 0;

	jpgheader = (FrameHeader*)(lml->codedata->frag[bufno].hdr+2);
	if(nbytes == sizeof(FrameHeader)){
		memmove(va, jpgheader, sizeof(FrameHeader));
		return sizeof(FrameHeader);
	}
	if(nbytes == 1){
		*(char *)va = bufno;
		return 1;
	}
	return 0;
}

static void lmlintr(Ureg *, void *);

static void
prepbuf(LML *lml)
{
	int i;
	CodeData *cd;

	cd = lml->codedata;
	for(i = 0; i < NBUF; i++){
		cd->statCom[i] = PADDR(&(cd->fragdesc[i]));
		cd->fragdesc[i].addr = PADDR(cd->frag[i].fb);
		/* Length is in double words, in position 1..20 */
		cd->fragdesc[i].leng = FRAGSIZE >> 1 | FRAGM_FINAL_B;
		memmove(cd->frag[i].hdr+2, &jpgheader, sizeof(FrameHeader)-2);
	}
}

static void
lmlreset(void)
{
	ulong regpa;
	char name[32];
	void *regva;
	LML *lml;
	Pcidev *pcidev;
	Physseg segbuf;

	pcidev = nil;

	for(nlml = 0; nlml < NLML && (pcidev = pcimatch(pcidev, VENDOR_ZORAN,
	    ZORAN_36067)); nlml++){
		lml = &lmls[nlml];
		lml->pcidev = pcidev;
		lml->codedata = (CodeData*)(((ulong)xalloc(Codedatasize+ BY2PG)
			+ BY2PG-1) & ~(BY2PG-1));
		if(lml->codedata == nil){
			print("devlml: xalloc(%ux, %ux, 0)\n", Codedatasize, BY2PG);
			return;
		}

		print("Installing Motion JPEG driver %s, irq %d\n",
			MJPG_VERSION, pcidev->intl);
		print("MJPG buffer at 0x%.8p, size 0x%.8ux\n", lml->codedata,
			Codedatasize);

		/* Get access to DMA memory buffer */
		lml->codedata->pamjpg = PADDR(lml->codedata->statCom);

		prepbuf(lml);

		print("zr36067 found at 0x%.8lux", pcidev->mem[0].bar & ~0x0F);

		regpa = pcidev->mem[0].bar & ~0x0F;
		regva = vmap(regpa, pcidev->mem[0].size);
		if(regva == 0){
			print("lml: failed to map registers\n");
			return;
		}
		lml->pciBaseAddr = (ulong)regva;
		print(", mapped at 0x%.8lux\n", lml->pciBaseAddr);

		memset(&segbuf, 0, sizeof(segbuf));
		segbuf.attr = SG_PHYSICAL;
		sprint(name, "lml%d.mjpg", nlml);
		kstrdup(&segbuf.name, name);
		segbuf.pa = PADDR(lml->codedata);
		segbuf.size = Codedatasize;
		if(addphysseg(&segbuf) == -1){
			print("lml: physsegment: %s\n", name);
			return;
		}

		memset(&segbuf, 0, sizeof(segbuf));
		segbuf.attr = SG_PHYSICAL;
		sprint(name, "lml%d.regs", nlml);
		kstrdup(&segbuf.name, name);
		segbuf.pa = (ulong)regpa;
		segbuf.size = pcidev->mem[0].size;
		if(addphysseg(&segbuf) == -1){
			print("lml: physsegment: %s\n", name);
			return;
		}

		/* set up interrupt handler */
		intrenable(pcidev->intl, lmlintr, lml, pcidev->tbdf, "lml");
	}
}

static Chan*
lmlattach(char *spec)
{
	if(debug&DBGFS)
		print("lmlattach\n");
	return devattach(L'Λ', spec);
}

static Walkqid*
lmlwalk(Chan *c, Chan *nc, char **name, int nname)
{
	if(debug&DBGFS)
		print("lmlwalk\n");
	return devwalk(c, nc, name, nname, lmldir, 3*nlml+1, devgen);
}

static int
lmlstat(Chan *c, uchar *db, int n)
{
	if(debug&DBGFS)
		print("lmlstat\n");
	return devstat(c, db, n, lmldir, 3*nlml+1, devgen);
}

static Chan*
lmlopen(Chan *c, int omode)
{
	int i;
	LML *lml;

	if(debug&DBGFS)
		print("lmlopen\n");
	if(omode != OREAD)
		error(Eperm);
	c->aux = 0;
	i = 0;
	switch((ulong)c->qid.path){
	case Qctl1:
		i++;
		/* fall through */
	case Qctl0:
		if(i >= nlml)
			error(Eio);
		break;
	case Qjpg1:
	case Qraw1:
		i++;
		/* fall through */
	case Qjpg0:
	case Qraw0:
		/* allow one open */
		if(i >= nlml)
			error(Eio);
		lml = lmls+i;
		if(lml->jpgopens)
			error(Einuse);
		lml->jpgopens = 1;
		lml->jpgframeno = 0;
		prepbuf(lml);
		break;
	}
	return devopen(c, omode, lmldir, 3*nlml+1, devgen);
}

static void
lmlclose(Chan *c)
{
	int i;

	if(debug&DBGFS)
		print("lmlclose\n");
	i = 0;
	switch((ulong)c->qid.path){
	case Qjpg1:
	case Qraw1:
		i++;
		/* fall through */
	case Qjpg0:
	case Qraw0:
		lmls[i].jpgopens = 0;
		break;
	}
}

static long
lmlread(Chan *c, void *va, long n, vlong voff)
{
	int i, len;
	long off = voff;
	uchar *buf = va;
	LML *lml;
	static char lmlinfo[1024];

	i = 0;
	switch((ulong)c->qid.path){
	case Qdir:
		n = devdirread(c, (char *)buf, n, lmldir, 3*nlml+1, devgen);
		if(debug&(DBGFS|DBGREAD))
			print("lmlread %ld\n", n);
		return n;
	case Qctl1:
		i++;
		/* fall through */
	case Qctl0:
		if(i >= nlml)
			error(Eio);
		lml = lmls+i;
		len = snprint(lmlinfo, sizeof lmlinfo, "lml%djpg	lml%draw\nlml%d.regs	0x%lux	0x%ux\nlml%d.mjpg	0x%lux	0x%ux\n",
			i, i,
			i, lml->pcidev->mem[0].bar & ~0x0F, lml->pcidev->mem[0].size,
			i, PADDR(lml->codedata), Codedatasize);
		if(voff > len)
			return 0;
		if(n > len - voff)
			n = len - voff;
		memmove(va, lmlinfo+voff, n);
		return n;
	case Qjpg1:
		i++;
		/* fall through */
	case Qjpg0:
		if(i >= nlml)
			error(Eio);
		return jpgread(lmls+i, buf, n, off, 1);
	case Qraw1:
		i++;
		/* fall through */
	case Qraw0:
		if(i >= nlml)
			error(Eio);
		return jpgread(lmls+i, buf, n, off, 0);
	}
	return -1;
}

static long
lmlwrite(Chan *, void *, long, vlong)
{
	error(Eperm);
	return 0;
}

Dev lmldevtab = {
	L'Λ',
	"video",

	lmlreset,
	devinit,
	devshutdown,
	lmlattach,
	lmlwalk,
	lmlstat,
	lmlopen,
	devcreate,
	lmlclose,
	lmlread,
	devbread,
	lmlwrite,
	devbwrite,
	devremove,
	devwstat,
};

static void
lmlintr(Ureg *, void *x)
{
	ulong fstart, fno, flags, statcom;
	FrameHeader *jpgheader;
	LML *lml;

	lml = x;
	flags = readl(lml->pciBaseAddr+INTR_STAT);
	/* Reset all interrupts from 067 */
	writel(0xff000000, lml->pciBaseAddr + INTR_STAT);

	if(flags & INTR_JPEGREP){

		if(debug&DBGINTR)
			print("MjpgDrv_intrHandler stat=0x%.8lux\n", flags);

		fstart = lml->jpgframeno & 3;
		for(;;){
			lml->jpgframeno++;
			fno = lml->jpgframeno & 3;
			if(lml->codedata->statCom[fno] & STAT_BIT)
				break;
			if(fno == fstart){
				if(debug & DBGINTR)
					print("Spurious lml jpg intr?\n");
				return;
			}
		}
		statcom = lml->codedata->statCom[fno];
		jpgheader = (FrameHeader *)(lml->codedata->frag[fno].hdr + 2);
		jpgheader->frameNo = lml->jpgframeno;
		jpgheader->ftime  = todget(nil);
		jpgheader->frameSize = (statcom & 0x00ffffff) >> 1;
		jpgheader->frameSeqNo = statcom >> 24;
		wakeup(&lml->sleepjpg);
	}
}
