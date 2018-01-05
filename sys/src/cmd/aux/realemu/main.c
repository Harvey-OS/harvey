#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

/* for fs */
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

#include <386/include/ureg.h>

enum {
	MEMSIZE = 0x100000,

	RMBUF = 0x9000,
	RMCODE = 0x8000,

	PITHZ = 1193182,
	PITNS = 1000000000/PITHZ,
};

static Cpu cpu;
static unsigned char memory[MEMSIZE+4];
static unsigned char pageregtmp[0x10];
static int portfd[5];
static int realmemfd;
static int cputrace;
static int porttrace;
static Pit pit[3];
static unsigned char rtcaddr;

static long long pitclock;

static void
startclock(void)
{
	print_func_entry();
	pitclock = nsec();
	print_func_exit();
}

static void
runclock(void)
{
	print_func_entry();
	long long now, dt;

	now = nsec();
	dt = now - pitclock;
	if(dt >= PITNS){
		clockpit(pit, dt/PITNS);
		pitclock = now;
	}
	print_func_exit();
}

static unsigned long
gw1(unsigned char *p)
{
	print_func_entry();
	print_func_exit();
	return p[0];
}
static unsigned long
gw2(unsigned char *p)
{
	print_func_entry();
	print_func_exit();
	return (unsigned long)p[0] | (unsigned long)p[1]<<8;
}
static unsigned long
gw4(unsigned char *p)
{
	print_func_entry();
	print_func_exit();
	return (unsigned long)p[0] | (unsigned long)p[1]<<8 | (unsigned long)p[2]<<16 | (unsigned long)p[3]<<24;
}
static unsigned long (*gw[9])(unsigned char *p) = {
	[1] = gw1,
	[2] = gw2,
	[4] = gw4,
	[8] = gw4,
};

static void
pw1(unsigned char *p, unsigned long w)
{
	print_func_entry();
	p[0] = w & 0xFF;
	print_func_exit();
}
static void
pw2(unsigned char *p, unsigned long w)
{
	print_func_entry();
	p[0] = w & 0xFF;
	p[1] = (w>>8) & 0xFF;
	print_func_exit();
}
static void
pw4(unsigned char *p, unsigned long w)
{
	print_func_entry();
	p[0] = w & 0xFF;
	p[1] = (w>>8) & 0xFF;
	p[2] = (w>>16) & 0xFF;
	p[3] = (w>>24) & 0xFF;
	print_func_exit();
}
static void (*pw[5])(unsigned char *p, unsigned long w) = {
	[1] pw1,
	[2] pw2,
	[4] pw4,
};

static unsigned long
rbad(void *aux, unsigned long off, int len)
{
	print_func_entry();
	fprint(2, "bad mem read %.5lux\n", off);
	trap(&cpu, EMEM);

	/* not reached */
	print_func_exit();
	return 0;
}

static void
wbad(void *aux, unsigned long off, unsigned long w, int len)
{
	print_func_entry();
	fprint(2, "bad mem write %.5lux\n", off);
	trap(&cpu, EMEM);
	print_func_exit();
}

static unsigned long
rmem(void *aux, unsigned long off, int len)
{
	print_func_entry();
	print_func_exit();
	return gw[len](memory + off);
}

static void
wmem(void *aux, unsigned long off, unsigned long w, int len)
{
	print_func_entry();
	pw[len](memory + off, w);
	print_func_exit();
}

static unsigned long
rrealmem(void *aux, unsigned long off, int len)
{
	print_func_entry();
	unsigned char data[4];

	if(pread(realmemfd, data, len, off) != len){
		fprint(2, "bad real mem read %.5lux: %r\n", off);
		trap(&cpu, EMEM);
	}
	print_func_exit();
	return gw[len](data);
}

static void
wrealmem(void *aux, unsigned long off, unsigned long w, int len)
{
	print_func_entry();
	unsigned char data[4];

	pw[len](data, w);
	if(pwrite(realmemfd, data, len, off) != len){
		fprint(2, "bad real mem write %.5lux: %r\n", off);
		trap(&cpu, EMEM);
	}
	print_func_exit();
}


static unsigned long
rport(void *aux, unsigned long p, int len)
{
	print_func_entry();
	unsigned char data[4];
	unsigned long w;

	switch(p){
	case 0x20:	/* PIC 1 */
	case 0x21:
		w = 0;
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		runclock();
		w = rpit(pit, p - 0x40);
		break;
	case 0x60:	/* keyboard data output buffer */
		w = 0;
		break;
	case 0x61:	/* keyboard controller port b */
		runclock();
		w = pit[2].out<<5 | pit[2].gate;
		break;
	case 0x62:	/* PPI (XT only) */
		runclock();
		w = pit[2].out<<5;
		break;
	case 0x63:	/* PPI (XT only) read dip switches */
		w = 0;
		break;
	case 0x65:	/* A20 gate */
		w = 1 << 2;
		break;
	case 0x70:	/* RTC addr */
		w = rtcaddr;
		break;
	case 0x71:	/* RTC data */
		w = 0xFF;
		break;
	case 0x80:	/* extra dma registers (temp) */
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x88:
	case 0x8c:
	case 0x8d:
	case 0x8e:
		w = pageregtmp[p-0x80];
		break;
	case 0x92:	/* A20 gate (system control port a) */
		w = 1 << 1;
		break;
	case 0xa0:	/* PIC 2 */
	case 0xa1:
		w = 0;
		break;
	default:
		if(pread(portfd[len], data, len, p) != len){
			fprint(2, "bad %d bit port read %.4lux: %r\n", len*8, p);
			trap(&cpu, EIO);
		}
		w = gw[len](data);
	}
	if(porttrace)
		fprint(2, "rport %.4lux %.*lux\n", p, len<<1, w);
	print_func_exit();
	return w;
}

static void
wport(void *aux, unsigned long p, unsigned long w, int len)
{
	print_func_entry();
	unsigned char data[4];

	if(porttrace)
		fprint(2, "wport %.4lux %.*lux\n", p, len<<1, w);

	switch(p){
	case 0x20:	/* PIC 1 */
	case 0x21:
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		runclock();
		wpit(pit, p - 0x40, w);
		break;
	case 0x60:	/* keyboard controller data port */
		break;
	case 0x61:	/* keyboard controller port B */
		setgate(&pit[2], w & 1);
		break;
	case 0x62:	/* PPI (XT only) */
	case 0x63:
	case 0x64:	/* KB controller input buffer (ISA, EISA) */
	case 0x65:	/* A20 gate (bit 2) */
		break;
	case 0x70:	/* RTC addr */
		rtcaddr = w & 0xFF;
		break;
	case 0x71:	/* RTC data */
		break;
	case 0x80:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x88:
	case 0x8c:
	case 0x8d:
	case 0x8e:
		pageregtmp[p-0x80] = w & 0xFF;
		break;
	case 0x92:	/* system control port a */
	case 0x94:	/* system port enable setup register */
	case 0x96:
		break;
	case 0xA0:	/* PIC 2 */
	case 0xA1:
		break;
	
	default:
		pw[len](data, w);
		if(pwrite(portfd[len], data, len, p) != len){
			fprint(2, "bad %d bit port write %.4lux: %r\n", len*8, p);
			trap(&cpu, EIO);
		}
	}
	print_func_exit();
}

static Bus memio[] = {
	/* 0 */ memory, rmem, wmem,	/* RAM: IVT, BIOS data area */
	/* 1 */ memory,	rmem, wmem,	/* custom */
	/* 2 */ nil,	rbad, wbad,
	/* 3 */ nil,	rbad, wbad,
	/* 4 */ nil,	rbad, wbad,
	/* 5 */ nil,	rbad, wbad,
	/* 6 */ nil,	rbad, wbad,
	/* 7 */ nil,	rbad, wbad,
	/* 8 */ nil,	rbad, wbad,
	/* 9 */ memory,	rmem, wmem,	/* RAM: extended BIOS data area */
	/* A */ nil,	rrealmem, wrealmem,	/* RAM: VGA framebuffer */
	/* B */ nil,	rrealmem, wrealmem,	/* RAM: VGA framebuffer */
	/* C */ memory,	rmem, wmem,	/* ROM: VGA BIOS */
	/* D */ nil,	rbad, wbad,
	/* E */ memory,	rmem, wmem,	/* ROM: BIOS */
	/* F */ memory,	rmem, wbad,	/* ROM: BIOS */
};

static Bus portio = {
	nil, rport, wport,
};

static void
cpuinit(void)
{
	print_func_entry();
	int i;

	fmtinstall('I', instfmt);
	fmtinstall('J', flagfmt);
	fmtinstall('C', cpufmt);

	if((portfd[1] = open("#P/iob", ORDWR)) < 0)
		sysfatal("open iob: %r");
	if((portfd[2] = open("#P/iow", ORDWR)) < 0)
		sysfatal("open iow: %r");
	if((portfd[4] = open("#P/iol", ORDWR)) < 0)
		sysfatal("open iol: %r");

	if((realmemfd = open("#P/realmodemem", ORDWR)) < 0)
		sysfatal("open realmodemem: %r");

	for(i=0; i<nelem(memio); i++){
		unsigned long off;

		if(memio[i].r != rmem)
			continue;

		off = (unsigned long)i << 16;
		seek(realmemfd, off, 0);
		if(readn(realmemfd, memory + off, 0x10000) != 0x10000)
			sysfatal("read real mem %lux: %r\n", off);
	}

	cpu.ic = 0;
	cpu.mem = memio;
	cpu.port = &portio;
	cpu.alen = cpu.olen = cpu.slen = 2;
	print_func_exit();
}

static char Ebusy[] = "device is busy";
static char Eintr[] = "interrupted";
static char Eperm[] = "permission denied";
static char Eio[] = "i/o error";
static char Emem[] = "bad memory access";
static char Enonexist[] = "file does not exist";
static char Ebadspec[] = "bad attach specifier";
static char Ewalk[] = "walk in non directory";
static char Ebadureg[] = "write a Ureg";
static char Ebadoff[] = "invalid offset";
static char Ebadtrap[] = "bad trap";

static char *trapstr[] = {
	[EDIV0] "division by zero",
	[EDEBUG] "debug exception",
	[ENMI] "not maskable interrupt",
	[EBRK] "breakpoint",
	[EINTO] "into overflow",
	[EBOUND] "bounds check",
	[EBADOP] "bad opcode",
	[ENOFPU] "no fpu installed",
	[EDBLF] "double fault",
	[EFPUSEG] "fpu segment overflow",
	[EBADTSS] "invalid task state segment",
	[ENP] "segment not present",
	[ESTACK] "stack fault",
	[EGPF] "general protection fault",
	[EPF] "page fault",
};

static int flushed(void *);

#define GETUREG(x)	gw[sizeof(u->x)]((unsigned char*)&u->x)
#define PUTUREG(x,y)	pw[sizeof(u->x)]((unsigned char*)&u->x,y)

static char*
realmode(Cpu *cpu, struct Ureg *u, void *r)
{
	print_func_entry();
	char *err;
	int i;

	/* N.B. it wasy always 16 bits in the kernel. */
	cpu->reg[RDI] = GETUREG(di) & 0xffff;
	cpu->reg[RSI] = GETUREG(si);
	cpu->reg[RBP] = GETUREG(bp);
	cpu->reg[RBX] = GETUREG(bx);
	cpu->reg[RDX] = GETUREG(dx);
	cpu->reg[RCX] = GETUREG(cx);
	cpu->reg[RAX] = GETUREG(ax);

	cpu->reg[RGS] = GETUREG(gs);
	cpu->reg[RFS] = GETUREG(fs);
	cpu->reg[RES] = GETUREG(di) >> 16;
	cpu->reg[RDS] = GETUREG(ds);

	cpu->reg[RFL] = GETUREG(flags);

	if(i = GETUREG(trap)){
		cpu->reg[RSS] = 0x0000;
		cpu->reg[RSP] = 0x7C00;
		cpu->reg[RCS] = (RMCODE>>4)&0xF000;
		cpu->reg[RIP] = RMCODE & 0xFFFF;
		memory[RMCODE] = 0xf4;	/* HLT instruction */
		if(intr(cpu, i) < 0)
			return Ebadtrap;
	} else {
		cpu->reg[RSS] = GETUREG(ss);
		cpu->reg[RSP] = GETUREG(sp);
		cpu->reg[RCS] = GETUREG(cs);
		cpu->reg[RIP] = GETUREG(pc);
	}

	startclock();
	for(;;){
		if(cputrace)
			fprint(2, "%C\n", cpu);
		switch(i = xec(cpu, (porttrace | cputrace) ? 1 : 100000)){
		case -1:
			if(flushed(r)){
				err = Eintr;
				break;
			}
			runclock();
			continue;

		/* normal interrupts */
		default:
			if(intr(cpu, i) < 0){
				err = Ebadtrap;
				break;
			}
			continue;

		/* pseudo-interrupts */
		case EHALT:
			err = nil;
			break;
		case EIO:
			err = Eio;
			break;
		case EMEM:
			err = Emem;
			break;

		/* processor traps */
		case EDIV0:
		case EDEBUG:
		case ENMI:
		case EBRK:
		case EINTO:
		case EBOUND:
		case EBADOP:
		case ENOFPU:
		case EDBLF:
		case EFPUSEG:
		case EBADTSS:
		case ENP:
		case ESTACK:
		case EGPF:
		case EPF:
			PUTUREG(trap, i);
			err = trapstr[i];
			break;
		}

		break;
	}

	if(err)
		fprint(2, "%s\n%C\n", err, cpu);

	PUTUREG(di, cpu->reg[RDI]);
	PUTUREG(si, cpu->reg[RSI]);
	PUTUREG(bp, cpu->reg[RBP]);
	PUTUREG(bx, cpu->reg[RBX]);
	PUTUREG(dx, cpu->reg[RDX]);
	PUTUREG(cx, cpu->reg[RCX]);
	PUTUREG(ax, cpu->reg[RAX]);

	PUTUREG(gs, cpu->reg[RGS]);
	PUTUREG(fs, cpu->reg[RFS]);
	PUTUREG(es, cpu->reg[RES]);
	PUTUREG(ds, cpu->reg[RDS]);

	PUTUREG(flags, cpu->reg[RFL]);

	PUTUREG(pc, cpu->reg[RIP]);
	PUTUREG(cs, cpu->reg[RCS]);
	PUTUREG(sp, cpu->reg[RSP]);
	PUTUREG(ss, cpu->reg[RSS]);

	print_func_exit();
	return err;
}

enum {
	Qroot,
	Qcall,
	Qmem,
	Nqid,
};

static struct Qtab {
	char *name;
	int mode;
	int type;
	int length;
} qtab[Nqid] = {
	"/",
		DMDIR|0555,
		QTDIR,
		0,

	"realmode",
		0666,
		0,
		0,

	"realmodemem",
		0666,	
		0,
		MEMSIZE,
};

static int
fillstat(unsigned long qid, Dir *d)
{
	print_func_entry();
	struct Qtab *t;

	memset(d, 0, sizeof(Dir));
	d->uid = "realemu";
	d->gid = "realemu";
	d->muid = "";
	d->qid = (Qid){qid, 0, 0};
	d->atime = time(0);
	t = qtab + qid;
	d->name = t->name;
	d->qid.type = t->type;
	d->mode = t->mode;
	d->length = t->length;
	print_func_exit();
	return 1;
}

static void
fsattach(Req *r)
{
	print_func_entry();
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, Ebadspec);
		print_func_exit();
		return;
	}
	r->fid->qid = (Qid){Qroot, 0, QTDIR};
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
	print_func_exit();
}

static void
fsstat(Req *r)
{
	print_func_entry();
	fillstat((unsigned long)r->fid->qid.path, &r->d);
	r->d.name = estrdup9p(r->d.name);
	r->d.uid = estrdup9p(r->d.uid);
	r->d.gid = estrdup9p(r->d.gid);
	r->d.muid = estrdup9p(r->d.muid);
	respond(r, nil);
	print_func_exit();
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	print_func_entry();
	int i;
	unsigned long path;

	path = fid->qid.path;
	switch(path){
	case Qroot:
		if (strcmp(name, "..") == 0) {
			*qid = (Qid){Qroot, 0, QTDIR};
			fid->qid = *qid;
			print_func_exit();
			return nil;
		}
		for(i = fid->qid.path; i<Nqid; i++){
			if(strcmp(name, qtab[i].name) != 0)
				continue;
			*qid = (Qid){i, 0, 0};
			fid->qid = *qid;
			print_func_exit();
			return nil;
		}
		print_func_exit();
		return Enonexist;
		
	default:
		print_func_exit();
		return Ewalk;
	}
}

static void
fsopen(Req *r)
{
	print_func_entry();
	static int need[4] = { 4, 2, 6, 1 };
	struct Qtab *t;
	int n;

	t = qtab + r->fid->qid.path;
	n = need[r->ifcall.mode & 3];
	if((n & t->mode) != n)
		respond(r, Eperm);
	else
		respond(r, nil);
		print_func_exit();
}

static int
readtopdir(Fid *fid, unsigned char *buf, long off, int cnt, int blen)
{
	print_func_entry();
	int i, m, n;
	long pos;
	Dir d;

	n = 0;
	pos = 0;
	for (i = 1; i < Nqid; i++){
		fillstat(i, &d);
		m = convD2M(&d, &buf[n], blen-n);
		if(off <= pos){
			if(m <= BIT16SZ || m > cnt)
				break;
			n += m;
			cnt -= m;
		}
		pos += m;
	}
	print_func_exit();
	return n;
}

static Channel *reqchan;

static void
cpuproc(void *data)
{
	print_func_entry();
	static struct Ureg rmu;
	unsigned long path;
	long long o;
	unsigned long n;
	char *p;
	Req *r;

	threadsetname("cpuproc");

	while(r = recvp(reqchan)){
		if(flushed(r)){
			respond(r, Eintr);
			continue;
		}

		path = r->fid->qid.path;

		p = r->ifcall.data;
		n = r->ifcall.count;
		o = r->ifcall.offset;

		switch(((int)r->ifcall.type<<8)|path){
		case (Tread<<8) | Qmem:
			readbuf(r, memory, MEMSIZE);
			respond(r, nil);
			break;

		case (Tread<<8) | Qcall:
			readbuf(r, &rmu, sizeof rmu);
			respond(r, nil);
			break;

		case (Twrite<<8) | Qmem:
			if(o < 0 || o >= MEMSIZE || o+n > MEMSIZE){
				respond(r, Ebadoff);
				break;
			}
			memmove(memory + o, p, n);
			r->ofcall.count = n;
			respond(r, nil);
			break;

		case (Twrite<<8) | Qcall:
			if(n != sizeof rmu){
				fprint(2, "n is %d, sizeof(rmu) %d: %s\n", n, sizeof(rmu), Ebadureg);
				respond(r, Ebadureg);
				break;
			}
			memmove(&rmu, p, n);
			if(p = realmode(&cpu, &rmu, r)){
				respond(r, p);
				break;
			}
			r->ofcall.count = n;
			respond(r, nil);
			break;
		}
	}
	print_func_exit();
}

static Channel *flushchan;

static int
flushed(void *r)
{
	print_func_entry();
	print_func_exit();
	return nbrecvp(flushchan) == r;
}

static void
fsflush(Req *r)
{
	print_func_entry();
	nbsendp(flushchan, r->oldreq);
	respond(r, nil);
	print_func_exit();
}

static void
dispatch(Req *r)
{
	print_func_entry();
	if(!nbsendp(reqchan, r))
		respond(r, Ebusy);
		print_func_exit();
}

static void
fsread(Req *r)
{
	print_func_entry();
	switch((unsigned long)r->fid->qid.path){
	case Qroot:
		r->ofcall.count = readtopdir(r->fid, (void*)r->ofcall.data, r->ifcall.offset,
			r->ifcall.count, r->ifcall.count);
		respond(r, nil);
		break;
	default:
		dispatch(r);
	}
	print_func_exit();
}

static void
fsend(Srv* srv)
{
	print_func_entry();
	threadexitsall(nil);
	print_func_exit();
}

static Srv fs = {
	.attach=		fsattach,
	.walk1=			fswalk1,
	.open=			fsopen,
	.read=			fsread,
	.write=			dispatch,
	.stat=			fsstat,
	.flush=			fsflush,
	.end=			fsend,
};

static void
usage(void)
{
	print_func_entry();
	fprint(2, "usgae:\t%s [-Dpt] [-s srvname] [-m mountpoint]\n", argv0);
	exits("usage");
	print_func_exit();
}

void
threadmain(int argc, char *argv[])
{
	char *mnt = "/dev";
	char *srv = nil;

	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'p':
		porttrace = 1;
		break;
	case 't':
		cputrace = 1;
		break;
	case 'x':
		set_printx(1);
		break;
	case 's':
		srv = EARGF(usage());
		mnt = nil;
		break;
	case 'm':
		mnt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	cpuinit();

	reqchan = chancreate(sizeof(Req*), 8);
	flushchan = chancreate(sizeof(Req*), 8);
	procrfork(cpuproc, nil, 16*1024, RFNAMEG|RFNOTEG);
	threadpostmountsrv(&fs, srv, mnt, MBEFORE);
}
