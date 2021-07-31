#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern
#include "sim.h"

Namedreg namedreg[] = {
	"PC",	&reg.pc,
	"IR",	&reg.ir,
	0,	0
};

static	ulong		Tstart;
static	ulong		Dstart;
static	ulong		Rtext;
char	*file = "9.out";
Biobuf	bp, bi;
Fhdr	fhdr;

void
usage(void)
{
	fprint(2, "usage:\t9i [-T textstart] [-R dataround] [-D datastart] prog args\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Tstart = ~0;
	Dstart = ~0;
	Rtext = 0;
	ARGBEGIN{
	case 'D':
		Dstart = strtoul(ARGF(), 0, 0);
		break;
	case 'T':
		Tstart = strtoul(ARGF(), 0, 0);
		break;
	case 'R':
		Rtext = strtoul(ARGF(), 0, 0);
		break;
	default:
		usage();	
	}ARGEND

	bioout = &bp;
	bin = &bi;
	Binit(bioout, 1, OWRITE);
	Binit(bin, 0, OREAD);

	tlb.on = 1;
	tlb.tlbsize = 24;

	if(argc)
		file = argv[0];
	argc--;
	argv++;

	text = open(file, OREAD);
	if(text < 0)
		fatal("open text '%s': %r", file);

	Bprint(bioout, "9i\n");
	inithdr(text);
	initstk(argc, argv);

	reg.fd[dreg(24)] = 0.0;		/* Normally initialised by the kernel */
	reg.ft[24] = FPd;
	reg.fd[dreg(26)] = 0.5;
	reg.ft[26] = FPd;
	reg.fd[dreg(28)] = 1.0;
	reg.ft[28] = FPd;
	reg.fd[dreg(30)] = 2.0;
	reg.ft[30] = FPd;
	cmd();
}

void
initmap()
{
	ulong t, d, b, hdoff, et;
	Segment *s;

	hdoff = fhdr.hdrsz;
	if(Tstart != ~0){
		Btext = Tstart;
		hdoff = 0;
	}else
		Btext = fhdr.txtaddr;
	Etext = Btext + fhdr.txtsz;
	et = 0;
	if(Rtext != 0)
		et = (Etext + (Rtext-1)) & ~(Rtext-1);

	if(Dstart != ~0)
		Bdata = Dstart;
	else if(Rtext != 0)
		Bdata = et;
	else
		Bdata = fhdr.dataddr;
	Edata = Bdata + fhdr.datsz + fhdr.bsssz;

	t = (Etext+(BY2PG-1)) & ~(BY2PG-1);
	d = (Bdata + fhdr.datsz + (BY2PG-1)) & ~(BY2PG-1);
	b = (Edata + (BY2PG-1)) & ~(BY2PG-1);

	s = &memory.seg[Text];
	s->type = Text;
	s->base = Btext - hdoff;
	s->end = t;
	s->fileoff = fhdr.txtoff - hdoff;
	s->fileend = s->fileoff + fhdr.txtsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	iprof = emalloc(((Etext-Btext)/PROFGRAN)*sizeof(long));

	s = &memory.seg[Data];
	s->type = Data;
	s->base = Bdata;
	s->end = d;
	s->fileoff = fhdr.datoff;
	s->fileend = s->fileoff + fhdr.datsz;
	datasize = fhdr.datsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	s = &memory.seg[Bss];
	s->type = Bss;
	s->base = d;
	s->end = b;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	s = &memory.seg[Stack];
	s->type = Stack;
	s->base = STACKTOP-STACKSIZE;
	s->end = STACKTOP;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	reg.pc = fhdr.entry;
}

void
inithdr(int fd)
{
	seek(fd, 0, 0);
	if(!crackhdr(fd, &fhdr))
		fatal("read text header");
	if(fhdr.type != F29000)
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
	reg.fd[dreg(24)] = 0.0;		/* Normally initialised by the kernel */
	reg.ft[24] = FPd;
	reg.fd[dreg(26)] = 0.5;
	reg.ft[26] = FPd;
	reg.fd[dreg(28)] = 1.0;
	reg.ft[28] = FPd;
	reg.fd[dreg(30)] = 2.0;
	reg.ft[30] = FPd;

	for(i = 0; i > Nseg; i++) {
		s = &memory.seg[i];
		l = ((s->end-s->base)/BY2PG)*BY2WD;
		for(m = 0; m < l; m++)
			if(s->table[m])
				free(s->table[m]);
		free(s->table);
	}
	free(iprof);
	memset(&memory, 0, sizeof(memory));

	for(b = bplist; b; b = b->next)
		b->done = b->count;
}

void
initstk(int argc, char *argv[])
{
	ulong size;
	ulong sp, ap;
	int i;
	char *p;

	initmap();
	sp = STACKTOP - 4;

	/* Build exec stack */
	size = strlen(file)+1+BY2WD+BY2WD+BY2WD;	
	for(i = 0; i < argc; i++)
		size += strlen(argv[i])+BY2WD+1;

	sp -= size;
	sp &= ~3;
	reg.r[REGSP] = sp;
	reg.r[REGARG] = STACKTOP-4;	/* Plan 9 profiling clock */

	/* Push argc */
	putmem_4(sp, argc+1);
	sp += BY2WD;

	/* Compute sizeof(argv) and push argv[0] */
	ap = sp+((argc+1)*BY2WD)+BY2WD;
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
fmtins(char *buf, int n, int mem, ulong dot, ulong val)
{
	int inc;
	extern int _a29000disinst(ulong, ulong, char*, int);

	inc = _a29000disinst(dot, mem ? getmem_4(dot) : val, buf, n);
	if(inc < 0){
		snprint(buf, n, "%r");
		return 0;
	}
	return inc;
}

int
ffmt(char *buf, int n, int mem, ulong dot, ulong val)
{
	ieeesftos(buf, n, mem ? getmem_4(dot) : val);
	return 4;
}

int
Ffmt(char *buf, int n, int mem, ulong dot, ulong val)
{
	if(mem)
		ieeedftos(buf, n, getmem_4(dot), getmem_4(dot+4));
	else
		ieeedftos(buf, n, val, 0);
	return 8;
}

void
error(char *fmt, ...)
{
	char buf[8192];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	Bprint(bioout, "%s\n", buf);
	longjmp(errjmp, 0);
}

void
fatal(char *fmt, ...)
{
	char buf[ERRLEN];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	fprint(2, "9i: %s\n", buf);
	exits(buf);
}

void
itrace(char *fmt, ...)
{
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	Bprint(bioout, "%8lux %.8lux %s\n", reg.pc, reg.ir, buf);	
}

void
dumpreg(void)
{
	int i;

	Bprint(bioout, "PC  #%-8lux SP  #%-8lux\n",
				reg.pc, reg.r[REGSP]);

	for(i = 64; i < 128; i++) {
		if((i%4) == 0 && i != 0)
			Bprint(bioout, "\n");
		Bprint(bioout, "R%-2d #%-8lux ", i, reg.r[i]);
	}
	Bprint(bioout, "\n");
}

void
dumpfreg(void)
{
	int i;
	char buf[64];

	i = 0;
	while(i < 32) {
		ieeesftos(buf, sizeof(buf), reg.di[i]);
		Bprint(bioout, "F%-2d %s\t", i, buf);
		i++;

		ieeesftos(buf, sizeof(buf), reg.di[i]);
		Bprint(bioout, "\t\t\tF%-2d %s\n", i, buf);
		i++;
	}
}

void
dumpdreg(void)
{
	int i;
	char buf[64];

	i = 0;
	while(i < 32) {
		if(reg.ft[i] == FPd)
			ieeedftos(buf, sizeof(buf), reg.di[i] ,reg.di[i+1]);
		else
			ieeedftos(buf, sizeof(buf), reg.di[i+1] ,reg.di[i]);
		Bprint(bioout, "F%-2d %s\t\t\t", i, buf);
		i += 2;

		if(reg.ft[i] == FPd)
			ieeedftos(buf, sizeof(buf), reg.di[i] ,reg.di[i+1]);
		else
			ieeedftos(buf, sizeof(buf), reg.di[i+1] ,reg.di[i]);
		Bprint(bioout, "F%-2d %s\n", i, buf);
		i += 2;
	}
}

void *
emalloc(ulong size)
{
	void *a;

	a = malloc(size);
	if(a == 0)
		fatal("no memory");

	memset(a, 0, size);
	return a;
}

void *
erealloc(void *a, ulong size)
{
	a = realloc(a, size);
	if(a == 0)
		fatal("out of memory");
	return a;
}

Mulu
mulu(ulong u1, ulong u2)
{
	ulong lo1, lo2, hi1, hi2, lo, hi, t1, t2, t;

	lo1 = u1 & 0xffff;
	lo2 = u2 & 0xffff;
	hi1 = u1 >> 16;
	hi2 = u2 >> 16;

	lo = lo1 * lo2;
	t1 = lo1 * hi2;
	t2 = lo2 * hi1;
	hi = hi1 * hi2;
	t = lo;
	lo += t1 << 16;
	if(lo < t)
		hi++;
	t = lo;
	lo += t2 << 16;
	if(lo < t)
		hi++;
	hi += (t1 >> 16) + (t2 >> 16);
	return (Mulu){lo, hi};
}

Mul
mul(long l1, long l2)
{
	Mulu m;
	ulong t, lo, hi;
	int sign;

	sign = 0;
	if(l1 < 0){
		sign ^= 1;
		l1 = -l1;
	}
	if(l2 < 0){
		sign ^= 1;
		l2 = -l2;
	}
	m = mulu(l1, l2);
	lo = m.lo;
	hi = m.hi;
	if(sign){
		t = lo = ~lo;
		hi = ~hi;
		lo++;
		if(lo < t)
			hi++;
	}
	return (Mul){lo, hi};
}

