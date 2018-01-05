/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Mirror one arena partition onto another.  
 * Be careful to copy only new data.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

Channel *writechan;

typedef struct Write Write;
struct Write
{
	uint8_t *p;
	int n;
	uint64_t o;
	int error;
};

Part *src;
Part *dst;
int force;
int verbose;
int dosha1 = 1;
char *status;
uint64_t astart, aend;

void
usage(void)
{
	fprint(2, "usage: mirrorarenas [-sv] src dst [ranges]\n");
	threadexitsall("usage");
}

char *tagged;

void
tag(char *fmt, ...)
{
	va_list arg;
	
	if(tagged){
		free(tagged);
		tagged = nil;
	}
	va_start(arg, fmt);
	tagged = vsmprint(fmt, arg);
	va_end(arg);
}

void
chat(char *fmt, ...)
{
	va_list arg;

	if(tagged){
		write(1, tagged, strlen(tagged));
		free(tagged);
		tagged = nil;
	}
	va_start(arg, fmt);
	vfprint(1, fmt, arg);
	va_end(arg);
}

#pragma varargck argpos tag 1
#pragma varargck argpos chat 1


int
ereadpart(Part *p, uint64_t offset, uint8_t *buf, uint32_t count)
{
	if(readpart(p, offset, buf, count) != count){
		chat("%T readpart %s at %#llx+%u: %r\n", p->name, offset, count);
		return -1;
	}
	return 0;
}
		
int
ewritepart(Part *p, uint64_t offset, uint8_t *buf, uint32_t count)
{
	if(writepart(p, offset, buf, count) != count || flushpart(p) < 0){
		chat("%T writepart %s at %#llx+%u: %r\n", p->name, offset, count);
		return -1;
	}
	return 0;
}

/*
 * Extra proc to do writes to dst, so that we can overlap reading
 * src with writing dst during copy.  This is an easy factor of two
 * (almost) in performance.
 */
static Write wsync;
static void
writeproc(void *v)
{
	Write *w;
	
	USED(v);
	while((w = recvp(writechan)) != nil){
		if(w == &wsync)
			continue;
		if(ewritepart(dst, w->o, w->p, w->n) < 0)
			w->error = 1;
	}
}

int
copy(uint64_t start, uint64_t end, char *what, DigestState *ds)
{
	int i, n;
	uint64_t o;
	static uint8_t tmp[2][1024*1024];
	Write w[2];
	
	assert(start <= end);
	assert(astart <= start && start < aend);
	assert(astart <= end && end <= aend);

	if(verbose && start != end)
		chat("%T   copy %,llud-%,llud %s\n", start, end, what);

	i = 0;
	memset(w, 0, sizeof w);
	for(o=start; o<end; o+=n){
		if(w[i].error)
			goto error;
		n = sizeof tmp[i];
		if(o+n > end)
			n = end - o;
		if(ereadpart(src, o, tmp[i], n) < 0)
			goto error;
		w[i].p = tmp[i];
		w[i].o = o;
		w[i].n = n;
		w[i].error = 0;
		sendp(writechan, &w[i]);
		if(ds)
			sha1(tmp[i], n, nil, ds);
		i = 1-i;
	}
	if(w[i].error)
		goto error;

	/*
	 * wait for queued write to finish
	 */
	sendp(writechan, &wsync);
	i = 1-i;
	if(w[i].error)
		return -1;
	return 0;

error:
	/*
	 * sync with write proc
	 */
	w[i].p = nil;
	w[i].o = 0;
	w[i].n = 0;
	w[i].error = 0;
	sendp(writechan, &w[i]);
	return -1;
}

/* single-threaded, for reference */
int
copy1(uint64_t start, uint64_t end, char *what, DigestState *ds)
{
	int n;
	uint64_t o;
	static uint8_t tmp[1024*1024];
	
	assert(start <= end);
	assert(astart <= start && start < aend);
	assert(astart <= end && end <= aend);

	if(verbose && start != end)
		chat("%T   copy %,llud-%,llud %s\n", start, end, what);

	for(o=start; o<end; o+=n){
		n = sizeof tmp;
		if(o+n > end)
			n = end - o;
		if(ereadpart(src, o, tmp, n) < 0)
			return -1;
		if(ds)
			sha1(tmp, n, nil, ds);
		if(ewritepart(dst, o, tmp, n) < 0)
			return -1;
	}
	return 0;
}

int
asha1(Part *p, uint64_t start, uint64_t end, DigestState *ds)
{
	int n;
	uint64_t o;
	static uint8_t tmp[1024*1024];

	if(start == end)
		return 0;
	assert(start < end);

	if(verbose)
		chat("%T   sha1 %,llud-%,llud\n", start, end);

	for(o=start; o<end; o+=n){
		n = sizeof tmp;
		if(o+n > end)
			n = end - o;
		if(ereadpart(p, o, tmp, n) < 0)
			return -1;
		sha1(tmp, n, nil, ds);
	}
	return 0;
}

uint64_t
rdown(uint64_t a, int b)
{
	return a-a%b;
}

uint64_t
rup(uint64_t a, int b)
{
	if(a%b == 0)
		return a;
	return a+b-a%b;
}

void
mirror(Arena *sa, Arena *da)
{
	int64_t v, si, di, end;
	int clumpmax, blocksize, sealed;
	static uint8_t buf[MaxIoSize];
	ArenaHead h;
	DigestState xds, *ds;
	int64_t shaoff, base;
	
	base = sa->base;
	blocksize = sa->blocksize;
	end = sa->base + sa->size;
	
	astart = base - blocksize;
	aend = end + blocksize;

	tag("%T %s (%,llud-%,llud)\n", sa->name, astart, aend);
	
	if(force){
		copy(astart, aend, "all", nil);
		return;
	}

	if(sa->diskstats.sealed && da->diskstats.sealed && scorecmp(da->score, zeroscore) != 0){
		if(scorecmp(sa->score, da->score) == 0){
			if(verbose)
				chat("%T %s: %V sealed mirrored\n", sa->name, sa->score);
			return;
		}
		chat("%T %s: warning: sealed score mismatch %V vs %V\n", sa->name, sa->score, da->score);
		/* Keep executing; will correct seal if possible. */
	}
	if(!sa->diskstats.sealed && da->diskstats.sealed && scorecmp(da->score, zeroscore) != 0){
		chat("%T %s: dst is sealed, src is not\n", sa->name);
		status = "errors";
		return;
	}
	if(sa->diskstats.used < da->diskstats.used){
		chat("%T %s: src used %,lld < dst used %,lld\n", sa->name, sa->diskstats.used, da->diskstats.used);
		status = "errors";
		return;
	}

	if(da->clumpmagic != sa->clumpmagic){
		/*
		 * Write this now to reduce the window in which
		 * the head and tail disagree about clumpmagic.
		 */
		da->clumpmagic = sa->clumpmagic;
		memset(buf, 0, sizeof buf);
		packarena(da, buf);
		if(ewritepart(dst, end, buf, blocksize) < 0)
			return;
	}
	
	memset(&h, 0, sizeof h);
	h.version = da->version;
	strcpy(h.name, da->name);
	h.blocksize = da->blocksize;
	h.size = da->size + 2*da->blocksize;
	h.clumpmagic = da->clumpmagic;
	memset(buf, 0, sizeof buf);
	packarenahead(&h, buf);
	if(ewritepart(dst, base - blocksize, buf, blocksize) < 0)
		return;

	shaoff = 0;
	ds = nil;
	sealed = sa->diskstats.sealed && scorecmp(sa->score, zeroscore) != 0;
	if(sealed && dosha1){
		/* start sha1 state with header */
		memset(&xds, 0, sizeof xds);
		ds = &xds;
		sha1(buf, blocksize, nil, ds);
		shaoff = base;
	}
	
	if(sa->diskstats.used != da->diskstats.used){
		di = base+rdown(da->diskstats.used, blocksize);
		si = base+rup(sa->diskstats.used, blocksize);
		if(ds && asha1(dst, shaoff, di, ds) < 0)
			return;
		if(copy(di, si, "data", ds) < 0)
			return;
		shaoff = si;
	}
	
	clumpmax = sa->clumpmax;
	di = end - da->diskstats.clumps/clumpmax * blocksize;
	si = end - (sa->diskstats.clumps+clumpmax-1)/clumpmax * blocksize;

	if(sa->diskstats.sealed){
		/*
		 * might be a small hole between the end of the 
		 * data and the beginning of the directory.
		 */
		v = base+rup(sa->diskstats.used, blocksize);
		if(ds && asha1(dst, shaoff, v, ds) < 0)
			return;
		if(copy(v, si, "hole", ds) < 0)
			return;
		shaoff = si;
	}

	if(da->diskstats.clumps != sa->diskstats.clumps){
		if(ds && asha1(dst, shaoff, si, ds) < 0)
			return;
		if(copy(si, di, "directory", ds) < 0)	/* si < di  because clumpinfo blocks grow down */
			return;
		shaoff = di;
	}

	da->ctime = sa->ctime;
	da->wtime = sa->wtime;
	da->diskstats = sa->diskstats;
	da->diskstats.sealed = 0;
	
	/*
	 * Repack the arena tail information
	 * and save it for next time...
	 */
	memset(buf, 0, sizeof buf);
	packarena(da, buf);
	if(ewritepart(dst, end, buf, blocksize) < 0)
		return;

	if(sealed){
		/*
		 * ... but on the final pass, copy the encoding
		 * of the tail information from the source
		 * arena itself.  There are multiple possible
		 * ways to write the tail info out (the exact
		 * details have changed as venti went through
		 * revisions), and to keep the SHA1 hash the
		 * same, we have to use what the disk uses.
		 */
		if(asha1(dst, shaoff, end, ds) < 0
		|| copy(end, end+blocksize-VtScoreSize, "tail", ds) < 0)
			return;
		if(dosha1){
			memset(buf, 0, VtScoreSize);
			sha1(buf, VtScoreSize, da->score, ds);
			if(scorecmp(sa->score, da->score) == 0){
				if(verbose)
					chat("%T %s: %V sealed mirrored\n", sa->name, sa->score);
				if(ewritepart(dst, end+blocksize-VtScoreSize, da->score, VtScoreSize) < 0)
					return;
			}else{
				chat("%T %s: sealing dst: score mismatch: %V vs %V\n", sa->name, sa->score, da->score);
				memset(&xds, 0, sizeof xds);
				asha1(dst, base-blocksize, end+blocksize-VtScoreSize, &xds);
				sha1(buf, VtScoreSize, 0, &xds);
				chat("%T   reseal: %V\n", da->score);
				status = "errors";
			}
		}else{
			if(verbose)
				chat("%T %s: %V mirrored\n", sa->name, sa->score);
			if(ewritepart(dst, end+blocksize-VtScoreSize, sa->score, VtScoreSize) < 0)
				return;
		}
	}else{
		chat("%T %s: %,lld used mirrored\n",
			sa->name, sa->diskstats.used);
	}
}

void
mirrormany(ArenaPart *sp, ArenaPart *dp, char *range)
{
	int i, lo, hi;
	char *s, *t;
	Arena *sa, *da;

	if(range == nil){
		for(i=0; i<sp->narenas; i++){
			sa = sp->arenas[i];
			da = dp->arenas[i];
			mirror(sa, da);
		}
		return;
	}
	if(strcmp(range, "none") == 0)
		return;

	for(s=range; *s; s=t){
		t = strchr(s, ',');
		if(t)
			*t++ = 0;
		else
			t = s+strlen(s);
		if(*s == '-')
			lo = 0;
		else
			lo = strtol(s, &s, 0);
		hi = lo;
		if(*s == '-'){
			s++;
			if(*s == 0)
				hi = sp->narenas-1;
			else
				hi = strtol(s, &s, 0);
		}
		if(*s != 0){
			chat("%T bad arena range: %s\n", s);
			continue;
		}
		for(i=lo; i<=hi; i++){
			sa = sp->arenas[i];
			da = dp->arenas[i];
			mirror(sa, da);
		}
	}	
}


void
threadmain(int argc, char **argv)
{
	int i;
	Arena *sa, *da;
	ArenaPart *s, *d;
	char *ranges;
	
	ventifmtinstall();

	ARGBEGIN{
	case 'F':
		force = 1;
		break;
	case 'v':
		verbose++;
		break;
	case 's':
		dosha1 = 0;
		break;
	default:
		usage();
	}ARGEND
	
	if(argc != 2 && argc != 3)
		usage();
	ranges = nil;
	if(argc == 3)
		ranges = argv[2];

	if((src = initpart(argv[0], OREAD)) == nil)
		sysfatal("initpart %s: %r", argv[0]);
	if((dst = initpart(argv[1], ORDWR)) == nil)
		sysfatal("initpart %s: %r", argv[1]);
	if((s = initarenapart(src)) == nil)
		sysfatal("initarenapart %s: %r", argv[0]);
	for(i=0; i<s->narenas; i++)
		delarena(s->arenas[i]);
	if((d = initarenapart(dst)) == nil)
		sysfatal("loadarenapart %s: %r", argv[1]);
	for(i=0; i<d->narenas; i++)
		delarena(d->arenas[i]);
	
	/*
	 * The arena geometries must match or all bets are off.
	 */
	if(s->narenas != d->narenas)
		sysfatal("arena count mismatch: %d vs %d", s->narenas, d->narenas);
	for(i=0; i<s->narenas; i++){
		sa = s->arenas[i];
		da = d->arenas[i];
		if(sa->version != da->version)
			sysfatal("arena %d: version mismatch: %d vs %d", i, sa->version, da->version);
		if(sa->blocksize != da->blocksize)
			sysfatal("arena %d: blocksize mismatch: %d vs %d", i, sa->blocksize, da->blocksize);
		if(sa->size != da->size)
			sysfatal("arena %d: size mismatch: %,lld vs %,lld", i, sa->size, da->size);
		if(strcmp(sa->name, da->name) != 0)
			sysfatal("arena %d: name mismatch: %s vs %s", i, sa->name, da->name);
	}
	
	/*
	 * Mirror one arena at a time.
	 */
	writechan = chancreate(sizeof(void*), 0);
	vtproc(writeproc, nil);
	mirrormany(s, d, ranges);
	sendp(writechan, nil);
	threadexitsall(status);
}
