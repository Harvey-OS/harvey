#include <u.h>
#include <libc.h>
#include <bio.h>
#include "snap.h"

void
panic(char *s)
{
	fprint(2, "%s\n", s);
	abort();
	exits(s);
}

static Proc*
findpid(Proc *plist, long pid)
{
	while(plist) {
		if(plist->pid == pid)
			break;
		plist = plist->link;
	}
	return plist;
}

Page*
findpage(Proc *plist, long pid, int type, ulong off)
{
	Seg *s;
	int i;

	plist = findpid(plist, pid);
	if(plist == nil)
		panic("can't find referenced pid");

	if(type == 't') {
		if(off%Pagesize)
			panic("bad text offset alignment");
		s = plist->text;
		if(off >= s->len)
			return nil;
		return s->pg[off/Pagesize];
	}

	s = nil;
	for(i=0; i<plist->nseg; i++) {
		s = plist->seg[i];
		if(s && s->offset <= off && off < s->offset+s->len)
			break;
		s = nil;
	}
	if(s == nil)
		return nil;

	off -= s->offset;
	if(off%Pagesize)
		panic("bad mem offset alignment");

	return s->pg[off/Pagesize];
}

static Data*
readdata(Biobuf *b)
{
	Data *d;
	char str[32];
	long len;

	if(Bread(b, str, 12) != 12)
		panic("can't read data hdr\n");

	len = atoi(str);
	d = emalloc(sizeof(*d) + len);
	if(Bread(b, d->data, len) != len)
		panic("can't read data body\n");
	d->len = len;
	return d;
}

static Seg*
readseg(Seg **ps, Biobuf *b, Proc *plist)
{
	Seg *s;
	Page **pp;
	int i, npg;
	int t;
	int len;
	ulong pid, off;
	char buf[Pagesize];
	static char zero[Pagesize];

	s = emalloc(sizeof *s);
	if(Bread(b, buf, 2*12) != 2*12)
		panic("error reading segment");

	s->offset = atoi(buf);
	s->len = atoi(buf+12);
	npg = (s->len + Pagesize-1)/Pagesize;
	s->npg = npg;

	pp = emalloc(sizeof(*pp)*npg);
	s->pg = pp;
	*ps = s;

	len = Pagesize;
	for(i=0; i<npg; i++) {
		if(i == npg-1)
			len = s->len - i*Pagesize;

		switch(t = Bgetc(b)) {
		case 'z':
			pp[i] = datapage(zero, len);
			if(debug)
				fprint(2, "0x%.8lux all zeros\n", s->offset+i*Pagesize);
			break;
		case 'm':
		case 't':
			if(Bread(b, buf, 2*12) != 2*12)
				panic("error reading segment");
			pid = atol(buf);
			off = atol(buf+12);
			pp[i] = findpage(plist, pid, t, off);
			if(pp[i] == nil)
				panic("bad page reference in snapshot");
			if(debug)
				fprint(2, "0x%.8lux same as %s pid %lud 0x%.8lux\n", s->offset+i*Pagesize, t=='m'?"mem":"text", pid, off);
			break;
		case 'r':
			if(Bread(b, buf, len) != len)
				panic("error reading segment");
			pp[i] = datapage(buf, len);
			if(debug)
				fprint(2, "0x%.8lux is raw data\n", s->offset+i*Pagesize);
			break;
		default:
			panic("error reading segment");
		}
	}
	return s;
}

Proc*
readsnap(Biobuf *b)
{
	char *q;
	char buf[12];
	long pid;
	Proc *p, *plist;
	int i, n;

	if((q = Brdline(b, '\n')) == nil)
		panic("error reading snapshot file");
	if(strncmp(q, "process snapshot", strlen("process snapshot")) != 0)
		panic("bad snapshot file format");

	plist = nil;
	while(q = Brdline(b, '\n')) {
		q[Blinelen(b)-1] = 0;
		pid = atol(q);
		q += 12;
		p = findpid(plist, pid);
		if(p == nil) {
			p = emalloc(sizeof(*p));
			p->link = plist;
			p->pid = pid;
			plist = p;
		}

		for(i=0; i<Npfile; i++) {
			if(strcmp(pfile[i], q) == 0) {
				p->d[i] = readdata(b);
				break;
			}
		}
		if(i != Npfile)
			continue;
		if(strcmp(q, "mem") == 0) {
			if(Bread(b, buf, 12) != 12) 
				panic("can't read memory section");
			n = atoi(buf);
			p->nseg = n;
			p->seg = emalloc(n*sizeof(*p->seg));
			for(i=0; i<n; i++)
				readseg(&p->seg[i], b, plist);
		} else if(strcmp(q, "text") == 0)
			readseg(&p->text, b, plist);
		else
			panic("unknown section");
	}
	return plist;
}
