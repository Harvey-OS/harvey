#include	"all.h"
#include	"io.h"

#define	DEBUG	0

extern	long	nhiob;
extern	Hiob	*hiob;

Iobuf*
getbuf(Device *d, long addr, int flag)
{
	Iobuf *p, *s;
	Hiob *hp;
	long h;

	if(DEBUG)
		print("getbuf %Z(%ld) f=%x\n", d, addr, flag);
	h = addr + (long)d*1009L;
	if(h < 0)
		h = ~h;
	h %= nhiob;
	hp = &hiob[h];

loop:
	lock(hp);

/*
 * look for it in the active list
 */
	s = hp->link;
	for(p=s;;) {
		if(p->addr == addr && p->dev == d) {
			if(p != s) {
				p->back->fore = p->fore;
				p->fore->back = p->back;
				p->fore = s;
				p->back = s->back;
				s->back = p;
				p->back->fore = p;
				hp->link = p;
			}
			unlock(hp);
			qlock(p);
			if(p->addr != addr || p->dev != d || iobufmap(p) == 0) {
				qunlock(p);
				goto loop;
			}
			p->flags |= flag;
			cons.bhit[0].count++;
			cons.bhit[1].count++;
			cons.bhit[2].count++;
			return p;
		}
		p = p->fore;
		if(p == s)
			break;
	}
	if(flag & Bprobe) {
		unlock(hp);
		return 0;
	}

/*
 * not found
 * take oldest unlocked entry in this queue
 */
xloop:
	p = s->back;
	if(!canqlock(p)) {
		if(p == hp->link) {
			unlock(hp);
			print("iobuf all locked\n");
			goto loop;
		}
		s = p;
		goto xloop;
	}

	/*
	 * its dangerous to flush the pseudo
	 * devices since they recursively call
	 * getbuf/putbuf. deadlock!
	 */
	if(p->flags & Bres) {
		qunlock(p);
		if(p == hp->link) {
			unlock(hp);
			print("iobuf all reserved\n");
			goto loop;
		}
		s = p;
		goto xloop;
	}
	if(p->flags & Bmod) {
		unlock(hp);
		if(iobufmap(p)) {	
			if(!devwrite(p->dev, p->addr, p->iobuf))
				p->flags &= ~(Bimm|Bmod);
			iobufunmap(p);
		}
		qunlock(p);
		goto loop;
	}
	hp->link = p;
	p->addr = addr;
	p->dev = d;
	p->flags = flag;
	unlock(hp);
	if(iobufmap(p)) {
		if(flag & Bread) {
			if(!devread(p->dev, p->addr, p->iobuf)) {
				cons.bread[0].count++;
				cons.bread[1].count++;
				cons.bread[2].count++;
				return p;
			}
			iobufunmap(p);
		} else {
			cons.binit[0].count++;
			cons.binit[1].count++;
			cons.binit[2].count++;
			return p;
		}
	} else
		print("iobuf cant map buffer\n");
	p->flags = 0;
	p->dev = devnone;
	p->addr = -1;
	qunlock(p);
	return 0;
}

/*
 * syncblock tries to put out a block per hashline
 * returns 0 all done,
 * returns 1 if it missed something
 */
int
syncblock(void)
{
	Iobuf *p, *s, *q;
	Hiob *hp;
	long h;
	int flag;

	flag = 0;
	for(h=0; h<nhiob; h++) {
		q = 0;
		hp = &hiob[h];
		lock(hp);
		s = hp->link;
		for(p=s;;) {
			if(p->flags & Bmod) {
				if(q)
					flag = 1;	/* more than 1 mod/line */
				q = p;
			}
			p = p->fore;
			if(p == s)
				break;
		}
		unlock(hp);
		if(q) {
			if(!canqlock(q)) {
				flag = 1;		/* missed -- was locked */
				continue;
			}
			if(!(q->flags & Bmod)) {
				qunlock(q);
				continue;
			}
			if(iobufmap(q)) {
				if(!devwrite(q->dev, q->addr, q->iobuf))
					q->flags &= ~(Bmod|Bimm);
				iobufunmap(q);
			} else
				flag = 1;
			qunlock(q);
		}
	}
	return flag;
}

void
sync(char *reason)
{
	long i;

	print("sync: %s\n", reason);
	for(i=10*nhiob; i>0; i--)
		if(!syncblock())
			return;
	print("sync shorted\n");
}

void
putbuf(Iobuf *p)
{

	if(canqlock(p))
		print("buffer not locked %Z(%ld)\n", p->dev, p->addr);
	if(p->flags & Bimm) {
		if(!(p->flags & Bmod))
			print("imm and no mod %Z(%ld)\n", p->dev, p->addr);
		if(!devwrite(p->dev, p->addr, p->iobuf))
			p->flags &= ~(Bmod|Bimm);
	}
	iobufunmap(p);
	qunlock(p);
}

int
checktag(Iobuf *p, int tag, long qpath)
{
	Tag *t;
	static long lastaddr;

	t = (Tag*)(p->iobuf+BUFSIZE);
	if(t->tag != tag) {
		if(p->flags & Bmod) {
			print("	tag = %d/%lud; expected %ld/%d -- not flushed\n",
				t->tag, t->path, qpath, tag);
			return 2;
		}
		if(p->dev != nil && p->dev->type == Devcw)
			cwfree(p->dev, p->addr);
		if(p->addr != lastaddr)
			print("	tag = %G/%lud; expected %G/%ld -- flushed (%ld)\n",
				t->tag, t->path, tag, qpath, p->addr);
		lastaddr = p->addr;
		p->dev = devnone;
		p->addr = -1;
		p->flags = 0;
		return 2;
	}
	if(qpath != QPNONE) {
		if((qpath ^ t->path) & ~QPDIR) {
			if(1 || CHAT(0))
				print("	tag/path = %lud; expected %d/%lux\n",
					t->path, tag, qpath);
			return 0;
		}
	}
	return 0;
}

void
settag(Iobuf *p, int tag, long qpath)
{
	Tag *t;

	t = (Tag*)(p->iobuf+BUFSIZE);
	t->tag = tag;
	if(qpath != QPNONE)
		t->path = qpath & ~QPDIR;
	p->flags |= Bmod;
}

int
qlmatch(QLock *q1, QLock *q2)
{

	return q1 == q2;
}

int
iobufql(QLock *q)
{
	Iobuf *p, *s;
	Hiob *hp;
	Tag *t;
	long h;
	int tag;

	for(h=0; h<nhiob; h++) {
		hp = &hiob[h];
		lock(hp);
		s = hp->link;
		for(p=s;;) {
			if(qlmatch(q, p)) {
				t = (Tag*)(p->iobuf+BUFSIZE);
				tag = t->tag;
				if(tag < 0 || tag >= MAXTAG)
					tag = Tnone;
				print("	Iobuf %Z(%ld) t=%s\n",
					p->dev, p->addr, tagnames[tag]);
				unlock(hp);
				return 1;
			}
			p = p->fore;
			if(p == s)
				break;
		}
		unlock(hp);
	}
	return 0;
}
