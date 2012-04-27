#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "snap.h"

/* research 16-bit crc.  good enough. */
static ulong
sumr(ulong sum, void *buf, int n)
{
	uchar *s, *send;

	if(buf == 0)
		return sum;
	for(s=buf, send=s+n; s<send; s++)
		if(sum & 1)
			sum = 0xffff & ((sum>>1)+*s+0x8000);
		else
			sum = 0xffff & ((sum>>1)+*s);
	return sum;
}

static int npage;
static Page *pgtab[1<<10];

Page*
datapage(char *p, long len)
{
	Page *pg;
	char *q, *ep;
	long	sum;
	int iszero;

	if(len > Pagesize) {
		fprint(2, "datapage cannot handle pages > 1024\n");
		exits("datapage");
	}

	sum = sumr(0, p, len) & (nelem(pgtab)-1);
	if(sum == 0) {
		iszero = 1;
		for(q=p, ep=p+len; q<ep; q++)
			if(*q != 0) {
				iszero = 0;
				break;
			}
	} else
		iszero = 0;

	for(pg = pgtab[sum]; pg; pg=pg->link)
		if(pg->len == len && memcmp(pg->data, p, len) == 0)
			break;
	if(pg)
		return pg;

	pg = emalloc(sizeof(*pg)+len);
	pg->data = (char*)&pg[1];
	pg->type = 0;
	pg->len = len;
	memmove(pg->data, p, len);
	pg->link = pgtab[sum];
	pgtab[sum] = pg;
	if(iszero) {
		pg->type = 'z';
		pg->written = 1;
	}

	++npage;
	return pg;
}

static Data*
readsection(long pid, char *sec)
{
	char buf[8192];
	int n, fd;
	int hdr, tot;
	Data *d = nil;

	snprint(buf, sizeof buf, "/proc/%ld/%s", pid, sec);
	if((fd = open(buf, OREAD)) < 0)
		return nil;

	tot = 0;
	hdr = (int)((Data*)0)->data;
	while((n = read(fd, buf, sizeof buf)) > 0) {
		d = erealloc(d, tot+n+hdr);
		memmove(d->data+tot, buf, n);
		tot += n;
	}
	close(fd);
	if(d == nil)
		return nil;
	d->len = tot;
	return d;
}

static Seg*
readseg(int fd, vlong off, ulong len, char *name)
{
	char buf[Pagesize];
	Page **pg;
	int npg;
	Seg *s;
	ulong i;
	int n;

	s = emalloc(sizeof(*s));
	s->name = estrdup(name);

	if(seek(fd, off, 0) < 0) {
		fprint(2, "seek fails\n");
		goto Die;
	}

	pg = nil;
	npg = 0;
	for(i=0; i<len; ) {
		n = Pagesize;
		if(n > len-i)
			n = len-i;
		if((n = readn(fd, buf, n)) <= 0)
			break;
		pg = erealloc(pg, sizeof(*pg)*(npg+1));
		pg[npg++] = datapage(buf, n);
		i += n;
		if(n != Pagesize)	/* any short read, planned or otherwise */
			break;
	}

	if(i==0 && len!=0)
		goto Die;

	s->offset = off;
	s->len = i;
	s->pg = pg;
	s->npg = npg;
	return s;

Die:
	free(s->name);
	free(s);
	return nil;
}

/* discover the stack pointer of the given process */
ulong
stackptr(Proc *proc, int fd)
{
	char *q;
	Fhdr f;
	Reglist *r;
	long textoff;
	int i;
	Data *dreg;

	textoff = -1;
	for(i=0; i<proc->nseg; i++)
		if(proc->seg[i] && strcmp(proc->seg[i]->name, "Text") == 0)
			textoff = proc->seg[i]->offset;

	if(textoff == -1)
		return 0;

	seek(fd, textoff, 0);
	if(crackhdr(fd, &f) == 0)
		return 0;

	machbytype(f.type);
	for(r=mach->reglist; r->rname; r++)
		if(strcmp(r->rname, mach->sp) == 0)
			break;
	if(r == nil) {
		fprint(2, "couldn't find stack pointer register?\n");
		return 0;
	}
	
	if((dreg = proc->d[Pregs]) == nil)
		return 0;

	if(r->roffs+mach->szreg > dreg->len) {
		fprint(2, "SP register too far into registers?\n");
		return 0;
	}

	q = dreg->data+r->roffs;
	switch(mach->szreg) {
	case 2:	return machdata->swab(*(ushort*)q);
	case 4:	return machdata->swal(*(ulong*)q);
	case 8:	return machdata->swav(*(uvlong*)q);
	default:
		fprint(2, "register size is %d bytes?\n", mach->szreg);
		return 0;
	}
}

Proc*
snap(long pid, int usetext)
{
	Data *d;
	Proc *proc;
	Seg **s;
	char *name, *segdat, *q, *f[128+1], buf[128];
	int fd, i, stacki, nf, np;
	uvlong off, len, stackoff, stacklen;
	uvlong sp;

	proc = emalloc(sizeof(*proc));
	proc->pid = pid;

	np = 0;
	for(i=0; i<Npfile; i++) {
		if(proc->d[i] = readsection(pid, pfile[i]))
			np++;
		else
			fprint(2, "warning: can't include /proc/%ld/%s\n", pid, pfile[i]);
	}
	if(np == 0)
		return nil;

	if(usetext) {
		snprint(buf, sizeof buf, "/proc/%ld/text", pid);
		if((fd = open(buf, OREAD)) >= 0) {
			werrstr("");
			if((proc->text = readseg(fd, 0, 1<<31, "textfile")) == nil)
				fprint(2, "warning: can't include %s: %r\n", buf);
			close(fd);
		} else
			fprint(2, "warning: can't include /proc/%ld/text\n", pid);
	}

	if((d=proc->d[Psegment]) == nil) {
		fprint(2, "warning: no segment table, no memory image\n");
		return proc;
	}

	segdat = emalloc(d->len+1);
	memmove(segdat, d->data, d->len);
	segdat[d->len] = 0;

	nf = getfields(segdat, f, nelem(f), 1, "\n");
	if(nf == nelem(f)) {
		nf--;
		fprint(2, "process %ld has >%d segments; only using first %d\n",
			pid, nf, nf);
	}
	if(nf <= 0) {
		fprint(2, "warning: couldn't understand segment table, no memory image\n");
		free(segdat);
		return proc;
	}

	snprint(buf, sizeof buf, "/proc/%ld/mem", pid);
	if((fd = open(buf, OREAD)) < 0) {
		fprint(2, "warning: can't include /proc/%ld/mem\n", pid);
		return proc;
	}

	s = emalloc(nf*sizeof(*s));
	stacklen = 0;
	stackoff = 0;
	stacki = 0;
	for(i=0; i<nf; i++) {
		if(q = strchr(f[i], ' ')) 
			*q = 0;
		name = f[i];
		off = strtoull(name+10, &q, 16);
		len = strtoull(q, &q, 16) - off;
		if(strcmp(name, "Stack") == 0) {
			stackoff = off;
			stacklen = len;
			stacki = i;
		} else
			s[i] = readseg(fd, off, len, name);
	}
	proc->nseg = nf;
	proc->seg = s;

	/* stack hack: figure sp so don't need to page in the whole segment */
	if(stacklen) {
		sp = stackptr(proc, fd);
		if(stackoff <= sp && sp < stackoff+stacklen) {
			off = (sp - Pagesize) & ~(Pagesize - 1);
			if(off < stackoff)
				off = stackoff;
			len = stacklen - (off - stackoff);
		} else {	/* stack pointer not in segment.  thread library? */
			off = stackoff + stacklen - 16*1024;
			len = 16*1024;
		}
		s[stacki] = readseg(fd, off, len, "Stack");
	}

	return proc;
}
