#include "all.h"
#include "io.h"

enum {
	Slop = 256,	/* room at the start of a message buf for proto hdrs */
};

Filsys*
fsstr(char *p)
{
	Filsys *fs;

	for(fs=filsys; fs->name; fs++)
		if(strcmp(fs->name, p) == 0)
			return fs;
	return 0;
}

Filsys*
dev2fs(Device *dev)
{
	Filsys *fs;

	for(fs=filsys; fs->name; fs++)
		if(fs->dev == dev)
			return fs;
	return 0;
}

/*
 * allocate 'count' contiguous channels
 * of type 'type' and return pointer to base
 */
Chan*
fs_chaninit(int type, int count, int data)
{
	uchar *p;
	Chan *cp, *icp;
	int i;

	p = malloc(count * (sizeof(Chan)+data));
	icp = (Chan*)p;
	for(i = 0; i < count; i++) {
		cp = (Chan*)p;
		cp->next = chans;
		chans = cp;
		cp->type = type;
		cp->chan = cons.chano;
		cons.chano++;
		strncpy(cp->whoname, "<none>", sizeof cp->whoname);
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
	for (h=0; h < nelem(flist); h++)
		for (prev=0, f = flist[h]; f; prev=f, f=f->next) {
			if(f->cp != cp)
				continue;
			if(prev) {
				prev->next = f->next;
				f->next = flist[h];
				flist[h] = f;
			}
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
			authfree(f->auth);
			f->auth = 0;
			f->cp = 0;
			qunlock(f);
			goto loop;
		}
	unlock(&flock);
}

enum { NOFID = (ulong)~0 };

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

	h = (long)(uintptr)cp + fid;
	if(h < 0)
		h = ~h;
	h %= nelem(flist);

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
	static int first;
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

	h = (long)(uintptr)cp + fp->fid;
	if(h < 0)
		h = ~h;
	h %= nelem(flist);

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
	if(f->uid != 0) {
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
	}

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
	Off qpath;
	Timet tim;
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

Off
qidpathgen(Device *dev)
{
	Iobuf *p;
	Superb *sb;
	Off path;

	p = getbuf(dev, superaddr(dev), Brd|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("newqid: super block");
	sb = (Superb*)p->iobuf;
	sb->qidgen++;
	path = sb->qidgen;
	putbuf(p);
	return path;
}

/* truncating to length > 0 */
static void
truncfree(Truncstate *ts, Device *dev, int d, Iobuf *p, int i)
{
	int pastlast;
	Off a;

	pastlast = ts->pastlast;
	a = ((Off *)p->iobuf)[i];
	if (d > 0 || pastlast)
		buffree(dev, a, d, ts);
	if (pastlast) {
		((Off *)p->iobuf)[i] = 0;
		p->flags |= Bmod|Bimm;
	} else if (d == 0 && ts->relblk == ts->lastblk)
		ts->pastlast = 1;
	if (d == 0)
		ts->relblk++;
}

/*
 * free the block at `addr' on dev.
 * if it's an indirect block (d [depth] > 0),
 * first recursively free all the blocks it names.
 *
 * ts->relblk is the block number within the file of this
 * block (or the first data block eventually pointed to via
 * this indirect block).
 */
void
buffree(Device *dev, Off addr, int d, Truncstate *ts)
{
	Iobuf *p;
	Off a;
	int i, pastlast;

	if(!addr)
		return;
	pastlast = (ts == nil? 1: ts->pastlast);
	/*
	 * if this is an indirect block, recurse and free any
	 * suitable blocks within it (possibly via further indirect blocks).
	 */
	if(d > 0) {
		d--;
		p = getbuf(dev, addr, Brd);
		if(p) {
			if (ts == nil)		/* common case: create */
				for(i=INDPERBUF-1; i>=0; i--) {
					a = ((Off *)p->iobuf)[i];
					buffree(dev, a, d, nil);
				}
			else			/* wstat truncation */
				for (i = 0; i < INDPERBUF; i++)
					truncfree(ts, dev, d, p, i);
			putbuf(p);
		}
	}
	if (!pastlast)
		return;
	/*
	 * having zeroed the pointer to this block, add it to the free list.
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
	p = getbuf(dev, superaddr(dev), Brd|Bmod);
	if(!p || checktag(p, Tsuper, QPSUPER))
		panic("buffree: super block");
	addfree(dev, addr, (Superb*)p->iobuf);
	putbuf(p);
}

Off
bufalloc(Device *dev, int tag, long qid, int uid)
{
	Iobuf *bp, *p;
	Superb *sb;
	Off a, n;

	p = getbuf(dev, superaddr(dev), Brd|Bmod);
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
		print("bufalloc: %Z: bad freelist\n", dev);
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
			print("fs %Z full uid=%d\n", dev, uid);
			return 0;
		}
		bp = getbuf(dev, a, Brd);
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
	if(tag == Tind1 || tag == Tind2 ||
#ifndef COMPAT32
	    tag == Tind3 || tag == Tind4 ||  /* add more Tind tags here ... */
#endif
	    tag == Tdir)
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
addfree(Device *dev, Off addr, Superb *sb)
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

/*
static int
Yfmt(Fmt* fmt)
{
	Chan *cp;
	char s[20];

	cp = va_arg(fmt->args, Chan*);
	sprint(s, "C%d.%.3d", cp->type, cp->chan);
	return fmtstrcpy(fmt, s);
}
 */

static int
Zfmt(Fmt* fmt)
{
	Device *d;
	int c, c1;
	char s[100];

	d = va_arg(fmt->args, Device*);
	if(d == nil) {
		sprint(s, "Z***");
		goto out;
	}
	c = c1 = '\0';
	switch(d->type) {
	default:
		sprint(s, "D%d", d->type);
		break;
	case Devwren:
		c = 'w';
		/* fallthrough */
	case Devworm:
		if (c == '\0')
			c = 'r';
		/* fallthrough */
	case Devlworm:
		if (c == '\0')
			c = 'l';
		if(d->wren.ctrl == 0 && d->wren.lun == 0)
			sprint(s, "%c%d", c, d->wren.targ);
		else
			sprint(s, "%c%d.%d.%d", c, d->wren.ctrl, d->wren.targ,
				d->wren.lun);
		break;
	case Devmcat:
		c = '(';
		c1 = ')';
		/* fallthrough */
	case Devmlev:
		if (c == '\0') {
			c = '[';
			c1 = ']';
		}
		/* fallthrough */
	case Devmirr:
		if (c == '\0') {
			c = '{';
			c1 = '}';
		}
		if(d->cat.first == d->cat.last)
			sprint(s, "%c%Z%c", c, d->cat.first, c1);
		else if(d->cat.first->link == d->cat.last)
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
	case Devfworm:
		sprint(s, "f%Z", d->fw.fw);
		break;
	case Devpart:
		sprint(s, "p(%Z)%ld.%ld", d->part.d, d->part.base, d->part.size);
		break;
	case Devswab:
		sprint(s, "x%Z", d->swab.d);
		break;
	case Devnone:
		sprint(s, "n");
		break;
	}
out:
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

void
formatinit(void)
{
	quotefmtinstall();
//	fmtinstall('Y', Yfmt);	/* print channels */
	fmtinstall('Z', Zfmt);	/* print devices */
	fmtinstall('G', Gfmt);	/* print tags */
	fmtinstall('T', Tfmt);	/* print times */
//	fmtinstall('E', eipfmt);	/* print ether addresses */
	fmtinstall('I', eipfmt);	/* print ip addresses */
}

void
rootream(Device *dev, Off addr)
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
	d->atime = time(nil);
	d->mtime = d->atime;
	d->muid = 0;
	putbuf(p);
}

void
superream(Device *dev, Off addr)
{
	Iobuf *p;
	Superb *s;
	Off i;

	p = getbuf(dev, addr, Bmod|Bimm);
	memset(p->iobuf, 0, RBUFSIZE);
	settag(p, Tsuper, QPSUPER);

	s = (Superb*)p->iobuf;
	s->fstart = 2;
	s->fsize = devsize(dev);
	s->fbuf.nfree = 1;
	s->qidgen = 10;
	for(i = s->fsize-1; i >= addr+2; i--)
		addfree(dev, i, s);
	putbuf(p);
}

struct
{
	Lock;
	Msgbuf	*smsgbuf;
	Msgbuf	*lmsgbuf;
} msgalloc;

/*
 * pre-allocate some message buffers at boot time.
 * if this supply is exhausted, more will be allocated as needed.
 */
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
		mb = malloc(sizeof(Msgbuf));
		mb->magic = Mbmagic;
		mb->xdata = malloc(LARGEBUF+Slop);
		mb->flags = LARGE;
		mbfree(mb);
		cons.nlarge++;
	}
	for(i=0; i<conf.nsmmsg; i++) {
		mb = malloc(sizeof(Msgbuf));
		mb->magic = Mbmagic;
		mb->xdata = malloc(SMALLBUF+Slop);
		mb->flags = 0;
		mbfree(mb);
		cons.nsmall++;
	}
	memset(mballocs, 0, sizeof(mballocs));

	lock(&rabuflock);
	unlock(&rabuflock);
	rabuffree = 0;
	for(i=0; i<1000; i++) {
		rb = malloc(sizeof(*rb));
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
		if(mb == nil) {
			mb = malloc(sizeof(Msgbuf));
			mb->xdata = malloc(LARGEBUF+Slop);
			cons.nlarge++;
		} else
			msgalloc.lmsgbuf = mb->next;
		mb->flags = LARGE;
	} else {
		mb = msgalloc.smsgbuf;
		if(mb == nil) {
			mb = malloc(sizeof(Msgbuf));
			mb->xdata = malloc(SMALLBUF+Slop);
			cons.nsmall++;
		} else
			msgalloc.smsgbuf = mb->next;
		mb->flags = 0;
	}
	mballocs[category]++;
	unlock(&msgalloc);
	mb->magic = Mbmagic;
	mb->count = count;
	mb->chan = cp;
	mb->next = 0;
	mb->param = 0;
	mb->category = category;
	mb->data = mb->xdata+Slop;
	return mb;
}

void
mbfree(Msgbuf *mb)
{
	if(mb == nil)
		return;
	assert(mb->magic == Mbmagic);
	if (mb->magic != Mbmagic)
		panic("mbfree: bad magic 0x%lux", mb->magic);
	if(mb->flags & BTRACE)
		print("mbfree: BTRACE cat=%d flags=%ux, caller %#p\n",
			mb->category, mb->flags, getcallerpc(&mb));

	if(mb->flags & FREE)
		panic("mbfree already free");

	lock(&msgalloc);
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
	mb->magic = 0;
	unlock(&msgalloc);
}

/*
 * returns 1 if n is prime
 * used for adjusting lengths
 * of hashing things.
 * there is no need to be clever
 */
int
prime(vlong n)
{
	long i;

	if((n%2) == 0)
		return 0;
	for(i=3;; i+=2) {
		if((n%i) == 0)
			return 0;
		if((vlong)i*i >= n)
			return 1;
	}
}

char*
getwrd(char *word, char *line)
{
	int c, n;

	while(isascii(*line) && isspace(*line) && *line != '\n')
		line++;
	for(n = 0; n < Maxword; n++) {
		c = *line;
		if(c == '\0' || isascii(c) && isspace(c))
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
	for(i = 0; i < n; i++) {
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
fs_recv(Queue *q, int)
{
	void *a;
	int i, c;

	if(q == nil)
		panic("recv null q");
	qlock(q);
	q->waitedfor = 1;
	while((c = q->count) <= 0)
		rsleep(&q->empty);
	i = q->loc;
	a = q->args[i];
	i++;
	if(i >= q->size)
		i = 0;
	q->loc = i;
	q->count = c-1;
	rwakeup(&q->full);			/* no longer full */
	qunlock(q);
	return a;
}

void
fs_send(Queue *q, void *a)
{
	int i, c;

	if(q == nil)
		panic("send null q");
	if(!q->waitedfor) {
		for (i = 0; i < 5 && !q->waitedfor; i++)
			sleep(1000);
		if(!q->waitedfor) {
			/* likely a bug; don't wait forever */
			print("no readers yet for %s q\n", q->name);
			abort();
		}
	}
	qlock(q);
	while((c = q->count) >= q->size)
		rsleep(&q->full);
	i = q->loc + c;
	if(i >= q->size)
		i -= q->size;
	q->args[i] = a;
	q->count = c+1;
	rwakeup(&q->empty);			/* no longer empty */
	qunlock(q);
}

Queue*
newqueue(int size, char *name)
{
	Queue *q;

	q = malloc(sizeof(Queue) + (size-1)*sizeof(void*));
	q->size = size;
	q->full.l = q->empty.l = &q->QLock;
	q->name = name;
	return q;
}

int
devread(Device *d, Off b, void *c)
{
	int e;

	for (;;)
		switch(d->type) {
		case Devcw:
			return cwread(d, b, c);

		case Devjuke:
			d = d->j.m;
			break;

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

		case Devmirr:
			return mirrread(d, b, c);

		case Devpart:
			return partread(d, b, c);

		case Devswab:
			e = devread(d->swab.d, b, c);
			if(e == 0)
				swab(c, 0);
			return e;

		case Devnone:
			print("read from device none(%lld)\n", (Wideoff)b);
			return 1;
		default:
			panic("illegal device in devread: %Z %lld",
				d, (Wideoff)b);
			return 1;
		}
}

int
devwrite(Device *d, Off b, void *c)
{
	int e;

	/*
	 * set readonly to non-0 to prevent all writes;
	 * mainly for trying dangerous experiments.
	 */
	if (readonly)
		return 0;
	for (;;)
		switch(d->type) {
		case Devcw:
			return cwwrite(d, b, c);

		case Devjuke:
			d = d->j.m;
			break;

		case Devro:
			print("write to ro device %Z(%lld)\n", d, (Wideoff)b);
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

		case Devmirr:
			return mirrwrite(d, b, c);

		case Devpart:
			return partwrite(d, b, c);

		case Devswab:
			swab(c, 1);
			e = devwrite(d->swab.d, b, c);
			swab(c, 0);
			return e;

		case Devnone:
			/* checktag() can generate blocks with type devnone */
			return 0;
		default:
			panic("illegal device in devwrite: %Z %lld",
				d, (Wideoff)b);
			return 1;
		}
}

Devsize
devsize(Device *d)
{
	for (;;)
		switch(d->type) {
		case Devcw:
		case Devro:
			return cwsize(d);

		case Devjuke:
			d = d->j.m;
			break;

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

		case Devmirr:
			return mirrsize(d);

		case Devpart:
			return partsize(d);

		case Devswab:
			d = d->swab.d;
			break;
		default:
			panic("illegal device in devsize: %Z", d);
			return 0;
		}
}

/* result is malloced */
char *
sdof(Device *d)
{
	static char name[256];

	for (;;)
		switch(d->type) {
		case Devjuke:
			d = d->j.j;		/* robotics */
			break;
		case Devwren:
			snprint(name, sizeof name, "/dev/sd%d%d", d->wren.ctrl,
				d->wren.targ);
			return strdup(name);
		case Devswab:
			d = d->swab.d;
			break;
		default:
			panic("illegal device in sdof: %Z", d);
			return nil;
		}
}

Off
superaddr(Device *d)
{
	for (;;)
		switch(d->type) {
		default:
			return SUPER_ADDR;
		case Devcw:
		case Devro:
			return cwsaddr(d);
		case Devswab:
			d = d->swab.d;
			break;
		}
}

Off
getraddr(Device *d)
{
	for (;;)
		switch(d->type) {
		default:
			return ROOT_ADDR;
		case Devcw:
		case Devro:
			return cwraddr(d);
		case Devswab:
			d = d->swab.d;
			break;
		}
}

void
devream(Device *d, int top)
{
	Device *l;

loop:
	print("\tdevream: %Z %d\n", d, top);
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
	case Devmirr:
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
	for (;;) {
		print("recover: %Z\n", d);
		switch(d->type) {
		default:
			print("recover: unknown dev type %Z\n", d);
			return;

		case Devcw:
			wlock(&mainlock);	/* recover */
			cwrecover(d);
			wunlock(&mainlock);
			return;

		case Devswab:
			d = d->swab.d;
			break;
		}
	}
}

void
devinit(Device *d)
{
	for (;;) {
		if(d->init)
			return;
		d->init = 1;
		print("\tdevinit %Z\n", d);
		switch(d->type) {
		default:
			print("devinit unknown device %Z\n", d);
			return;

		case Devro:
			cwinit(d->ro.parent);
			return;

		case Devcw:
			cwinit(d);
			return;

		case Devjuke:
			jukeinit(d);
			return;

		case Devwren:
			wreninit(d);
			return;

		case Devworm:
		case Devlworm:
			return;

		case Devfworm:
			fworminit(d);
			return;

		case Devmcat:
			mcatinit(d);
			return;

		case Devmlev:
			mlevinit(d);
			return;

		case Devmirr:
			mirrinit(d);
			return;

		case Devpart:
			partinit(d);
			return;

		case Devswab:
			d = d->swab.d;
			break;

		case Devnone:
			print("devinit of Devnone\n");
			return;
		}
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

void
swab8(void *c)
{
	uchar *p;
	int t;

	p = c;

	t = p[0];
	p[0] = p[7];
	p[7] = t;

	t = p[1];
	p[1] = p[6];
	p[6] = t;

	t = p[2];
	p[2] = p[5];
	p[5] = t;

	t = p[3];
	p[3] = p[4];
	p[4] = t;
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
	Off *l;

	/* swab the tag */
	p = (uchar*)c;
	t = (Tag*)(p + BUFSIZE);
	if(!flag) {
		swab2(&t->pad);
		swab2(&t->tag);
		swaboff(&t->path);
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
		swaboff(&s->fbuf.nfree);
		for(i=0; i<FEPERBUF; i++)
			swaboff(&s->fbuf.free[i]);
		swaboff(&s->fstart);
		swaboff(&s->fsize);
		swaboff(&s->tfree);
		swaboff(&s->qidgen);
		swaboff(&s->cwraddr);
		swaboff(&s->roraddr);
		swaboff(&s->last);
		swaboff(&s->next);
		break;

	case Tdir:
		for(i=0; i<DIRPERBUF; i++) {
			d = (Dentry*)p + i;
			swab2(&d->uid);
			swab2(&d->gid);
			swab2(&d->mode);
			swab2(&d->muid);
			swaboff(&d->qid.path);
			swab4(&d->qid.version);
			swaboff(&d->size);
			for(j=0; j<NDBLOCK; j++)
				swaboff(&d->dblock[j]);
			for (j = 0; j < NIBLOCK; j++)
				swaboff(&d->iblocks[j]);
			swab4(&d->atime);
			swab4(&d->mtime);
		}
		break;

	case Tind1:
	case Tind2:
#ifndef COMPAT32
	case Tind3:
	case Tind4:
	/* add more Tind tags here ... */
#endif
		l = (Off *)p;
		for(i=0; i<INDPERBUF; i++) {
			swaboff(l);
			l++;
		}
		break;

	case Tfree:
		f = (Fbuf*)p;
		swaboff(&f->nfree);
		for(i=0; i<FEPERBUF; i++)
			swaboff(&f->free[i]);
		break;

	case Tbuck:
		for(i=0; i<BKPERBLK; i++) {
			b = (Bucket*)p + i;
			swab4(&b->agegen);
			for(j=0; j<CEPERBK; j++) {
				swab2(&b->entry[j].age);
				swab2(&b->entry[j].state);
				swaboff(&b->entry[j].waddr);
			}
		}
		break;

	case Tcache:
		h = (Cache*)p;
		swaboff(&h->maddr);
		swaboff(&h->msize);
		swaboff(&h->caddr);
		swaboff(&h->csize);
		swaboff(&h->fsize);
		swaboff(&h->wsize);
		swaboff(&h->wmax);
		swaboff(&h->sbaddr);
		swaboff(&h->cwraddr);
		swaboff(&h->roraddr);
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
		swaboff(&t->path);
	}
}
