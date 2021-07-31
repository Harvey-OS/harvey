#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

int
dosfs(Xfs *xf)
{
	Iosect *p = 0;
	Dosboot *b = 0;
	Dosbpb *bp;
	Dospart *dp;
	int i; long offset;

	for(i=2; i>0; i--){
		p = getsect(xf, 0);
		if(p == 0)
			return -1;
		b = (Dosboot *)p->iobuf;
		if(b->magic[0] == 0xe9)
			break;
		if(b->magic[0] == 0xeb && b->magic[2] == 0x90)
			break;
		if(i < 2 || p->iobuf[0x1fe] != 0x55 || p->iobuf[0x1ff] != 0xaa){
			i = 0;
			break;
		}
		dp = (Dospart *)&p->iobuf[0x1be];
		chat("0x%2.2ux (%d,%d) 0x%2.2ux (%d,%d) %d %d...",
			dp->active, dp->hstart, GSHORT(dp->cylstart),
			dp->type, dp->hend, GSHORT(dp->cylend),
			GLONG(dp->start), GLONG(dp->length));
		offset = GLONG(dp->start)*Sectorsize;
		putsect(p);
		purgebuf(xf);
		xf->offset = offset;
	}
	if(i <= 0){
		chat("bad magic...");
		putsect(p);
		return -1;
	}
	if(chatty)
		bootdump(2, b);

	bp = malloc(sizeof(Dosbpb));
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

	bp->fataddr = bp->nresrv;
	bp->rootaddr = bp->fataddr + bp->nfats*bp->fatsize;
	i = bp->rootsize*sizeof(Dosdir) + bp->sectsize-1;
	i /= bp->sectsize;
	bp->dataaddr = bp->rootaddr + i;
	bp->fatclusters = (bp->volsize - bp->dataaddr)/bp->clustsize;
	if(bp->fatclusters < 4087)
		bp->fatbits = 12;
	else
		bp->fatbits = 16;
	bp->freeptr = 2;
	chat("fatbits=%d (%d clusters)...", bp->fatbits, bp->fatclusters);
	for(i=0; i<b->nfats; i++)
		chat("fat %d: %d...", i, bp->fataddr+i*bp->fatsize);
	chat("root: %d...", bp->rootaddr);
	chat("data: %d...", bp->dataaddr);
	putsect(p);
	return 0;
}

int
getfile(Xfile *f)
{
	Dosptr *dp = f->ptr;
	Iosect *p;
	Dosdir *d;

	if(dp->p)
		panic("getfile");
	p = getsect(f->xf, dp->addr);
	if(p == 0)
		return -1;
	if(dp->addr != 0){
		d = (Dosdir *)&p->iobuf[dp->offset];
		if((f->qid.path & ~CHDIR) != GSHORT(d->start)){
			chat("qid mismatch f=0x%x d=0x%x...",
				f->qid.path, GSHORT(d->start));
			putsect(p);
			errno = Enonexist;
			return -1;
		}
		dp->d = d;
	}
	dp->p = p;
	return 0;
}

void
putfile(Xfile *f)
{
	Dosptr *dp = f->ptr;

	if(!dp->p)
		panic("putfile");
	putsect(dp->p);
	dp->p = 0;
	dp->d = 0;
}

int
fileaddr(Xfile *f, int isect, int cflag)
{
	Dosbpb *bp = f->xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	int clust, iclust, nskip, next = 0;

	if(dp->addr == 0){
		if(isect*bp->sectsize >= bp->rootsize*sizeof(Dosdir))
			return -1;
		return bp->rootaddr + isect;
	}
	iclust = isect/bp->clustsize;
	if(dp->clust == 0 || iclust < dp->iclust){
		clust = GSHORT(d->start);
		nskip = iclust;
	}else{
		clust = dp->clust;
		nskip = iclust - dp->iclust;
	}
	if(clust <= 0)
		return -1;
	if(nskip > 0){
		lock(bp);
		while(--nskip >= 0){
			next = getfat(f->xf, clust);
			if(next > 0){
				clust = next;
				continue;
			}else if(!cflag)
				break;
			next = falloc(f->xf);
			if(next < 0)
				break;
			putfat(f->xf, clust, next);
			clust = next;
		}
		unlock(bp);
		if(next <= 0)
			return -1;
		dp->clust = clust;
		dp->iclust = iclust;
	}
	if(chatty > 1)
		chat("clust(0x%x)=0x%x...", iclust, clust);
	return bp->dataaddr + (clust-2)*bp->clustsize + isect%bp->clustsize;
}

int
searchdir(Xfile *f, char *name, Dosptr *dp, int cflag)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	char uname[16], buf[16];
	int isect, addr, o;
	int addr1 = 0, o1 = 0;
	Iosect *p;
	Dosdir *d, tmpdir;

	putname(name, &tmpdir);
	getname(uname, &tmpdir);
	memset(dp, 0, sizeof(Dosptr));
	dp->paddr = ((Dosptr *)f->ptr)->addr;
	dp->poffset = ((Dosptr *)f->ptr)->offset;

	for(isect=0;; isect++){
		addr = fileaddr(f, isect, cflag);
		if(addr < 0)
			break;
		p = getsect(xf, addr);
		if(p == 0)
			break;
		for(o=0; o<bp->sectsize; o+=sizeof(Dosdir)){
			d = (Dosdir *)&p->iobuf[o];
			if(d->name[0] == 0x00){
				chat("end dir(0)...");
				putsect(p);
				if(!cflag)
					return -1;
				if(addr1 == 0){
					addr1 = addr;
					o1 = o;
				}
				dp->addr = addr1;
				dp->offset = o1;
				return 0;
			}
			if(d->name[0] == 0xe5){
				if(addr1 == 0){
					addr1 = addr;
					o1 = o;
				}
				continue;
			}
			/*dirdump(d);*/
			if(d->attr&DVLABEL)
				continue;
			getname(buf, d);
			if(strcmp(buf, uname) != 0)
				continue;
			dirdump(d);
			if(cflag){
				putsect(p);
				return -1;
			}
			dp->addr = addr;
			dp->offset = o;
			dp->p = p;
			dp->d = d;
			return 0;
		}
		putsect(p);
	}
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
		for(o=0; o<bp->sectsize; o+=sizeof(Dosdir)){
			d = (Dosdir *)&p->iobuf[o];
			if(d->name[0] == 0x00){
				putsect(p);
				return 0;
			}
			if(d->name[0] == 0xe5)
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
readdir(Xfile *f, char *buf, long offset, long count)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dir dir;
	int isect, addr, o;
	Iosect *p;
	Dosdir *d;
	long rcnt = 0;

	if(count <= 0)
		return 0;
	for(isect=0;; isect++){
		addr = fileaddr(f, isect, 0);
		if(addr < 0)
			break;
		p = getsect(xf, addr);
		if(p == 0)
			return -1;
		for(o=0; o<bp->sectsize; o+=sizeof(Dosdir)){
			d = (Dosdir *)&p->iobuf[o];
			if(d->name[0] == 0x00){
				putsect(p);
				return rcnt;
			}
			if(d->name[0] == 0xe5)
				continue;
			/*dirdump(d);*/
			if(d->name[0] == '.'){
				if(d->name[1] == ' ' || d->name[1] == 0)
					continue;
				if(d->name[1] == '.' &&
				  (d->name[2] == ' ' || d->name[2] == 0))
					continue;
			}
			if(d->attr&DVLABEL)
				continue;
			if(offset > 0){
				offset -= DIRLEN;
				continue;
			}
			getdir(&dir, d);
			rcnt += convD2M(&dir, &buf[rcnt]);
			if(rcnt >= count){
				putsect(p);
				return rcnt;
			}
		}
		putsect(p);
	}
	return rcnt;
}

int
walkup(Xfile *f, Dosptr *ndp)
{
	Dosbpb *bp = f->xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *xd;
	Iosect *p;
	int k, o, so, start, pstart, ppstart;

	memset(ndp, 0, sizeof(Dosptr));
	ndp->addr = dp->paddr;
	ndp->offset = dp->poffset;
	chat("walkup: paddr=0x%x...", dp->paddr);
	if(dp->paddr == 0)
		return 0;
	p = getsect(f->xf, dp->paddr);
	if(p == 0)
		goto error;
	xd = (Dosdir *)&p->iobuf[dp->poffset];
	dirdump(xd);
	start = GSHORT(xd->start);
	chat("start=0x%x...", start);
	if(start == 0)
		goto error;
	putsect(p);
	p = getsect(f->xf, bp->dataaddr + (start-2)*bp->clustsize);
	if(p == 0)
		goto error;
	xd = (Dosdir *)p->iobuf;
	dirdump(xd);
	if(xd->name[0]!='.' || xd->name[1]!=' ' || start!=GSHORT(xd->start))
		goto error;
	xd++;
	dirdump(xd);
	if(xd->name[0] != '.' || xd->name[1] != '.')
		goto error;
	pstart = GSHORT(xd->start);
	putsect(p);
	if(pstart == 0)
		return 0;
	p = getsect(f->xf, bp->dataaddr + (pstart-2)*bp->clustsize);
	if(p == 0)
		goto error;
	xd = (Dosdir *)p->iobuf;
	if(xd->name[0]!='.' || xd->name[1]!=' ' || pstart!=GSHORT(xd->start))
		goto error;
	xd++;
	if(xd->name[0] != '.' || xd->name[1] != '.')
		goto error;
	ppstart = GSHORT(xd->start);
	putsect(p);
	k = ppstart ? (bp->dataaddr + (ppstart-2)*bp->clustsize) : bp->rootaddr;
	p = getsect(f->xf, k);
	if(p == 0)
		goto error;
	xd = (Dosdir *)p->iobuf;
	if(ppstart)
		if(xd->name[0]!='.' || xd->name[1]!=' ' || ppstart!=GSHORT(xd->start))
			goto error;
	for(so=1;; so++){
		for(o=0; o<bp->sectsize; o+=sizeof(Dosdir)){
			xd = (Dosdir *)&p->iobuf[o];
			if(xd->name[0] == 0x00)
				goto error;
			if(xd->name[0] == 0xe5)
				continue;
			if(GSHORT(xd->start) == pstart)
				goto out;
		}
		if(ppstart){
			lock(bp);
			ppstart = getfat(f->xf, ppstart);
			unlock(bp);
			if(ppstart < 0)
				goto error;
			k = bp->dataaddr + (ppstart-2)*bp->clustsize;
		}else{
			if(so*bp->sectsize >= bp->rootsize*sizeof(Dosdir))
				goto error;
			k = bp->rootaddr + so;
		}
		putsect(p);
		p = getsect(f->xf, k);
		if(p == 0)
			goto error;
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
readfile(Xfile *f, uchar *buf, long offset, long count)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	int isect, addr, o, c;
	Iosect *p;
	long length = GLONG(d->length), rcnt = 0;

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
writefile(Xfile *f, uchar *buf, long offset, long count)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	int isect, addr, o, c;
	Iosect *p;
	long length, rcnt = 0;

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
			if(p == 0)
				return -1;
			p->flags = 0;
		}else{
			p = getsect(xf, addr);
			if(p == 0)
				return -1;
		}
		memmove(&p->iobuf[o], &buf[rcnt], c);
		p->flags |= BMOD;
		putsect(p);
		count -= c;
		rcnt += c;
		o = 0;
	}
	length = offset+rcnt;
	if(length > GLONG(d->length)){
		d->length[0] = length;
		d->length[1] = length>>8;
		d->length[2] = length>>16;
		d->length[3] = length>>24;
	}
	puttime(d);
	dp->p->flags |= BMOD;
	return rcnt;
}

int
truncfile(Xfile *f, int totally)
{
	Xfs *xf = f->xf;
	Dosbpb *bp = xf->ptr;
	Dosptr *dp = f->ptr;
	Dosdir *d = dp->d;
	int clust, next;

	lock(bp);
	clust = GSHORT(d->start);
	if(totally){
		d->start[0] = 0;
		d->start[1] = 0;
	}else{
		next = getfat(xf, clust);
		putfat(xf, clust, 0xffff);
		clust = next;
	}
	while(clust >= 0){
		next = getfat(xf, clust);
		putfat(xf, clust, 0);
		clust = next;
	}
	unlock(bp);
	d->length[0] = 0;
	d->length[1] = 0;
	d->length[2] = 0;
	d->length[3] = 0;
	dp->iclust = 0;
	dp->clust = 0;
	dp->p->flags |= BMOD;
	return 0;
}

void
getdir(Dir *dp, Dosdir *d)
{
	char *p;

	memset(dp, 0, sizeof(Dir));
	if(d == 0){
		dp->name[0] = '/';
		dp->qid.path = CHDIR;
		dp->mode = CHDIR|0777;
	}else{
		getname(dp->name, d);
		for(p=dp->name; *p; p++)
			*p = tolower(*p);
		dp->qid.path = GSHORT(d->start);
		if(d->attr & DRONLY)
			dp->mode = 0444;
		else
			dp->mode = 0666;
		dp->atime = gtime(d);
		dp->mtime = dp->atime;
		if(d->attr & DDIR){
			dp->qid.path |= CHDIR;
			dp->mode |= CHDIR|0111;
		}else
			dp->length = GLONG(d->length);
	}
	strcpy(dp->uid, "bill");
	strcpy(dp->gid, "trog");
}

void
getname(char *p, Dosdir *d)
{
	int c, i;

	for(i=0; i<8; i++){
		c = d->name[i];
		if(c == 0 || c == ' ')
			break;
		if(i == 0 && c == 0x05)
			c = 0xe5;
		*p++ = c;
	}
	for(i=0; i<3; i++){
		c = d->ext[i];
		if(c == 0 || c == ' ')
			break;
		if(i == 0)
			*p++ = '.';
		*p++ = c;
	}
	*p = 0;
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

int
getfat(Xfs *xf, int n)
{
	Dosbpb *bp = xf->ptr;
	ulong k = 0, sect; Iosect *p;
	int o;

	if(n < 2 || n >= bp->fatclusters)
		return -1;
	switch(bp->fatbits){
	case 12:
		k = (3*n)/2; break;
	case 16:
		k = 2*n; break;
	}
	if(k >= bp->fatsize*bp->sectsize)
		panic("getfat");
	sect = k/bp->sectsize + bp->fataddr;
	o = k%bp->sectsize;
	p = getsect(xf, sect);
	if(p == 0)
		return -1;
	k = p->iobuf[o++];
	if(o >= bp->sectsize){
		putsect(p);
		p = getsect(xf, sect+1);
		if(p == 0)
			return -1;
		o = 0;
	}
	k |= p->iobuf[o]<<8;
	putsect(p);
	if(bp->fatbits == 12){
		if(n&1)
			k >>= 4;
		else
			k &= 0xfff;
		if(k >= 0xff8)
			k |= 0xf000;
	}
	if(chatty > 1)
		chat("fat(0x%x)=0x%x...", n, k);
	return k < 0xfff8 ? k : -1;
}

void
putfat(Xfs *xf, int n, int val)
{
	Dosbpb *bp = xf->ptr;
	ulong k = 0, sect; Iosect *p;
	int o;

	switch(bp->fatbits){
	case 12:
		k = (3*n)/2; break;
	case 16:
		k = 2*n; break;
	}
	if(k >= bp->fatsize*bp->sectsize)
		panic("putfat");
	sect = k/bp->sectsize + bp->fataddr;
	for(; sect<bp->rootaddr; sect+=bp->fatsize){
		o = k%bp->sectsize;
		p = getsect(xf, sect);
		if(p == 0)
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
					if(p == 0)
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
					if(p == 0)
						continue;
					o = 0;
				}
				p->iobuf[o] &= 0xf0;
				p->iobuf[o] |= (val>>8)&0x0f;
			}
			break;
		case 16:
			p->iobuf[o++] = val;
			p->iobuf[o] = val>>8;
			break;
		}
		p->flags |= BMOD;
		putsect(p);
	}
}

int
falloc(Xfs *xf)
{
	Dosbpb *bp = xf->ptr;
	int n, i, k;
	Iosect *p;

	n = bp->freeptr;
	for(;;){
		if(getfat(xf, n) == 0)
			break;
		if(++n >= bp->fatclusters)
			n = 2;
		if(n == bp->freeptr)
			return -1;
	}
	bp->freeptr = n+1;
	if(bp->freeptr >= bp->fatclusters)
		bp->freeptr = 2;
	putfat(xf, n, 0xffff);
	k = bp->dataaddr + (n-2)*bp->clustsize;
	for(i=0; i<bp->clustsize; i++){
		p = getosect(xf, k+i);
		if(p == 0)
			break;
		memset(p->iobuf, 0, bp->sectsize);
		p->flags = BMOD;
		putsect(p);
	}
	return n;
}

void
bootdump(int fd, Dosboot *b)
{
	Biobuf *bp;

	bp = malloc(sizeof(Biobuf));
	if(bp == 0)
		panic("malloc");
	Binit(bp, fd, OWRITE);
	Bprint(bp, "magic: 0x%2.2x 0x%2.2x 0x%2.2x\n",
		b->magic[0], b->magic[1], b->magic[2]);
	Bprint(bp, "version: \"%8.8s\"\n", b->version);
	Bprint(bp, "sectsize: %d\n", GSHORT(b->sectsize));
	Bprint(bp, "allocsize: %d\n", b->clustsize);
	Bprint(bp, "nresrv: %d\n", GSHORT(b->nresrv));
	Bprint(bp, "nfats: %d\n", b->nfats);
	Bprint(bp, "rootsize: %d\n", GSHORT(b->rootsize));
	Bprint(bp, "volsize: %d\n", GSHORT(b->volsize));
	Bprint(bp, "mediadesc: 0x%2.2x\n", b->mediadesc);
	Bprint(bp, "fatsize: %d\n", GSHORT(b->fatsize));
	Bprint(bp, "trksize: %d\n", GSHORT(b->trksize));
	Bprint(bp, "nheads: %d\n", GSHORT(b->nheads));
	Bprint(bp, "nhidden: %d\n", GLONG(b->nhidden));
	Bprint(bp, "bigvolsize: %d\n", GLONG(b->bigvolsize));
	Bprint(bp, "driveno: %d\n", b->driveno);
	Bprint(bp, "reserved0: 0x%2.2x\n", b->reserved0);
	Bprint(bp, "bootsig: 0x%2.2x\n", b->bootsig);
	Bprint(bp, "volid: 0x%8.8x\n", GLONG(b->volid));
	Bprint(bp, "label: \"%11.11s\"\n", b->label);
	Bclose(bp);
	free(bp);
}

void
puttime(Dosdir *d)
{
	Tm *t = localtime(time(0));
	ushort x;

	x = (t->hour<<11) | (t->min<<5) | (t->sec>>1);
	d->time[0] = x;
	d->time[1] = x>>8;
	x = ((t->year-80)<<9) | ((t->mon+1)<<5) | t->mday;
	d->date[0] = x;
	d->date[1] = x>>8;
}

static char	dmsize[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};
static int
dysize(int y)
{

	if((y%4) == 0)
		return 366;
	return 365;
}
long
gtime(Dosdir *dp)
{
	long t, noonGMT = 12*60*60;
	int i, y, M, d, h, m, s;

	i = GSHORT(dp->time);
	h = i>>11; m = (i>>5)&63; s = (i&31)<<1;
	i = GSHORT(dp->date);
	y = 80+(i>>9); M = (i>>5)&15; d = i&31;

	if (M < 1 || M > 12)
		return 0;
	if (d < 1 || d > dmsize[M-1])
		return 0;
	if (h > 23)
		return 0;
	if (m > 59)
		return 0;
	if (s > 59)
		return 0;
	y += 1900;
	t = 0;
	for(i=1970; i<y; i++)
		t += dysize(i);
	if (dysize(y)==366 && M >= 3)
		t++;
	while(--M)
		t += dmsize[M-1];
	t += d-1;
	noonGMT += 24*60*60*t;
	t = 24*t + h;
	t = 60*t + m;
	t = 60*t + s;
	t += (12-localtime(noonGMT)->hour)*60*60;
	return t;
}

void
dirdump(void *dbuf)
{
	static char attrchar[] = "rhsvda67";
	char buf[72];
	Dosdir *d = dbuf;
	int i, n = 0;
	char *p;

	if(!chatty)
		return;
	n += sprint(buf+n, "\"%.8s.%.3s\" ", d->name, d->ext);
	p = attrchar+7;
	for(i=0x80; i; i>>=1,p--)
		n += sprint(buf+n, "%c", d->attr&i ? *p : '-');
	i = GSHORT(d->time);
	n += sprint(buf+n, " %2.2d:%2.2d:%2.2d", i>>11, (i>>5)&63, (i&31)<<1);
	i = GSHORT(d->date);
	n += sprint(buf+n, " %2.2d.%2.2d.%2.2d", 80+(i>>9), (i>>5)&15, i&31);
	sprint(buf+n, " %d %d", GSHORT(d->start), GSHORT(d->length));
	chat("%s\n", buf);
}
