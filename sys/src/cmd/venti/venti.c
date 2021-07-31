#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int debug;

static Packet	*srvRead(VtSession *z, uchar score[VtScoreSize], int type, int n);
static int	srvWrite(VtSession *z, uchar score[VtScoreSize], int type, Packet *p);
static void	srvSync(VtSession *z);
static void	srvClosing(VtSession *z, int clean);
static void	daemon(void *rock);

static VtServerVtbl serverVtbl = {
.read	srvRead,
.write	srvWrite,
.sync	srvSync,
.closing srvClosing,
};

void
main(int argc, char *argv[])
{
	char *config, *haddr, *vaddr;
	u32int mem, icmem, bcmem;
	int background, cfd;
	char *adir;
	Config conf;

	vaddr = nil;
	haddr = nil;
	config = nil;
	mem = 0xFFFFFFFFUL;
	icmem = 0;
	bcmem = 0;
	background = 1;
	ARGBEGIN{
	case 'a':
		vaddr = ARGF();
		if(vaddr == nil)
			goto usage;
		break;
	case 'B':
		bcmem = unittoull(ARGF());
		break;
	case 's':
		background = 0;
		break;
	case 'c':
		config = ARGF();
		if(config == nil)
			goto usage;
		break;
	case 'C':
		mem = unittoull(ARGF());
		break;
	case 'd':
		debug = 1;
		break;
	case 'h':
		haddr = ARGF();
		if(haddr == nil)
			goto usage;
		break;
	case 'I':
		icmem = unittoull(ARGF());
		break;
	case 'w':
		queueWrites = 1;
		break;
	default:
		goto usage;
	}ARGEND

	if(argc){
  usage:
		fprint(2, "usage: venti [-dsw] [-a ventiaddress] [-h httpaddress] [-c config] [-C cachesize] [-I icachesize] [-B blockcachesize]\n");
		exits("usage");
	}

	if(config == nil)
		config = "venti.conf";

	vtAttach();

	if(!initArenaSum())
		fprint(2, "warning: can't initialize arena summing process: %R");

//fprint(2, "initVenti %s\n", config);
	if(!initVenti(config, &conf))
		fatal("can't init server: %R");
	if(vaddr == nil){
		vaddr = conf.vaddr;
		if(vaddr == nil)
			vaddr = "tcp!*!venti";
	}
	if(haddr == nil)
		haddr = conf.haddr;
	if(bcmem == 0)
		bcmem = conf.bcmem;
	if(icmem == 0)
		icmem = conf.icmem;
	if(mem == 0xFFFFFFFFUL){
		mem = conf.mem;
		if(mem == 0xFFFFFFFFUL)
			mem = 1 * 1024 * 1024;
	}
	if(queueWrites == 0)
		queueWrites = conf.queueWrites;

//	fprint(2, "initialize %d bytes of lump cache for %d lumps\n", mem, mem / (8 * 1024));
	initLumpCache(mem, mem / (8 * 1024));

	icmem = u64log2(icmem / (sizeof(IEntry) + sizeof(IEntry*)) / ICacheDepth);
	if(icmem < 4)
		icmem = 4;
if(0)	fprint(2, "initialize %d bytes of index cache for %d index entries\n",
		(sizeof(IEntry) + sizeof(IEntry*)) * (1 << icmem) * ICacheDepth, (1 << icmem) * ICacheDepth);
	initICache(icmem, ICacheDepth);

	/*
	 * need a block for every arena and every process
	 */
	if(bcmem < maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16))
		bcmem = maxBlockSize * (mainIndex->narenas + mainIndex->nsects * 4 + 16);
if(0)	fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initDCache(bcmem);

if(0)	fprint(2, "sync arenas and index...\n");
	if(!syncIndex(mainIndex, 1))
		fatal("can't sync server: %R");

	if(queueWrites){
	//	fprint(2, "initialize write queue...\n");
		if(!initLumpQueues(mainIndex->nsects)){
			fprint(2, "can't initialize lump queues, disabling write queueing: %R");
			queueWrites = 0;
		}
	}

	if(haddr){
	//	fprint(2, "starting http server at %s\n", haddr);
		if(httpdInit(haddr) < 0)
			fprint(2, "warning: can't start http server: %R");
	}

if(0)	fprint(2, "starting server\n");
	adir = vtMemAlloc(100);
	cfd = announce(vaddr, adir);
	if(cfd < 0)
		fatal("can't announce: %s", vtOSError());
if(0)fprint(2, "announced %s\n", vaddr);
	if(background)
		vtThread(daemon, adir);
	else
		daemon(adir);

	vtDetach();

	exits(0);
}

static void
daemon(void *rock)
{
	int lcfd, fd;
	char *adir;
	char ldir[100];
	VtSession *z;

	adir = rock;
	for(;;){
		/* listen for a call */
		lcfd = listen(adir, ldir);
		if(lcfd < 0)
			fatal("listen: %s", vtOSError());
//fprint(2, "new call on %s\n", ldir);
checkLumpCache();
checkDCache();
//printStats();
		fd = accept(lcfd, ldir);
		close(lcfd);
		z = vtServerAlloc(&serverVtbl);
		vtSetDebug(z, debug);
		vtSetFd(z, fd);
		vtExport(z);
	}
}

static Packet *
srvRead(VtSession *z, uchar score[VtScoreSize], int type, int n)
{
	Packet *p;
	USED(z);

	p = readLump(score, type, n);
	return p;
}

static int
srvWrite(VtSession *z, uchar score[VtScoreSize], int type, Packet *p)
{
	int ok;

	USED(z);
	ok = writeLump(p, score, type, 0);
	return ok;
}

static void
srvSync(VtSession *z)
{
	USED(z);
	if(queueWrites)
		queueFlush();
}

static void
srvClosing(VtSession *z, int clean)
{
//	long rt, t[4];

	USED(clean);
	USED(z);
//	if(!clean)
//		fprint(2, "not a clean exit: %s\n", vtGetError());
//	rt = times(t);
//	fprint(2, "times: %.2fu %.2fs %.2fr\n", t[0]*.001, t[1]*.001, rt*.001);
packetStats();
}

