#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

static uchar	isdos[256];

int
isdosfs(uchar *buf)
{
	/*
	 * When dynamic disc managers move the disc partition,
	 * they make it start with 0xE9.
	 */
	if(buf[0] == 0xE9)
		return 1;

	/*
	 * Check if the jump displacement (magic[1]) is too short for a FAT.
	 *
	 * check now omitted due to digital cameras that use a 0 jump.
	 * the ecma-107 standard says this is okay and that interoperable fat
	 * implementations shouldn't assume this:
	 * http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-107.pdf,
	 * page 11.
	 */
	if(buf[0] == 0xEB && buf[2] == 0x90 /* && buf[1] >= 0x30 */)
		return 1;
	if(chatty)
		fprint(2, "bad sig %.2ux %.2ux %.2uxn", buf[0], buf[1], buf[2]);

	return 0;
}

int
dosfs(Xfs *xf)
{
	Iosect *p, *p1;
	Dosboot *b;
	Fatinfo *fi;
	Dosboot32 *b32;
	Dosbpb *bp;
	long fisec, extflags;
	int i;

	if(!isdos['a']){
		for(i = 'a'; i <= 'z'; i++)
			isdos[i] = 1;
		for(i = 'A'; i <= 'Z'; i++)
			isdos[i] = 1;
		for(i = '0'; i <= '9'; i++)
			isdos[i] = 1;
		isdos['$'] = 1;
		isdos['%'] = 1;
		isdos['''] = 1;
		isdos['-'] = 1;
		isdos['_'] = 1;
		isdos['@'] = 1;
		isdos['~'] = 1;
		isdos['`'] = 1;
		isdos['!'] = 1;
		isdos['('] = 1;
		isdos[')'] = 1;
		isdos['{'] = 1;
		isdos['}'] = 1;
		isdos['^'] = 1;
		isdos['#'] = 1;
		isdos['&'] = 1;
	}

	p = getsect(xf, 0);
	if(p == 0)
		return -1;

	b = (Dosboot*)p->iobuf;
	if(b->clustsize == 0 || isdosfs(p->iobuf) == 0){
		putsect(p);
		return -1;
	}

	bp = malloc(sizeof(Dosbpb));
	memset(bp, 0, sizeof(Dosbpb));	/* clear lock */
	xf->ptr = bp;
	xf->fmt = 1;

	bp->sectsize = GSHORT(b->sectsize);
	bp->clustsize = b->clustsize;
	bp->nresrv = GSHORT(b->nresrv);
	bp->nfats = b->nfats;
	bp->rootsize = GSHORT(b->rootsize);
	bp->volsize = GSHORT(b->volsize);
	if(bp->volsize == 0)
		bp->volsize = GLONG(b->bigvolsize);
	bp->mediadesc = b->mediadesc;
	bp->fatsize = GSHORT(b->fatsize);
	bp->fataddr = GSHORT(b->nresrv);

	bp->fatinfo = 0;

	if(bp->fatsize == 0){	/* is FAT32 */
		if(chatty)
			bootsecdump32(2, xf, (Dosboot32*)b);
		xf->isfat32 = 1;
		b32 = (Dosboot32*)b;
		bp->fatsize = GLONG(b32->fatsize32);
		if(bp->fatsize == 0){
			putsect(p);
			if(chatty)
				fprint(2, "fatsize 0\n");
			return -1;
		}
		bp->dataaddr = bp->fataddr + bp->nfats*bp->fatsize;
		bp->rootaddr = 0;
		bp->rootstart = GLONG(b32->rootstart);

		/*
		 * disable fat mirroring?
		 */
		extflags = GSHORT(b32->extflags);
		if(extflags & 0x0080){
			for(i = 0; i < 4; i++){
				if(extflags & (1 << i)){
					bp->fataddr += i * bp->fatsize;
					bp->nfats = 1;
					break;
				}
			}
		}

		/*
		 * fat free list info
		 */
		bp->freeptr = FATRESRV;
		fisec = GSHORT(b32->infospec);
		if(fisec != 0 && fisec < GSHORT(b32->nresrv)){
			p1 = getsect(xf, fisec);
			if(p1 != nil){
				fi = (Fatinfo*)p1->iobuf;
				if(GLONG(fi->sig1) == FATINFOSIG1 && GLONG(fi->sig) == FATINFOSIG){
					bp->fatinfo = fisec;
					bp->freeptr = GLONG(fi->nextfree);
					bp->freeclusters = GLONG(fi->freeclust);
					chat("fat info: %ld free clusters, next free %ld\n", bp->freeclusters, bp->freeptr);
				}
				putsect(p1);
			}
		}
	}else{
		if(chatty)
			bootdump(2, b);
		bp->rootaddr = bp->fataddr + bp->nfats*bp->fatsize;
		bp->rootstart = 0;
		i = bp->rootsize*DOSDIRSIZE + bp->sectsize-1;
		i /= bp->sectsize;
		bp->dataaddr = bp->rootaddr + i;
		bp->freeptr = FATRESRV;
	}
	bp->fatclusters = FATRESRV+(bp->volsize - bp->dataaddr)/bp->clustsize;

	if(xf->isfat32)
		bp->fatbits = 32;
	else if(bp->fatclusters < 4087)
		bp->fatbits = 12;
	else
		bp->fatbits = 16;

	chat("fatbits=%d (%d clusters)...", bp->fatbits, bp->fatclusters);
	for(i=0; i<b->nfats; i++)
		chat("fat %d: %ld...", i, bp->fataddr+i*bp->fatsize);
	chat("root: %ld...", bp->rootaddr);
	chat("data: %ld...", bp->dataaddr);
	putsect(p);
	return 0;
}

/*
 * initialize f to the root directory
 * this file has no Dosdir entry,
 * so we special case it all over.
 */
void
rootfile(Xfile *f)
{
	Dosptr *dp;

	dp = f->ptr;
	memset(dp, 0, sizeof(Dosptr));
	dp->prevaddr = -1;
}

int
isroot(ulong addr)
{
	return addr == 0;
}

int
getfile(Xfile *f)
{
	Dosptr *dp;
	Iosect *p;

	dp = f->ptr;
	if(dp->p)
		panic("getfile");
	p = getsect(f->xf, dp->addr);
	if(p == nil)
		return -1;

	/*
	 * we could also make up a Dosdir for the root
	 */
	dp->d = nil;
	if(!isroot(dp->addr)){
		if(f->qid.path != QIDPATH(dp)){
			chat("qid mismatch f=%#llux d=%#lux...", f->qid.path, QIDPATH(dp));
			putsect(p);
			errno = Enonexist;
			return -1;
		}
		dp->d = (Dosdir *)&p->iobuf[dp->offset];
	}
	dp->p = p;
	return 0;
}

void
putfile(Xfile *f)
{
	Dosptr *dp;

	dp = f->ptr;
	if(!dp->p)
		panic("putfile");
	putsect(dp->p);
	dp->p = nil;
	dp->d = nil;
}

long
getstart(Xfs *xf, Dosdir *d)
{
	long start;

	start = GSHORT(d->start);
	if(xf->isfat32)
		start |= GSHORT(d->hstart)<<16;
	return start;
}

void
putstart(Xfs *xf, Dosdir *d, long start)
{
	PSHORT(d->start, start);
	if(xf->isfat32)
		PSHORT(d->hstart, start>>16);
}

/*
 * return the disk cluster for the iclust cluster in f
 */
long
fileclust(Xfile *f, long iclust, int cflag)
{
	Dosbpb *bp;
	Dosptr *dp;
	Dosdir *d;
	long start, clust, nskip, next;

	bp = f->xf->ptr;
	dp = f->ptr;
	d = dp->d;
	next = 0;

	/* 
	 * asking for the cluster of the root directory
	 * is not a well-formed question, since the root directory
	 * does not begin on a cluster boundary.
	 */
	if(!f->xf->isfat32 && isroot(dp->addr))
		return -1;

	if(f->xf->isfat32 && isroot(dp->addr)){
		start = bp->rootstart;
	}else{
		start = getstart(f->xf, d);
		if(start == 0){
			if(!cflag)
				return -1;
			mlock(bp);
			start = falloc(f->xf);
			unmlock(bp);
			if(start <= 0)
				return -1;
			puttime(d, 0);
			putstart(f->xf, d, start);
			dp->p->flags |= BMOD;
			dp->clust = 0;
		}
	}
	if(dp->clust == 0 || iclust < dp->iclust){
		clust = start;
		nskip = iclust;
	}else{
		clust = dp->clust;
		nskip = iclust - dp->iclust;
	}
	if(chatty > 1 && nskip > 0)
		chat("clust %#lx, skip %ld...", clust, nskip);
	if(clust <= 0)
		return -1;
	if(nskip > 0){
		mlock(bp);
		while(--nskip >= 0){
			next = getfat(f->xf, clust);
			if(chatty > 1)
				chat("->%#lx", next);
			if(next > 0){
				clust = next;
				continue;
			}else if(!cflag)
				break;
			if(d && (d->attr&DSYSTEM)){
				next = cfalloc(f);
				if(next < 0)
					break;
				/* cfalloc will call putfat for us, since clust may change */
			} else {
				next = falloc(f->xf);
				if(next < 0)
					break;
				putfat(f->xf, clust, next);
			}
			clust = next;
		}
		unmlock(bp);
		if(next <= 0)
			return -1;
		dp->clust = clust;
		dp->iclust = iclust;
	}
	if(chatty > 1)
		chat(" clust(%#lx)=%#lx...", iclust, clust);
	return clust;
}

/*
 * return the disk sector for the isect disk sector in f 
 */
long
fileaddr(Xfile *f, long isect, int cflag)
{
	Dosbpb *bp;
	Dosptr *dp;
	long clust;

	bp = f->xf->ptr;
	dp = f->ptr;
	if(!f->xf->isfat32 && isroot(dp->addr)){
		if(isect*bp->sectsize >= bp->rootsize*DOSDIRSIZE)
			return -1;
		return bp->rootaddr + isect;
	}
	clust = fileclust(f, isect/bp->clustsize, cflag);
	if(clust < 0)
		return -1;

	return clust2sect(bp, clust) + isect%bp->clustsize;
}

/*
 * translate names
 */
void
fixname(char *buf)
{
	int c;
	char *p;

	p = buf;
	while(c = *p){
		if(c == ':' && trspaces)
			*p = ' ';
		p++;
	}
}

/*
 * classify the file name as one of 
 *	Invalid - contains a bad character
 *	Short - short valid 8.3 name, no lowercase letters
 *	ShortLower - short valid 8.3 name except for lowercase letters
 *	Long - long name 
 */
int
classifyname(char *buf)
{
	char *p, *dot;
	int c, isextended, is8dot3, islower, ndot;

	p = buf;
	isextended = 0;
	islower = 0;
	dot = nil;
	ndot = 0;
	while(c = (uchar)*p){
		if(c&0x80)	/* UTF8 */
			isextended = 1;
		else if(c == '.'){
			dot = p;
			ndot++;
		}else if(strchr("+,:;=[] ", c))
			isextended = 1;
		else if(!isdos[c])
			return Invalid;
		if('a' <= c && c <= 'z')
			islower = 1;
		p++;
	}

	is8dot3 = (ndot==0 && p-buf <= 8) || (ndot==1 && dot-buf <= 8 && p-(dot+1) <= 3);
	
	if(!isextended && is8dot3){
		if(islower)
			return ShortLower;
		return Short;
	}
	return Long;
}
		
/*
 * make an alias for a valid long file name
 */
void
mkalias(char *name, char *sname, int id)
{
	Rune r;
	char *s, *e, sid[10];
	int i, esuf, v;

	e = strrchr(name, '.');
	if(e == nil)
		e = strchr(name, '\0');

	s = name;
	i = 0;
	while(s < e && i < 6){
		if(isdos[(uchar)*s])
			sname[i++] = *s++;
		else
			s += chartorune(&r, s);
	}

	v = snprint(sid, 10, "%d", id);
	if(i + 1 + v > 8)
		i = 8 - 1 - v;
	sname[i++] = '~';
	strcpy(&sname[i], sid);
	i += v;

	sname[i++] = '.';
	esuf = i + 3;
	if(esuf > 12)
		panic("bad mkalias");
	while(*e && i < esuf){
		if(isdos[(uchar)*e])
			sname[i++] = *e++;
		else
			e += chartorune(&r, e);
	}
	if(sname[i-1] == '.')
		i--;
	sname[i] = '\0';
}

/*
 * check for valid plan 9 names,
 * rewrite ' ' to ':'
 */
char isfrog[256]={
	/*NUL*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*BKS*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*DLE*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*CAN*/	1, 1, 1, 1, 1, 1, 1, 1,
/*	[' ']	1,	let's try this -rsc */
	['/']	1,
	[0x7f]	1,
};

int
nameok(char *elem)
{
	while(*elem) {
		if(*elem == ' ' && trspaces)
			*elem = ':';
		if(isfrog[*(uchar*)elem])
			return 0;
		elem++;
	}
	return 1;
}

/*
 * look for a directory entry matching name
 * always searches for long names which match a short name
 */
int
searchdir(Xfile *f, char *name, Dosptr *dp, int cflag, int longtype)
{
	Xfs *xf;
	Iosect *p;
	Dosbpb *bp;
	Dosdir *d;
	char buf[261], *bname;
	int isect, addr, o, addr1, addr2, prevaddr, prevaddr1, o1, islong, have, need, sum;

	xf = f->xf;
	bp = xf->ptr;
	addr1 = -1;
	addr2 = -1;
	prevaddr1 = -1;
	o1 = 0;
	islong = 0;
	sum = -1;

	need = 1;
	if(longtype!=Short && cflag)
		need += (utflen(name) + DOSRUNE-1) / DOSRUNE;

	memset(dp, 0, sizeof(Dosptr));
	dp->prevaddr = -1;
	dp->naddr = -1;
	dp->paddr = ((Dosptr *)f->ptr)->addr;
	dp->poffset = ((Dosptr *)f->ptr)->offset;

	have = 0;
	addr = -1;
	bname = nil;
	for(isect=0;; isect++){
		prevaddr = addr;
		addr = fileaddr(f, isect, cflag);
		if(addr < 0)
			break;
		p = getsect(xf, addr);
		if(p == 0)
			break;
		for(o=0; o<bp->sectsize; o+=DOSDIRSIZE){
			d = (Dosdir *)&p->iobuf[o];
			if(d->name[0] == 0x00){
				chat("end dir(0)...");
				putsect(p);
				if(!cflag)
					return -1;

				/*
				 * addr1 & o1 are the start of the dirs
				 * addr2 is the optional second cluster used if the long name
				 * entry does not fit within the addr1 cluster
				 *
				 * have tells us the number of contiguous free dirs
				 * starting at addr1.o1; need are necessary to hold the long name.
				 */
				if(addr1 < 0){
					addr1 = addr;
					prevaddr1 = prevaddr;
					o1 = o;
				}
				if(addr2 < 0 && (bp->sectsize-o)/DOSDIRSIZE + have < need){
					addr2 = fileaddr(f, isect+1, cflag);
					if(addr2 < 0)
						goto breakout;
				}else if(addr2 < 0)
					addr2 = addr;
				if(addr2 == addr1)
					addr2 = -1;
				dp->addr = addr1;
				dp->offset = o1;
				dp->prevaddr = prevaddr1;
				dp->naddr = addr2;
				return 0;
			}
			if(d->name[0] == DOSEMPTY){
				if(chatty)
					fprint(2, "empty dir\n");

				have++;
				if(addr1 == -1){
					addr1 = addr;
					o1 = o;
					prevaddr1 = prevaddr;
				}
				if(addr2 == -1 && have >= need)
					addr2 = addr;
				continue;
			}
			have = 0;
			if(addr2 == -1)
				addr1 = -1;

			dirdump(d);
			if((d->attr & 0xf) == 0xf){
				bname = getnamesect(buf, bname, p->iobuf + o, &islong, &sum, 1);
				continue;
			}
			if(d->attr & DVLABEL){
				islong = 0;
				continue;
			}
			if(islong != 1 || sum != aliassum(d) || cistrcmp(bname, name) != 0){
				bname = buf;
				getname(buf, d);
			}
			islong = 0;
			if(cistrcmp(bname, name) != 0)
				continue;
			if(chatty)
				fprint(2, "found\n");
			if(cflag){
				putsect(p);
				return -1;
			}
			dp->addr = addr;
			dp->prevaddr = prevaddr;
			dp->offset = o;
			dp->p = p;
			dp->d = d;
			return 0;
		}
		putsect(p);
	}
breakout:
	chat("end dir(1)...");
	return -1;
}

int
emptydir(Xfile *f)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	int isect, addr, o;
	Iosect *p;
	Dosdir *d;

	for(isect=0;; isect++){
		addr = fileaddr(f, isect, 0);
		if(addr < 0)
			break;
		p = getsect(xf, addr);
		if(p == 0)
			return -1;
		for(o=0; o<bp->sectsize; o+=DOSDIRSIZE){
			d = (Dosdir *)&p->iobuf[o];
			if(d->name[0] == 0x00){
				putsect(p);
				return 0;
			}
			if(d->name[0] == DOSEMPTY)
				continue;
			if(d->name[0] == '.')
				continue;
			if(d->attr&DVLABEL)
				continue;
			putsect(p);
			return -1;
		}
		putsect(p);
	}
	return 0;
}

long
readdir(Xfile *f, void *vbuf, long offset, long count)
{
	Xfs *xf;
	Dosbpb *bp;
	Dir dir;
	int isect, addr, o, islong, sum;
	Iosect *p;
	Dosdir *d;
	long rcnt, n;
	char *name, snamebuf[8+1+3+1], namebuf[DOSNAMELEN];
	uchar *buf;

	buf = vbuf;
	rcnt = 0;
	xf = f->xf;
	bp = xf->ptr;
	if(count <= 0)
		return 0;
	islong = 0;
	sum = -1;
	name = nil;
	for(isect=0;; isect++){
		addr = fileaddr(f, isect, 0);
		if(addr < 0)
			break;
		p = getsect(xf, addr);
		if(p == 0)
			return -1;
		for(o=0; o<bp->sectsize; o+=DOSDIRSIZE){
			d = (Dosdir *)&p->iobuf[o];
			if(d->name[0] == 0x00){
				putsect(p);
				return rcnt;
			}
			if(d->name[0] == DOSEMPTY)
				continue;
			dirdump(d);
			if(d->name[0] == '.'){
				if(d->name[1] == ' ' || d->name[1] == 0)
					continue;
				if(d->name[1] == '.' &&
				  (d->name[2] == ' ' || d->name[2] == 0))
					continue;
			}
			if((d->attr & 0xf) == 0xf){
				name = getnamesect(namebuf, name, p->iobuf+o, &islong, &sum, 1);
				continue;
			}
			if(d->attr & DVLABEL){
				islong = 0;
				continue;
			}
			dir.name = snamebuf;
			getdir(xf, &dir, d, addr, o);
			if(islong == 1 && nameok(name) && sum == aliassum(d))
				dir.name = name;
			islong = 0;
			n = convD2M(&dir, &buf[rcnt], count - rcnt);
			name = nil;
			if(n <= BIT16SZ){	/* no room for next entry */
				putsect(p);
				return rcnt;
			}
			rcnt += n;
			if(offset > 0){
				offset -= rcnt;
				rcnt = 0;
				islong = 0;
				continue;
			}
			if(rcnt == count){
				putsect(p);
				return rcnt;
			}
		}
		putsect(p);
	}
	return rcnt;
}

/*
 * set up ndp for a directory's parent
 * the hardest part is setting up paddr
 */
int
walkup(Xfile *f, Dosptr *ndp)
{
	Dosbpb *bp;
	Dosptr *dp;
	Dosdir *xd;
	Iosect *p;
	long k, o, so, start, pstart, ppstart, st, ppclust;

	bp = f->xf->ptr;
	dp = f->ptr;
	memset(ndp, 0, sizeof(Dosptr));
	ndp->prevaddr = -1;
	ndp->naddr = -1;
	ndp->addr = dp->paddr;
	ndp->offset = dp->poffset;

	chat("walkup: paddr=%#lx...", dp->paddr);

	/*
	 * root's paddr is always itself
	 */
	if(isroot(dp->paddr))
		return 0;

	/*
	 * find the start of our parent's directory
	 */
	p = getsect(f->xf, dp->paddr);
	if(p == nil)
		goto error;
	xd = (Dosdir *)&p->iobuf[dp->poffset];
	dirdump(xd);
	start = getstart(f->xf, xd);
	chat("start=%#lx...", start);
	if(start == 0)
		goto error;
	putsect(p);

	/*
	 * verify that parent's . points to itself
	 */
	p = getsect(f->xf, clust2sect(bp, start));
	if(p == nil)
		goto error;
	xd = (Dosdir *)p->iobuf;
	dirdump(xd);
	st = getstart(f->xf, xd);
	if(xd->name[0]!='.' || xd->name[1]!=' ' || start!=st)
		goto error;

	/*
	 * parent's .. is the next entry, and has start of parent's parent
	 */
	xd++;
	dirdump(xd);
	if(xd->name[0] != '.' || xd->name[1] != '.')
		goto error;
	pstart = getstart(f->xf, xd);
	putsect(p);

	/*
	 * we're done if parent is root
	 */
	if(pstart == 0 || f->xf->isfat32 && pstart == bp->rootstart)
		return 0;

	/*
	 * verify that parent's . points to itself
	 */
	p = getsect(f->xf, clust2sect(bp, pstart));
	if(p == 0){
		chat("getsect %ld failed\n", pstart);
		goto error;
	}
	xd = (Dosdir *)p->iobuf;
	dirdump(xd);
	st = getstart(f->xf, xd);
	if(xd->name[0]!='.' || xd->name[1]!=' ' || pstart!=st)
		goto error;

	/*
	 * parent's parent's .. is the next entry, and has start of parent's parent's parent
	 */
	xd++;
	dirdump(xd);
	if(xd->name[0] != '.' || xd->name[1] != '.')
		goto error;
	ppstart = getstart(f->xf, xd);
	putsect(p);

	/*
	 * open parent's parent's parent, and walk through it until parent's parent is found
	 * need this to find parent's parent's addr and offset
	 */
	ppclust = ppstart;
	if(f->xf->isfat32 && ppclust == 0){
		ppclust = bp->rootstart;
		chat("ppclust 0, resetting to rootstart\n");
	}
	k = ppclust ? clust2sect(bp, ppclust) : bp->rootaddr;
	p = getsect(f->xf, k);
	if(p == nil){
		chat("getsect %ld failed\n", k);
		goto error;
	}
	xd = (Dosdir *)p->iobuf;
	dirdump(xd);
	if(ppstart){
		st = getstart(f->xf, xd);
		if(xd->name[0]!='.' || xd->name[1]!=' ' || ppstart!=st)
			goto error;
	}
	for(so=1;; so++){
		for(o=0; o<bp->sectsize; o+=DOSDIRSIZE){
			xd = (Dosdir *)&p->iobuf[o];
			if(xd->name[0] == 0x00){
				chat("end dir\n");
				goto error;
			}
			if(xd->name[0] == DOSEMPTY)
				continue;
			st = getstart(f->xf, xd);
			if(st == pstart)
				goto out;
		}
		if(ppclust){
			if(so%bp->clustsize == 0){
				mlock(bp);
				ppclust = getfat(f->xf, ppclust);
				unmlock(bp);
				if(ppclust < 0){
					chat("getfat %ld failed\n", ppclust);
					goto error;
				}
			}
			k = clust2sect(bp, ppclust) + 
				so%bp->clustsize;
		}else{
			if(so*bp->sectsize >= bp->rootsize*DOSDIRSIZE)
				goto error;
			k = bp->rootaddr + so;
		}
		putsect(p);
		p = getsect(f->xf, k);
		if(p == 0){
			chat("getsect %ld failed\n", k);
			goto error;
		}
	}
out:
	putsect(p);
	ndp->paddr = k;
	ndp->poffset = o;
	return 0;

error:
	if(p)
		putsect(p);
	return -1;
}

long
readfile(Xfile *f, void *vbuf, long offset, long count)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	int isect, addr, o, c;
	Iosect *p;
	uchar *buf;
	long length, rcnt;

	rcnt = 0;
	length = GLONG(d->length);
	buf = vbuf;
	if(offset >= length)
		return 0;
	if(offset+count >= length)
		count = length - offset;
	isect = offset/bp->sectsize;
	o = offset%bp->sectsize;
	while(count > 0){
		addr = fileaddr(f, isect++, 0);
		if(addr < 0)
			break;
		c = bp->sectsize - o;
		if(c > count)
			c = count;
		p = getsect(xf, addr);
		if(p == 0)
			return -1;
		memmove(&buf[rcnt], &p->iobuf[o], c);
		putsect(p);
		count -= c;
		rcnt += c;
		o = 0;
	}
	return rcnt;
}

long
writefile(Xfile *f, void *vbuf, long offset, long count)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	int isect, addr = 0, o, c;
	Iosect *p;
	uchar *buf;
	long length, rcnt = 0, dlen;

	buf = vbuf;
	isect = offset/bp->sectsize;
	o = offset%bp->sectsize;
	while(count > 0){
		addr = fileaddr(f, isect++, 1);
		if(addr < 0)
			break;
		c = bp->sectsize - o;
		if(c > count)
			c = count;
		if(c == bp->sectsize){
			p = getosect(xf, addr);
			p->flags = 0;
		}else{
			p = getsect(xf, addr);
			if(p == nil)
				return -1;
		}
		memmove(&p->iobuf[o], &buf[rcnt], c);
		p->flags |= BMOD;
		putsect(p);
		count -= c;
		rcnt += c;
		o = 0;
	}
	if(rcnt <= 0 && addr < 0)
		return -1;
	length = 0;
	dlen = GLONG(d->length);
	if(rcnt > 0)
		length = offset+rcnt;
	else if(dp->addr && dp->clust){
		c = bp->clustsize*bp->sectsize;
		if(dp->iclust > (dlen+c-1)/c)
			length = c*dp->iclust;
	}
	if(length > dlen)
		PLONG(d->length, length);
	puttime(d, 0);
	dp->p->flags |= BMOD;
	return rcnt;
}

int
truncfile(Xfile *f, long length)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	long clust, next, n;

	mlock(bp);
	clust = getstart(f->xf, d);
	n = length;
	if(n <= 0)
		putstart(f->xf, d, 0);
	else
		n -= bp->sectsize;
	while(clust > 0){
		next = getfat(xf, clust);
		if(n <= 0)
			putfat(xf, clust, 0);
		else
			n -= bp->clustsize*bp->sectsize;
		clust = next;
	}
	unmlock(bp);
	PLONG(d->length, length);
	dp->iclust = 0;
	dp->clust = 0;
	dp->p->flags |= BMOD;
	return 0;
}

void
putdir(Dosdir *d, Dir *dp)
{
	if(dp->mode != ~0){
		if(dp->mode & 2)
			d->attr &= ~DRONLY;
		else
			d->attr |= DRONLY;
		if(dp->mode & DMEXCL)
			d->attr |= DSYSTEM;
		else
			d->attr &= ~DSYSTEM;
	}
	if(dp->mtime != ~0)
		puttime(d, dp->mtime);
}

/*
 * should extend this to deal with
 * creation and access dates
 */
void
getdir(Xfs *xfs, Dir *dp, Dosdir *d, int addr, int offset)
{
	if(d == nil || addr == 0)
		panic("getdir on root");
	dp->type = 0;
	dp->dev = 0;
	getname(dp->name, d);

	dp->qid.path = addr*(Sectorsize/DOSDIRSIZE) +
			offset/DOSDIRSIZE;
	dp->qid.vers = 0;

	if(d->attr & DRONLY)
		dp->mode = 0444;
	else
		dp->mode = 0666;
	dp->atime = gtime(d);
	dp->mtime = dp->atime;
	dp->qid.type = QTFILE;
	if(d->attr & DDIR){
		dp->qid.type = QTDIR;
		dp->mode |= DMDIR|0111;
		dp->length = 0;
	}else
		dp->length = GLONG(d->length);
	if(d->attr & DSYSTEM){
		dp->mode |= DMEXCL;
		if(iscontig(xfs, d))
			dp->mode |= DMAPPEND;
	}

	dp->uid = "bill";
	dp->muid = "bill";
	dp->gid = "trog";
}

void
getname(char *p, Dosdir *d)
{
	int c, i;

	for(i=0; i<8; i++){
		c = d->name[i];
		if(c == '\0' || c == ' ')
			break;
		if(i == 0 && c == 0x05)
			c = 0xe5;
		*p++ = c;
	}
	for(i=0; i<3; i++){
		c = d->ext[i];
		if(c == '\0' || c == ' ')
			break;
		if(i == 0)
			*p++ = '.';
		*p++ = c;
	}
	*p = 0;
}

static char*
getnamerunes(char *dst, uchar *buf, int step)
{
	int i;
	Rune r;
	char dbuf[DOSRUNE * UTFmax + 1], *d;

	d = dbuf;
	r = 1;
	for(i = 1; r && i < 11; i += 2){
		r = buf[i] | (buf[i+1] << 8);
		d += runetochar(d, &r);
	}
	for(i = 14; r && i < 26; i += 2){
		r = buf[i] | (buf[i+1] << 8);
		d += runetochar(d, &r);
	}
	for(i = 28; r && i < 32; i += 2){
		r = buf[i] | (buf[i+1] << 8);
		d += runetochar(d, &r);
	}

	if(step == 1)
		dst -= d - dbuf;

	memmove(dst, dbuf, d - dbuf);

	if(step == -1){
		dst += d - dbuf;
		*dst = '\0';
	}

	return dst;
}

char*
getnamesect(char *dbuf, char *d, uchar *buf, int *islong, int *sum, int step)
{
	/*
	 * validation checks to make sure we're
	 * making up a consistent name
	 */
	if(buf[11] != 0xf || buf[12] != 0){
		*islong = 0;
		return nil;
	}
	if(step == 1){
		if((buf[0] & 0xc0) == 0x40){
			*islong = buf[0] & 0x3f;
			*sum = buf[13];
			d = dbuf + DOSNAMELEN;
			*--d = '\0';
		}else if(*islong && *islong == buf[0] + 1 && *sum == buf[13]){
			*islong = buf[0];
		}else{
			*islong = 0;
			return nil;
		}
	}else{
		if(*islong + 1 == (buf[0] & 0xbf) && *sum == buf[13]){
			*islong = buf[0] & 0x3f;
			if(buf[0] & 0x40)
				*sum = -1;
		}else{
			*islong = 0;
			*sum = -1;
			return nil;
		}
	}
	if(*islong > 20){
		*islong = 0;
		*sum = -1;
		return nil;
	}

	return getnamerunes(d, buf, step);
}

void
putname(char *p, Dosdir *d)
{
	int i;

	memset(d->name, ' ', sizeof d->name+sizeof d->ext);
	for(i=0; i<sizeof d->name; i++){
		if(*p == 0 || *p == '.')
			break;
		d->name[i] = toupper(*p++);
	}
	p = strrchr(p, '.');
	if(p){
		for(i=0; i<sizeof d->ext; i++){
			if(*++p == 0)
				break;
			d->ext[i] = toupper(*p);
		}
	}
}

static void
putnamesect(uchar *slot, Rune *longname, int curslot, int first, int sum)
{
	Rune r;
	Dosdir ds;
	int i, j;

	memset(&ds, 0xff, sizeof ds);
	ds.attr = 0xf;
	ds.reserved[0] = 0;
	ds.reserved[1] = sum;
	ds.start[0] = 0;
	ds.start[1] = 0;
	if(first)
		ds.name[0] = 0x40 | curslot;
	else 
		ds.name[0] = curslot;
	memmove(slot, &ds, sizeof ds);

	j = (curslot-1) * DOSRUNE;

	for(i = 1; i < 11; i += 2){
		r = longname[j++];
		slot[i] = r;
		slot[i+1] = r >> 8;
		if(r == 0)
			return;
	}
	for(i = 14; i < 26; i += 2){
		r = longname[j++];
		slot[i] = r;
		slot[i+1] = r >> 8;
		if(r == 0)
			return;
	}
	for(i = 28; i < 32; i += 2){
		r = longname[j++];
		slot[i] = r;
		slot[i+1] = r >> 8;
		if(r == 0)
			return;
	}
}

int
aliassum(Dosdir *d)
{
	int i, sum;

	if(d == nil)
		return -1;
	sum = 0;
	for(i = 0; i < 11; i++)
		sum = (((sum&1)<<7) | ((sum&0xfe)>>1)) + d->name[i];
	return sum & 0xff;
}

int
putlongname(Xfs *xf, Dosptr *ndp, char *name, char sname[13])
{
	Dosbpb *bp;
	Dosdir tmpd;
	Rune longname[DOSNAMELEN+1];
	int i, first, sum, nds, len;

	/* calculate checksum */
	putname(sname, &tmpd);
	sum = aliassum(&tmpd);

	bp = xf->ptr;
	first = 1;
	len = utftorunes(longname, name, DOSNAMELEN);
	if(chatty){
		chat("utftorunes %s =", name);
		for(i=0; i<len; i++)
			chat(" %.4X", longname[i]);
		chat("\n");
	}
	for(nds = (len + DOSRUNE-1) / DOSRUNE; nds > 0; nds--){
		putnamesect(&ndp->p->iobuf[ndp->offset], longname, nds, first, sum);
		first = 0;
		ndp->offset += 32;
		if(ndp->offset == bp->sectsize){
			chat("long name moving over sector boundary\n");
			ndp->p->flags |= BMOD;
			putsect(ndp->p);
			ndp->p = nil;

			/*
			 * switch to the next cluster for a long entry
			 * naddr should be set up correctly by searchdir
			 */
			ndp->prevaddr = ndp->addr;
			ndp->addr = ndp->naddr;
			ndp->naddr = -1;
			if(ndp->addr == -1)
				return -1;
			ndp->p = getsect(xf, ndp->addr);
			if(ndp->p == nil)
				return -1;
			ndp->offset = 0;
			ndp->d = (Dosdir *)&ndp->p->iobuf[ndp->offset];
		}
	}
	return 0;
}

long
getfat(Xfs *xf, int n)
{
	Dosbpb *bp = xf->ptr;
	Iosect *p;
	ulong k, sect;
	int o, fb;

	if(n < FATRESRV || n >= bp->fatclusters)
		return -1;
	fb = bp->fatbits;
	k = (fb * n) >> 3;
	if(k >= bp->fatsize*bp->sectsize)
		panic("getfat");
	sect = k/bp->sectsize + bp->fataddr;
	o = k%bp->sectsize;
	p = getsect(xf, sect);
	if(p == nil)
		return -1;
	k = p->iobuf[o++];
	if(o >= bp->sectsize){
		putsect(p);
		p = getsect(xf, sect+1);
		if(p == nil)
			return -1;
		o = 0;
	}
	k |= p->iobuf[o++]<<8;
	if(fb == 32){
		/* fat32 is really fat28 */
		k |= p->iobuf[o++] << 16;
		k |= (p->iobuf[o] & 0x0f) << 24;
		fb = 28;
	}
	putsect(p);
	if(fb == 12){
		if(n&1)
			k >>= 4;
		else
			k &= 0xfff;
	}
	if(chatty > 1)
		chat("fat(%#x)=%#lx...", n, k);

	/*
	 * This is a very strange check for out of range.
	 * As a concrete example, for a 16-bit FAT,
	 * FFF8 through FFFF all signify ``end of cluster chain.''
	 * This generalizes to other-sized FATs.
	 */
	if(k >= (1 << fb) - 8)
		return -1;

	return k;
}

void
putfat(Xfs *xf, int n, ulong val)
{
	Fatinfo *fi;
	Dosbpb *bp;
	Iosect *p;
	ulong k, sect, esect;
	int o;

	bp = xf->ptr;
	if(n < FATRESRV || n >= bp->fatclusters)
		panic("putfat n=%d", n);
	k = (bp->fatbits * n) >> 3;
	if(k >= bp->fatsize*bp->sectsize)
		panic("putfat");
	sect = k/bp->sectsize + bp->fataddr;
	esect = sect + bp->nfats * bp->fatsize;
	for(; sect<esect; sect+=bp->fatsize){
		o = k%bp->sectsize;
		p = getsect(xf, sect);
		if(p == nil)
			continue;
		switch(bp->fatbits){
		case 12:
			if(n&1){
				p->iobuf[o] &= 0x0f;
				p->iobuf[o++] |= val<<4;
				if(o >= bp->sectsize){
					p->flags |= BMOD;
					putsect(p);
					p = getsect(xf, sect+1);
					if(p == nil)
						continue;
					o = 0;
				}
				p->iobuf[o] = val>>4;
			}else{
				p->iobuf[o++] = val;
				if(o >= bp->sectsize){
					p->flags |= BMOD;
					putsect(p);
					p = getsect(xf, sect+1);
					if(p == nil)
						continue;
					o = 0;
				}
				p->iobuf[o] &= 0xf0;
				p->iobuf[o] |= (val>>8) & 0x0f;
			}
			break;
		case 16:
			p->iobuf[o++] = val;
			p->iobuf[o] = val>>8;
			break;
		case 32:	/* fat32 is really fat28 */
			p->iobuf[o++] = val;
			p->iobuf[o++] = val>>8;
			p->iobuf[o++] = val>>16;
			p->iobuf[o] = (p->iobuf[o] & 0xf0) | ((val>>24) & 0x0f);
			break;
		default:
			panic("putfat fatbits");
		}
		p->flags |= BMOD;
		putsect(p);
	}

	if(val == 0)
		bp->freeclusters++;
	else
		bp->freeclusters--;

	if(bp->fatinfo){
		p = getsect(xf, bp->fatinfo);
		if(p != nil){
			fi = (Fatinfo*)p->iobuf;
			PLONG(fi->nextfree, bp->freeptr);
			PLONG(fi->freeclust, bp->freeclusters);
			p->flags |= BMOD;
			putsect(p);
		}
	}
}

/*
 * Contiguous falloc; if we can, use lastclust+1.
 * Otherwise, move the file to get some space.
 * If there just isn't enough contiguous space
 * anywhere on disk, fail.
 */
int
cfalloc(Xfile *f)
{
	int l;

	if((l=makecontig(f, 8)) >= 0)
		return l;
	return makecontig(f, 1);
}

/*
 * Check whether a file is contiguous.
 */
int
iscontig(Xfs *xf, Dosdir *d)
{
	long clust, next;

	clust = getstart(xf, d);
	if(clust <= 0)
		return 1;

	for(;;) {
		next = getfat(xf, clust);
		if(next < 0)
			return 1;
		if(next != clust+1)
			return 0;
		clust = next;
	}
}

/*
 * Make a file contiguous, with nextra clusters of 
 * free space after it for later expansion.
 * Return the number of the first new cluster.
 */
int
makecontig(Xfile *f, int nextra)
{
	Dosbpb *bp;
	Dosdir *d;
	Dosptr *dp;
	Xfs *xf;
	Iosect *wp, *rp;
	long clust, next, last, start, rclust, wclust, eclust, ostart;
	int isok, i, n, nclust, nrun, rs, ws;

	xf = f->xf;
	bp = xf->ptr;
	dp = f->ptr;
	d = dp->d;

	isok = 1;
	nclust = 0;
	clust = fileclust(f, 0, 0);
	chat("clust %#lux", clust);
	if(clust != -1) {
		for(;;) {
			nclust++;
			chat(".");
			next = getfat(xf, clust);
			if(next <= 0)
				break;
			if(next != clust+1)
				isok = 0;
			clust = next;
		}
	}
	chat("nclust %d\n", nclust);

	if(isok && clust != -1) {
		eclust = clust+1;	/* eclust = first cluster past file */
		assert(eclust == fileclust(f, 0, 0)+nclust);
		for(i=0; i<nextra; i++)
			if(getfat(xf, eclust+i) != 0)
				break;
		if(i == nextra) {	/* they were all free */
			chat("eclust=%#lx, getfat eclust-1 = %#lux\n", eclust, getfat(xf, eclust-1));
			assert(getfat(xf, eclust-1) == 0xffffffff);
			putfat(xf, eclust-1, eclust);
			putfat(xf, eclust, 0xffffffff);
			bp->freeptr = clust+1;	/* to help keep the blocks free */
			return eclust;
		}
	}

	/* need to search for nclust+nextra contiguous free blocks */
	last = -1;
	n = bp->freeptr;
	nrun = 0;
	for(;;){
		if(getfat(xf, n) == 0) {
			if(last+1 == n)
				nrun++;
			else
				nrun = 1;
			if(nrun >= nclust+nextra)
				break;
			last = n;
		}
		if(++n >= bp->fatclusters)
			n = FATRESRV;
		if(n == bp->freeptr) {
			errno = Econtig;
			return -1;
		}
	}
	bp->freeptr = n+1;

	/* copy old data over */
	start = n+1 - nrun;

	/* sanity check */
	for(i=0; i<nclust+nextra; i++)
		assert(getfat(xf, start+i) == 0);

	chat("relocate chain %lux -> 0x%lux len %d\n", fileclust(f, 0, 0), start, nclust);

	wclust = start;
	for(rclust = fileclust(f, 0, 0); rclust > 0; rclust = next){
		rs = clust2sect(bp, rclust);
		ws = clust2sect(bp, wclust);
		for(i=0; i<bp->clustsize; i++, rs++, ws++){
			rp = getsect(xf, rs);
			if(rp == nil)
				return -1;
			wp = getosect(xf, ws);
			assert(wp != nil);
			memmove(wp->iobuf, rp->iobuf, bp->sectsize);
			wp->flags = BMOD;
			putsect(rp);
			putsect(wp);
		}
		chat("move cluster %#lx -> %#lx...", rclust, wclust);
		next = getfat(xf, rclust);
		putfat(xf, wclust, wclust+1);
		wclust++;
	}

	/* now wclust points at the first new cluster; chain it in */
	chat("wclust 0x%lux start 0x%lux (fat->0x%lux) nclust %d\n", wclust, start, getfat(xf, start), nclust);
	assert(wclust == start+nclust);
	putfat(xf, wclust, 0xffffffff);	/* end of file */

	/* update directory entry to point at new start */
	ostart = fileclust(f, 0, 0);
	putstart(xf, d, start);

	/* check our work */
	i = 0;
	clust = fileclust(f, 0, 0);
	if(clust != -1) {
		for(;;) {
			i++;
			next = getfat(xf, clust);
			if(next <= 0)
				break;
			assert(next == clust+1);
			clust = next;
		}
	}
	chat("chain check: len %d\n", i);
	assert(i == nclust+1);

	/* succeeded; remove old chain. */
	for(rclust = ostart; rclust > 0; rclust = next){
		next = getfat(xf, rclust);
		putfat(xf, rclust, 0);	/* free cluster */
	}

	return start+nclust;
}	

int
falloc(Xfs *xf)
{
	Dosbpb *bp = xf->ptr;
	Iosect *p;
	int n, i, k;

	n = bp->freeptr;
	for(;;){
		if(getfat(xf, n) == 0)
			break;
		if(++n >= bp->fatclusters)
			n = FATRESRV;
		if(n == bp->freeptr)
			return -1;
	}
	bp->freeptr = n+1;
	if(bp->freeptr >= bp->fatclusters)
		bp->freeptr = FATRESRV;
	putfat(xf, n, 0xffffffff);
	k = clust2sect(bp, n);
	for(i=0; i<bp->clustsize; i++){
		p = getosect(xf, k+i);
		memset(p->iobuf, 0, bp->sectsize);
		p->flags = BMOD;
		putsect(p);
	}
	return n;
}

void
ffree(Xfs *xf, long start)
{
	putfat(xf, start, 0);
}

long
clust2sect(Dosbpb *bp, long clust)
{
	return bp->dataaddr + (clust - FATRESRV) * bp->clustsize;
}

long
sect2clust(Dosbpb *bp, long sect)
{
	long c;

	c = (sect - bp->dataaddr) / bp->clustsize + FATRESRV;
	assert(sect == clust2sect(bp, c));
	return c;
}

void
puttime(Dosdir *d, long s)
{
	Tm *t;
	ushort x;

	if(s == 0)
		s = time(0);
	t = localtime(s);

	x = (t->hour<<11) | (t->min<<5) | (t->sec>>1);
	PSHORT(d->time, x);
	x = ((t->year-80)<<9) | ((t->mon+1)<<5) | t->mday;
	PSHORT(d->date, x);
}

long
gtime(Dosdir *dp)
{
	Tm tm;
	int i;

	i = GSHORT(dp->time);
	tm.hour = i >> 11;
	tm.min = (i >> 5) & 63;
	tm.sec = (i & 31) << 1;
	i = GSHORT(dp->date);
	tm.year = 80 + (i >> 9);
	tm.mon = ((i >> 5) & 15) - 1;
	tm.mday = i & 31;
	tm.zone[0] = '\0';
	tm.tzoff = 0;
	tm.yday = 0;

	return tm2sec(&tm);
}

/*
 * structure dumps for debugging
 */
void
bootdump(int fd, Dosboot *b)
{
	Biobuf bp;

	Binit(&bp, fd, OWRITE);
	Bprint(&bp, "magic: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		b->magic[0], b->magic[1], b->magic[2]);
	Bprint(&bp, "version: \"%8.8s\"\n", (char*)b->version);
	Bprint(&bp, "sectsize: %d\n", GSHORT(b->sectsize));
	Bprint(&bp, "clustsize: %d\n", b->clustsize);
	Bprint(&bp, "nresrv: %d\n", GSHORT(b->nresrv));
	Bprint(&bp, "nfats: %d\n", b->nfats);
	Bprint(&bp, "rootsize: %d\n", GSHORT(b->rootsize));
	Bprint(&bp, "volsize: %d\n", GSHORT(b->volsize));
	Bprint(&bp, "mediadesc: 0x%2.2x\n", b->mediadesc);
	Bprint(&bp, "fatsize: %d\n", GSHORT(b->fatsize));
	Bprint(&bp, "trksize: %d\n", GSHORT(b->trksize));
	Bprint(&bp, "nheads: %d\n", GSHORT(b->nheads));
	Bprint(&bp, "nhidden: %ld\n", GLONG(b->nhidden));
	Bprint(&bp, "bigvolsize: %ld\n", GLONG(b->bigvolsize));
	Bprint(&bp, "driveno: %d\n", b->driveno);
	Bprint(&bp, "reserved0: 0x%2.2x\n", b->reserved0);
	Bprint(&bp, "bootsig: 0x%2.2x\n", b->bootsig);
	Bprint(&bp, "volid: 0x%8.8lux\n", GLONG(b->volid));
	Bprint(&bp, "label: \"%11.11s\"\n", (char*)b->label);
	Bterm(&bp);
}

void
bootdump32(int fd, Dosboot32 *b)
{
	Biobuf bp;

	Binit(&bp, fd, OWRITE);
	Bprint(&bp, "magic: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		b->magic[0], b->magic[1], b->magic[2]);
	Bprint(&bp, "version: \"%8.8s\"\n", (char*)b->version);
	Bprint(&bp, "sectsize: %d\n", GSHORT(b->sectsize));
	Bprint(&bp, "clustsize: %d\n", b->clustsize);
	Bprint(&bp, "nresrv: %d\n", GSHORT(b->nresrv));
	Bprint(&bp, "nfats: %d\n", b->nfats);
	Bprint(&bp, "rootsize: %d\n", GSHORT(b->rootsize));
	Bprint(&bp, "volsize: %d\n", GSHORT(b->volsize));
	Bprint(&bp, "mediadesc: 0x%2.2x\n", b->mediadesc);
	Bprint(&bp, "fatsize: %d\n", GSHORT(b->fatsize));
	Bprint(&bp, "trksize: %d\n", GSHORT(b->trksize));
	Bprint(&bp, "nheads: %d\n", GSHORT(b->nheads));
	Bprint(&bp, "nhidden: %ld\n", GLONG(b->nhidden));
	Bprint(&bp, "bigvolsize: %ld\n", GLONG(b->bigvolsize));
	Bprint(&bp, "fatsize32: %ld\n", GLONG(b->fatsize32));
	Bprint(&bp, "extflags: %d\n", GSHORT(b->extflags));
	Bprint(&bp, "version: %d\n", GSHORT(b->version1));
	Bprint(&bp, "rootstart: %ld\n", GLONG(b->rootstart));
	Bprint(&bp, "infospec: %d\n", GSHORT(b->infospec));
	Bprint(&bp, "backupboot: %d\n", GSHORT(b->backupboot));
	Bprint(&bp, "reserved: %d %d %d %d %d %d %d %d %d %d %d %d\n",
		b->reserved[0], b->reserved[1], b->reserved[2], b->reserved[3],
		b->reserved[4], b->reserved[5], b->reserved[6], b->reserved[7],
		b->reserved[8], b->reserved[9], b->reserved[10], b->reserved[11]);
	Bterm(&bp);
}

void
bootsecdump32(int fd, Xfs *xf, Dosboot32 *b32)
{
	Fatinfo *fi;
	Iosect *p1;
	int fisec, bsec, res;

	fprint(fd, "\nfat32\n");
	bootdump32(fd, b32);
	res = GSHORT(b32->nresrv);
	bsec = GSHORT(b32->backupboot);
	if(bsec < res && bsec != 0){
		p1 = getsect(xf, bsec);
		if(p1 == nil)
			fprint(fd, "\ncouldn't get backup boot sector: %r\n");
		else{
			fprint(fd, "\nbackup boot\n");
			bootdump32(fd, (Dosboot32*)p1->iobuf);
			putsect(p1);
		}
	}else if(bsec != 0xffff)
		fprint(fd, "bad backup boot sector: %d reserved %d\n", bsec, res);
	fisec = GSHORT(b32->infospec);
	if(fisec < res && fisec != 0){
		p1 = getsect(xf, fisec);
		if(p1 == nil)
			fprint(fd, "\ncouldn't get fat info sector: %r\n");
		else{
			fprint(fd, "\nfat info %d\n", fisec);
			fi = (Fatinfo*)p1->iobuf;
			fprint(fd, "sig1: 0x%lux sb 0x%lux\n", GLONG(fi->sig1), FATINFOSIG1);
			fprint(fd, "sig: 0x%lux sb 0x%lux\n", GLONG(fi->sig), FATINFOSIG);
			fprint(fd, "freeclust: %lud\n", GLONG(fi->freeclust));
			fprint(fd, "nextfree: %lud\n", GLONG(fi->nextfree));
			fprint(fd, "reserved: %lud %lud %lud\n", GLONG(fi->resrv), GLONG(fi->resrv+4), GLONG(fi->resrv+8));
			putsect(p1);
		}
	}else if(fisec != 0xffff)
		fprint(2, "bad fat info sector: %d reserved %d\n", bsec, res);
}

void
dirdump(void *vdbuf)
{
	static char attrchar[] = "rhsvda67";
	Dosdir *d;
	char *name, namebuf[DOSNAMELEN];
	char buf[128], *s, *ebuf;
	uchar *dbuf;
	int i;

	if(!chatty)
		return;

	d = vdbuf;

	ebuf = buf + sizeof(buf);
	if(d->attr == 0xf){
		dbuf = vdbuf;
		name = namebuf + DOSNAMELEN;
		*--name = '\0';
		name = getnamerunes(name, dbuf, 1);
		seprint(buf, ebuf, "\"%s\" %2.2x %2.2ux %2.2ux %d", name, dbuf[0], dbuf[12], dbuf[13], GSHORT(d->start));
	}else{
		s = seprint(buf, ebuf, "\"%.8s.%.3s\" ", (char*)d->name, (char*)d->ext);
		for(i=7; i>=0; i--)
			*s++ = d->attr&(1<<i) ? attrchar[i] : '-';
	
		i = GSHORT(d->time);
		s = seprint(s, ebuf, " %2.2d:%2.2d:%2.2d", i>>11, (i>>5)&63, (i&31)<<1);
		i = GSHORT(d->date);
		s = seprint(s, ebuf, " %2.2d.%2.2d.%2.2d", 80+(i>>9), (i>>5)&15, i&31);
	
		i = GSHORT(d->ctime);
		s = seprint(s, ebuf, " %2.2d:%2.2d:%2.2d", i>>11, (i>>5)&63, (i&31)<<1);
		i = GSHORT(d->cdate);
		s = seprint(s, ebuf, " %2.2d.%2.2d.%2.2d", 80+(i>>9), (i>>5)&15, i&31);
	
		i = GSHORT(d->adate);
		s = seprint(s, ebuf, " %2.2d.%2.2d.%2.2d", 80+(i>>9), (i>>5)&15, i&31);

		seprint(s, ebuf, " %d %d", GSHORT(d->start), GSHORT(d->length));
	}
	chat("%s\n", buf);
}

int
cistrcmp(char *s1, char *s2)
{
	int c1, c2;

	while(*s1){
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';

		if(c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';

		if(c1 != c2)
			return c1 - c2;
	}
	return -*s2;
}

int
utftorunes(Rune *rr, char *s, int n)
{
	Rune *r, *re;
	int c;

	r = rr;
	re = r + n - 1;
	while(c = (uchar)*s){
		if(c < Runeself){
			*r = c;
			s++;
		}else
			s += chartorune(r, s);
		r++;
		if(r >= re)
			break;
	}
	*r = 0;
	return r - rr;
}
