#include <u.h>
#include <libc.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"
#include "iso9660.h"

static void	ireset(void);
static int	iattach(Xfile*);
static void	iclone(Xfile*, Xfile*);
static void	iwalkup(Xfile*);
static void	iwalk(Xfile*, char*);
static void	iopen(Xfile*, int);
static void	icreate(Xfile*, char*, long, int);
static long	ireaddir(Xfile*, void*, long, long);
static long	iread(Xfile*, void*, long, long);
static long	iwrite(Xfile*, void*, long, long);
static void	iclunk(Xfile*);
static void	iremove(Xfile*);
static void	istat(Xfile*, Dir*);
static void	iwstat(Xfile*, Dir*);

static char*	nstr(void*, int);
static char*	rdate(void*, int);
static int	getdrec(Xfile*, void*);
static int	opendotdot(Xfile*, Xfile*);
static int	showdrec(int, int, void*);
static long	gtime(void*);
static long	l16(void*);
static long	l32(void*);
static void	newdrec(Xfile*, Drec*);
static int	rzdir(int, Dir*, int, Drec*);

Xfsub	isosub =
{
	ireset, iattach, iclone, iwalkup, iwalk, iopen, icreate,
	ireaddir, iread, iwrite, iclunk, iremove, istat, iwstat
};

static void
ireset(void)
{}

static int
iattach(Xfile *root)
{
	Xfs *cd = root->xf;
	Iobuf *p; Voldesc *v; Isofile *fp; Drec *dp;
	int fmt, blksize;

	p = getbuf(cd->d, VOLDESC);
	v = (Voldesc *)(p->iobuf);
	if(memcmp(v->byte, "\01CD001\01", 7) == 0){		/* iso */
		fmt = 'z';
		dp = (Drec *)v->z.desc.rootdir;
		blksize = l16(v->z.desc.blksize);
		chat("iso, blksize=%d...", blksize);
	}else if(memcmp(&v->byte[8], "\01CDROM\01", 7) == 0){	/* high sierra */
		fmt = 'r';
		dp = (Drec *)v->r.desc.rootdir;
		blksize = l16(v->r.desc.blksize);
		chat("high sierra, blksize=%d...", blksize);
	}else{
		putbuf(p);
		return -1;
	}
	if(chatty)
		showdrec(2, fmt, dp);
	if(blksize > Sectorsize){
		chat("blksize too big...");
		putbuf(p);
		return -1;
	}
	if(waserror()){
		putbuf(p);
		nexterror();
	}
	root->len = sizeof(Isofile) - sizeof(Drec) + dp->reclen;
	root->ptr = fp = ealloc(root->len);
	root->xf->isplan9 = (strncmp((char*)v->z.boot.sysid, "PLAN 9", 6)==0);
	fp->fmt = fmt;
	fp->blksize = blksize;
	fp->offset = 0;
	fp->doffset = 0;
	memmove(&fp->d, dp, dp->reclen);
	root->qid.path = CHDIR|l32(dp->addr);
	putbuf(p);
	poperror();
	return 0;
}

static void
iclone(Xfile *of, Xfile *nf)
{
	USED(of, nf);
}

static void
iwalkup(Xfile *f)
{
	long paddr;
	uchar dbuf[256];
	Drec *d = (Drec *)dbuf;
	Xfile pf, ppf;
	Isofile piso, ppiso;

	memset(&pf, 0, sizeof pf);
	memset(&ppf, 0, sizeof ppf);
	pf.ptr = &piso;
	ppf.ptr = &ppiso;
	if(opendotdot(f, &pf) < 0)
		error("can't open pf");
	paddr = l32(((Isofile *)pf.ptr)->d.addr);
	if(l32(((Isofile *)f->ptr)->d.addr) == paddr)
		return;
	if(opendotdot(&pf, &ppf) < 0)
		error("can't open ppf");
	while(getdrec(&ppf, d) >= 0){
		if(l32(d->addr) == paddr){
			newdrec(f, d);
			f->qid.path = paddr|CHDIR;
			return;
		}
	}
	error("can't find addr of ..");
}

static void
iwalk(Xfile *f, char *name)
{
	Isofile *ip = f->ptr;
	uchar dbuf[256];
	char nbuf[NAMELEN];
	Drec *d = (Drec*)dbuf;
	Dir dir;
	char *p;
	int len, vers, dvers;

	vers = -1;
	if(p = strchr(name, ';')) {	/* assign = */
		len = p-name;
		if(len >= NAMELEN)
			len = NAMELEN-1;
		memmove(nbuf, name, len);
		vers = strtoul(p+1, 0, 10);
		name = nbuf;
	}
	len = strlen(name);
	if(len >= NAMELEN)
		len = NAMELEN-1;
	name[len] = 0;

	chat("%d \"%s\"...", len, name);
	ip->offset = 0;
	while(getdrec(f, d) >= 0) {
		dvers = rzdir(f->xf->isplan9, &dir, 'z', d);
		if(strcmp(name, dir.name) != 0)
			continue;
		newdrec(f, d);
		f->qid.path = dir.qid.path;
		USED(dvers);
		return;
	}
	USED(vers);
	error(Enonexist);
}

static void
iopen(Xfile *f, int mode)
{
	mode &= ~OCEXEC;
	if(mode != OREAD && mode != OEXEC)
		error(Eperm);
	((Isofile*)f->ptr)->offset = 0;
	((Isofile*)f->ptr)->doffset = 0;
}

static void
icreate(Xfile *f, char *name, long perm, int mode)
{
	USED(f, name, perm, mode);
	error(Eperm);
}

static long
ireaddir(Xfile *f, char *buf, long offset, long count)
{
	Isofile *ip = f->ptr;
	Dir d;
	uchar dbuf[256];
	Drec *drec = (Drec *)dbuf;
	int rcnt = 0;

	if(offset < ip->doffset){
		ip->offset = 0;
		ip->doffset = 0;
	}
	while(rcnt < count && getdrec(f, drec) >= 0){
		if(drec->namelen == 1){
			if(drec->name[0] == 0)
				continue;
			if(drec->name[0] == 1)
				continue;
		}
		if(ip->doffset < offset){
			ip->doffset += DIRLEN;
			continue;
		}
		rzdir(f->xf->isplan9, &d, ip->fmt, drec);
		d.qid.vers = f->qid.vers;
		rcnt += convD2M(&d, &buf[rcnt]);
	}
	ip->doffset += rcnt;
	return rcnt;
}

static long
iread(Xfile *f, char *buf, long offset, long count)
{
	Isofile *ip = f->ptr;
	long size, addr, o, n;
	int rcnt = 0;
	Iobuf *p;

	size = l32(ip->d.size);
	if(offset >= size)
		return 0;
	if(offset+count > size)
		count = size - offset;
	addr = (l32(ip->d.addr)+ip->d.attrlen)*ip->blksize + offset;
	o = addr % Sectorsize;
	addr /= Sectorsize;
	/*chat("d.addr=0x%x, addr=0x%x, o=0x%x...", l32(ip->d.addr), addr, o);*/
	n = Sectorsize - o;

	while(count > 0){
		if(n > count)
			n = count;
		p = getbuf(f->xf->d, addr);
		memmove(&buf[rcnt], &p->iobuf[o], n);
		putbuf(p);
		count -= n;
		rcnt += n;
		++addr;
		o = 0;
		n = Sectorsize;
	}
	return rcnt;
}

static long
iwrite(Xfile *f, uchar *buf, long offset, long count)
{
	USED(f, buf, offset, count);
	error(Eperm);
	return 0;
}

static void
iclunk(Xfile *f)
{
	USED(f);
}

static void
iremove(Xfile *f)
{
	USED(f);
	error(Eperm);
}

static void
istat(Xfile *f, Dir *d)
{
	Isofile *ip = f->ptr;

	rzdir(f->xf->isplan9, d, ip->fmt, &ip->d);
	d->qid.vers = f->qid.vers;
	if(d->qid.path==f->xf->rootqid.path)
		d->qid.path = CHDIR;
}

static void
iwstat(Xfile *f, Dir *d)
{
	USED(f, d);
	error(Eperm);
}

static int
showdrec(int fd, int fmt, void *x)
{
	Drec *d = (Drec *)x;
	int namelen, syslen;

	if(d->reclen == 0)
		return 0;
	fprint(fd, "%d %d %d %d ",
		d->reclen, d->attrlen, l32(d->addr), l32(d->size));
	fprint(fd, "%s 0x%2.2x %d %d %d ",
		rdate(d->date, fmt), (fmt=='z' ? d->flags : d->r_flags),
		d->unitsize, d->gapsize, l16(d->vseqno));
	fprint(fd, "%d %s", d->namelen, nstr(d->name, d->namelen));
	namelen = d->namelen + (1-(d->namelen&1));
	syslen = d->reclen - 33 - namelen;
	if(syslen != 0)
		fprint(fd, " %s", nstr(&d->name[namelen], syslen));
	fprint(fd, "\n");
	return d->reclen + (d->reclen&1);
}

static void
newdrec(Xfile *f, Drec *dp)
{
	Isofile *x = f->ptr;
	Isofile *n;
	int len;

	len = sizeof(Isofile) - sizeof(Drec) + dp->reclen;
	n = ealloc(len);
	n->fmt = x->fmt;
	n->blksize = x->blksize;
	n->offset = 0;
	n->doffset = 0;
	memmove(&n->d, dp, dp->reclen);
	free(x);
	f->ptr = n;
	f->len = len;
}

static int
getdrec(Xfile *f, void *buf)
{
	Isofile *ip = f->ptr;
	int len = 0, boff = 0;
	long size, addr;
	Iobuf *p = 0;

	if(!ip)
		return -1;
	size = l32(ip->d.size);
	while(ip->offset<size){
		addr = (l32(ip->d.addr)+ip->d.attrlen)*ip->blksize + ip->offset;
		boff = addr % Sectorsize;
		if(boff > Sectorsize-34){
			ip->offset += Sectorsize-boff;
			continue;
		}
		p = getbuf(f->xf->d, addr/Sectorsize);
		len = p->iobuf[boff];
		if(len >= 34)
			break;
		putbuf(p);
		p = 0;
		ip->offset += Sectorsize-boff;
	}
	if(p) {
		memmove(buf, &p->iobuf[boff], len);
		putbuf(p);
		ip->offset += len + (len&1);
	}
	if(p)
		return 0;
	return -1;
}

static int
opendotdot(Xfile *f, Xfile *pf)
{
	uchar dbuf[256];
	Drec *d = (Drec *)dbuf;
	Isofile *ip = f->ptr, *pip = pf->ptr;

	ip->offset = 0;
	if(getdrec(f, d) < 0){
		chat("opendotdot: getdrec(.) failed...");
		return -1;
	}
	if(d->namelen != 1 || d->name[0] != 0){
		chat("opendotdot: no . entry...");
		return -1;
	}
	if(l32(d->addr) != l32(ip->d.addr)){
		chat("opendotdot: bad . address...");
		return -1;
	}
	if(getdrec(f, d) < 0){
		chat("opendotdot: getdrec(..) failed...");
		return -1;
	}
	if(d->namelen != 1 || d->name[0] != 1){
		chat("opendotdot: no .. entry...");
		return -1;
	}

	pf->xf = f->xf;
	pip->fmt = ip->fmt;
	pip->blksize = ip->blksize;
	pip->offset = 0;
	pip->doffset = 0;
	pip->d = *d;
	return 0;
}

static int
rzdir(int isplan9, Dir *d, int fmt, Drec *dp)
{
	int n, flags, i, nl, vers;
	uchar *s;
	char *p;

	flags = 0;
	vers = -1;
	d->qid.path = l32(dp->addr);
	d->qid.vers = 0;
	n = dp->namelen;
	if(n >= NAMELEN)
		n = NAMELEN-1;
	memset(d->name, 0, NAMELEN);
	if(n == 1) {
		switch(dp->name[0]){
		case 1:
			d->name[1] = '.';
			/* fall through */
		case 0:
			d->name[0] = '.';
			break;
		default:
			d->name[0] = tolower(dp->name[0]);
		}
	} else {
		for(i=0; i<n; i++)
			d->name[i] = tolower(dp->name[i]);
	}

	if(isplan9 && dp->reclen>34+dp->namelen) {
		/*
		 * get gid, uid, mode and possibly name
		 * from plan9 directory extension
		 */
		s = (uchar*)dp->name + dp->namelen;
		if(((ulong)s) & 1)
			s++;
		nl = *s;
		if(nl >= NAMELEN)
			nl = NAMELEN-1;
		if(nl) {
			memset(d->name, 0, NAMELEN);
			memmove(d->name, s+1, nl);
		}
		s += 1 + *s;
		nl = *s;
		if(nl >= NAMELEN)
			nl = NAMELEN-1;
		memset(d->uid, 0, NAMELEN);
		memmove(d->uid, s+1, nl);
		s += 1 + *s;
		nl = *s;
		if(nl >= NAMELEN)
			nl = NAMELEN-1;
		memset(d->gid, 0, NAMELEN);
		memmove(d->gid, s+1, nl);
		s += 1 + *s;
		if(((ulong)s) & 1)
			s++;
		d->mode = l32(s);
		if(d->mode & CHDIR)
			d->qid.path |= CHDIR;
	} else {
		d->mode = 0444;
		switch(fmt) {
		case 'z':
			strcpy(d->gid, "iso");
			flags = dp->flags;
			break;
		case 'r':
			strcpy(d->gid, "sierra");
			flags = dp->r_flags;
			break;
		}
		if(flags & 0x02){
			d->qid.path |= CHDIR;
			d->mode |= CHDIR|0111;
		}
		strcpy(d->uid, "cdrom");
		p = strchr(d->name, ';');
		if(p != 0) {
			vers = strtoul(p+1, 0, 10);
			memset(p, 0, NAMELEN-(p-d->name));
		}
	}
	d->length = 0;
	if((d->mode & CHDIR) == 0)
		d->length = l32(dp->size);
	d->type = 0;
	d->dev = 0;
	d->atime = gtime(dp->date);
	d->mtime = d->atime;
	return vers;
}

static char *
nstr(uchar *p, int n)
{
	static char buf[132];
	char *q = buf;

	while(--n >= 0){
		if(*p == '\\')
			*q++ = '\\';
		if(' ' <= *p && *p <= '~')
			*q++ = *p++;
		else
			q += sprint(q, "\\%2.2ux", *p++);
	}
	*q = 0;
	return buf;
}

static char *
rdate(uchar *p, int fmt)
{
	static char buf[64];
	int htz, s, n;

	n = sprint(buf, "%2.2d.%2.2d.%2.2d %2.2d:%2.2d:%2.2d",
		p[0], p[1], p[2], p[3], p[4], p[5]);
	if(fmt == 'z'){
		htz = p[6];
		if(htz >= 128){
			htz = 256-htz;
			s = '-';
		}else
			s = '+';
		sprint(&buf[n], " (%c%.1f)", s, (float)htz/2);
	}
	return buf;
}

static char
dmsize[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

static int
dysize(int y)
{

	if((y%4) == 0)
		return 366;
	return 365;
}

static long
gtime(uchar *p)	/* yMdhmsz */
{
	long t;
	int i, y, M, d, h, m, s, tz;
	y=p[0]; M=p[1]; d=p[2];
	h=p[3]; m=p[4]; s=p[5]; tz=p[6];
	USED(tz);
	if (y < 70)
		return 0;
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
	t = 24*t + h;
	t = 60*t + m;
	t = 60*t + s;
	return t;
}

#define	p	((uchar*)arg)

static long
l16(void *arg)
{
	long v;

	v = ((long)p[1]<<8)|p[0];
	if (v >= 0x8000L)
		v -= 0x10000L;
	return v;
}

static long
l32(void *arg)
{
	return ((((((long)p[3]<<8)|p[2])<<8)|p[1])<<8)|p[0];
}

#undef	p
