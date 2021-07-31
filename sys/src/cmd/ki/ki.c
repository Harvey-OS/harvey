#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern
#include "sparc.h"

char	*file = "k.out";
int	datasize;
ulong	textbase;
Biobuf	bp, bi;
Fhdr	fhdr;

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

	Bprint(bioout, "ki\n");
	inithdr(text);
	initstk(argc, argv);

	reg.fd[13] = 0.5;	/* Normally initialised by the kernel */
	reg.fd[12] = 0.0;
	reg.fd[14] = 1.0;
	reg.fd[15] = 2.0;
	cmd();
}

void
initmap(void)
{

	ulong t, d, b, bssend;
	Segment *s;

	t = (fhdr.txtaddr+fhdr.txtsz+(BY2PG-1)) & ~(BY2PG-1);
	d = (t + fhdr.datsz + (BY2PG-1)) & ~(BY2PG-1);
	bssend = t + fhdr.datsz + fhdr.bsssz;
	b = (bssend + (BY2PG-1)) & ~(BY2PG-1);

	s = &memory.seg[Text];
	s->type = Text;
	s->base = fhdr.txtaddr - fhdr.hdrsz;
	s->end = t;
	s->fileoff = fhdr.txtoff - fhdr.hdrsz;
	s->fileend = s->fileoff + fhdr.txtsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));

	iprof = emalloc(((s->end-s->base)/PROFGRAN)*sizeof(long));
	textbase = s->base;

	s = &memory.seg[Data];
	s->type = Data;
	s->base = t;
	s->end = t+(d-t);
	s->fileoff = fhdr.datoff;
	s->fileend = s->fileoff + fhdr.datsz;
	datasize = fhdr.datsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));

	s = &memory.seg[Bss];
	s->type = Bss;
	s->base = d;
	s->end = d+(b-d);
	s->table = emalloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));

	s = &memory.seg[Stack];
	s->type = Stack;
	s->base = STACKTOP-STACKSIZE;
	s->end = STACKTOP;
	s->table = emalloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));

	reg.pc = fhdr.entry;
}

void
inithdr(int fd)
{
	Symbol s;

	extern Machdata sparcmach;

	seek(fd, 0, 0);
	if (!crackhdr(fd, &fhdr))
		fatal(0, "read text header");

	if(fhdr.type != FSPARC)
		fatal(0, "bad magic number");

	if(syminit(fd, &fhdr) < 0)
		fatal(0, "%r\n");
	symmap = loadmap(symmap, fd, &fhdr);
	if (mach->sbreg && lookup(0, mach->sbreg, &s))
		mach->sb = s.value;
	machdata = &sparcmach;
	asstype = ASUNSPARC;
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
seginit(int fd, Segment *s, int idx, ulong vastart, ulong vaend)
{
	int n;

	while(vastart < vaend) {
		seek(fd, vastart, 0);
		s->table[idx] = emalloc(BY2PG);
		n = read(fd, s->table[idx], BY2PG);
		if(n != BY2PG)
			fatal(1, "data read");
		vastart += BY2PG;
		idx++;
	}
}

void
procinit(int pid)
{
	char *p;
	Segment *s;
	int n, m, sg, i;
	ulong vastart, vaend;
	char mfile[128], tfile[128], sfile[1024];

	sprint(mfile, "/proc/%d/mem", pid);
	sprint(tfile, "/proc/%d/text", pid);
	sprint(sfile, "/proc/%d/segment", pid);

	text = open(tfile, OREAD);
	if(text < 0)
		fatal(1, "open text %s", tfile);
	inithdr(text);

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

	initmap();

	p = strstr(sfile, "Data");
	if(p == 0)
		fatal(0, "no data");

	vastart = strtoul(p+9, 0, 16);
	vaend = strtoul(p+18, 0, 16);
	s = &memory.seg[Data];
	if(s->base != vastart || s->end != vaend) {
		s->base = vastart;
		s->end = vaend;
		free(s->table);
		s->table = malloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));
	}
	seginit(m, s, 0, vastart, vaend);
	
	p = strstr(sfile, "Bss");
	if(p == 0)
		fatal(0, "no bss");

	vastart = strtoul(p+9, 0, 16);
	vaend = strtoul(p+18, 0, 16);
	s = &memory.seg[Bss];
	if(s->base != vastart || s->end != vaend) {
		s->base = vastart;
		s->end = vaend;
		free(s->table);
		s->table = malloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));
	}
	seginit(m, s, 0, vastart, vaend);

	reg.pc = greg(m, REGOFF(pc));
	reg.r[1] = greg(m, REGOFF(sp));
	reg.r[30] = greg(m, REGOFF(r30));
	reg.r[31] = greg(m, REGOFF(r31));

	for(i = 1; i < 29; i++)
		reg.r[i] = greg(m, roff[i-1]);

	s = &memory.seg[Stack];
	vastart = reg.r[1] & ~(BY2PG-1);
	seginit(m, s, (vastart-s->base)/BY2PG, vastart, STACKTOP);
	close(m);
	Bprint(bioout, "ki\n"); 
}

void
reset(void)
{
	int i, l, m;
	Segment *s;
	Breakpoint *b;

	memset(&reg, 0, sizeof(Registers));
	reg.fd[13] = 0.5;	/* Normally initialised by the kernel */
	reg.fd[12] = 0.0;
	reg.fd[14] = 1.0;
	reg.fd[15] = 2.0;
	for(i = 0; i > Nseg; i++) {
		s = &memory.seg[i];
		l = ((s->end-s->base)/BY2PG)*sizeof(uchar*);
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
	ulong size, sp, ap;
	int i;
	char *p;

	initmap();
	sp = STACKTOP - 4;

	/* Build exec stack */
	size = strlen(file)+1+BY2WD+BY2WD+(BY2WD*2);	
	for(i = 0; i < argc; i++)
		size += strlen(argv[i])+BY2WD+1;

	sp -= size;
	sp &= ~7;
	reg.r[1] = sp;
	reg.r[7] = STACKTOP-4;	/* Plan 9 profiling clock */

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
	char buf[ERRMAX], *s;
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	s = "ki: %s\n";
	if(syserr)
		s = "ki: %s: %r\n";
	fprint(2, s, buf);
	exits(buf);
}

void
itrace(char *fmt, ...)
{
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	Bprint(bioout, "%8lux %.8lux %s\n", reg.pc, reg.ir, buf);	
}

void
dumpreg(void)
{
	int i;

	Bprint(bioout, "PC  #%-8lux SP  #%-8lux Y   #%-8lux PSR #%-8lux\n",
				reg.pc, reg.r[1], reg.Y, reg.psr);

	for(i = 0; i < 32; i++) {
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
		Bprint(bioout, "\tF%-2d %s\n", i, buf);
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
		ieeedftos(buf, sizeof(buf), reg.di[i] ,reg.di[i+1]);
		Bprint(bioout, "F%-2d %s\t", i, buf);
		i += 2;
		ieeedftos(buf, sizeof(buf), reg.di[i] ,reg.di[i+1]);
		Bprint(bioout, "\tF%-2d %s\n", i, buf);
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
