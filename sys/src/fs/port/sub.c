#include	"all.h"
#include	"io.h"

Filsys*
fsstr(char *p)
{
	Filsys *fs;

	for(fs=filsys; fs->name; fs++)
		if(strcmp(fs->name, p) == 0)
			return fs;
	return 0;
}

/*
 * allocate 'count' contiguous channels
 * of type 'type' and return pointer to base
 */
Chan*
chaninit(int type, int count)
{
	Chan *cp, *icp;
	int i;

	icp = ialloc(count * sizeof(*icp), 0);
	cp = icp;
	for(i=0; i<count; i++) {
		cp->next = chans;
		chans = cp;
		cp->type = type;
		cp->chan = cons.chano;
		cons.chano++;
		strncpy(cp->whoname, "<none>", sizeof(cp->whoname));
		fileinit(cp);
		wlock(&cp->reflock);
		wunlock(&cp->reflock);
		rlock(&cp->reflock);
		runlock(&cp->reflock);
		cp++;
	}
	return icp;
}

void
fileinit(Chan *cp)
{
	File *f, *prev;
	Tlock *t;
	int h;

loop:
	lock(&flock);
	for(h=0; h<nelem(flist); h++) {
		for(prev=0,f=flist[h]; f; prev=f,f=f->next) {
			if(f->cp != cp)
				continue;
			if(prev) {
				prev->next = f->next;
				f->next = flist[h];
				flist[h] = f;
			}
			goto out;
		}
	}
	unlock(&flock);
	return;

out:
	flist[h] = f->next;
	unlock(&flock);

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
	File *f;
	int h;

	if(fid == NOF)
		return 0;

	h = (long)cp + fid;
	if(h < 0)
		h = ~h;
	h = h % nelem(flist);

loop:
	lock(&flock);
	for(f=flist[h]; f; f=f->next)
		if(f->fid == fid && f->cp == cp)
			goto out;

	if(flag) {
		f = newfp();
		if(f) {
			f->fid = fid;
			f->cp = cp;
			f->wpath = 0;
			f->tlock = 0;
			f->next = flist[h];
			flist[h] = f;
			goto out;
		}
	}
	unlock(&flock);
	return 0;

out:
	unlock(&flock);
	qlock(f);
	if(f->fid == fid && f->cp == cp)
		return f;
	qunlock(f);
	goto loop;
}

/*
 * always called with flock locked
 */
File*
newfp(void)
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

		first = i;

		return f;
	} while(i != start);

	print("out of files\n");
	return 0;
}

void
freefp(File *fp)
{
	Chan *cp;
	File *f, *prev;
	int h;

	if(!fp || !(cp = fp->cp))
		return;

	h = (long)cp + fp->fid;
	if(h < 0)
		h = ~h;
	h = h % nelem(flist);

	lock(&flock);
	for(prev=0,f=flist[h]; f; prev=f,f=f->next)
		if(f == fp) {
			if(prev)
				prev->next = f->next;
			else
				flist[h] = f->next;
			break;
		}
	fp->cp = 0;
	unlock(&flock);
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

void
buffree(Device dev, long addr, int d)
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
				buffree(dev, a, d);
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
	if(dev.type == Devcw) {
		i = cwfree(dev, addr);
		if(i)
			return;
	}
	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("buffree: super block");
	addfree(dev, addr, (Superb*)p->iobuf);
	putbuf(p);
}

long
bufalloc(Device dev, int tag, long qid)
{
	Iobuf *bp, *p;
	Superb *sb;
	long a;
	int n;

	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("bufalloc: super block");
	sb = (Superb*)p->iobuf;

loop:
	n = --sb->fbuf.nfree;
	sb->tfree--;
	if(n < 0 || n >= FEPERBUF)
		panic("bufalloc: bad freelist");
	a = sb->fbuf.free[n];
	if(n <= 0) {
		if(a == 0) {
			sb->tfree = 0;
			sb->fbuf.nfree = 1;
			if(dev.type == Devcw)
				if(cwgrow(dev, sb))
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
		sb->fbuf = *(Fbuf*)bp->iobuf;
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
addfree(Device dev, long addr, Superb *sb)
{
	int n;
	Iobuf *p;

	n = sb->fbuf.nfree;
	if(n < 0 || n > FEPERBUF)
		panic("addfree: bad freelist");
	if(n >= FEPERBUF) {
		p = getbuf(dev, addr, Bmod);
		if(p == 0)
			panic("addfree: getbuf");
		*(Fbuf*)p->iobuf = sb->fbuf;
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

int
Econv(Op *o)
{
	char s[64];
	uchar *p;

	p = *((uchar**)o->argp);
	sprint(s, "%.2lux%.2lux%.2lux%.2lux%.2lux%.2lux",
		p[0], p[1], p[2], p[3], p[4], p[5]);
	strconv(s, o, o->f1, o->f2);
	return sizeof(uchar*);
}

int
Iconv(Op *o)
{
	char s[64];
	uchar *p;

	p = *((uchar**)o->argp);
	sprint(s, "%d.%d.%d.%d",
		p[0], p[1], p[2], p[3]);
	strconv(s, o, o->f1, o->f2);
	return sizeof(uchar*);
}

int
Nconv(Op *o)
{
	char s[64];
	uchar *p;
	long n;

	p = *((uchar**)o->argp);
	n = (p[0]<<8) | p[1];
	if(!(o->f3 & FSHORT))
		n = (n<<16) | (p[2]<<8) | p[3];
	sprint(s, "%lud", n);
	strconv(s, o, o->f1, o->f2);
	return sizeof(uchar*);
}

void
formatinit(void)
{

	fmtinstall('C', Cconv);	/* print channels */
	fmtinstall('D', Dconv);	/* print devices */
	fmtinstall('F', Fconv);	/* print filters */
	fmtinstall('G', Fconv);	/* print tags */
	fmtinstall('T', Tconv);	/* print times */
	fmtinstall('E', Econv);	/* print ether addresses */
	fmtinstall('I', Iconv);	/* print ip addresses */
	fmtinstall('N', Nconv);	/* print network order integers */
}

int
nzip(uchar ip[Pasize])
{
	if(ip[0] || ip[1] || ip[2] || ip[3])
		return 1;
	return 0;
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
	s->fstart = 2;
	s->fsize = devsize(dev);
	s->fbuf.nfree = 1;
	s->qidgen = 10;
	for(i=s->fsize-1; i>=addr+2; i--)
		addfree(dev, i, s);
	putbuf(p);
}

struct
{
	Lock;
	Msgbuf	*smsgbuf;
	Msgbuf	*lmsgbuf;
} msgalloc;

void
mbinit(void)
{
	Msgbuf *mb;
	Rabuf *rb;
	int i;

	lock(&msgalloc);
	unlock(&msgalloc);
	msgalloc.lmsgbuf = 0;
	msgalloc.smsgbuf = 0;
	for(i=0; i<conf.nlgmsg; i++) {
		mb = ialloc(sizeof(Msgbuf), 0);
if(1)
mb->xdata = ialloc(LARGEBUF+256, 256);
else
		mb->xdata = ialloc(LARGEBUF+OFFMSG, LINESIZE);
		mb->flags = LARGE;
		mbfree(mb);
		cons.nlarge++;
	}
	for(i=0; i<conf.nsmmsg; i++) {
		mb = ialloc(sizeof(Msgbuf), 0);
if(1)
mb->xdata = ialloc(SMALLBUF+256, 256);
else
		mb->xdata = ialloc(SMALLBUF+OFFMSG, LINESIZE);
		mb->flags = 0;
		mbfree(mb);
		cons.nsmall++;
	}
	memset(mballocs, 0, sizeof(mballocs));

	lock(&rabuflock);
	unlock(&rabuflock);
	rabuffree = 0;
	for(i=0; i<1000; i++) {
		rb = ialloc(sizeof(*rb), 0);
		rb->link = rabuffree;
		rabuffree = rb;
	}
}

Msgbuf*
mballoc(int count, Chan *cp, int category)
{
	Msgbuf *mb;

	lock(&msgalloc);
	if(count > SMALLBUF) {
		if(count > LARGEBUF)
			panic("msgbuf count");
		mb = msgalloc.lmsgbuf;
		if(mb == 0) {
			mb = ialloc(sizeof(Msgbuf), 0);
if(1)
mb->xdata = ialloc(LARGEBUF+256, 256);
else
			mb->xdata = ialloc(LARGEBUF+OFFMSG, LINESIZE);
			cons.nlarge++;
		} else
			msgalloc.lmsgbuf = mb->next;
		mb->flags = LARGE;
	} else {
		mb = msgalloc.smsgbuf;
		if(mb == 0) {
			mb = ialloc(sizeof(Msgbuf), 0);
if(1)
mb->xdata = ialloc(SMALLBUF+256, 256);
else
			mb->xdata = ialloc(SMALLBUF+OFFMSG, LINESIZE);
			cons.nsmall++;
		} else
			msgalloc.smsgbuf = mb->next;
		mb->flags = 0;
	}
	mballocs[category]++;
	unlock(&msgalloc);
	mb->count = count;
	mb->chan = cp;
	mb->param = 0;
	mb->category = category;
if(1)
mb->data = mb->xdata+256;
else
	mb->data = mb->xdata+OFFMSG;
	return mb;
}

void
mbfree(Msgbuf *mb)
{
	lock(&msgalloc);
	mballocs[mb->category]--;
	mb->category = 0;
	if(mb->flags & FREE)
		panic("mbfree already free");
	mb->flags |= FREE;
	if(mb->flags & LARGE) {
		mb->next = msgalloc.lmsgbuf;
		msgalloc.lmsgbuf = mb;
	} else {
		mb->next = msgalloc.smsgbuf;
		msgalloc.smsgbuf = mb;
	}
	mb->data = 0;
	unlock(&msgalloc);
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

char*
getwd(char *word, char *line)
{
	int c, n;

	while(*line == ' ')
		line++;
	for(n=0; n<80; n++) {
		c = *line;
		if(c == ' ' || c == 0 || c == '\n')
			break;
		line++;
		*word++ = c;
	}
	*word = 0;
	return line;
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

void*
recv(Queue *q, int tim)
{
	User *p;
	void *a;
	int i, c;


	USED(tim);
	if(q == 0)
		panic("recv null q");
loop:
	lock(q);
	c = q->count;
	if(c > 0) {
		i = q->loc;
		a = q->args[i];
		i++;
		if(i >= q->size)
			i = 0;
		q->loc = i;
		q->count = c-1;
		p = q->whead;
		if(p) {
			q->whead = p->qnext;
			if(q->whead == 0)
				q->wtail = 0;
			ready(p);
		}
		unlock(q);
		return a;
	}
	p = q->rtail;
	if(p == 0)
		q->rhead = u;
	else
		p->qnext = u;
	q->rtail = u;
	u->qnext = 0;
	u->state = Recving;
	unlock(q);
	sched();
	goto loop;
}

void
send(Queue *q, void *a)
{
	User *p;
	int i, c;

	if(q == 0)
		panic("send null q");
loop:
	lock(q);
	c = q->count;
	if(c < q->size) {
		i = q->loc + c;
		if(i >= q->size)
			i -= q->size;
		q->args[i] = a;
		q->count = c+1;
		p = q->rhead;
		if(p) {
			q->rhead = p->qnext;
			if(q->rhead == 0)
				q->rtail = 0;
			ready(p);
		}
		unlock(q);
		return;
	}
	p = q->wtail;
	if(p == 0)
		q->whead = u;
	else
		p->qnext = u;
	q->wtail = u;
	u->qnext = 0;
	u->state = Sending;
	unlock(q);
	sched();
	goto loop;
}

Queue*
newqueue(int size)
{
	Queue *q;

	q = ialloc(sizeof(Queue) + (size-1)*sizeof(void*), 0);
	q->size = size;
	lock(q);
	unlock(q);
	return q;
}

no(void *a)
{

	USED(a);
	return 0;
}

int
devread(Device a, long b, void *c)
{

	switch(a.type)
	{
	case Devcw:
		return cwread(a, b, c);

	case Devro:
		return roread(a, b, c);

	case Devwren:
		return wrenread(a, b, c);

	case Devworm:
		return wormread(a, b, c);

	case Devfworm:
		return fwormread(a, b, c);

	case Devmcat:
		return mcatread(a, b, c);

	case Devmlev:
		return mlevread(a, b, c);

	case Devpart:
		return partread(a, b, c);
	}
	panic("illegal device in read: %D %ld", a, b);
	return 1;
}

int
devwrite(Device a, long b, void *c)
{

	switch(a.type)
	{
	case Devcw:
		return cwwrite(a, b, c);

	case Devro:
		print("write to ro device %D(%ld)\n", a, b);
		return 1;

	case Devwren:
		return wrenwrite(a, b, c);

	case Devworm:
		return wormwrite(a, b, c);

	case Devfworm:
		return fwormwrite(a, b, c);

	case Devmcat:
		return mcatwrite(a, b, c);

	case Devmlev:
		return mlevwrite(a, b, c);

	case Devpart:
		return partwrite(a, b, c);
	}
	panic("illegal device in write: %D %ld", a, b);
	return 1;
}

long
devsize(Device d)
{

	switch(d.type)
	{
	case Devcw:
	case Devro:
		return cwsize(d);

	case Devwren:
		return wrensize(d);

	case Devworm:
		return wormsize(d);

	case Devfworm:
		return fwormsize(d);

	case Devmcat:
		return mcatsize(d);

	case Devmlev:
		return mlevsize(d);

	case Devpart:
		return partsize(d);
	}
	panic("illegal device in dev_size: %D", d);
	return 0;
}

long
superaddr(Device d)
{

	switch(d.type) {
	default:
		return SUPER_ADDR;

	case Devcw:
	case Devro:
		return cwsaddr(d);
	}
}

long
getraddr(Device d)
{

	switch(d.type) {
	default:
		return ROOT_ADDR;

	case Devcw:
		return cwraddr(d, 0);

	case Devro:
		return cwraddr(d, 1);
	}
}

void
devream(Device d)
{
	Device wdev;

	print("ream: %D\n", d);
	switch(d.type) {
	default:
	bad:
		print("ream: unknown dev type %D\n", d);
		return;

	case Devcw:
		wdev = WDEV(d);
		if(wdev.type == Devfworm)
			fwormream(wdev);
		wlock(&mainlock);	/* ream cw */
		cwream(d);
		wunlock(&mainlock);
		break;

	case Devpart:
	case Devmlev:
	case Devmcat:
	case Devwren:
		devinit(d);
		wlock(&mainlock);	/* ream wren */
		rootream(d, ROOT_ADDR);
		superream(d, SUPER_ADDR);
		wunlock(&mainlock);
		break;
	}
}

void
devrecover(Device d)
{

	print("recover: %D\n", d);
	switch(d.type) {
	default:
		print("recover: unknown dev type %D\n", d);
		return;

	case Devcw:
		wlock(&mainlock);	/* recover */
		cwrecover(d);
		wunlock(&mainlock);
		break;
	}
}

void
devinit(Device d)
{
	Filsys *fs;

	print("	devinit %D\n", d);
	switch(d.type) {
	default:
		print("devinit unknown device %D\n", d);

	case Devcw:
		cwinit(d);
		break;

	case Devro:
		for(fs=filsys; fs->name; fs++)
			if(fs->dev.type == Devcw)
				d = fs->dev;
		d.type = Devro;
		for(fs=filsys; fs->name; fs++)
			if(fs->dev.type == Devro)
				fs->dev = d;
		break;

	case Devwren:
		wreninit(d);
		break;

	case Devworm:
		break;

	case Devfworm:
		fworminit(d);
		break;

	case Devmcat:
		mcatinit(d);
		break;

	case Devmlev:
		mlevinit(d);
		break;

	case Devpart:
		partinit(d);
		break;
	}
}
