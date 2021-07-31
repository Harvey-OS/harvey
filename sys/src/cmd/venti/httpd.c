#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "httpd.h"
#include "xml.h"

typedef struct HttpObj	HttpObj;

enum
{
	ObjNameSize	= 64,
	MaxObjs		= 16
};

struct HttpObj
{
	char	name[ObjNameSize];
	int	(*f)(HConnect*);
};

static HttpObj	objs[MaxObjs];

static	void		dolisten(void*);
static	int		estats(HConnect *c);
static	int		darenas(HConnect *c);
static	int		dindex(HConnect *c);
static	int		xindex(HConnect *c);
static	int		sindex(HConnect *c);
static	int		notfound(HConnect *c);
static	int		httpdObj(char *name, int (*f)(HConnect*));

int
httpdInit(char *address)
{
	fmtinstall('D', hdatefmt);
	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);

	if(address == nil)
		address = "tcp!*!http";

	httpdObj("/stats", estats);
	httpdObj("/index", dindex);
	httpdObj("/storage", sindex);
	httpdObj("/xindex", xindex);

	if(vtThread(dolisten, address) < 0)
		return 0;
	return 1;
}

static int
httpdObj(char *name, int (*f)(HConnect*))
{
	int i;

	if(name == nil || strlen(name) >= ObjNameSize)
		return 0;
	for(i = 0; i < MaxObjs; i++){
		if(objs[i].name[0] == '\0'){
			strcpy(objs[i].name, name);
			objs[i].f = f;
			return 1;
		}
		if(strcmp(objs[i].name, name) == 0)
			return 0;
	}
	return 0;
}

static HConnect*
mkconnect(void)
{
	HConnect *c;

	c = mallocz(sizeof(HConnect), 1);
	if(c == nil)
		sysfatal("out of memory");
	c->replog = nil;
	c->hpos = c->header;
	c->hstop = c->header;
	return c;
}

static void
dolisten(void *vaddress)
{
	HConnect *c;
	char *address, ndir[NETPATHLEN], dir[NETPATHLEN];
	int ctl, nctl, data, ok, t, i;

	address = vaddress;
	ctl = announce(address, dir);
	if(ctl < 0){
		fprint(2, "venti: httpd can't announce on %s: %r\n", address);
		return;
	}

	for(;;){

		/*
		 *  wait for a call (or an error)
		 */
		nctl = listen(dir, ndir);
		if(nctl < 0){
			fprint(2, "venti: httpd can't listen on %s: %r\m", address);
			return;
		}

		/*
		 *  start a process for the service
		 */
		switch(rfork(RFFDG|RFPROC|RFNOWAIT|RFNAMEG|RFMEM)){
		case -1:
			close(nctl);
			continue;
		case 0:
			/*
			 *  see if we know the service requested
			 */
			data = accept(ctl, ndir);
			if(data < 0){
				fprint(2, "venti: httpd can't open %s/data: %r", ndir);
				exits(nil);
			}
			close(ctl);
			close(nctl);

			c = mkconnect();

			hinit(&c->hin, data, Hread);
			hinit(&c->hout, data, Hwrite);

			for(t = 15*60*1000; ; t = 15*1000){
				if(hparsereq(c, t) <= 0)
					break;

				ok = -1;
				for(i = 0; i < MaxObjs && objs[i].name[0]; i++){
					if(strcmp(c->req.uri, objs[i].name) == 0){
						ok = (*objs[i].f)(c);
						break;
					}
				}
				if(i == MaxObjs)
					ok = notfound(c);
				if(c->head.closeit)
					ok = -1;
				hreqcleanup(c);

				if(ok < 0)
					break;
			}
			hreqcleanup(c);
			free(c);
			exits(nil);
		default:
			close(nctl);
			break;
		}
	}
}

static int
percent(long v, long total)
{
	if(total == 0)
		total = 1;
	if(v < 1000*1000)
		return (v * 100) / total;
	total /= 100;
	if(total == 0)
		total = 1;
	return v / total;
}

static int
preq(HConnect *c)
{
	if(hparseheaders(c, 15*60*1000) < 0)
		return -1;
	if(strcmp(c->req.meth, "GET") != 0
	&& strcmp(c->req.meth, "HEAD") != 0)
		return hunallowed(c, "GET, HEAD");
	if(c->head.expectother || c->head.expectcont)
		return hfail(c, HExpectFail, nil);
	return 1;
}

static int
preqtext(HConnect *c)
{
	Hio *hout;
	int r;

	r = preq(c);
	if(r <= 0)
		return r;

	hout = &c->hout;
	if(c->req.vermaj){
		hokheaders(c);
		hprint(hout, "Content-type: text/plain\r\n");
		if(http11(c))
			hprint(hout, "Transfer-Encoding: chunked\r\n");
		hprint(hout, "\r\n");
	}

	if(http11(c))
		hxferenc(hout, 1);
	else
		c->head.closeit = 1;
	return 1;
}

static
notfound(HConnect *c)
{
	int r;

	r = preq(c);
	if(r <= 0)
		return r;
	return hfail(c, HNotFound, c->req.uri);
}

static int
estats(HConnect *c)
{
	Hio *hout;
	int r;

	r = preqtext(c);
	if(r <= 0)
		return r;

	hout = &c->hout;
	hprint(hout, "lump writes=%,ld\n", stats.lumpWrites);
	hprint(hout, "lump reads=%,ld\n", stats.lumpReads);
	hprint(hout, "lump cache read hits=%,ld\n", stats.lumpHit);
	hprint(hout, "lump cache read misses=%,ld\n", stats.lumpMiss);

	hprint(hout, "clump disk writes=%,ld\n", stats.clumpWrites);
	hprint(hout, "clump disk bytes written=%,lld\n", stats.clumpBWrites);
	hprint(hout, "clump disk bytes compressed=%,lld\n", stats.clumpBComp);
	hprint(hout, "clump disk reads=%,ld\n", stats.clumpReads);
	hprint(hout, "clump disk bytes read=%,lld\n", stats.clumpBReads);
	hprint(hout, "clump disk bytes uncompressed=%,lld\n", stats.clumpBUncomp);

	hprint(hout, "clump directory disk writes=%,ld\n", stats.ciWrites);
	hprint(hout, "clump directory disk reads=%,ld\n", stats.ciReads);

	hprint(hout, "index disk writes=%,ld\n", stats.indexWrites);
	hprint(hout, "index disk reads=%,ld\n", stats.indexReads);
	hprint(hout, "index disk reads for modify=%,ld\n", stats.indexWReads);
	hprint(hout, "index disk reads for allocation=%,ld\n", stats.indexAReads);

	hprint(hout, "index cache lookups=%,ld\n", stats.icLookups);
	hprint(hout, "index cache hits=%,ld %d%%\n", stats.icHits,
		percent(stats.icHits, stats.icLookups));
	hprint(hout, "index cache fills=%,ld %d%%\n", stats.icFills,
		percent(stats.icFills, stats.icLookups));
	hprint(hout, "index cache inserts=%,ld\n", stats.icInserts);

	hprint(hout, "disk cache hits=%,ld\n", stats.pcHit);
	hprint(hout, "disk cache misses=%,ld\n", stats.pcMiss);
	hprint(hout, "disk cache reads=%,ld\n", stats.pcReads);
	hprint(hout, "disk cache bytes read=%,lld\n", stats.pcBReads);

	hprint(hout, "disk writes=%,ld\n", stats.diskWrites);
	hprint(hout, "disk bytes written=%,lld\n", stats.diskBWrites);
	hprint(hout, "disk reads=%,ld\n", stats.diskReads);
	hprint(hout, "disk bytes read=%,lld\n", stats.diskBReads);

	hflush(hout);
	return 1;
}

static int
sindex(HConnect *c)
{
	Hio *hout;
	Index *ix;
	Arena *arena;
	vlong clumps, cclumps, uncsize, used, size;
	int i, r, active;

	r = preqtext(c);
	if(r <= 0)
		return r;
	hout = &c->hout;

	ix = mainIndex;

	hprint(hout, "index=%s\n", ix->name);

	active = 0;
	clumps = 0;
	cclumps = 0;
	uncsize = 0;
	used = 0;
	size = 0;
	for(i = 0; i < ix->narenas; i++){
		arena = ix->arenas[i];
		if(arena != nil && arena->clumps != 0){
			active++;
			clumps += arena->clumps;
			cclumps += arena->cclumps;
			uncsize += arena->uncsize;
			used += arena->used;
		}
		size += arena->size;
	}
	hprint(hout, "total arenas=%d active=%d\n", ix->narenas, active);
	hprint(hout, "total space=%lld used=%lld\n", size, used + clumps * ClumpInfoSize);
	hprint(hout, "clumps=%lld compressed clumps=%lld data=%lld compressed data=%lld\n",
		clumps, cclumps, uncsize, used - clumps * ClumpSize);
	hflush(hout);
	return 1;
}

static void
dArena(Hio *hout, Arena *arena)
{
	hprint(hout, "arena='%s' on %s at [%lld,%lld)\n\tversion=%d created=%d modified=%d",
		arena->name, arena->part->name, arena->base, arena->base + arena->size + 2 * arena->blockSize,
		arena->version, arena->ctime, arena->wtime);
	if(arena->sealed)
		hprint(hout, " sealed\n");
	else
		hprint(hout, "\n");
	if(!scoreEq(zeroScore, arena->score))
		hprint(hout, "\tscore=%V\n", arena->score);

	hprint(hout, "\tclumps=%d compressed clumps=%d data=%lld compressed data=%lld disk storage=%lld\n",
		arena->clumps, arena->cclumps, arena->uncsize,
		arena->used - arena->clumps * ClumpSize,
		arena->used + arena->clumps * ClumpInfoSize);
}

static int
dindex(HConnect *c)
{
	Hio *hout;
	Index *ix;
	int i, r;

	r = preqtext(c);
	if(r <= 0)
		return r;
	hout = &c->hout;


	ix = mainIndex;
	hprint(hout, "index=%s version=%d blockSize=%d tabSize=%d\n",
		ix->name, ix->version, ix->blockSize, ix->tabSize);
	hprint(hout, "\tbuckets=%d div=%d\n", ix->buckets, ix->div);
	for(i = 0; i < ix->nsects; i++)
		hprint(hout, "\tsect=%s for buckets [%lld,%lld)\n", ix->smap[i].name, ix->smap[i].start, ix->smap[i].stop);
	for(i = 0; i < ix->narenas; i++){
		if(ix->arenas[i] != nil && ix->arenas[i]->clumps != 0){
			hprint(hout, "arena=%s at index [%lld,%lld)\n\t", ix->amap[i].name, ix->amap[i].start, ix->amap[i].stop);
			dArena(hout, ix->arenas[i]);
		}
	}
	hflush(hout);
	return 1;
}

static int
xindex(HConnect *c)
{
	Hio *hout;
	int r;

	r = preq(c);
	if(r <= 0)
		return r;

	hout = &c->hout;
	if(c->req.vermaj){
		hokheaders(c);
		hprint(hout, "Content-type: text/xml\r\n");
		if(http11(c))
			hprint(hout, "Transfer-Encoding: chunked\r\n");
		hprint(hout, "\r\n");
	}

	if(http11(c))
		hxferenc(hout, 1);
	else
		c->head.closeit = 1;
	xmlIndex(hout, mainIndex, "index", 0);
	hflush(hout);
	return 1;
}

void
xmlIndent(Hio *hout, int indent)
{
	int i;

	for(i = 0; i < indent; i++)
		hputc(hout, '\t');
}

void
xmlAName(Hio *hout, char *v, char *tag)
{
	hprint(hout, " %s=\"%s\"", tag, v);
}

void
xmlScore(Hio *hout, u8int *v, char *tag)
{
	if(scoreEq(zeroScore, v))
		return;
	hprint(hout, " %s=\"%V\"", tag, v);
}

void
xmlSealed(Hio *hout, int v, char *tag)
{
	if(!v)
		return;
	hprint(hout, " %s=\"yes\"", tag);
}

void
xmlU32int(Hio *hout, u32int v, char *tag)
{
	hprint(hout, " %s=\"%ud\"", tag, v);
}

void
xmlU64int(Hio *hout, u64int v, char *tag)
{
	hprint(hout, " %s=\"%llud\"", tag, v);
}
