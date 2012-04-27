#include "all.h"

/*
 * File blocks.
 * see dk.h
 */

void
rwlock(Memblk *f, int iswr)
{
	xrwlock(f->mf, iswr);
}

void
rwunlock(Memblk *f, int iswr)
{
	xrwunlock(f->mf, iswr);
}

void
isfile(Memblk *f)
{
	if(f->type != DBfile || f->mf == nil)
		fatal("isfile: not a file at pc %#p", getcallerpc(&f));
}

void
isrwlocked(Memblk *f, int iswr)
{
	if(f->type != DBfile || f->mf == nil)
		fatal("isrwlocked: not a file  at pc %#p", getcallerpc(&f));
	if((iswr && canrlock(f->mf)) || (!iswr && canwlock(f->mf)))
		fatal("is%clocked at pc %#p", iswr?'w':'r', getcallerpc(&f));
}

static void
isdir(Memblk *f)
{
	if(f->type != DBfile || f->mf == nil)
		fatal("isdir: not a file at pc %#p", getcallerpc(&f));
	if((f->d.mode&DMDIR) == 0)
		fatal("isdir: not a dir at pc %#p", getcallerpc(&f));
}

/* for dfblk only */
static Memblk*
getmelted(uint isdir, uint type, daddrt *addrp, int *chg)
{
	Memblk *b, *nb;

	*chg = 0;
	if(*addrp == 0){
		b = dballoc(type);
		*addrp = b->addr;
		*chg = 1;
		return b;
	}

	b = dbget(type, *addrp);
	nb = nil;
	if(!b->frozen)
		return b;
	if(catcherror()){
		mbput(b);
		mbput(nb);
		error(nil);
	}
	nb = dbdup(b);
	assert(type == b->type);
	if(isdir && type == DBdata)
		dupdentries(nb->d.data, Dblkdatasz/Daddrsz);
	USED(&nb);		/* for error() */
	*addrp = nb->addr;
	*chg = 1;
	dbput(b, b->type, b->addr);
	noerror();
	mbput(b);
	return nb;
}

/*
 * Get a file data block, perhaps allocating it on demand
 * if mkit. The file must be r/wlocked and melted if mkit.
 *
 * Adds disk refs for dir entries copied during melts and
 * considers that /archive is always melted.
 *
 * Read-ahead is not considered here. The file only records
 * the last accessed block number, to help the caller do RA.
 *
 */
static Memblk*
dfblk(Memblk *f, ulong bno, int mkit)
{
	ulong prev, nblks;
	int i, idx, nindir, type, isdir, chg;
	Memblk *b, *pb;
	daddrt *addrp;

	if(mkit)
		ismelted(f);
	isdir = (f->d.mode&DMDIR);

	if(bno != f->mf->lastbno){
		f->mf->sequential = (!mkit && bno == f->mf->lastbno + 1);
		f->mf->lastbno = bno;
	}

	/*
	 * bno: block # relative to the the block we are looking at.
	 * prev: # of blocks before the current one.
	 */
	prev = 0;
	chg = 0;

	/*
	 * Direct block?
	 */
	if(bno < nelem(f->d.dptr)){
		if(mkit)
			b = getmelted(isdir, DBdata, &f->d.dptr[bno], &chg);
		else
			b = dbget(DBdata, f->d.dptr[bno]);
		if(chg)
			changed(f);
		return b;
	}
	bno -= nelem(f->d.dptr);
	prev += nelem(f->d.dptr);

	/*
	 * Indirect block
	 * nblks: # of data blocks addressed by the block we look at.
	 */
	nblks = Dptrperblk;
	for(i = 0; i < nelem(f->d.iptr); i++){
		if(bno < nblks)
			break;
		bno -= nblks;
		prev += nblks;
		nblks *= Dptrperblk;
	}
	if(i == nelem(f->d.iptr))
		error("offset exceeds file capacity");
	ainc(&fs->nindirs[i]);
	type = DBptr0+i;
	dFprint("dfblk: indirect %s nblks %uld (ppb %ud) bno %uld\n",
		tname(type), nblks, Dptrperblk, bno);

	addrp = &f->d.iptr[i];
	if(mkit)
		b = getmelted(isdir, type, addrp, &chg);
	else
		b = dbget(type, *addrp);
	if(chg)
		changed(f);
	pb = b;
	if(catcherror()){
		mbput(pb);
		error(nil);
	}

	/* at the loop header:
	 * 	pb: parent of b
	 * 	b: DBptr block we are looking at.
	 * 	addrp: ptr to b within fb.
	 * 	nblks: # of data blocks addressed by b
	 */
	for(nindir = i+1; nindir >= 0; nindir--){
		chg = 0;
		dFprint("indir %s d%#ullx nblks %uld ptrperblk %d bno %uld\n\n",
			tname(DBdata+nindir), *addrp, nblks, Dptrperblk, bno);
		idx = 0;
		if(nindir > 0){
			nblks /= Dptrperblk;
			idx = bno/nblks;
		}
		if(*addrp == 0 && !mkit){
			/* hole */
			fprint(2, "HOLE\n");
			b = nil;
		}else{
			assert(type >= DBdata);
			if(mkit)
				b = getmelted(isdir, type, addrp, &chg);
			else
				b = dbget(type, *addrp);
			if(chg)
				changed(pb);
			addrp = &b->d.ptr[idx];
		}
		mbput(pb);
		pb = b;
		USED(&b);	/* force to memory in case of error */
		USED(&pb);	/* force to memory in case of error */
		bno -= idx * nblks;
		prev +=  idx * nblks;
		type--;
	}
	noerror();

	return b;
}

/*
 * Remove [bno:bend) file data blocks.
 * The file must be r/wlocked and melted.
 */
void
dfdropblks(Memblk *f, ulong bno, ulong bend)
{
	Memblk *b;

	isrwlocked(f, Wr);
	ismelted(f);

	dprint("dfdropblks: could remove d%#ullx[%uld:%uld]\n",
		f->addr, bno, bend);
	/*
	 * Instead of releasing the references on the data blocks,
	 * considering that the file might grow again, we keep them.
	 * Consider recompiling again and again and...
	 *
	 * The length has been adjusted and data won't be returned
	 * before overwritten.
	 *
	 * We only have to zero the data, because the file might
	 * grow using holes and the holes must read as zero, and also
	 * for safety.
	 */
	for(; bno < bend; bno++){
		if(catcherror())
			continue;
		b = dfblk(f, bno, Dontmk);
		noerror();
		memset(b->d.data, 0, Dblkdatasz);
		changed(b);
		mbput(b);
	}
}

/*
 * block # for the given offset (first block in file is 0).
 * embedded data accounts also as block #0.
 * If boffp is not nil it returns the offset within that block
 * for the given offset.
 */
ulong
dfbno(Memblk *f, uvlong off, ulong *boffp)
{
	ulong doff, dlen;

	doff = embedattrsz(f);
	dlen = Embedsz - doff;
	if(off < dlen){
		*boffp = doff + off;
		return 0;
	}
	off -= dlen;
	if(boffp != nil)
		*boffp = off%Dblkdatasz;
	return off/Dblkdatasz;
}

/*
 * Return a block slice for data in f.
 * The slice returned is resized to keep in a single block.
 * If there's a hole in the file, Blksl.data == nil && Blksl.len > 0.
 *
 * If mkit, the data block (and any pointer block crossed)
 * is allocated/melted if needed, but the file length is NOT updated.
 *
 * The file must be r/wlocked by the caller, and melted if mkit.
 * The block is returned referenced but unlocked,
 * (it's still protected by the file lock.)
 */
Blksl
dfslice(Memblk *f, ulong len, uvlong off, int iswr)
{
	Blksl sl;
	ulong boff, doff, dlen, bno;

	memset(&sl, 0, sizeof sl);

	if(iswr)
		ismelted(f);
	else
		if(off >= f->d.length)
			goto done;

	doff = embedattrsz(f);
	dlen = Embedsz - doff;

	if(off < dlen){
		sl.b = f;
		incref(f);
		sl.data = f->d.embed + doff + off;
		sl.len = dlen - off;
	}else{
		bno = (off-dlen) / Dblkdatasz;
		boff = (off-dlen) % Dblkdatasz;

		sl.b = dfblk(f, bno, iswr);
		if(iswr)
			ismelted(sl.b);
		if(sl.b != nil)
			sl.data = sl.b->d.data + boff;
		sl.len = Dblkdatasz - boff;
	}

	if(sl.len > len)
		sl.len = len;
	if(off + sl.len > f->d.length)
		if(!iswr)
			sl.len = f->d.length - off;
		/* else the file size will be updated by the caller */
done:
	if(sl.b == nil){
		dFprint("slice m%#p[%#ullx:+%#ulx]%c -> 0[%#ulx]\n",
			f, off, len, iswr?'w':'r', sl.len);
		return sl;
	}
	if(sl.b->type == DBfile)
		dFprint("slice m%#p[%#ullx:+%#ulx]%c -> m%#p:e+%#uld[%#ulx]\n",
			f, off, len, iswr?'w':'r',
			sl.b, (uchar*)sl.data - sl.b->d.embed, sl.len);
	else
		dFprint("slice m%#p[%#ullx:+%#ulx]%c -> m%#p:%#uld[%#ulx]\n",
			f, off, len, iswr?'w':'r',
			sl.b, (uchar*)sl.data - sl.b->d.data, sl.len);

	assert(sl.b->ref > 1);
	return sl;
}

/*
 * Find a dir entry for addr (perhaps 0 == avail) and change it to
 * naddr. If iswr, the entry is allocated if needed and the blocks
 * melted on demand.
 * Return the offset for the entry in the file or Noaddr
 * Does not adjust disk refs.
 */
uvlong
dfchdentry(Memblk *d, u64int addr, u64int naddr, int mkit)
{
	Blksl sl;
	daddrt *de;
	uvlong off;
	int i;

	assert(d->d.length/Daddrsz >= d->d.ndents);

	dAprint("dfchdentry d%#ullx -> d%#ullx\nin %H\n", addr, naddr, d);
	isrwlocked(d, mkit?Wr:Rd);
	isdir(d);

	if(addr == naddr)
		fatal("dfchdentry: it did happen. now, why?");

	off = 0;

	for(;;){
		sl = dfslice(d, Dblkdatasz, off, mkit);
		if(sl.len == 0){
			assert(sl.b == nil);
			break;
		}
		if(sl.b == nil){
			if(addr == 0 && !mkit)
				return off;
			continue;
		}
		if(catcherror()){
			mbput(sl.b);
			error(nil);
		}
		de = sl.data;
		for(i = 0; i < sl.len/Daddrsz; i++){
			if(de[i] == addr){
				off += i*Daddrsz;
				if(naddr != addr){
					de[i] = naddr;
					changed(sl.b);
					if(addr == 0 && naddr != 0){
						if(d->d.length < off+Daddrsz)
							d->d.length = off+Daddrsz;
						d->d.ndents++;
						changed(d);
					}else if(addr != 0 && naddr == 0){
						d->d.ndents--;
						changed(d);
					}
				}
				noerror();
				mbput(sl.b);
				assert(d->d.length/Daddrsz >= d->d.ndents);
				return off;
			}
		}
		off += sl.len;
		noerror();
		mbput(sl.b);
	}
	if(mkit)
		fatal("dfchdentry: bug");
	return Noaddr;
}

static daddrt
dfdirnth(Memblk *d, int n)
{
	Blksl sl;
	daddrt *de;
	uvlong off;
	int i, tot;

	isdir(d);
	off = 0;
	tot = 0;
	for(;;){
		sl = dfslice(d, Dblkdatasz, off, Rd);
		if(sl.len == 0){
			assert(sl.b == nil);
			break;
		}
		if(sl.b == nil)
			continue;
		de = sl.data;
		for(i = 0; i < sl.len/Daddrsz; i++)
			if(de[i] != 0 && tot++ >= n){
				dFprint("dfdirnth d%#ullx[%d] = d%#ullx\n",
					d->addr, n, de[i]);
				mbput(sl.b);
				return de[i];
			}
		off += sl.len;
		mbput(sl.b);
	}
	return 0;
}

static Memblk*
xfchild(Memblk *f, int n, int disktoo)
{
	daddrt addr;
	Memblk *b;

	addr = dfdirnth(f, n);
	if(addr == 0)
		return nil;
	b = mbget(DBfile, addr, Dontmk);
	if(b != nil || disktoo == Mem)
		return b;
	return dbget(DBfile, addr);
}

Memblk*
dfchild(Memblk *f, int n)
{
	return xfchild(f, n, Disk);
}

static Memblk*
mfchild(Memblk *f, int n)
{
	return xfchild(f, n, Mem);
}

/*
 * does not dbincref(f)
 * caller locks both d and f
 */
void
dflink(Memblk *d, Memblk *f)
{
	ismelted(d);
	isdir(d);

	dfchdentry(d, 0, f->addr, Mkit);
}

/*
 * does not dbput(f)
 * caller locks both d and f
 */
static void
dfunlink(Memblk *d, Memblk *f)
{
	ismelted(d);
	isdir(d);

	dfchdentry(d, f->addr, 0, Mkit);
}

/*
 * Walk to a child and return it referenced.
 * If iswr, d must not be frozen and the child is returned melted.
 */
Memblk*
dfwalk(Memblk *d, char *name, int iswr)
{
	Memblk *f;
	Blksl sl;
	daddrt *de;
	uvlong off;
	int i;
	
	if(strcmp(name, "..") == 0)
		fatal("dfwalk: '..'");
	isdir(d);
	if(iswr)
		ismelted(d);

	off = 0;
	f = nil;
	for(;;){
		sl = dfslice(d, Dblkdatasz, off, Rd);
		if(sl.len == 0){
			assert(sl.b == nil);
			break;
		}
		if(sl.b == nil)
			continue;
		if(catcherror()){
			dprint("dfwalk d%#ullx '%s': %r\n", d->addr, name);
			mbput(sl.b);
			error(nil);
		}
		for(i = 0; i < sl.len/Daddrsz; i++){
			de = sl.data;
			de += i;
			if(*de == 0)
				continue;
			f = dbget(DBfile, *de);
			if(strcmp(f->mf->name, name) != 0){
				mbput(f);
				continue;
			}

			/* found  */
			noerror();
			mbput(sl.b);
			if(!iswr || !f->frozen)
				goto done;
			fatal("dfwalk: frozen");
		}
		noerror();
		mbput(sl.b);
		off += sl.len;
	}
	error("file not found");

done:
	dprint("dfwalk d%#ullx '%s' -> d%#ullx\n", d->addr, name, f->addr);
	return f;
}

/*
 * Return the last version for *fp, wlocked, be it frozen or melted.
 */
void
followmelted(Memblk **fp, int iswr)
{
	Memblk *f;

	f = *fp;
	isfile(f);
	rwlock(f, iswr);
	while(f->mf->melted != nil){
		incref(f->mf->melted);
		*fp = f->mf->melted;
		rwunlock(f, iswr);
		mbput(f);
		f = *fp;
		rwlock(f, iswr);
		if(!f->frozen)
			return;
	}
}

/*
 * Advance path to use the most recent version of each file.
 */
Path*
dflast(Path **pp, int nth)
{
	Memblk *f;
	Path *p;
	int i;

	p = *pp;
	for(i = 0; i < nth; i++){
		f = p->f[i];
		if(f != nil && f->mf != nil && f->mf->melted != nil)
			break;
	}
	if(i == nth)
		return p;	/* all files have the last version */

	ownpath(pp);
	p = *pp;
	for(i = 0; i < nth; i++){
		followmelted(&p->f[i], Rd);
		rwunlock(p->f[i], Rd);
	}
	return p;
}

/*
 * Caller walked down p, and now requires the nth element to be
 * melted, and wlocked for writing. (nth count starts at 1);
 * 
 * Return the path with the version of f that we must use,
 * locked for writing and melted.
 * References kept in the path are traded for the ones returned.
 */
Path*
dfmelt(Path **pp, int nth)
{
	int i;
	Memblk *f, **fp, *nf;
	Path *p;

	ownpath(pp);
	p = *pp;
	assert(nth >= 1 && p->nf >= nth && p->nf >= 2);
	assert(p->f[0] == fs->root);
	fp = &p->f[nth-1];

	/*
	 * 1. Optimistic: Try to get a loaded melted version for f.
	 */
	followmelted(fp, Wr);
	f = *fp;
	if(!f->frozen)
		return p;
	ainc(&fs->nmelts);
	rwunlock(f, Wr);

	/*
	 * 2. Realistic:
	 * walk down the path, melting every frozen thing until we
	 * reach f. Keep wlocks so melted files are not frozen while we walk.
	 * /active is special, because it's only frozen temporarily while
	 * creating a frozen version of the tree. Instead of melting it,
	 * we should just wait for it.
	 * p[0] is /
	 * p[1] is /active
	 */
	for(;;){
		followmelted(&p->f[1], Wr);
		if(p->f[1]->frozen == 0)
			break;
		rwunlock(p->f[1], Wr);
		yield();
	}
	/*
	 * At loop header, parent is p->f[i-1], melted and wlocked.
	 * At the end of the loop, p->f[i] is melted and wlocked.
	 */
	for(i = 2; i < nth; i++){
		followmelted(&p->f[i], Wr);
		if(!p->f[i]->frozen){
			rwunlock(p->f[i-1], Wr);
			continue;
		}
		if(catcherror()){
			rwunlock(p->f[i-1], Wr);
			rwunlock(p->f[i], Wr);
			error(nil);
		}

		nf = dbdup(p->f[i]);
		rwlock(nf, Wr);

		if(catcherror()){
			rwunlock(nf, Wr);
			mbput(nf);
			error(nil);
		}
		dfchdentry(p->f[i-1], p->f[i]->addr, nf->addr, Mkit);
		noerror();
		noerror();
		/* committed */
		rwunlock(p->f[i-1], Wr);		/* parent */
		rwunlock(p->f[i], Wr);		/* old frozen version */
		f = p->f[i];
		p->f[i] = nf;
		assert(f->ref > 1);
		mbput(f);			/* ref from path */
		if(!catcherror()){
			dbput(f, f->type, f->addr); /* p->f[i] ref from disk */
			noerror();
		}
	}
	return p;
}

void
dfused(Path *p)
{
	Memblk *f;

	f = p->f[p->nf-1];
	isfile(f);
	rwlock(f, Wr);
	f->d.atime = nsec();
	rwunlock(f, Wr);
}

/*
 * Report that a file has been modified.
 * Modification times propagate up to the root of the file tree.
 * But frozen files are never changed.
 */
void
dfchanged(Path *p, int muid)
{
	Memblk *f;
	u64int t, u;
	int i;

	t = nsec();
	u = muid;
	for(i = 0; i < p->nf; i++){
		f = p->f[i];
		rwlock(f, Wr);
		if(f->frozen == 0)
			if(!catcherror()){
				f->d.mtime = t;
				f->d.atime = t;
				f->d.muid = muid;
				changed(f);
				noerror();
			}
		rwunlock(f, Wr);
	}
}

/*
 * May be called with null parent, for root and ctl files.
 * The first call with a null parent is root, all others are ctl
 * files linked at root.
 */
Memblk*
dfcreate(Memblk *parent, char *name, int uid, ulong mode)
{
	Memblk *nf;
	Mfile *m;
	int isctl;

	if(fsfull())
		error("file system full");
	isctl = parent == nil;
	if(parent == nil)
		parent = fs->root;

	if(parent != nil){
		dprint("dfcreate '%s' %M at\n%H\n", name, mode, parent);
		isdir(parent);
		isrwlocked(parent, Wr);
		ismelted(parent);
	}else
		dprint("dfcreate '%s' %M", name, mode);

	if(isctl)
		nf = dballoc(DBctl);
	else
		nf = dballoc(DBfile);
	if(catcherror()){
		mbput(nf);
		if(parent != nil)
			rwunlock(parent, Wr);
		error(nil);
	}

	m = nf->mf;
	nf->d.id = nsec();
	nf->d.mode = mode;
	nf->d.mtime = nf->d.id;
	nf->d.atime = nf->d.id;
	nf->d.length = 0;
	m->uid = usrname(uid);
	nf->d.uid = uid;
	m->gid = m->uid;
	nf->d.gid = nf->d.uid;
	m->muid = m->uid;
	nf->d.muid = nf->d.uid;
	m->name = name;
	nf->d.asize = pmeta(nf->d.embed, Embedsz, nf);
	changed(nf);

	if(parent != nil){
		m->gid = parent->mf->gid;
		nf->d.gid = parent->d.gid;
		dflink(parent, nf);
	}
	noerror();
	dprint("dfcreate-> %H\n within %H\n", nf, parent);
	return nf;
}

void
dfremove(Memblk *p, Memblk *f)
{
	vlong n;

	/* funny as it seems, we may need extra blocks to melt */
	if(fsfull())
		error("file system full");

	isrwlocked(f, Wr);
	isrwlocked(p, Wr);
	ismelted(p);
	if((f->d.mode&DMDIR) != 0 && f->d.ndents > 0)
		error("directory not empty");
	incref(p);
	if(catcherror()){
		mbput(p);
		error(nil);
	}
	dfunlink(p, f);
	/* shouldn't fail now. it's unlinked */

	if(p->d.ndents == 0 && p->d.length > 0){	/* all gone, make it public */
		p->d.length = 0;
		changed(p);
	}

	noerror();
	rwunlock(f, Wr);
	if(!catcherror()){
		n = dfput(f);
		dprint("dfput d%#ullx: %lld blks\n", f->addr, n);
		noerror();
	}
	mbput(f);
	mbput(p);
}

/*
 * It's ok if a is nil, for reading ahead.
 */
ulong
dfpread(Memblk *f, void *a, ulong count, uvlong off)
{
	Blksl sl;
	ulong tot;
	char *p;

	p = a;
	isrwlocked(f, Rd);
	for(tot = 0; tot < count; tot += sl.len){
		sl = dfslice(f, count-tot, off+tot, Rd);
		if(sl.len == 0){
			assert(sl.b == nil);
			break;
		}
		if(sl.data == nil){
			memset(p+tot, 0, sl.len);
			assert(sl.b == nil);
			continue;
		}
		if(p != nil)
			memmove(p+tot, sl.data, sl.len);
		mbput(sl.b);
	}
	return tot;
}

ulong
dfpwrite(Memblk *f, void *a, ulong count, uvlong *off)
{
	Blksl sl;
	ulong tot;
	char *p;

	if(fsfull())
		error("file system full");

	isrwlocked(f, Wr);
	ismelted(f);
	p = a;
	if(f->d.mode&DMAPPEND)
		*off = f->d.length;
	for(tot = 0; tot < count;){
		sl = dfslice(f, count-tot, *off+tot, Wr);
		if(sl.len == 0 || sl.data == nil)
			fatal("dfpwrite: bug");
		memmove(sl.data, p+tot, sl.len);
		changed(sl.b);
		mbput(sl.b);
		tot += sl.len;
		if(*off+tot > f->d.length){
			f->d.length = *off+tot;
			changed(f);
		}
	}
	return tot;
}

static int
ptrmap(daddrt addr, int nind, Blkf f, int isdisk)
{
	int i;
	Memblk *b;
	long tot;

	if(addr == 0)
		return 0;
	if(isdisk)
		b = dbget(DBdata+nind, addr);
	else{
		b = mbget(DBdata+nind, addr, Dontmk);
		if(b == nil)
			return 0;	/* on disk */
	}
	if(catcherror()){
		mbput(b);
		error(nil);
	}
	tot = 0;
	if(f == nil || f(b) == 0){
		tot++;
		if(nind > 0){
			for(i = 0; i < Dptrperblk; i++)
				tot += ptrmap(b->d.ptr[i], nind-1, f, isdisk);
		}
	}
	noerror();
	mbput(b);
	return tot;
}

/*
 * CAUTION: debug: no locks.
 */
int
dfdump(Memblk *f, int isdisk)
{
	int i;
	Memblk *b;
	Memblk *(*child)(Memblk*, int);
	long tot;
	extern int mbtab;

	isfile(f);
	tot = 1;
	/* visit the blocks to fetch them if needed. */
	for(i = 0; i < nelem(f->d.dptr); i++)
		tot += ptrmap(f->d.dptr[i], 0, nil, isdisk);
	for(i = 0; i < nelem(f->d.iptr); i++)
		tot += ptrmap(f->d.iptr[i], i+1, nil, isdisk);
	fprint(2, "%H\n", f);
	if((f->d.mode&DMDIR) != 0){
		mbtab++;
		child = dfchild;
		if(!isdisk)
			child = mfchild;
		for(i = 0; i < f->d.ndents; i++){
			b = child(f, i);
			if(b == nil)
				continue;
			if(!catcherror()){
				tot += dfdump(b, isdisk);
				noerror();
			}
			mbput(b);
		}
		mbtab--;
	}

	return tot;
}

static int
bfreeze(Memblk *b)
{
	if(b->frozen)
		return -1;
	b->frozen = 1;
	return 0;
}

int
dffreeze(Memblk *f)
{
	int i;
	Memblk *b;
	long tot;

	isfile(f);
	if(f->frozen && f != fs->active && f != fs->archive)
		return 0;
	rwlock(f, Wr);
	if(catcherror()){
		rwunlock(f, Wr);
		error(nil);
	}
	f->frozen = 1;
	tot = 1;
	for(i = 0; i < nelem(f->d.dptr); i++)
		tot += ptrmap(f->d.dptr[i], 0, bfreeze, Mem);
	for(i = 0; i < nelem(f->d.iptr); i++)
		tot += ptrmap(f->d.iptr[i], i+1, bfreeze, Mem);
	if((f->d.mode&DMDIR) != 0){
		for(i = 0; i < f->d.length/Daddrsz; i++){
			b = mfchild(f, i);
			if(b == nil)
				continue;
			if(!catcherror()){
				tot += dffreeze(b);
				noerror();
			}
			mbput(b);
		}
	}
	noerror();
	rwunlock(f, Wr);
	return tot;
}

static int
countref(daddrt addr)
{
	ulong idx;
	int old;

	idx = addr/Dblksz;
	old = fs->dchk[idx];
	if(fs->dchk[idx] == 0xFE)
		fprint(2, "fscheck: d%#010ullx: too many refs, ignoring some\n",
			addr);
	else
		fs->dchk[idx]++;
	return old;
}

static int
bcountrefs(Memblk *b)
{
	if(countref(b->addr) != 0)
		return -1;
	return 0;
}

static void
countfree(daddrt addr)
{
	long i;

	i = addr/Dblksz;
	if(fs->dchk[i] != 0 && fs->dchk[i] <= 0xFE)
		fprint(2, "fscheck: d%#010ullx: free block in use\n", addr);
	else if(fs->dchk[i] == 0xFF)
		fprint(2, "fscheck: d%#010ullx: double free\n", addr);
	else
		fs->dchk[i] = 0xFF;
}

void
dfcountfree(void)
{
	daddrt addr;

	dprint("list...\n");
	addr = fs->super->d.free;
	while(addr != 0){
		if(addr >fs->limit){
			fprint(2, "fscheck: d%#010ullx: free overflow\n", addr);
			break;
		}
		countfree(addr);
		addr = dbgetref(addr);
	}
	/* heading unused part */
	dprint("hdr...\n");
	for(addr = 0; addr < Dblk0addr; addr += Dblksz)
		countfree(addr);
	/* DBref blocks */
	dprint("refs...\n");
	for(addr = Dblk0addr; addr < fs->super->d.eaddr; addr += Dblksz*Nblkgrpsz){
		countfree(addr);	/* even DBref */
		countfree(addr+Dblksz);	/* odd DBref */
	}
}

long
dfcountrefs(Memblk *f)
{
	Memblk *b;
	int i;
	long nfails;

	nfails = 0;
	isfile(f);
	if((f->addr&Fakeaddr) == 0 && f->addr >= fs->limit){
		fprint(2, "fscheck: '%s' d%#010ullx: out of range\n",
			f->mf->name, f->addr);
		return 1;
	}
	if((f->addr&Fakeaddr) == 0)
		if(countref(f->addr) != 0)		/* already visited */
			return 0;			/* skip children */
	rwlock(f, Rd);
	if(catcherror()){
		fprint(2, "fscheck: '%s' d%#010ullx: data: %r\n",
			f->mf->name, f->addr);
		rwunlock(f, Rd);
		return 1;
	}
	for(i = 0; i < nelem(f->d.dptr); i++)
		ptrmap(f->d.dptr[i], 0, bcountrefs, Disk);
	for(i = 0; i < nelem(f->d.iptr); i++)
		ptrmap(f->d.iptr[i], i+1, bcountrefs, Disk);
	if(f->d.mode&DMDIR)
		for(i = 0; i < f->d.length/Daddrsz; i++){
			b = dfchild(f, i);
			if(catcherror()){
				fprint(2, "fscheck: '%s'  d%#010ullx:"
					" child[%d]: %r\n",
					f->mf->name, f->addr, i);
				nfails++;
			}else{
				nfails += dfcountrefs(b);
				noerror();
			}
			mbput(b);
		}
	noerror();
	rwunlock(f, Rd);
	return nfails;
}

/*
 * Drop one disk reference for f and reclaim its storage if it's gone.
 * The given memory reference is not released.
 * For directories, all files contained have their disk references adjusted,
 * and they are also reclaimed if no further references exist.
 *
 * NB: Time ago, directories were not in compact form (they had holes
 * due to removals) and this had a bug while reclaiming that could lead
 * to double frees of disk blocks.
 * The bug was fixed, but since then, directories have changed again to
 * have holes. If the a double free happens again, this is the place where
 * to look, besides dbdup and dfchdentry.
 */
int
dfput(Memblk *f)
{
	int i;
	Memblk *b;
	long tot;

	isfile(f);
	dKprint("dfput %H\n", f);
	/*
	 * Remove children if it's the last disk ref before we drop data blocks.
	 * No new disk refs may be added, so there's no race here.
	 */
	tot = 0;
	if(dbgetref(f->addr) == 1 && (f->d.mode&DMDIR) != 0){
		rwlock(f, Wr);
		if(catcherror()){
			rwunlock(f, Wr);
			error(nil);
		}
		for(i = 0; i < f->d.ndents; i++){
			b = dfchild(f, i);
			if(!catcherror()){
				tot += dfput(b);
				noerror();
			}
			mbput(b);
		}
		noerror();
		rwunlock(f, Wr);
	}

	if(dbput(f, f->type, f->addr) == 0)
		tot++;
	return tot;
}
