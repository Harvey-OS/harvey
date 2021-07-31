#include	"all.h"

Filsys*
fsstr(char *p)
{
	Filsys *fs;

	for(fs=filsys; fs->name; fs++)
		if(strcmp(fs->name, p) == 0)
			return fs;
	return 0;
}

void
fileinit(Chan *cp)
{
	File *f;
	Tlock *t;

loop:
	lock(&cp->flock);
	f = cp->flist;
	if(!f) {
		unlock(&cp->flock);
		return;
	}
	cp->flist = f->next;
	unlock(&cp->flock);

	qlock(f);
	if(t = f->tlock) {
		t->time = 0;
		f->tlock = 0;
	}
	if(f->open & FREMOV)
		doremove(f, 0);
	freewp(f->wpath);
	f->open = 0;
	f->cp = 0;
	qunlock(f);

	goto loop;
}

/*
 * returns a locked file structure
 */
File*
filep(Chan *cp, int fid, int flag)
{
	File *f, *prev;

	if(fid == NOF)
		return 0;

loop:
	lock(&cp->flock);
	for(prev=0,f=cp->flist; f; prev=f,f=f->next) {
		if(f->fid != fid)
			continue;
		if(prev) {
			prev->next = f->next;
			f->next = cp->flist;
			cp->flist = f;
		}
		goto out;
	}
	if(flag) {
		f = newfp(cp);
		if(f) {
			f->fid = fid;
			goto out;
		}
	}
	unlock(&cp->flock);
	return 0;

out:
	unlock(&cp->flock);
	qlock(f);
	if(f->fid != fid) {
		qunlock(f);
		goto loop;
	}
	return f;
}

/*
 * always called with cp->flock locked
 */
File*
newfp(Chan *cp)
{
	static first;
	File *f;
	int start, i;

	i = first;
	start = i;
	do {
		f = &files[i];
		i++;
		if(i >= conf.nfile)
			i = 0;
		if(f->cp)
			continue;
		lock(&newfplock);
		if(f->cp) {
			unlock(&newfplock);
			continue;
		}
		f->cp = cp;
		f->next = cp->flist;
		f->wpath = 0;
		f->tlock = 0;
		cp->flist = f;
		first = i;
		unlock(&newfplock);
		return f;
	} while(i != start);

	print("%d: out of files\n", cp->chan);
	return 0;
}

void
freefp(File *fp)
{
	Chan *cp;
	File *f, *prev;

	if(!fp || !(cp = fp->cp))
		return;
	lock(&cp->flock);
	for(prev=0,f=cp->flist; f; prev=f,f=f->next) {
		if(f != fp)
			continue;
		if(prev)
			prev->next = f->next;
		else
			cp->flist = f->next;
		f->cp = 0;
		break;
	}
	unlock(&cp->flock);
}

int
iaccess(File *f, Dentry *d, int m)
{
	if(wstatallow)
		return 0;
	/*
	 * other is easiest
	 */
	if(m & d->mode)
		return 0;
	/*
	 * owner is next
	 */
	if(f->uid == d->uid)
		if((m<<6) & d->mode)
			return 0;
	/*
	 * group membership is hard
	 */
	if(ingroup(f->uid, d->gid))
		if((m<<3) & d->mode)
			return 0;
	return 1;
}

Tlock*
tlocked(Iobuf *p, Dentry *d)
{
	Tlock *t, *t1;
	long qpath, tim;
	Device dev;

	tim = toytime();
	qpath = d->qid.path;
	dev = p->dev;
	t1 = 0;
	for(t=tlocks+NTLOCK-1; t>=tlocks; t--) {
		if(t->qpath == qpath)
		if(t->time >= tim)
		if(devcmp(t->dev, dev) == 0)
			return 0;		/* its locked */
		if(!t1 && t->time < tim)
			t1 = t;			/* steal first lock */
	}
	if(t1) {
		t1->dev = dev;
		t1->qpath = qpath;
		t1->time = tim + TLOCK;
	}
	/* botch
	 * out of tlock nodes simulates
	 * a locked file
	 */
	return t1;
}

Wpath*
newwp(void)
{
	static int si = 0;
	int i;
	Wpath *w, *sw, *ew;

	i = si + 1;
	if(i < 0 || i >= conf.nwpath)
		i = 0;
	si = i;
	sw = &wpaths[i];
	ew = &wpaths[conf.nwpath];
	for(w=sw;;) {
		w++;
		if(w >= ew)
			w = &wpaths[0];
		if(w == sw) {
			print("out of wpaths\n");
			return 0;
		}
		if(w->refs)
			continue;
		lock(&wpathlock);
		if(w->refs) {
			unlock(&wpathlock);
			continue;
		}
		w->refs = 1;
		w->up = 0;
		unlock(&wpathlock);
		return w;
	}

}

void
freewp(Wpath *w)
{
	lock(&wpathlock);
	for(; w; w=w->up)
		w->refs--;
	unlock(&wpathlock);
}

Qid
newqid(Device dev)
{
	Iobuf *p;
	Superb *sb;
	Qid qid;

	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("newqid: super block");
	sb = (Superb*)p->iobuf;
	sb->qidgen++;
	qid.path = sb->qidgen;
	qid.version = 0;
	putbuf(p);
	return qid;
}

/*
 * what are legal characters in a name?
 * only disallow control characters.
 * a) utf avoids control characters.
 * b) '/' may not be the separator
 */
int
checkname(char *n)
{
	int i, c;

	for(i=0; i<NAMELEN; i++) {
		c = *n & 0xff;
		if(c == 0) {
			if(i == 0)
				return 1;
			memset(n, 0, NAMELEN-i);
			return 0;
		}
		if(c <= 040)
			return 1;
		n++;
	}
	return 1;	/* too long */
}

void
bfree(Device dev, long addr, int d)
{
	Iobuf *p;
	long a;
	int i;

	if(!addr)
		return;
	if(d > 0) {
		d--;
		p = getbuf(dev, addr, Bread);
		if(p) {
			for(i=INDPERBUF-1; i>=0; i--) {
				a = ((long*)p->iobuf)[i];
				bfree(dev, a, d);
			}
			putbuf(p);
		}
	}
	/*
	 * stop outstanding i/o
	 */
	p = getbuf(dev, addr, Bprobe);
	if(p) {
		p->flags &= ~(Bmod|Bimm);
		putbuf(p);
	}
	/*
	 * dont put written worm
	 * blocks into free list
	 */
	if(nofree(dev, addr))
		return;
	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("bfree: super block");
	addfree(dev, addr, (Superb*)p->iobuf);
	putbuf(p);
}

long
balloc(Device dev, int tag, long qid)
{
	Iobuf *bp, *p;
	Superb *sb;
	long a;
	int n;

	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("balloc: super block");
	sb = (Superb*)p->iobuf;

loop:
	n = --sb->fbuf.nfree;
	sb->tfree--;
	if(n < 0 || n >= FEPERBUF)
		panic("balloc: bad freelist");
	a = sb->fbuf.free[n];
	if(n <= 0) {
		if(a == 0) {
			sb->tfree = 0;
			sb->fbuf.nfree = 1;
			if(devgrow(dev, sb))
				goto loop;
			putbuf(p);
			return 0;
		}
		bp = getbuf(dev, a, Bread);
		if(!bp || checktag(bp, Tfree, QPNONE)) {
			if(bp)
				putbuf(bp);
			putbuf(p);
			return 0;
		}
		memmove(&sb->fbuf, bp->iobuf, (FEPERBUF+1)*sizeof(long));
		putbuf(bp);
	}
	bp = getbuf(dev, a, Bmod);
	memset(bp->iobuf, 0, RBUFSIZE);
	settag(bp, tag, qid);
	if(tag == Tind1 || tag == Tind2 || tag == Tdir)
		bp->flags |= Bimm;
	putbuf(bp);
	putbuf(p);
	return a;
}

void
addfree(Device dev, long addr, Superb *sb)
{
	int n;
	Iobuf *p;

	if(addr >= sb->fsize){
		print("addfree: bad addr %lux\n", addr);
		return;
	}
	n = sb->fbuf.nfree;
	if(n < 0 || n > FEPERBUF)
		panic("addfree: bad freelist");
	if(n >= FEPERBUF) {
		p = getbuf(dev, addr, Bmod);
		if(p == 0)
			panic("addfree: getbuf");
		memmove(p->iobuf, &sb->fbuf, (FEPERBUF+1)*sizeof(long));
		settag(p, Tfree, QPNONE);
		putbuf(p);
		n = 0;
	}
	sb->fbuf.free[n++] = addr;
	sb->fbuf.nfree = n;
	sb->tfree++;
	if(addr >= sb->fsize)
		sb->fsize = addr+1;
}

int
Cconv(Op *o)
{
	Chan *cp;
	char s[20];

	cp = *(Chan**)o->argp;
	sprint(s, "C%d.%.3d", cp->type, cp->chan);
	strconv(s, o, o->f1, o->f2);
	return sizeof(cp);
}

int
Dconv(Op *o)
{
	Device d;
	char s[20];

	d = *(Device*)o->argp;
	sprint(s, "D%d.%d.%d.%d", d.type, d.ctrl, d.unit, d.part);
	strconv(s, o, o->f1, o->f2);
	return sizeof(d);
}

int
Fconv(Op *o)
{
	Filta a;
	char s[30];

	a = *(Filta*)o->argp;

	sprint(s, "%6lud %6lud %6lud",
		fdf(a.f->filter[0], a.scale*60),
		fdf(a.f->filter[1], a.scale*600),
		fdf(a.f->filter[2], a.scale*6000));
	strconv(s, o, o->f1, o->f2);
	return sizeof(Filta);
}

int
Gconv(Op *o)
{
	int t;
	char s[20];

	t = *(int*)o->argp;
	strcpy(s, "<badtag>");
	if(t >= 0 && t < MAXTAG)
		sprint(s, "%s", tagnames[t]);
	strconv(s, o, o->f1, o->f2);
	return sizeof(t);
}

void
formatinit(void)
{

	fmtinstall('C', Cconv);	/* print channels */
	fmtinstall('D', Dconv);	/* print devices */
	fmtinstall('F', Fconv);	/* print filters */
	fmtinstall('G', Gconv);	/* print tags */
	fmtinstall('T', Tconv);	/* print times */
}

int
devcmp(Device d1, Device d2)
{

	if(d1.type == d2.type)
	if(d1.ctrl == d2.ctrl)
	if(d1.unit == d2.unit)
	if(d1.part == d2.part)
		return 0;
	return 1;
}

void
rootream(Device dev, long addr)
{
	Iobuf *p;
	Dentry *d;

	p = getbuf(dev, addr, Bmod|Bimm);
	memset(p->iobuf, 0, RBUFSIZE);
	settag(p, Tdir, QPROOT);
	d = getdir(p, 0);
	strcpy(d->name, "/");
	d->uid = -1;
	d->gid = -1;
	d->mode = DALLOC | DDIR |
		((DREAD|DWRITE|DEXEC) << 6) |
		((DREAD|DWRITE|DEXEC) << 3) |
		((DREAD|DWRITE|DEXEC) << 0);
	d->qid = QID(QPROOT|QPDIR,0);
	d->atime = time();
	d->mtime = d->atime;
	putbuf(p);
}

void
superream(Device dev, long addr)
{
	Iobuf *p;
	Superb *s;
	long i;

	p = getbuf(dev, addr, Bmod|Bimm);
	memset(p->iobuf, 0, RBUFSIZE);
	settag(p, Tsuper, QPSUPER);

	s = (Superb*)p->iobuf;
	s->fstart = 1;
	s->fsize = devsize(dev);
	s->fbuf.nfree = 1;
	s->qidgen = 10;
	for(i=s->fsize-1; i>=addr+2; i--)
		addfree(dev, i, s);
	putbuf(p);
}

/*
 * returns 1 if n is prime
 * used for adjusting lengths
 * of hashing things.
 * there is no need to be clever
 */
int
prime(long n)
{
	long i;

	if((n%2) == 0)
		return 0;
	for(i=3;; i+=2) {
		if((n%i) == 0)
			return 0;
		if(i*i >= n)
			return 1;
	}
}

void
hexdump(void *a, int n)
{
	char s1[30], s2[4];
	uchar *p;
	int i;

	p = a;
	s1[0] = 0;
	for(i=0; i<n; i++) {
		sprint(s2, " %.2ux", p[i]);
		strcat(s1, s2);
		if((i&7) == 7) {
			print("%s\n", s1);
			s1[0] = 0;
		}
	}
	if(s1[0])
		print("%s\n", s1);
}
