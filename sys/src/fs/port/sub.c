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
chaninit(int type, int count, int data)
{
	uchar *p;
	Chan *cp, *icp;
	int i;

	p = ialloc(count * (sizeof(Chan)+data), 0);
	icp = (Chan*)p;
	for(i=0; i<count; i++) {
		cp = (Chan*)p;
		cp->next = chans;
		chans = cp;
		cp->type = type;
		cp->chan = cons.chano;
		cons.chano++;
		strncpy(cp->whoname, "<none>", sizeof(cp->whoname));
		dofilter(&cp->work, C0a, C0b, 1);
		dofilter(&cp->rate, C0a, C0b, 1000);
		wlock(&cp->reflock);
		wunlock(&cp->reflock);
		rlock(&cp->reflock);
		runlock(&cp->reflock);

		p += sizeof(Chan);
		if(data){
			cp->pdata = p;
			p += data;
		}
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
		if(t->file == f)
			t->time = 0;	/* free the lock */
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

#define NOFID (ulong)~0

/*
 * returns a locked file structure
 */
File*
filep(Chan *cp, ulong fid, int flag)
{
	File *f;
	int h;

	if(fid == NOFID)
		return 0;

	h = (long)cp + fid;
	if(h < 0)
		h = ~h;
	h = h % nelem(flist);

loop:
	lock(&flock);
	for(f=flist[h]; f; f=f->next)
		if(f->fid == fid && f->cp == cp){
			/*
			 * Already in use is an error
			 * when called from attach or clone (walk
			 * in 9P2000). The console uses FID[12] and
			 * never clunks them so catch that case.
			 */
			if(flag == 0 || cp == cons.chan)
				goto out;
			unlock(&flock);
			return 0;
		}

	if(flag) {
		f = newfp();
		if(f) {
			f->fid = fid;
			f->cp = cp;
			f->wpath = 0;
			f->tlock = 0;
			f->doffset = 0;
			f->dslot = 0;
			f->auth = 0;
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

	/* uid none gets only other permissions */
	if(f->uid == 0)
		goto doother;

	/*
	 * owner
	 */
	if(f->uid == d->uid)
		if((m<<6) & d->mode)
			return 0;
	/*
	 * group membership
	 */
	if(ingroup(f->uid, d->gid))
		if((m<<3) & d->mode)
			return 0;

doother:
	/*
	 * other
	 */
	if(m & d->mode) {
		if((d->mode & DDIR) && (m == DEXEC))
			return 0;
		if(!ingroup(f->uid, 9999))
			return 0;
	}

	/*
	 * various forms of superuser
	 */
	if(wstatallow)
		return 0;
	if(duallow != 0 && duallow == f->uid)
		if((d->mode & DDIR) && (m == DREAD || m == DEXEC))
			return 0;

	return 1;
}

Tlock*
tlocked(Iobuf *p, Dentry *d)
{
	Tlock *t, *t1;
	long qpath, tim;
	Device *dev;

	tim = toytime();
	qpath = d->qid.path;
	dev = p->dev;

again:
	t1 = 0;
	for(t=tlocks+NTLOCK-1; t>=tlocks; t--) {
		if(t->qpath == qpath)
		if(t->time >= tim)
		if(t->dev == dev)
			return nil;		/* its locked */
		if(t1 != nil && t->time == 0)
			t1 = t;			/* remember free lock */
	}
	if(t1 == 0) {
		// reclaim old locks
		lock(&tlocklock);
		for(t=tlocks+NTLOCK-1; t>=tlocks; t--)
			if(t->time < tim) {
				t->time = 0;
				t1 = t;
			}
		unlock(&tlocklock);
	}
	if(t1) {
		lock(&tlocklock);
		if(t1->time != 0) {
			unlock(&tlocklock);
			goto again;
		}
		t1->dev = dev;
		t1->qpath = qpath;
		t1->time = tim + TLOCK;
		unlock(&tlocklock);
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

long
qidpathgen(Device *dev)
{
	Iobuf *p;
	Superb *sb;
	long path;

	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("newqid: super block");
	sb = (Superb*)p->iobuf;
	sb->qidgen++;
	path = sb->qidgen;
	putbuf(p);
	return path;
}

void
buffree(Device *dev, long addr, int d)
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
	if(dev->type == Devcw) {
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
bufalloc(Device *dev, int tag, long qid, int uid)
{
	Iobuf *bp, *p;
	Superb *sb;
	long a;
	int n;

	p = getbuf(dev, superaddr(dev), Bread|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER)) {
		print("bufalloc: super block\n");
		if(p)
			putbuf(p);
		return 0;
	}
	sb = (Superb*)p->iobuf;

loop:
	n = --sb->fbuf.nfree;
	sb->tfree--;
	if(n < 0 || n >= FEPERBUF) {
		print("bufalloc: bad freelist\n");
		n = 0;
		sb->fbuf.free[0] = 0;
	}
	a = sb->fbuf.free[n];
	if(n <= 0) {
		if(a == 0) {
			sb->tfree = 0;
			sb->fbuf.nfree = 1;
			if(dev->type == Devcw) {
				n = uid;
				if(n < 0 || n >= nelem(growacct))
					n = 0;
				growacct[n]++;
				if(cwgrow(dev, sb, uid))
					goto loop;
			}
			putbuf(p);
			print("fs full uid=%d\n", uid);
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
addfree(Device *dev, long addr, Superb *sb)
{
	int n;
	Iobuf *p;

	n = sb->fbuf.nfree;
	if(n < 0 || n > FEPERBUF)
		panic("addfree: bad freelist");
	if(n >= FEPERBUF) {
		p = getbuf(dev, addr, Bmod|Bimm);
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

static int
Yfmt(Fmt* fmt)
{
	Chan *cp;
	char s[20];

	cp = va_arg(fmt->args, Chan*);
	sprint(s, "C%d.%.3d", cp->type, cp->chan);

	return fmtstrcpy(fmt, s);
}

static int
Zfmt(Fmt* fmt)
{
	Device *d;
	int c, c1;
	char s[100];

	d = va_arg(fmt->args, Device*);
	if(d == 0) {
		sprint(s, "Z***");
		goto out;
	}
	switch(d->type) {
	default:
		sprint(s, "D%d", d->type);
		break;
	case Devwren:
		c = 'w';
		goto d1;
	case Devworm:
		c = 'r';
		goto d1;
	case Devlworm:
		c = 'l';
		goto d1;
	d1:
		if(d->wren.ctrl == 0 && d->wren.lun == 0)
			sprint(s, "%c%d", c, d->wren.targ);
		else
			sprint(s, "%c%d.%d.%d", c, d->wren.ctrl, d->wren.targ, d->wren.lun);
		break;
	case Devmcat:
		c = '(';
		c1 = ')';
		goto d2;
	case Devmlev:
		c = '[';
		c1 = ']';
	d2:
		if(d->cat.first == d->cat.last)
			sprint(s, "%c%Z%c", c, d->cat.first, c1);
		else
		if(d->cat.first->link == d->cat.last)
			sprint(s, "%c%Z%Z%c", c, d->cat.first, d->cat.last, c1);
		else
			sprint(s, "%c%Z-%Z%c", c, d->cat.first, d->cat.last, c1);
		break;
	case Devro:
		sprint(s, "o%Z%Z", d->ro.parent->cw.c, d->ro.parent->cw.w);
		break;
	case Devcw:
		sprint(s, "c%Z%Z", d->cw.c, d->cw.w);
		break;
	case Devjuke:
		sprint(s, "j%Z%Z", d->j.j, d->j.m);
		break;
	case Devpart:
		sprint(s, "p%ld.%ld", d->part.base, d->part.size);
		break;
	case Devswab:
		sprint(s, "x%Z", d->swab.d);
		break;
	}
out:
	return fmtstrcpy(fmt, s);
}

static int
Wfmt(Fmt* fmt)
{
	Filter* a;
	char s[30];

	a = va_arg(fmt->args, Filter*);
	sprint(s, "%lud", fdf(a->filter, a->c3*a->c1));

	return fmtstrcpy(fmt, s);
}

static int
Gfmt(Fmt* fmt)
{
	int t;
	char *s;

	t = va_arg(fmt->args, int);
	s = "<badtag>";
	if(t >= 0 && t < MAXTAG)
		s = tagnames[t];
	return fmtstrcpy(fmt, s);
}

static int
Efmt(Fmt* fmt)
{
	char s[64];
	uchar *p;

	p = va_arg(fmt->args, uchar*);
	sprint(s, "%.2ux%.2ux%.2ux%.2ux%.2ux%.2ux",
		p[0], p[1], p[2], p[3], p[4], p[5]);

	return fmtstrcpy(fmt, s);
}

static int
Ifmt(Fmt* fmt)
{
	char s[64];
	uchar *p;

	p = va_arg(fmt->args, uchar*);
	sprint(s, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	return fmtstrcpy(fmt, s);
}

void
formatinit(void)
{
	quotefmtinstall();
	fmtinstall('Y', Yfmt);	/* print channels */
	fmtinstall('Z', Zfmt);	/* print devices */
	fmtinstall('W', Wfmt);	/* print filters */
	fmtinstall('G', Gfmt);	/* print tags */
	fmtinstall('T', Tfmt);	/* print times */
	fmtinstall('E', Efmt);	/* print ether addresses */
	fmtinstall('I', Ifmt);	/* print ip addresses */
}

void
rootream(Device *dev, long addr)
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
		((DREAD|DEXEC) << 6) |
		((DREAD|DEXEC) << 3) |
		((DREAD|DEXEC) << 0);
	d->qid = QID9P1(QPROOT|QPDIR,0);
	d->atime = time();
	d->mtime = d->atime;
	d->muid = 0;
	putbuf(p);
}

void
superream(Device *dev, long addr)
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

	ilock(&msgalloc);
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
	iunlock(&msgalloc);
	mb->count = count;
	mb->chan = cp;
	mb->next = 0;
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
	if(mb->flags & BTRACE)
		print("mbfree: BTRACE cat=%d flags=%ux, caller 0x%lux\n",
			mb->category, mb->flags, getcallerpc(&mb));
	if(mb->flags & FREE)
		panic("mbfree already free");

	ilock(&msgalloc);
	mballocs[mb->category]--;
	mb->flags |= FREE;
	if(mb->flags & LARGE) {
		mb->next = msgalloc.lmsgbuf;
		msgalloc.lmsgbuf = mb;
	} else {
		mb->next = msgalloc.smsgbuf;
		msgalloc.smsgbuf = mb;
	}
	mb->data = 0;
	iunlock(&msgalloc);
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
recv(Queue *q, int)
{
	User *p;
	void *a;
	int i, c;
	long s;

	if(q == 0)
		panic("recv null q");
	for(;;) {
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
		s = splhi();
		u->state = Recving;
		unlock(q);
		sched();
		splx(s);
	}
	return 0;
}

void
send(Queue *q, void *a)
{
	User *p;
	int i, c;
	long s;

	if(q == 0)
		panic("send null q");
	for(;;) {
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
		s = splhi();
		u->state = Sending;
		unlock(q);
		sched();
		splx(s);
	}
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

no(void*)
{
	return 0;
}

int
devread(Device *d, long b, void *c)
{
	int e;

loop:
	switch(d->type)
	{
	case Devcw:
		return cwread(d, b, c);

	case Devjuke:
		d = d->j.m;
		goto loop;

	case Devro:
		return roread(d, b, c);

	case Devwren:
		return wrenread(d, b, c);

	case Devworm:
	case Devlworm:
		return wormread(d, b, c);

	case Devfworm:
		return fwormread(d, b, c);

	case Devmcat:
		return mcatread(d, b, c);

	case Devmlev:
		return mlevread(d, b, c);

	case Devpart:
		return partread(d, b, c);

	case Devswab:
		e = devread(d->swab.d, b, c);
		if(e == 0)
			swab(c, 0);
		return e;
	}
	panic("illegal device in read: %Z %ld", d, b);
	return 1;
}

int
devwrite(Device *d, long b, void *c)
{
	int e;

loop:
	switch(d->type)
	{
	case Devcw:
		return cwwrite(d, b, c);

	case Devjuke:
		d = d->j.m;
		goto loop;

	case Devro:
		print("write to ro device %Z(%ld)\n", d, b);
		return 1;

	case Devwren:
		return wrenwrite(d, b, c);

	case Devworm:
	case Devlworm:
		return wormwrite(d, b, c);

	case Devfworm:
		return fwormwrite(d, b, c);

	case Devmcat:
		return mcatwrite(d, b, c);

	case Devmlev:
		return mlevwrite(d, b, c);

	case Devpart:
		return partwrite(d, b, c);

	case Devswab:
		swab(c, 1);
		e = devwrite(d->swab.d, b, c);
		swab(c, 0);
		return e;
	}
	panic("illegal device in write: %Z %ld", d, b);
	return 1;
}

long
devsize(Device *d)
{

loop:
	switch(d->type)
	{
	case Devcw:
	case Devro:
		return cwsize(d);

	case Devjuke:
		d = d->j.m;
		goto loop;

	case Devwren:
		return wrensize(d);

	case Devworm:
	case Devlworm:
		return wormsize(d);

	case Devfworm:
		return fwormsize(d);

	case Devmcat:
		return mcatsize(d);

	case Devmlev:
		return mlevsize(d);

	case Devpart:
		return partsize(d);

	case Devswab:
		d = d->swab.d;
		goto loop;
	}
	panic("illegal device in dev_size: %Z", d);
	return 0;
}

long
superaddr(Device *d)
{

loop:
	switch(d->type) {
	default:
		return SUPER_ADDR;

	case Devcw:
	case Devro:
		return cwsaddr(d);

	case Devswab:
		d = d->swab.d;
		goto loop;
	}
}

long
getraddr(Device *d)
{

loop:
	switch(d->type) {
	default:
		return ROOT_ADDR;

	case Devcw:
	case Devro:
		return cwraddr(d);

	case Devswab:
		d = d->swab.d;
		goto loop;
	}
}

void
devream(Device *d, int top)
{
	Device *l;

loop:
	print("	devream: %Z %d\n", d, top);
	switch(d->type) {
	default:
		print("ream: unknown dev type %Z\n", d);
		return;

	case Devcw:
		devream(d->cw.w, 0);
		devream(d->cw.c, 0);
		if(top) {
			wlock(&mainlock);
			cwream(d);
			wunlock(&mainlock);
		}
		devinit(d);
		return;

	case Devfworm:
		devream(d->fw.fw, 0);
		fwormream(d);
		break;

	case Devpart:
		devream(d->part.d, 0);
		break;

	case Devmlev:
	case Devmcat:
		for(l=d->cat.first; l; l=l->link)
			devream(l, 0);
		break;

	case Devjuke:
	case Devworm:
	case Devlworm:
	case Devwren:
		break;

	case Devswab:
		d = d->swab.d;
		goto loop;
	}
	devinit(d);
	if(top) {
		wlock(&mainlock);
		rootream(d, ROOT_ADDR);
		superream(d, SUPER_ADDR);
		wunlock(&mainlock);
	}
}

void
devrecover(Device *d)
{

loop:
	print("recover: %Z\n", d);
	switch(d->type) {
	default:
		print("recover: unknown dev type %Z\n", d);
		return;

	case Devcw:
		wlock(&mainlock);	/* recover */
		cwrecover(d);
		wunlock(&mainlock);
		break;

	case Devswab:
		d = d->swab.d;
		goto loop;
	}
}

void
devinit(Device *d)
{

loop:
	if(d->init)
		return;
	d->init = 1;
	print("	devinit %Z\n", d);
	switch(d->type) {
	default:
		print("devinit unknown device %Z\n", d);
		return;

	case Devro:
		cwinit(d->ro.parent);
		break;

	case Devcw:
		cwinit(d);
		break;

	case Devjuke:
		jukeinit(d);
		break;

	case Devwren:
		wreninit(d);
		break;

	case Devworm:
	case Devlworm:
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

	case Devswab:
		d = d->swab.d;
		goto loop;
	}
}

void
swab2(void *c)
{
	uchar *p;
	int t;

	p = c;

	t = p[0];
	p[0] = p[1];
	p[1] = t;
}

void
swab4(void *c)
{
	uchar *p;
	int t;

	p = c;

	t = p[0];
	p[0] = p[3];
	p[3] = t;

	t = p[1];
	p[1] = p[2];
	p[2] = t;
}

/*
 * swab a block
 *	flag = 0 -- convert from foreign to native
 *	flag = 1 -- convert from native to foreign
 */
void
swab(void *c, int flag)
{
	uchar *p;
	Tag *t;
	int i, j;
	Dentry *d;
	Cache *h;
	Bucket *b;
	Superb *s;
	Fbuf *f;
	long *l;

	/* swab the tag */
	p = (uchar*)c;
	t = (Tag*)(p + BUFSIZE);
	if(!flag) {
		swab2(&t->pad);
		swab2(&t->tag);
		swab4(&t->path);
	}

	/* swab each block type */
	switch(t->tag) {

	default:
		print("no swab for tag=%G rw=%d\n", t->tag, flag);
		for(j=0; j<16; j++)
			print(" %.2x", p[BUFSIZE+j]);
		print("\n");
		for(i=0; i<16; i++) {
			print("%.4x", i*16);
			for(j=0; j<16; j++)
				print(" %.2x", p[i*16+j]);
			print("\n");
		}
		panic("swab");
		break;

	case Tsuper:
		s = (Superb*)p;
		swab4(&s->fbuf.nfree);
		for(i=0; i<FEPERBUF; i++)
			swab4(&s->fbuf.free[i]);
		swab4(&s->fstart);
		swab4(&s->fsize);
		swab4(&s->tfree);
		swab4(&s->qidgen);
		swab4(&s->cwraddr);
		swab4(&s->roraddr);
		swab4(&s->last);
		swab4(&s->next);
		break;

	case Tdir:
		for(i=0; i<DIRPERBUF; i++) {
			d = (Dentry*)p + i;
			swab2(&d->uid);
			swab2(&d->gid);
			swab2(&d->mode);
			swab2(&d->muid);
			swab4(&d->qid.path);
			swab4(&d->qid.version);
			swab4(&d->size);
			for(j=0; j<NDBLOCK; j++)
				swab4(&d->dblock[j]);
			swab4(&d->iblock);
			swab4(&d->diblock);
			swab4(&d->atime);
			swab4(&d->mtime);
		}
		break;

	case Tind1:
	case Tind2:
		l = (long*)p;
		for(i=0; i<INDPERBUF; i++) {
			swab4(l);
			l++;
		}
		break;

	case Tfree:
		f = (Fbuf*)p;
		swab4(&f->nfree);
		for(i=0; i<FEPERBUF; i++)
			swab4(&f->free[i]);
		break;

	case Tbuck:
		for(i=0; i<BKPERBLK; i++) {
			b = (Bucket*)p + i;
			swab4(&b->agegen);
			for(j=0; j<CEPERBK; j++) {
				swab2(&b->entry[j].age);
				swab2(&b->entry[j].state);
				swab4(&b->entry[j].waddr);
			}
		}
		break;

	case Tcache:
		h = (Cache*)p;
		swab4(&h->maddr);
		swab4(&h->msize);
		swab4(&h->caddr);
		swab4(&h->csize);
		swab4(&h->fsize);
		swab4(&h->wsize);
		swab4(&h->wmax);
		swab4(&h->sbaddr);
		swab4(&h->cwraddr);
		swab4(&h->roraddr);
		swab4(&h->toytime);
		swab4(&h->time);
		break;

	case Tnone:	// unitialized
	case Tfile:	// someone elses problem
	case Tvirgo:	// bit map -- all bytes
	case Tconfig:	// configuration string -- all bytes
		break;
	}

	/* swab the tag */
	if(flag) {
		swab2(&t->pad);
		swab2(&t->tag);
		swab4(&t->path);
	}
}
