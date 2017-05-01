/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Rebuild the index from scratch, in place.
 */
#include "stdinc.h"
#include "dat.h"
#include "fns.h"

enum
{
	MinBufSize = 64*1024,
	MaxBufSize = 4*1024*1024,
};

int		dumb;
int		errors;
char		**isect;
int		nisect;
int		bloom;
int		zero;

uint32_t	isectmem;
uint64_t	totalbuckets;
uint64_t	totalclumps;
Channel	*arenadonechan;
Channel	*isectdonechan;
Index	*ix;

uint64_t	arenaentries;
uint64_t	skipentries;
uint64_t	indexentries;

static int shouldprocess(ISect*);
static void	isectproc(void*);
static void	arenapartproc(void*);

void
usage(void)
{
	fprint(2, "usage: buildindex [-b] [-i isect]... [-M imem] venti.conf\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	int fd, i, napart, nfinish, maxdisks;
	uint32_t bcmem, imem;
	Config conf;
	Part *p;

	maxdisks = 100000;
	ventifmtinstall();
	imem = 256*1024*1024;
	ARGBEGIN{
	case 'b':
		bloom = 1;
		break;
	case 'd':	/* debugging - make sure to run all 3 passes */
		dumb = 1;
		break;
	case 'i':
		isect = vtrealloc(isect, (nisect+1)*sizeof(isect[0]));
		isect[nisect++] = EARGF(usage());
		break;
	case 'M':
		imem = unittoull(EARGF(usage()));
		break;
	case 'm':	/* temporary - might go away */
		maxdisks = atoi(EARGF(usage()));
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();

	if(initventi(argv[0], &conf) < 0)
		sysfatal("can't init venti: %r");
	ix = mainindex;
	if(nisect == 0 && ix->bloom)
		bloom = 1;
	if(bloom && ix->bloom && resetbloom(ix->bloom) < 0)
		sysfatal("loadbloom: %r");
	if(bloom && !ix->bloom)
		sysfatal("-b specified but no bloom filter");
	if(!bloom)
		ix->bloom = nil;
	isectmem = imem/ix->nsects;

	/*
	 * safety first - only need read access to arenas
	 */
	p = nil;
	for(i=0; i<ix->narenas; i++){
		if(ix->arenas[i]->part != p){
			p = ix->arenas[i]->part;
			if((fd = open(p->filename, OREAD)) < 0)
				sysfatal("cannot reopen %s: %r", p->filename);
			dup(fd, p->fd);
			close(fd);
		}
	}

	/*
	 * need a block for every arena
	 */
	bcmem = maxblocksize * (mainindex->narenas + 16);
	if(0) fprint(2, "initialize %d bytes of disk block cache\n", bcmem);
	initdcache(bcmem);

	totalclumps = 0;
	for(i=0; i<ix->narenas; i++)
		totalclumps += ix->arenas[i]->diskstats.clumps;

	totalbuckets = 0;
	for(i=0; i<ix->nsects; i++)
		totalbuckets += ix->sects[i]->blocks;
	fprint(2, "%,lld clumps, %,lld buckets\n", totalclumps, totalbuckets);

	/* start index procs */
	fprint(2, "%T read index\n");
	isectdonechan = chancreate(sizeof(void*), 0);
	for(i=0; i<ix->nsects; i++){
		if(shouldprocess(ix->sects[i])){
			ix->sects[i]->writechan = chancreate(sizeof(IEntry), 0);
			vtproc(isectproc, ix->sects[i]);
		}
	}

	for(i=0; i<nisect; i++)
		if(isect[i])
			fprint(2, "warning: did not find index section %s\n", isect[i]);

	/* start arena procs */
	p = nil;
	napart = 0;
	nfinish = 0;
	arenadonechan = chancreate(sizeof(void*), 0);
	for(i=0; i<ix->narenas; i++){
		if(ix->arenas[i]->part != p){
			p = ix->arenas[i]->part;
			vtproc(arenapartproc, p);
			if(++napart >= maxdisks){
				recvp(arenadonechan);
				nfinish++;
			}
		}
	}

	/* wait for arena procs to finish */
	for(nfinish=0; nfinish<napart; nfinish++)
		recvp(arenadonechan);

	/* tell index procs to finish */
	for(i=0; i<ix->nsects; i++)
		if(ix->sects[i]->writechan)
			send(ix->sects[i]->writechan, nil);

	/* wait for index procs to finish */
	for(i=0; i<ix->nsects; i++)
		if(ix->sects[i]->writechan)
			recvp(isectdonechan);

	if(ix->bloom && writebloom(ix->bloom) < 0)
		fprint(2, "writing bloom filter: %r\n");

	fprint(2, "%T done arenaentries=%,lld indexed=%,lld (nskip=%,lld)\n",
		arenaentries, indexentries, skipentries);
	threadexitsall(nil);
}

static int
shouldprocess(ISect *is)
{
	int i;

	if(nisect == 0)
		return 1;

	for(i=0; i<nisect; i++)
		if(isect[i] && strcmp(isect[i], is->name) == 0){
			isect[i] = nil;
			return 1;
		}
	return 0;
}

static void
add(uint64_t *a, uint64_t n)
{
	static Lock l;

	lock(&l);
	*a += n;
	unlock(&l);
}

/*
 * Read through an arena partition and send each of its IEntries
 * to the appropriate index section.  When finished, send on
 * arenadonechan.
 */
enum
{
	ClumpChunks = 32*1024,
};
static void
arenapartproc(void *v)
{
	int i, j, n, nskip, x;
	uint32_t clump;
	uint64_t addr, tot;
	Arena *a;
	ClumpInfo *ci, *cis;
	IEntry ie;
	Part *p;

	p = v;
	threadsetname("arenaproc %s", p->name);

	nskip = 0;
	tot = 0;
	cis = MKN(ClumpInfo, ClumpChunks);
	for(i=0; i<ix->narenas; i++){
		a = ix->arenas[i];
		if(a->part != p)
			continue;
		if(a->memstats.clumps)
			fprint(2, "%T arena %s: %d entries\n",
				a->name, a->memstats.clumps);
		/*
		 * Running the loop backwards accesses the
		 * clump info blocks forwards, since they are
		 * stored in reverse order at the end of the arena.
		 * This speeds things slightly.
		 */
		addr = ix->amap[i].start + a->memstats.used;
		for(clump=a->memstats.clumps; clump > 0; clump-=n){
			n = ClumpChunks;
			if(n > clump)
				n = clump;
			if(readclumpinfos(a, clump-n, cis, n) != n){
				fprint(2, "%T arena %s: directory read: %r\n", a->name);
				errors = 1;
				break;
			}
			for(j=n-1; j>=0; j--){
				ci = &cis[j];
				ie.ia.type = ci->type;
				ie.ia.size = ci->uncsize;
				addr -= ci->size + ClumpSize;
				ie.ia.addr = addr;
				ie.ia.blocks = (ci->size + ClumpSize + (1<<ABlockLog)-1) >> ABlockLog;
				scorecp(ie.score, ci->score);
				if(ci->type == VtCorruptType)
					nskip++;
				else{
					tot++;
					x = indexsect(ix, ie.score);
					assert(0 <= x && x < ix->nsects);
					if(ix->sects[x]->writechan)
						send(ix->sects[x]->writechan, &ie);
					if(ix->bloom)
						markbloomfilter(ix->bloom, ie.score);
				}
			}
		}
		if(addr != ix->amap[i].start)
			fprint(2, "%T arena %s: clump miscalculation %lld != %lld\n", a->name, addr, ix->amap[i].start);
	}
	add(&arenaentries, tot);
	add(&skipentries, nskip);
	sendp(arenadonechan, p);
}

/*
 * Convert score into relative bucket number in isect.
 * Can pass a packed ientry instead of score - score is first.
 */
static uint32_t
score2bucket(ISect *is, uint8_t *score)
{
	uint32_t b;

	b = hashbits(score, 32)/ix->div;
	if(b < is->start || b >= is->stop){
		fprint(2, "score2bucket: score=%V div=%d b=%u start=%u stop=%u\n",
			score, ix->div, b, is->start, is->stop);
	}
	assert(is->start <= b && b < is->stop);
	return b - is->start;
}

/*
 * Convert offset in index section to bucket number.
 */
static uint32_t
offset2bucket(ISect *is, uint64_t offset)
{
	uint32_t b;

	assert(is->blockbase <= offset);
	offset -= is->blockbase;
	b = offset/is->blocksize;
	assert(b < is->stop-is->start);
	return b;
}

/*
 * Convert bucket number to offset.
 */
static uint64_t
bucket2offset(ISect *is, uint32_t b)
{
	assert(b <= is->stop-is->start);
	return is->blockbase + (uint64_t)b*is->blocksize;
}

/*
 * IEntry buffers to hold initial round of spraying.
 */
typedef struct Buf Buf;
struct Buf
{
	Part *part;			/* partition being written */
	uint8_t *bp;		/* current block */
	uint8_t *ep;		/* end of block */
	uint8_t *wp;		/* write position in block */
	uint64_t boffset;		/* start offset */
	uint64_t woffset;		/* next write offset */
	uint64_t eoffset;		/* end offset */
	uint32_t nentry;		/* number of entries written */
};

static void
bflush(Buf *buf)
{
	uint32_t bufsize;

	if(buf->woffset >= buf->eoffset)
		sysfatal("buf index chunk overflow - need bigger index");
	bufsize = buf->ep - buf->bp;
	if(writepart(buf->part, buf->woffset, buf->bp, bufsize) < 0){
		fprint(2, "write %s: %r\n", buf->part->name);
		errors = 1;
	}
	buf->woffset += bufsize;
	memset(buf->bp, 0, bufsize);
	buf->wp = buf->bp;
}

static void
bwrite(Buf *buf, IEntry *ie)
{
	if(buf->wp+IEntrySize > buf->ep)
		bflush(buf);
	assert(buf->bp <= buf->wp && buf->wp < buf->ep);
	packientry(ie, buf->wp);
	buf->wp += IEntrySize;
	assert(buf->bp <= buf->wp && buf->wp <= buf->ep);
	buf->nentry++;
}

/*
 * Minibuffer.  In-memory data structure holds our place
 * in the buffer but has no block data.  We are writing and
 * reading the minibuffers at the same time.  (Careful!)
 */
typedef struct Minibuf Minibuf;
struct Minibuf
{
	uint64_t boffset;		/* start offset */
	uint64_t roffset;		/* read offset */
	uint64_t woffset;		/* write offset */
	uint64_t eoffset;		/* end offset */
	uint32_t nentry;		/* # entries left to read */
	uint32_t nwentry;	/* # entries written */
};

/*
 * Index entry pool.  Used when trying to shuffle around
 * the entries in a big buffer into the corresponding M minibuffers.
 * Sized to hold M*EntriesPerBlock entries, so that there will always
 * either be room in the pool for another block worth of entries
 * or there will be an entire block worth of sorted entries to
 * write out.
 */
typedef struct IEntryLink IEntryLink;
typedef struct IPool IPool;

struct IEntryLink
{
	uint8_t ie[IEntrySize];		/* raw IEntry */
	IEntryLink *next;		/* next in chain */
};

struct IPool
{
	ISect *isect;
	uint32_t buck0;			/* first bucket in pool */
	uint32_t mbufbuckets;	/* buckets per minibuf */
	IEntryLink *entry;		/* all IEntryLinks */
	uint32_t nentry;			/* # of IEntryLinks */
	IEntryLink *free;		/* free list */
	uint32_t nfree;			/* # on free list */
	Minibuf *mbuf;			/* all minibufs */
	uint32_t nmbuf;			/* # of minibufs */
	IEntryLink **mlist;		/* lists for each minibuf */
	uint32_t *mcount;		/* # on each mlist[i] */
	uint32_t bufsize;			/* block buffer size */
	uint8_t *rbuf;			/* read buffer */
	uint8_t *wbuf;			/* write buffer */
	uint32_t epbuf;			/* entries per block buffer */
};

/*
static int
countsokay(IPool *p)
{
	int i;
	uint64_t n;

	n = 0;
	for(i=0; i<p->nmbuf; i++)
		n += p->mcount[i];
	n += p->nfree;
	if(n != p->nentry){
		print("free %u:", p->nfree);
		for(i=0; i<p->nmbuf; i++)
			print(" %u", p->mcount[i]);
		print(" = %lld nentry: %u\n", n, p->nentry);
	}
	return n == p->nentry;
}
*/

static IPool*
mkipool(ISect *isect, Minibuf *mbuf, uint32_t nmbuf,
	uint32_t mbufbuckets, uint32_t bufsize)
{
	uint32_t i, nentry;
	uint8_t *data;
	IPool *p;
	IEntryLink *l;

	nentry = (nmbuf+1)*bufsize / IEntrySize;
	p = ezmalloc(sizeof(IPool)
		+nentry*sizeof(IEntry)
		+nmbuf*sizeof(IEntryLink*)
		+nmbuf*sizeof(uint32_t)
		+3*bufsize);

	p->isect = isect;
	p->mbufbuckets = mbufbuckets;
	p->bufsize = bufsize;
	p->entry = (IEntryLink*)(p+1);
	p->nentry = nentry;
	p->mlist = (IEntryLink**)(p->entry+nentry);
	p->mcount = (uint32_t*)(p->mlist+nmbuf);
	p->nmbuf = nmbuf;
	p->mbuf = mbuf;
	data = (uint8_t*)(p->mcount+nmbuf);
	data += bufsize - (uintptr)data%bufsize;
	p->rbuf = data;
	p->wbuf = data+bufsize;
	p->epbuf = bufsize/IEntrySize;

	for(i=0; i<p->nentry; i++){
		l = &p->entry[i];
		l->next = p->free;
		p->free = l;
		p->nfree++;
	}
	return p;
}

/*
 * Add the index entry ie to the pool p.
 * Caller must know there is room.
 */
static void
ipoolinsert(IPool *p, uint8_t *ie)
{
	uint32_t buck, x;
	IEntryLink *l;

	assert(p->free != nil);

	buck = score2bucket(p->isect, ie);
	x = (buck-p->buck0) / p->mbufbuckets;
	if(x >= p->nmbuf){
		fprint(2, "buck=%u mbufbucket=%u x=%u\n",
			buck, p->mbufbuckets, x);
	}
	assert(x < p->nmbuf);

	l = p->free;
	p->free = l->next;
	p->nfree--;
	memmove(l->ie, ie, IEntrySize);
	l->next = p->mlist[x];
	p->mlist[x] = l;
	p->mcount[x]++;
}

/*
 * Pull out a block containing as many
 * entries as possible for minibuffer x.
 */
static uint32_t
ipoolgetbuf(IPool *p, uint32_t x)
{
	uint8_t *bp, *ep, *wp;
	IEntryLink *l;
	uint32_t n;

	bp = p->wbuf;
	ep = p->wbuf + p->bufsize;
	n = 0;
	assert(x < p->nmbuf);
	for(wp=bp; wp+IEntrySize<=ep && p->mlist[x]; wp+=IEntrySize){
		l = p->mlist[x];
		p->mlist[x] = l->next;
		p->mcount[x]--;
		memmove(wp, l->ie, IEntrySize);
		l->next = p->free;
		p->free = l;
		p->nfree++;
		n++;
	}
	memset(wp, 0, ep-wp);
	return n;
}

/*
 * Read a block worth of entries from the minibuf
 * into the pool.  Caller must know there is room.
 */
static void
ipoolloadblock(IPool *p, Minibuf *mb)
{
	uint32_t i, n;

	assert(mb->nentry > 0);
	assert(mb->roffset >= mb->woffset);
	assert(mb->roffset < mb->eoffset);

	n = p->bufsize/IEntrySize;
	if(n > mb->nentry)
		n = mb->nentry;
	if(readpart(p->isect->part, mb->roffset, p->rbuf, p->bufsize) < 0)
		fprint(2, "readpart %s: %r\n", p->isect->part->name);
	else{
		for(i=0; i<n; i++)
			ipoolinsert(p, p->rbuf+i*IEntrySize);
	}
	mb->nentry -= n;
	mb->roffset += p->bufsize;
}

/*
 * Write out a block worth of entries to minibuffer x.
 * If necessary, pick up the data there before overwriting it.
 */
static void
ipoolflush0(IPool *pool, uint32_t x)
{
	uint32_t bufsize;
	Minibuf *mb;

	mb = pool->mbuf+x;
	bufsize = pool->bufsize;
	mb->nwentry += ipoolgetbuf(pool, x);
	if(mb->nentry > 0 && mb->roffset == mb->woffset){
		assert(pool->nfree >= pool->bufsize/IEntrySize);
		/*
		 * There will be room in the pool -- we just
		 * removed a block worth.
		 */
		ipoolloadblock(pool, mb);
	}
	if(writepart(pool->isect->part, mb->woffset, pool->wbuf, bufsize) < 0)
		fprint(2, "writepart %s: %r\n", pool->isect->part->name);
	mb->woffset += bufsize;
}

/*
 * Write out some full block of entries.
 * (There must be one -- the pool is almost full!)
 */
static void
ipoolflush1(IPool *pool)
{
	uint32_t i;

	assert(pool->nfree <= pool->epbuf);

	for(i=0; i<pool->nmbuf; i++){
		if(pool->mcount[i] >= pool->epbuf){
			ipoolflush0(pool, i);
			return;
		}
	}
	/* can't be reached - someone must be full */
	sysfatal("ipoolflush1");
}

/*
 * Flush all the entries in the pool out to disk.
 * Nothing more to read from disk.
 */
static void
ipoolflush(IPool *pool)
{
	uint32_t i;

	for(i=0; i<pool->nmbuf; i++)
		while(pool->mlist[i])
			ipoolflush0(pool, i);
	assert(pool->nfree == pool->nentry);
}

/*
 * Third pass.  Pick up each minibuffer from disk into
 * memory and then write out the buckets.
 */

/*
 * Compare two packed index entries.
 * Usual ordering except break ties by putting higher
 * index addresses first (assumes have duplicates
 * due to corruption in the lower addresses).
 */
static int
ientrycmpaddr(const void *va, const void *vb)
{
	int i;
	uint8_t *a, *b;

	a = (uint8_t*)va;
	b = (uint8_t*)vb;
	i = ientrycmp(a, b);
	if(i)
		return i;
	return -memcmp(a+IEntryAddrOff, b+IEntryAddrOff, 8);
}

static void
zerorange(Part *p, uint64_t o, uint64_t e)
{
	static uint8_t zero[MaxIoSize];
	uint32_t n;

	for(; o<e; o+=n){
		n = sizeof zero;
		if(o+n > e)
			n = e-o;
		if(writepart(p, o, zero, n) < 0)
			fprint(2, "writepart %s: %r\n", p->name);
	}
}

/*
 * Load a minibuffer into memory and write out the
 * corresponding buckets.
 */
static void
sortminibuffer(ISect *is, Minibuf *mb, uint8_t *buf, uint32_t nbuf,
	       uint32_t bufsize)
{
	uint8_t *buckdata, *p, *q, *ep;
	uint32_t b, lastb, memsize, n;
	uint64_t o;
	IBucket ib;
	Part *part;

	part = is->part;
	buckdata = emalloc(is->blocksize);

	if(mb->nwentry == 0)
		return;

	/*
	 * read entire buffer.
	 */
	assert(mb->nwentry*IEntrySize <= mb->woffset-mb->boffset);
	assert(mb->woffset-mb->boffset <= nbuf);
	if(readpart(part, mb->boffset, buf, mb->woffset-mb->boffset) < 0){
		fprint(2, "readpart %s: %r\n", part->name);
		errors = 1;
		return;
	}
	assert(*(uint*)buf != 0xa5a5a5a5);

	/*
	 * remove fragmentation due to IEntrySize
	 * not evenly dividing Bufsize
	 */
	memsize = (bufsize/IEntrySize)*IEntrySize;
	for(o=mb->boffset, p=q=buf; o<mb->woffset; o+=bufsize){
		memmove(p, q, memsize);
		p += memsize;
		q += bufsize;
	}
	ep = buf + mb->nwentry*IEntrySize;
	assert(ep <= buf+nbuf);

	/*
	 * sort entries
	 */
	qsort(buf, mb->nwentry, IEntrySize, ientrycmpaddr);

	/*
	 * write buckets out
	 */
	n = 0;
	lastb = offset2bucket(is, mb->boffset);
	for(p=buf; p<ep; p=q){
		b = score2bucket(is, p);
		for(q=p; q<ep && score2bucket(is, q)==b; q+=IEntrySize)
			;
		if(lastb+1 < b && zero)
			zerorange(part, bucket2offset(is, lastb+1), bucket2offset(is, b));
		if(IBucketSize+(q-p) > is->blocksize)
			sysfatal("bucket overflow - make index bigger");
		memmove(buckdata+IBucketSize, p, q-p);
		ib.n = (q-p)/IEntrySize;
		n += ib.n;
		packibucket(&ib, buckdata, is->bucketmagic);
		if(writepart(part, bucket2offset(is, b), buckdata, is->blocksize) < 0)
			fprint(2, "write %s: %r\n", part->name);
		lastb = b;
	}
	if(lastb+1 < is->stop-is->start && zero)
		zerorange(part, bucket2offset(is, lastb+1), bucket2offset(is, is->stop - is->start));

	if(n != mb->nwentry)
		fprint(2, "sortminibuffer bug: n=%u nwentry=%u have=%ld\n", n, mb->nwentry, (ep-buf)/IEntrySize);

	free(buckdata);
}

static void
isectproc(void *v)
{
	uint32_t buck, bufbuckets, bufsize, epbuf, i, j;
	uint32_t mbufbuckets, n, nbucket, nn, space;
	uint32_t nbuf, nminibuf, xminiclump, prod;
	uint64_t blocksize, offset, xclump;
	uint8_t *data, *p;
	Buf *buf;
	IEntry ie;
	IPool *ipool;
	ISect *is;
	Minibuf *mbuf, *mb;

	is = v;
	blocksize = is->blocksize;
	nbucket = is->stop - is->start;

	/*
	 * Three passes:
	 *	pass 1 - write index entries from arenas into
	 *		large sequential sections on index disk.
	 *		requires nbuf * bufsize memory.
	 *
	 *	pass 2 - split each section into minibufs.
	 *		requires nminibuf * bufsize memory.
	 *
	 *	pass 3 - read each minibuf into memory and
	 *		write buckets out.
	 *		requires entries/minibuf * IEntrySize memory.
	 *
	 * The larger we set bufsize the less seeking hurts us.
	 *
	 * The fewer sections and minibufs we have, the less
	 * seeking hurts us.
	 *
	 * The fewer sections and minibufs we have, the
	 * more entries we end up with in each minibuf
	 * at the end.
	 *
	 * Shoot for using half our memory to hold each
	 * minibuf.  The chance of a random distribution
	 * getting off by 2x is quite low.
	 *
	 * Once that is decided, figure out the smallest
	 * nminibuf and nsection/biggest bufsize we can use
	 * and still fit in the memory constraints.
	 */

	/* expected number of clump index entries we'll see */
	xclump = nbucket * (double)totalclumps/totalbuckets;

	/* number of clumps we want to see in a minibuf */
	xminiclump = isectmem/2/IEntrySize;

	/* total number of minibufs we need */
	prod = (xclump+xminiclump-1) / xminiclump;

	/* if possible, skip second pass */
	if(!dumb && prod*MinBufSize < isectmem){
		nbuf = prod;
		nminibuf = 1;
	}else{
		/* otherwise use nsection = sqrt(nmini) */
		for(nbuf=1; nbuf*nbuf<prod; nbuf++)
			;
		if(nbuf*MinBufSize > isectmem)
			sysfatal("not enough memory");
		nminibuf = nbuf;
	}
	if (nbuf == 0) {
		fprint(2, "%s: brand-new index, no work to do\n", argv0);
		threadexitsall(0);
	}

	/* size buffer to use extra memory */
	bufsize = MinBufSize;
	while(bufsize*2*nbuf <= isectmem && bufsize < MaxBufSize)
		bufsize *= 2;
	data = emalloc(nbuf*bufsize);
	epbuf = bufsize/IEntrySize;
	fprint(2, "%T %s: %,ud buckets, %,ud groups, %,ud minigroups, %,ud buffer\n",
		is->part->name, nbucket, nbuf, nminibuf, bufsize);
	/*
	 * Accept index entries from arena procs.
	 */
	buf = MKNZ(Buf, nbuf);
	p = data;
	offset = is->blockbase;
	bufbuckets = (nbucket+nbuf-1)/nbuf;
	for(i=0; i<nbuf; i++){
		buf[i].part = is->part;
		buf[i].bp = p;
		buf[i].wp = p;
		p += bufsize;
		buf[i].ep = p;
		buf[i].boffset = offset;
		buf[i].woffset = offset;
		if(i < nbuf-1){
			offset += bufbuckets*blocksize;
			buf[i].eoffset = offset;
		}else{
			offset = is->blockbase + nbucket*blocksize;
			buf[i].eoffset = offset;
		}
	}
	assert(p == data+nbuf*bufsize);

	n = 0;
	while(recv(is->writechan, &ie) == 1){
		if(ie.ia.addr == 0)
			break;
		buck = score2bucket(is, ie.score);
		i = buck/bufbuckets;
		assert(i < nbuf);
		bwrite(&buf[i], &ie);
		n++;
	}
	add(&indexentries, n);

	nn = 0;
	for(i=0; i<nbuf; i++){
		bflush(&buf[i]);
		buf[i].bp = nil;
		buf[i].ep = nil;
		buf[i].wp = nil;
		nn += buf[i].nentry;
	}
	if(n != nn)
		fprint(2, "isectproc bug: n=%u nn=%u\n", n, nn);

	free(data);

	fprint(2, "%T %s: reordering\n", is->part->name);

	/*
	 * Rearrange entries into minibuffers and then
	 * split each minibuffer into buckets.
	 * The minibuffer must be sized so that it is
	 * a multiple of blocksize -- ipoolloadblock assumes
	 * that each minibuf starts aligned on a blocksize
	 * boundary.
	 */
	mbuf = MKN(Minibuf, nminibuf);
	mbufbuckets = (bufbuckets+nminibuf-1)/nminibuf;
	while(mbufbuckets*blocksize % bufsize)
		mbufbuckets++;
	for(i=0; i<nbuf; i++){
		/*
		 * Set up descriptors.
		 */
		n = buf[i].nentry;
		nn = 0;
		offset = buf[i].boffset;
		memset(mbuf, 0, nminibuf*sizeof(mbuf[0]));
		for(j=0; j<nminibuf; j++){
			mb = &mbuf[j];
			mb->boffset = offset;
			offset += mbufbuckets*blocksize;
			if(offset > buf[i].eoffset)
				offset = buf[i].eoffset;
			mb->eoffset = offset;
			mb->roffset = mb->boffset;
			mb->woffset = mb->boffset;
			mb->nentry = epbuf * (mb->eoffset - mb->boffset)/bufsize;
			if(mb->nentry > buf[i].nentry)
				mb->nentry = buf[i].nentry;
			buf[i].nentry -= mb->nentry;
			nn += mb->nentry;
		}
		if(n != nn)
			fprint(2, "isectproc bug2: n=%u nn=%u (i=%d)\n", n, nn, i);;
		/*
		 * Rearrange.
		 */
		if(!dumb && nminibuf == 1){
			mbuf[0].nwentry = mbuf[0].nentry;
			mbuf[0].woffset = buf[i].woffset;
		}else{
			ipool = mkipool(is, mbuf, nminibuf, mbufbuckets, bufsize);
			ipool->buck0 = bufbuckets*i;
			for(j=0; j<nminibuf; j++){
				mb = &mbuf[j];
				while(mb->nentry > 0){
					if(ipool->nfree < epbuf){
						ipoolflush1(ipool);
						/* ipoolflush1 might change mb->nentry */
						continue;
					}
					assert(ipool->nfree >= epbuf);
					ipoolloadblock(ipool, mb);
				}
			}
			ipoolflush(ipool);
			nn = 0;
			for(j=0; j<nminibuf; j++)
				nn += mbuf[j].nwentry;
			if(n != nn)
				fprint(2, "isectproc bug3: n=%u nn=%u (i=%d)\n", n, nn, i);
			free(ipool);
		}

		/*
		 * Make buckets.
		 */
		space = 0;
		for(j=0; j<nminibuf; j++)
			if(space < mbuf[j].woffset - mbuf[j].boffset)
				space = mbuf[j].woffset - mbuf[j].boffset;

		data = emalloc(space);
		for(j=0; j<nminibuf; j++){
			mb = &mbuf[j];
			sortminibuffer(is, mb, data, space, bufsize);
		}
		free(data);
	}

	sendp(isectdonechan, is);
}



