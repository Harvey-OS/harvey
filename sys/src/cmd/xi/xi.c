#include <u.h>
#include <libc.h>
#include <bio.h>
#define Extern
#include <mach.h>
#include "3210.h"

Namedreg namedreg[] = {
	"pc",	&reg.pc,
	"ip",	&reg.ip,
	"ir",	&reg.ir,
	"loop",	&reg.loop,
	"last",	&reg.last,
	"start",	&reg.start,
	"count",	&reg.count,
	"size",	&reg.size,
	"iter",	&reg.iter,
	"sp",	&reg.r[REGSP],
	"ps",	&reg.c[PS],
	"pcw",	&reg.c[PCW],
	"ctr",	&reg.c[CTR],
	"emr",	&reg.c[EMR],
	"dauc",	&reg.c[DAUC],
	"a0",	&reg.f[0].val,
	"a1",	&reg.f[1].val,
	"a2",	&reg.f[2].val,
	"a3",	&reg.f[3].val,
	0,	0
};

char	*file = "x.out";
Biobuf	bp, bi;
Fhdr fhdr;

void
main(int argc, char **argv)
{
	argv0 = "xi";
	Tstart = ~0;
	Dstart = ~0;
	syscalls = 1;
	ARGBEGIN{
	case 'D':
		Dstart = strtoul(ARGF(), 0, 0);
		break;
	case 'T':
		Tstart = strtoul(ARGF(), 0, 0);
		break;
	case 'e':
		isbigend = 1;
		break;
	case 's':
		syscalls = 0;
		break;
	}ARGEND

	bioout = &bp;
	bin = &bi;
	Binit(bioout, 1, OWRITE);
	Binit(bin, 0, OREAD);
	fmtinstall('X', Xconv);

	if(argc)
		file = argv[0];
	argc--;
	argv++;

	text = open(file, OREAD);
	if(text < 0)
		fatal("open text '%s': %r", file);

	Bprint(bioout, "%s\n", argv0); 
	reset();
	inithdr(text);
	init(argc, argv);

	cmd();
}

void
inithdr(int fd)
{
	seek(fd, 0, 0);
	if(!crackhdr(fd, &fhdr))
		fatal("read text header");
	if(fhdr.type != F3210)
		fatal("bad magic number");
	if(syminit(fd, &fhdr) < 0)
		print("%s: initializing symbol table: %r\n", argv0);
	if(Tstart != ~0)
		textseg(Tstart, &fhdr);
}

void
reset(void)
{
	int i, l, m;
	Segment *s;
	Breakpoint *b;

	memset(&reg, 0, sizeof(Registers));
	for(i = 0; i < Nseg; i++) {
		s = &segments[i];
		l = s->ntbl;
		for(m = 0; m < l; m++)
			if(s->table[m])
				free(s->table[m]);
		if(s->table)
			free(s->table);
		s->table = 0;
		s->ntbl = 0;
		s->refs = 0;
		s->rss = 0;
	}

	resetld();
	resetfpu();
	clearprof();

	if(iprof)
		free(iprof);
	iprof = 0;
	mprof = &notext;
	for(b = bplist; b; b = b->next)
		b->done = b->count;
}

void
readexec(int fstart, int start, int len)
{
	char *v;
	ulong p, pe, r;
	int off;

	seek(text, fstart, 0);
	off = start & (BY2PG-1);
	pe = start + len;
	for(p = start & ~(BY2PG-1); p < pe; p += BY2PG){
		v = vaddr(p, 0, 0);
		r = pe - p;
		if(r > BY2PG)
			r = BY2PG;
		r -= off;
		if(r != BY2PG)
			memset(v, 0, BY2PG);
		if(read(text, v+off, r) != r)
			fatal("can't read from executable file");
		off = 0;
	}
}

void
init(int argc, char *argv[])
{
	char *p;
	ulong sp, ap, size, off;
	int i;

	/*
	 * internal memory location
	 */
	dspmem = 0x50030000;
	Bram = dspmem + 0xe000;
	Eram = Bram + 8192;

	off = fhdr.txtoff;
	if(Tstart != ~0)
		Btext = Tstart;
	else
		Btext = fhdr.txtaddr;
	Etext = Btext + fhdr.txtsz;
	if(Dstart != ~0)
		Bdata = Dstart;
	else
		Bdata = fhdr.dataddr;
	Edata = fhdr.dataddr + fhdr.datsz + fhdr.bsssz;
	readexec(off, Btext, fhdr.txtsz);
	readexec(fhdr.datoff, Bdata, fhdr.datsz);
	iprof = emalloc(((Etext-Btext)/PROFGRAN)*sizeof(*iprof));

	reg.ip = fhdr.entry;
	reg.pc = reg.ip + 4;

	sp = STACKTOP - 4;

	reg.r[REGARG] = sp;	/* Plan 9 profiling clock */
	sp -= BY2WD;

	/* Build exec stack */
	size = strlen(file) + 1 + 3 * BY2WD;		/* argc, argv[0], argv[n], arg 0 */	
	for(i = 0; i < argc; i++)
		size += strlen(argv[i]) + 1 + BY2WD;	/* argv[i], arg i */

	sp -= size;
	sp -= BY2WD;					/* for main to store return address */
	sp &= ~(BY2WD - 1);
	reg.r[REGSP] = sp;
	sp += BY2WD;

	/* Push argc */
	putmem_4(sp, argc + 1);
	sp += BY2WD;

	/* Compute sizeof(argv) and push argv[0] */
	ap = sp + ((argc+1) * BY2WD) + BY2WD;
	putmem_4(sp, ap);
	sp += BY2WD;

	/* Build argv[0] string into stack */
	for(p = file; *p; p++)
		putmem_1(ap++, *p);

	putmem_1(ap++, '\0');

	/* Loop through pushing the arguments */
	for(i = 0; i < argc; i++) {
		putmem_4(sp, ap);
		sp += BY2WD;
		for(p = argv[i]; *p; p++)
			putmem_1(ap++, *p);
		putmem_1(ap++, '\0');
	}
	/* Null terminate argv */
	putmem_4(sp, 0);
}

int
ffmt(char *buf, int n, int mem, ulong dot, ulong val)
{
	snprint(buf, n, "%.7g", dsptod(mem ? getmem_4(dot) : val));
	return 4;
}

int
Ffmt(char *buf, int n, int mem, ulong dot, ulong val)
{
	return ffmt(buf, n, mem, dot, val);
}

void
itrace(char *fmt, ...)
{
	char buf[1024];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	Bprint(bioout, "%8lux %.8lux %s\n", reg.ip, reg.ir, buf);	
}

void
warn(char *fmt, ...)
{
	char buf[1024];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	Bprint(bioout, "%s\n", buf);	
	longjmp(errjmp, 0);
}

int
Xconv(void *o, Fconv *fp)
{
	char str[128];
	long v;

	v = *(long*)o;
	if(v < 0)
		sprint(str, "-#%lux", -v);
	else
		sprint(str, "#%lux", v);
	strconv(str, fp);
	return sizeof v;
}

void
dumpreg(void)
{
	int i;

	for(i = 0; namedreg[i].name; i++){
		if((i%4) == 0 && i != 0)
			Bprint(bioout, "\n");
		Bprint(bioout, "%-3s #%-8lux ", namedreg[i].name, *namedreg[i].reg);
	}
	Bprint(bioout, "\n");
	for(i = 0; i < NREG; i++) {
		if((i%4) == 0 && i != 0)
			Bprint(bioout, "\n");
		Bprint(bioout, "r%-2d #%-8lux ", i, reg.r[i]);
	}
	Bprint(bioout, "\n");
}

void
dumpfreg(void)
{
	int i;

	for(i = 0; i < NFREG; i++)
		Bprint(bioout, "a%d %.7g\n", i, dsptod(reg.f[i].val));
}

void
dumpFreg(void)
{
	int i;

	for(i = 0; i < NFREG; i++)
		Bprint(bioout, "a%d #%.8lux #%.2lux\n", i, reg.f[i].val, reg.f[i].guard);
}
