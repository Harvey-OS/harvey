#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern
#include "mips.h"

char	*file = "v.out";
int	datasize;
int	textbase;
Biobuf	bp, bi;

void
main(int argc, char **argv)
{
	int pid;

	argc--;
	argv++;

	bioout = &bp;
	bin = &bi;
	Binit(bioout, 1, OWRITE);
	Binit(bin, 0, OREAD);

	tlb.on = 1;
	tlb.tlbsize = 24;

	if(argc) {
		pid = atoi(argv[0]);
		if(pid != 0) {
			procinit(pid);
			cmd();
		}
		file = argv[0];
	}
	argc--;
	argv++;

	text = open(file, OREAD);
	if(text < 0)
		fatal(1, "open text '%s'", file);

	Bprint(bioout, "vi\n"); 
	init(argc, argv);

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
inithdr(int fd, Fhdr *fp)
{
	seek(fd, 0, 0);
	if (!crackhdr(fd, fp))
		fatal(0, "read text header");

	if(fp->type != FMIPS)
		fatal(0, "bad magic number");

	if (syminit(fd, fp) < 0)
		if (symerror)
			fatal(0, "%s\n", symerror);
}

ulong
greg(int f, ulong off)
{
	int n;
	ulong l;
	uchar wd[BY2WD];
	
	seek(f, off, 0);
	n = read(f, wd, BY2WD);
	if(n != BY2WD)
		fatal(1, "read register");

	l  = wd[0]<<24;
	l |= wd[1]<<16;
	l |= wd[2]<<8;
	l |= wd[3];
	return l;
}

ulong
roff[] = {
	REGOFF(r1),	REGOFF(r2),	REGOFF(r3),
	REGOFF(r4),	REGOFF(r5),	REGOFF(r6),
	REGOFF(r7),	REGOFF(r8),	REGOFF(r9),
	REGOFF(r10),	REGOFF(r11),	REGOFF(r12),
	REGOFF(r13),	REGOFF(r14),	REGOFF(r15),
	REGOFF(r16),	REGOFF(r17),	REGOFF(r18),
	REGOFF(r19),	REGOFF(r20),	REGOFF(r21),
	REGOFF(r22),	REGOFF(r23),	REGOFF(r24),
	REGOFF(r25),	REGOFF(r26),	REGOFF(r27),
	REGOFF(r28)
};

void
procinit(int pid)
{
	char *p;
	Segment *s;
	int n, m, sg, i;
	ulong vastart, vaend, t;
	Fhdr f;
	char mfile[128], tfile[128], sfile[1024];

	sprint(mfile, "/proc/%d/mem", pid);
	sprint(tfile, "/proc/%d/text", pid);
	sprint(sfile, "/proc/%d/segment", pid);

	text = open(tfile, OREAD);
	if(text < 0)
		fatal(1, "open text %s", tfile);
	inithdr(text, &f);

	sg = open(sfile, OREAD);
	if(sg < 0)
		fatal(1, "open text %s", sfile);

	n = read(sg, sfile, sizeof(sfile));
	if(n >= sizeof(sfile))
		fatal(0, "segment file buffer too small");
	close(sg);

	m = open(mfile, OREAD);
	if(m < 0)
		fatal(1, "open %s", mfile);

	t = (f.txtaddr+f.txtsz+(BY2PG-1)) & ~(BY2PG-1);
	s = &memory.seg[Text];
	s->type = Text;
	s->base = f.txtaddr;
	s->end = t;
	s->fileoff = f.txtoff;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	iprof = emalloc(((s->end-s->base)/PROFGRAN)*sizeof(long));
	textbase = s->base;

	p = strstr(sfile, "Data");
	if(p == 0)
		fatal(0, "no data");

	vastart = strtoul(p+9, 0, 16);
	vaend = strtoul(p+18, 0, 16);
	s = &memory.seg[Data];
	s->type = Data;
	s->base = vastart;
	s->end	= vaend;
	s->fileoff = f.datoff;
	s->fileend = s->fileoff + f.datsz;
	datasize = f.datsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);
	i = 0;
	while(vastart < vaend) {
		seek(m, vastart, 0);
		s->table[i] = emalloc(BY2PG);
		n = read(m, s->table[i], BY2PG);
		if(n != BY2PG)
			fatal(1, "data read");
		vastart += BY2PG;
		i++;
	}
	
	p = strstr(sfile, "Bss");
	if(p == 0)
		fatal(0, "no bss");

	vastart = strtoul(p+9, 0, 16);
	vaend = strtoul(p+18, 0, 16);
	s = &memory.seg[Bss];
	s->type = Bss;
	s->base = vastart;
	s->end = vaend;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);
	i = 0;
	while(vastart < vaend) {
		seek(m, vastart, 0);
		s->table[i] = emalloc(BY2PG);
		n = read(m, s->table[i], BY2PG);
		if(n != BY2PG)
			fatal(1, "data read");
		vastart += BY2PG;
		i++;
	}

	reg.pc = greg(m, REGOFF(pc));
	reg.r[29] = greg(m, REGOFF(sp));
	reg.r[30] = greg(m, REGOFF(r30));
	reg.r[31] = greg(m, REGOFF(r31));

	reg.mhi = greg(m, REGOFF(hi));
	reg.mlo = greg(m, REGOFF(lo));

	for(i = 1; i < 29; i++)
		reg.r[i] = greg(m, roff[i-1]);

	s = &memory.seg[Stack];
	s->type = Stack;
	s->base = STACKTOP-STACKSIZE;
	s->end = STACKTOP;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);
	vastart = reg.r[29] & ~(BY2PG-1);
	i = (vastart-s->base)/BY2PG;
	while(vastart < STACKTOP) {
		seek(m, vastart, 0);
		s->table[i] = emalloc(BY2PG);
		n = read(m, s->table[i], BY2PG);
		if(n != BY2PG)
			fatal(1, "data read");
		vastart += BY2PG;
		i++;
	}
	close(m);
	Bprint(bioout, "vi\n"); 
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
	memset(&memory, 0, sizeof(memory));

	for(b = bplist; b; b = b->next)
		b->done = b->count;
}

void
init(int argc, char *argv[])
{
	ulong size, t, d, b, bssend;
	ulong sp, ap;
	Segment *s;
	int i;
	char *p;
	Fhdr f;

	inithdr(text, &f);
	t = (f.txtaddr+f.txtsz+(BY2PG-1)) & ~(BY2PG-1);
	d = (t + f.datsz + (BY2PG-1)) & ~(BY2PG-1);
	bssend = t + f.datsz + f.bsssz;
	b = (bssend + (BY2PG-1)) & ~(BY2PG-1);

	s = &memory.seg[Text];
	s->type = Text;
	s->base = f.txtaddr;
	s->end = t;
	s->fileoff = f.txtoff;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	iprof = emalloc(((s->end-s->base)/PROFGRAN)*sizeof(long));
	textbase = s->base;

	s = &memory.seg[Data];
	s->type = Data;
	s->base = t;
	s->end	= t+(d-t);
	s->fileoff = f.datoff;
	s->fileend = s->fileoff + f.datsz;
	datasize = f.datsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	s = &memory.seg[Bss];
	s->type = Bss;
	s->base = d;
	s->end = d+(b-d);
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	s = &memory.seg[Stack];
	s->type = Stack;
	s->base = STACKTOP-STACKSIZE;
	s->end = STACKTOP;
	s->table = emalloc(((s->end-s->base)/BY2PG)*BY2WD);

	reg.pc = f.entry;
	sp = STACKTOP - 4;

	/* Build exec stack */
	size = strlen(file)+1+BY2WD+BY2WD+BY2WD;	
	for(i = 0; i < argc; i++)
		size += strlen(argv[i])+BY2WD+1;

	sp -= size;
	sp &= ~3;
	reg.r[29] = sp;
	reg.r[1] = STACKTOP-4;	/* Plan 9 profiling clock */

	/* Push argc */
	putmem_w(sp, argc+1);
	sp += BY2WD;

	/* Compute sizeof(argv) and push argv[0] */
	ap = sp+((argc+1)*BY2WD)+BY2WD;
	putmem_w(sp, ap);
	sp += BY2WD;
	
	/* Build argv[0] string into stack */
	for(p = file; *p; p++)
		putmem_b(ap++, *p);

	putmem_b(ap++, '\0');

	/* Loop through pushing the arguments */
	for(i = 0; i < argc; i++) {
		putmem_w(sp, ap);
		sp += BY2WD;
		for(p = argv[i]; *p; p++)
			putmem_b(ap++, *p);
		putmem_b(ap++, '\0');
	}
	/* Null terminate argv */
	putmem_w(sp, 0);

}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRLEN], *s;

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	s = "vi: %s\n";
	if(syserr)
		s = "vi: %s: %r\n";
	fprint(2, s, buf);
	exits(buf);
}

void
itrace(char *fmt, ...)
{
	char buf[128];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	Bprint(bioout, "%8lux %.8lux %s\n", reg.pc, reg.ir, buf);	
}

void
dumpreg(void)
{
	int i;

	Bprint(bioout, "PC  #%-8lux SP  #%-8lux HI  #%-8lux LO  #%-8lux\n",
				reg.pc, reg.r[29], reg.mhi, reg.mlo);

	for(i = 0; i < 32; i++) {
		if((i%4) == 0 && i != 0)
			Bprint(bioout, "\n");
		Bprint(bioout, "R%-2d #%-8lux ", i, reg.r[i]);
	}
	Bprint(bioout, "\n");
}

static char fpbuf[64];

char*
ieeedtos(char *fmt, ulong h, ulong l)
{
	double fr;
	int exp;
	char *p = fpbuf;

	if(h & (1L<<31)){
		*p++ = '-';
		h &= ~(1L<<31);
	}else
		*p++ = ' ';
	if(l == 0 && h == 0){
		strcpy(p, "0.");
		goto ret;
	}
	exp = (h>>20) & ((1L<<11)-1L);
	if(exp == 0){
		sprint(p, "DeN(%.8lux%.8lux)", h, l);
		goto ret;
	}
	if(exp == ((1L<<11)-1L)){
		if(l==0 && (h&((1L<<20)-1L)) == 0)
			sprint(p, "Inf");
		else
			sprint(p, "NaN(%.8lux%.8lux)", h&((1L<<20)-1L), l);
		goto ret;
	}
	exp -= (1L<<10) - 2L;
	fr = l & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (l>>16) & ((1L<<16)-1L);
	fr /= 1L<<16;
	fr += (h & (1L<<20)-1L) | (1L<<20);
	fr /= 1L<<21;
	fr = ldexp(fr, exp);
	sprint(p, fmt, fr);
    ret:
	return fpbuf;
}

char*
ieeeftos(char *fmt, ulong h)
{
	double fr;
	int exp;
	char *p = fpbuf;

	if(h & (1L<<31)){
		*p++ = '-';
		h &= ~(1L<<31);
	}else
		*p++ = ' ';
	if(h == 0){
		strcpy(p, "0.");
		goto ret;
	}
	exp = (h>>23) & ((1L<<8)-1L);
	if(exp == 0){
		sprint(p, "DeN(%.8lux)", h);
		goto ret;
	}
	if(exp == ((1L<<8)-1L)){
		if((h&((1L<<23)-1L)) == 0)
			sprint(p, "Inf");
		else
			sprint(p, "NaN(%.8lux)", h&((1L<<23)-1L));
		goto ret;
	}
	exp -= (1L<<7) - 2L;
	fr = (h & ((1L<<23)-1L)) | (1L<<23);
	fr /= 1L<<24;
	fr = ldexp(fr, exp);
	sprint(p, fmt, fr);
    ret:
	return fpbuf;
}

void
dumpfreg(void)
{
	int i;

	i = 0;
	while(i < 32) {
		Bprint(bioout, "F%-2d %s\t", i, ieeeftos("%.9g", reg.di[i]));
		i++;
		Bprint(bioout, "\t\t\tF%-2d %s\n", i, ieeeftos("%.9g", reg.di[i]));
		i++;
	}
}

void
dumpdreg(void)
{
	int i;

	i = 0;
	while(i < 32) {
		if(reg.ft[i] == FPd)
			Bprint(bioout, "F%-2d %s\t\t\t", i, 
				ieeedtos("%.9g", reg.di[i] ,reg.di[i+1]));
		else
			Bprint(bioout, "F%-2d %s\t\t\t", i, 
				ieeedtos("%.9g", reg.di[i+1] ,reg.di[i]));
		i += 2;
		if(reg.ft[i] == FPd)
			Bprint(bioout, "F%-2d %s\n", i, 
				ieeedtos("%.9g", reg.di[i] ,reg.di[i+1]));
		else
			Bprint(bioout, "F%-2d %s\n", i, 
				ieeedtos("%.9g", reg.di[i+1] ,reg.di[i]));
		i += 2;
	}
}

void *
emalloc(ulong size)
{
	void *a;

	a = malloc(size);
	if(a == 0)
		fatal(0, "no memory");

	memset(a, 0, size);
	return a;
}

void *
erealloc(void *a, ulong oldsize, ulong size)
{
	void *n;

	n = malloc(size);
	if(n == 0)
		fatal(0, "no memory");
	memset(n, 0, size);
	if(size > oldsize)
		size = oldsize;
	memmove(n, a, size);
	return n;
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

