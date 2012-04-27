#ifdef PLAN9PORT
#include <u.h>
#include <signal.h>
#endif
#include "stdinc.h"
#include <bio.h>
#include "dat.h"
#include "fns.h"

#include "whack.h"

typedef struct Allocs Allocs;
struct Allocs {
	u32int	mem;
	u32int	bcmem;
	u32int	icmem;
	u32int	stfree;				/* free memory at start */
	uint	mempcnt;
};

int debug;
int nofork;
int mainstacksize = 256*1024;
VtSrv *ventisrv;

static void	ventiserver(void*);

static ulong
freemem(void)
{
	int nf, pgsize = 0;
	uvlong size, userpgs = 0, userused = 0;
	char *ln, *sl;
	char *fields[2];
	Biobuf *bp;

	size = 64*1024*1024;
	bp = Bopen("#c/swap", OREAD);
	if (bp != nil) {
		while ((ln = Brdline(bp, '\n')) != nil) {
			ln[Blinelen(bp)-1] = '\0';
			nf = tokenize(ln, fields, nelem(fields));
			if (nf != 2)
				continue;
			if (strcmp(fields[1], "pagesize") == 0)
				pgsize = atoi(fields[0]);
			else if (strcmp(fields[1], "user") == 0) {
				sl = strchr(fields[0], '/');
				if (sl == nil)
					continue;
				userpgs = atoll(sl+1);
				userused = atoll(fields[0]);
			}
		}
		Bterm(bp);
		if (pgsize > 0 && userpgs > 0 && userused > 0)
			size = (userpgs - userused) * pgsize;
	}
	/* cap it to keep the size within 32 bits */
	if (size >= 3840UL * 1024 * 1024)
		size = 3840UL * 1024 * 1024;
	return size;
}

static void
allocminima(Allocs *all)			/* enforce minima for sanity */
{
	if (all->icmem < 6 * 1024 * 1024)
		all->icmem = 6 * 1024 * 1024;
	if (all->mem < 1024 * 1024 || all->mem == Unspecified)  /* lumps */
		all->mem = 1024 * 1024;
	if (all->bcmem < 2 * 1024 * 1024)
		all->bcmem = 2 * 1024 * 1024;
}

/* automatic memory allocations sizing per venti(8) guidelines */
static Allocs
allocbypcnt(u32int mempcnt, u32int stfree)
{
	u32int avail;
	vlong blmsize;
	Allocs all;
	static u32int free;

	all.mem = Unspecified;
	all.bcmem = all.icmem = 0;
	all.mempcnt = mempcnt;
	all.stfree = stfree;

	if (free == 0)
		free = freemem();
	blmsize = stfree - free;
	if (blmsize <= 0)
		blmsize = 0;
	avail = ((vlong)stfree * mempcnt) / 100;
	if (blmsize >= avail || (avail -= blmsize) <= (1 + 2 + 6) * 1024 * 1024)
		fprint(2, "%s: bloom filter bigger than mem pcnt; "
			"resorting to minimum values (9MB total)\n", argv0);
	else {
		if (avail >= 3840UL * 1024 * 1024)
			avail = 3840UL * 1024 * 1024;	/* sanity */
		avail /= 2;
		all.icmem = avail;
		avail /= 3;
		all.mem = avail;
		all.bcmem = 2 * avail;
	}
	return all;
}

/*
 * we compute default values for allocations,
 * which can be overridden by (in order):
 *	configuration file parameters,
 *	command-line options other than -m, and -m.
 */
static Allocs
sizeallocs(Allocs opt, Config *cfg)
{
	Allocs all;

	/* work out sane defaults */
	all = allocbypcnt(20, opt.stfree);

	/* config file parameters override */
	if (cfg->mem && cfg->mem != Unspecified)
		all.mem = cfg->mem;
	if (cfg->bcmem)
		all.bcmem = cfg->bcmem;
	if (cfg->icmem)
		all.icmem = cfg->icmem;

	/* command-line options override */
	if (opt.mem && opt.mem != Unspecified)
		all.mem = opt.mem;
	if (opt.bcmem)
		all.bcmem = opt.bcmem;
	if (opt.icmem)
		all.icmem = opt.icmem;

	/* automatic memory sizing? */
	if(opt.mempcnt > 0)
		all = allocbypcnt(opt.mempcnt, opt.stfree);

	allocminima(&all);
	return all;
}

void
usage(void)
{
	fprint(2, "usage: venti [-Ldrsw] [-a ventiaddr] [-c config] "
"[-h httpaddr] [-m %%mem] [-B blockcachesize] [-C cachesize] [-I icachesize] "
"[-W webroot]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *configfile, *haddr, *vaddr, *webroot;
	u32int mem, icmem, bcmem, minbcmem, mempcnt, stfree;
	Allocs allocs;
	Config config;

	traceinit();
	threadsetname("main");
	mempcnt = 0;
	vaddr = nil;
	haddr = nil;
	configfile = nil;
	webroot = nil;
	mem = Unspecified;
	icmem = 0;
	bcmem = 0;
	ARGBEGIN{
	case 'a':
		vaddr = EARGF(usage());
		break;
	case 'B':
		bcmem = unittoull(EARGF(usage()));
		break;
	case 'c':
		configfile = EARGF(usage());
		break;
	case 'C':
		mem = unittoull(EARGF(usage()));
		break;
	case 'D':
		settrace(EARGF(usage()));
		break;
	case 'd':
		debug = 1;
		nofork = 1;
		break;
	case 'h':
		haddr = EARGF(usage());
		break;
	case 'm':
		mempcnt = atoi(EARGF(usage()));
		if (mempcnt <= 0 || mempcnt >= 100)
			usage();
		break;
	case 'I':
		icmem = unittoull(EARGF(usage()));
		break;
	case 'L':
		ventilogging = 1;
		break;
	case 'r':
		readonly = 1;
		break;
	case 's':
		nofork = 1;
		break;
	case 'w':			/* compatibility with old venti */
		queuewrites = 1;
		break;
	case 'W':
		webroot = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc)
		usage();

	if(!nofork)
		rfork(RFNOTEG);

#ifdef PLAN9PORT
	{
		/* sigh - needed to avoid signals when writing to hungup networks */
		struct sigaction sa;
		memset(&sa, 0, sizeof sa);
		sa.sa_handler = SIG_IGN;
		sigaction(SIGPIPE, &sa, nil);
	}
#endif

	ventifmtinstall();
	trace(TraceQuiet, "venti started");
	fprint(2, "%T venti: ");

	if(configfile == nil)
		configfile = "venti.conf";

	/* remember free memory before initventi & loadbloom, for auto-sizing */
	stfree = freemem(); 	 
	fprint(2, "conf...");
	if(initventi(configfile, &config) < 0)
		sysfatal("can't init server: %r");
	/*
	 * load bloom filter
	 */
	if(mainindex->bloom && loadbloom(mainindex->bloom) < 0)
		sysfatal("can't load bloom filter: %r");

	/*
	 * size memory allocations; assumes bloom filter is loaded
	 */
	allocs = sizeallocs((Allocs){mem, bcmem, icmem, stfree, mempcnt},
		&config);
	mem = allocs.mem;
	bcmem = allocs.bcmem;
	icmem = allocs.icmem;
	fprint(2, "%s: mem %,ud bcmem %,ud icmem %,ud...",
		argv0, mem, bcmem, icmem);

	/*
	 * default other configuration-file parameters
	 */
	if(haddr == nil)
		haddr = config.haddr;
	if(vaddr == nil)
		vaddr = config.vaddr;
	if(vaddr == nil)
		vaddr = "tcp!*!venti";
	if(webroot == nil)
		webroot = config.webroot;
	if(queuewrites == 0)
		queuewrites = config.queuewrites;

	if(haddr){
		fprint(2, "httpd %s...", haddr);
		if(httpdinit(haddr, webroot) < 0)
			fprint(2, "warning: can't start http server: %r");
	}
	fprint(2, "init...");

	/*
	 * lump cache
	 */
	if(0) fprint(2, "initialize %d bytes of lump cache for %d lumps\n",
		mem, mem / (8 * 1024));
	initlumpcache(mem, mem / (8 * 1024));

	/*
	 * index cache
	 */
	initicache(icmem);
	initicachewrite();

	/*
	 * block cache: need a block for every arena and every process
	 */
	minbcmem = maxblocksize * 
		(mainindex->narenas + mainindex->nsects*4 + 16);
	if(bcmem < minbcmem)
		bcmem = minbcmem;
	if(0) fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initdcache(bcmem);

	if(mainindex->bloom)
		startbloomproc(mainindex->bloom);

	fprint(2, "sync...");
	if(!readonly && syncindex(mainindex) < 0)
		sysfatal("can't sync server: %r");

	if(!readonly && queuewrites){
		fprint(2, "queue...");
		if(initlumpqueues(mainindex->nsects) < 0){
			fprint(2, "can't initialize lump queues,"
				" disabling write queueing: %r");
			queuewrites = 0;
		}
	}

	if(initarenasum() < 0)
		fprint(2, "warning: can't initialize arena summing process: %r");

	fprint(2, "announce %s...", vaddr);
	ventisrv = vtlisten(vaddr);
	if(ventisrv == nil)
		sysfatal("can't announce %s: %r", vaddr);

	fprint(2, "serving.\n");
	if(nofork)
		ventiserver(nil);
	else
		vtproc(ventiserver, nil);

	threadexits(nil);
}

static void
vtrerror(VtReq *r, char *error)
{
	r->rx.msgtype = VtRerror;
	r->rx.error = estrdup(error);
}

static void
ventiserver(void *v)
{
	Packet *p;
	VtReq *r;
	char err[ERRMAX];
	uint ms;
	int cached, ok;

	USED(v);
	threadsetname("ventiserver");
	trace(TraceWork, "start");
	while((r = vtgetreq(ventisrv)) != nil){
		trace(TraceWork, "finish");
		trace(TraceWork, "start request %F", &r->tx);
		trace(TraceRpc, "<- %F", &r->tx);
		r->rx.msgtype = r->tx.msgtype+1;
		addstat(StatRpcTotal, 1);
		if(0) print("req (arenas[0]=%p sects[0]=%p) %F\n",
			mainindex->arenas[0], mainindex->sects[0], &r->tx);
		switch(r->tx.msgtype){
		default:
			vtrerror(r, "unknown request");
			break;
		case VtTread:
			ms = msec();
			r->rx.data = readlump(r->tx.score, r->tx.blocktype, r->tx.count, &cached);
			ms = msec() - ms;
			addstat2(StatRpcRead, 1, StatRpcReadTime, ms);
			if(r->rx.data == nil){
				addstat(StatRpcReadFail, 1);
				rerrstr(err, sizeof err);
				vtrerror(r, err);
			}else{
				addstat(StatRpcReadBytes, packetsize(r->rx.data));
				addstat(StatRpcReadOk, 1);
				if(cached)
					addstat2(StatRpcReadCached, 1, StatRpcReadCachedTime, ms);
				else
					addstat2(StatRpcReadUncached, 1, StatRpcReadUncachedTime, ms);
			}
			break;
		case VtTwrite:
			if(readonly){
				vtrerror(r, "read only");
				break;
			}
			p = r->tx.data;
			r->tx.data = nil;
			addstat(StatRpcWriteBytes, packetsize(p));
			ms = msec();
			ok = writelump(p, r->rx.score, r->tx.blocktype, 0, ms);
			ms = msec() - ms;
			addstat2(StatRpcWrite, 1, StatRpcWriteTime, ms);

			if(ok < 0){
				addstat(StatRpcWriteFail, 1);
				rerrstr(err, sizeof err);
				vtrerror(r, err);
			}
			break;
		case VtTsync:
			flushqueue();
			flushdcache();
			break;
		}
		trace(TraceRpc, "-> %F", &r->rx);
		vtrespond(r);
		trace(TraceWork, "start");
	}
	flushdcache();
	flushicache();
	threadexitsall(0);
}
