#include "stdinc.h"
#include <bio.h>
#include <mach.h>
#include <ureg.h>
#include "/sys/src/libthread/threadimpl.h"
#include "dat.h"
#include "fns.h"

typedef struct Ureg Ureg;
typedef struct Debug Debug;

struct Debug
{
	int textfd;
	QLock lock;
	Fhdr fhdr;
	Map *map;
	Fmt *fmt;
	int pid;
	char *stkprefix;
	int pcoff;
	int spoff;
};

static Debug debug = { -1 };

static int
text(int pid)
{
	int fd;
	char buf[100];

	if(debug.textfd >= 0){
		close(debug.textfd);
		debug.textfd = -1;
	}
	memset(&debug.fhdr, 0, sizeof debug.fhdr);
	
	snprint(buf, sizeof buf, "#p/%d/text", pid);
	fd = open(buf, OREAD);
	if(fd < 0)
		return -1;
	if(crackhdr(fd, &debug.fhdr) < 0){
		close(fd);
		return -1;
	}
	if(syminit(fd, &debug.fhdr) < 0){
		memset(&debug.fhdr, 0, sizeof debug.fhdr);
		close(fd);
		return -1;
	}
	debug.textfd = fd;
	machbytype(debug.fhdr.type);
	return 0;
}

static void
unmap(Map *m)
{
	int i;
	
	for(i=0; i<m->nsegs; i++)
		if(m->seg[i].inuse)
			close(m->seg[i].fd);
	free(m);
}

static Map*
map(int pid)
{
	int mem;
	char buf[100];
	Map *m;
	
	snprint(buf, sizeof buf, "#p/%d/mem", pid);
	mem = open(buf, OREAD);
	if(mem < 0)
		return nil;

	m = attachproc(pid, 0, mem, &debug.fhdr);
	if(m == 0){
		close(mem);
		return nil;
	}
	
	if(debug.map)
		unmap(debug.map);
	debug.map = m;
	debug.pid = pid;
	return m;
}

static void
dprint(char *fmt, ...)
{
	va_list arg;
	
	va_start(arg, fmt);
	fmtvprint(debug.fmt, fmt, arg);
	va_end(arg);
}

static void
openfiles(void)
{
	char buf[4096];
	int fd, n;
	
	snprint(buf, sizeof buf, "#p/%d/fd", getpid());
	if((fd = open(buf, OREAD)) < 0){
		dprint("open %s: %r\n", buf);
		return;
	}
	n = readn(fd, buf, sizeof buf-1);
	close(fd);
	if(n >= 0){
		buf[n] = 0;
		fmtstrcpy(debug.fmt, buf);
	}
}

/*
 *	dump the raw symbol table
 */
static void
printsym(void)
{
	int i;
	Sym *sp;

	for (i = 0; sp = getsym(i); i++) {
		switch(sp->type) {
		case 't':
		case 'l':
			dprint("%16#llux t %s\n", sp->value, sp->name);
			break;
		case 'T':
		case 'L':
			dprint("%16#llux T %s\n", sp->value, sp->name);
			break;
		case 'D':
		case 'd':
		case 'B':
		case 'b':
		case 'a':
		case 'p':
		case 'm':
			dprint("%16#llux %c %s\n", sp->value, sp->type, sp->name);
			break;
		default:
			break;
		}
	}
}

static void
printmap(char *s, Map *map)
{
	int i;

	if (!map)
		return;
	dprint("%s\n", s);
	for (i = 0; i < map->nsegs; i++) {
		if (map->seg[i].inuse)
			dprint("%-16s %-16#llux %-16#llux %-16#llux\n",
				map->seg[i].name, map->seg[i].b,
				map->seg[i].e, map->seg[i].f);
	}
}

static void
printlocals(Map *map, Symbol *fn, uintptr fp)
{
	int i;
	uintptr w;
	Symbol s;
	char buf[100];

	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CAUTO)
			continue;
		snprint(buf, sizeof buf, "%s%s/", debug.stkprefix, s.name);
		if (geta(map, fp - s.value, (uvlong*)&w) > 0)
			dprint("\t%-10s %10#p %ld\n", buf, w, w);
		else
			dprint("\t%-10s ?\n", buf);
	}
}

static void
printparams(Map *map, Symbol *fn, uintptr fp)
{
	int i;
	Symbol s;
	uintptr w;
	int first = 0;

	fp += mach->szaddr;			/* skip saved pc */
	s = *fn;
	for (i = 0; localsym(&s, i); i++) {
		if (s.class != CPARAM)
			continue;
		if (first++)
			dprint(", ");
		if (geta(map, fp + s.value, (uvlong *)&w) > 0)
			dprint("%s=%#p", s.name, w);
	}
}

static void
printsource(uintptr dot)
{
	char str[100];

	if (fileline(str, sizeof str, dot))
		dprint("%s", str);
}


/*
 *	callback on stack trace
 */
static uintptr nextpc;

static void
ptrace(Map *map, uvlong pc, uvlong sp, Symbol *sym)
{
	if(nextpc == 0)
		nextpc = sym->value;
	if(debug.stkprefix == nil)
		debug.stkprefix = "";
	dprint("%s%s(", debug.stkprefix, sym->name);
	printparams(map, sym, sp);
	dprint(")");
	if(nextpc != sym->value)
		dprint("+%#llux ", nextpc - sym->value);
	printsource(nextpc);
	dprint("\n");
	printlocals(map, sym, sp);
	nextpc = pc;
}

static void
stacktracepcsp(Map *m, uintptr pc, uintptr sp)
{
	nextpc = 0;
	if(machdata->ctrace==nil)
		dprint("no machdata->ctrace\n");
	else if(machdata->ctrace(m, pc, sp, 0, ptrace) <= 0)
		dprint("no stack frame: pc=%#p sp=%#p\n", pc, sp);
}

static void
ureginit(void)
{
	Reglist *r;

	for(r = mach->reglist; r->rname; r++)
		if (strcmp(r->rname, "PC") == 0)
			debug.pcoff = r->roffs;
		else if (strcmp(r->rname, "SP") == 0)
			debug.spoff = r->roffs;
}

static void
stacktrace(Map *m)
{
	uintptr pc, sp;
	
	if(geta(m, debug.pcoff, (uvlong *)&pc) < 0){
		dprint("geta pc: %r");
		return;
	}
	if(geta(m, debug.spoff, (uvlong *)&sp) < 0){
		dprint("geta sp: %r");
		return;
	}
	stacktracepcsp(m, pc, sp);
}

static uintptr
star(uintptr addr)
{
	uintptr x;
	static int warned;

	if(addr == 0)
		return 0;

	if(debug.map == nil){
		if(!warned++)
			dprint("no debug.map\n");
		return 0;
	}
	if(geta(debug.map, addr, (uvlong *)&x) < 0){
		dprint("geta %#p (pid=%d): %r\n", addr, debug.pid);
		return 0;
	}
	return x;
}

static uintptr
resolvev(char *name)
{
	Symbol s;

	if(lookup(nil, name, &s) == 0)
		return 0;
	return s.value;
}

static uintptr
resolvef(char *name)
{
	Symbol s;

	if(lookup(name, nil, &s) == 0)
		return 0;
	return s.value;
}

#define FADDR(type, p, name) ((p) + offsetof(type, name))
#define FIELD(type, p, name) star(FADDR(type, p, name))

static uintptr threadpc;

static int
strprefix(char *big, char *pre)
{
	return strncmp(big, pre, strlen(pre));
}
static void
tptrace(Map *map, uvlong pc, uvlong sp, Symbol *sym)
{
	char buf[512];

	USED(map);
	USED(sym);
	USED(sp);

	if(threadpc != 0)
		return;
	if(!fileline(buf, sizeof buf, pc))
		return;
	if(strprefix(buf, "/sys/src/libc/") == 0)
		return;
	if(strprefix(buf, "/sys/src/libthread/") == 0)
		return;
	threadpc = pc;
}

static char*
threadstkline(uintptr t)
{
	uintptr pc, sp;
	static char buf[500];

	if(FIELD(Thread, t, state) == Running){
		geta(debug.map, debug.pcoff, (uvlong *)&pc);
		geta(debug.map, debug.spoff, (uvlong *)&sp);
	}else{
		// pc = FIELD(Thread, t, sched[JMPBUFPC]);
		pc = resolvef("longjmp");
		sp = FIELD(Thread, t, sched[JMPBUFSP]);
	}
	if(machdata->ctrace == nil)
		return "";
	threadpc = 0;
	machdata->ctrace(debug.map, pc, sp, 0, tptrace);
	if(!fileline(buf, sizeof buf, threadpc))
		buf[0] = 0;
	return buf;
}

static void
proc(uintptr p)
{
	dprint("p=(Proc)%#p pid %d ", p, FIELD(Proc, p, pid));
	if(FIELD(Proc, p, thread) == 0)
		dprint(" Sched\n");
	else
		dprint(" Running\n");
}

static void
fmtbufinit(Fmt *f, char *buf, int len)
{
	memset(f, 0, sizeof *f);
	f->runes = 0;
	f->start = buf;
	f->to = buf;
	f->stop = buf + len - 1;
	f->flush = nil;
	f->farg = nil;
	f->nfmt = 0;
}

static char*
fmtbufflush(Fmt *f)
{
	*(char*)f->to = 0;
	return (char*)f->start;
}

static char*
debugstr(uintptr s)
{
	static char buf[4096];
	char *p, *e;
	
	p = buf;
	e = buf+sizeof buf - 1;
	while(p < e){
		if(get1(debug.map, s++, (uchar*)p, 1) < 0)
			break;
		if(*p == 0)
			break;
		p++;
	}
	*p = 0;
	return buf;
}

static char*
threadfmt(uintptr t)
{
	static char buf[4096];
	Fmt fmt;
	int s;

	fmtbufinit(&fmt, buf, sizeof buf);
	
	fmtprint(&fmt, "t=(Thread)%#p ", t);
	switch(s = FIELD(Thread, t, state)){
	case Running:
		fmtprint(&fmt, " Running   ");
		break;
	case Ready:
		fmtprint(&fmt, " Ready     ");
		break;
	case Rendezvous:
		fmtprint(&fmt, " Rendez    ");
		break;
	default:
		fmtprint(&fmt, " bad state %d ", s);
		break;
	}
	
	fmtprint(&fmt, "%s", threadstkline(t));
	
	if(FIELD(Thread, t, moribund) == 1)
		fmtprint(&fmt, " Moribund");
	if(s = FIELD(Thread, t, cmdname)){
		fmtprint(&fmt, " [%s]", debugstr(s));
	}

	fmtbufflush(&fmt);
	return buf;
}


static void
thread(uintptr t)
{
	dprint("%s\n", threadfmt(t));
}

static void
threadapply(uintptr p, void (*fn)(uintptr))
{
	int oldpid, pid;
	uintptr tq, t;
	
	oldpid = debug.pid;
	pid = FIELD(Proc, p, pid);
	if(map(pid) == nil)
		return;
	tq = FADDR(Proc, p, threads);
	t = FIELD(Tqueue, tq, head);
	while(t != 0){
		fn(t);
		t = FIELD(Thread, t, nextt);
	}
	map(oldpid);
}

static void
pthreads1(uintptr t)
{
	dprint("\t");
	thread(t);
}

static void
pthreads(uintptr p)
{
	threadapply(p, pthreads1);
}

static void
lproc(uintptr p)
{
	proc(p);
	pthreads(p);
}

static void
procapply(void (*fn)(uintptr))
{
	uintptr proc, pq;
	
	pq = resolvev("_threadpq");
	if(pq == 0){
		dprint("no thread run queue\n");
		return;
	}

	proc = FIELD(Pqueue, pq, head);
	while(proc){
		fn(proc);
		proc = FIELD(Proc, proc, next);
	}
}

static void
threads(HConnect *c)
{
	USED(c);
	procapply(lproc);
}

static void
procs(HConnect *c)
{
	USED(c);
	procapply(proc);
}

static void
threadstack(uintptr t)
{
	uintptr pc, sp;

	if(FIELD(Thread, t, state) == Running){
		stacktrace(debug.map);
	}else{
		// pc = FIELD(Thread, t, sched[JMPBUFPC]);
		pc = resolvef("longjmp");
		sp = FIELD(Thread, t, sched[JMPBUFSP]);
		stacktracepcsp(debug.map, pc, sp);
	}
}


static void
tstacks(uintptr t)
{
	dprint("\t");
	thread(t);
	threadstack(t);
	dprint("\n");
}

static void
pstacks(uintptr p)
{
	proc(p);
	threadapply(p, tstacks);
}

static void
stacks(HConnect *c)
{
	USED(c);
	debug.stkprefix = "\t\t";
	procapply(pstacks);
	debug.stkprefix = "";
}

static void
symbols(HConnect *c)
{
	USED(c);
	printsym();
}

static void
segments(HConnect *c)
{
	USED(c);
	printmap("segments", debug.map);
}

static void
fds(HConnect *c)
{
	USED(c);
	openfiles();
}

static void
all(HConnect *c)
{
	dprint("/proc/segment\n");
	segments(c);
	dprint("\n/proc/fd\n");
	fds(c);
	dprint("\n/proc/procs\n");
	procs(c);
	dprint("\n/proc/threads\n");
	threads(c);
	dprint("\n/proc/stacks\n");
	stacks(c);
	dprint("\n# /proc/symbols\n");
	// symbols(c);
}

int
hproc(HConnect *c)
{
	void (*fn)(HConnect*);
	Fmt fmt;
	static int beenhere;
	static char buf[65536];

	if (!beenhere) {
		beenhere = 1;
		ureginit();
	}
	if(strcmp(c->req.uri, "/proc/all") == 0)
		fn = all;
	else if(strcmp(c->req.uri, "/proc/segment") == 0)
		fn = segments;
	else if(strcmp(c->req.uri, "/proc/fd") == 0)
		fn = fds;
	else if(strcmp(c->req.uri, "/proc/procs") == 0)
		fn = procs;
	else if(strcmp(c->req.uri, "/proc/threads") == 0)
		fn = threads;
	else if(strcmp(c->req.uri, "/proc/stacks") == 0)
		fn = stacks;
	else if(strcmp(c->req.uri, "/proc/symbols") == 0)
		fn = symbols;
	else
		return hnotfound(c);

	if(hsettext(c) < 0)
		return -1;
	if(!canqlock(&debug.lock)){
		hprint(&c->hout, "debugger is busy\n");
		return 0;
	}
	if(debug.textfd < 0){
		if(text(getpid()) < 0){
			hprint(&c->hout, "cannot attach self text: %r\n");
			goto out;
		}
	}
	if(map(getpid()) == nil){
		hprint(&c->hout, "cannot map self: %r\n");
		goto out;
	}

	fmtbufinit(&fmt, buf, sizeof buf);
	debug.fmt = &fmt;
	fn(c);
	hprint(&c->hout, "%s\n", fmtbufflush(&fmt));
	debug.fmt = nil;
out:
	qunlock(&debug.lock);
	return 0;
}
