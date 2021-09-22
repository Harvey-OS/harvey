#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <tos.h>
#define Extern
#include "arm64.h"

char	*file = "7.out";
int	datasize;
ulong	textbase;
Biobuf	bp, bi;
Fhdr	fhdr;
ulong	bits[32];

void
main(int argc, char **argv)
{
	int pid, i;

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

	Bprint(bioout, "7i\n");
	inithdr(text);
	initstk(argc, argv);

	for(i=0; i<32; i++)
		bits[i] = 1L << (31-i);

	fpreginit();
	cmd();
}

/*
 * we're rounding text and data base to the nearest 1MB on arm64 now,
 * and mach->pgsize is actually what to round segment boundaries up to.
 */
#define SEGROUND mach->pgsize

void
initmap(void)
{

	ulong t, d, b, bssend;
	Segment *s;

	t = (fhdr.txtaddr+fhdr.txtsz+(SEGROUND-1)) & ~(SEGROUND-1);
	d = (t + fhdr.datsz + (BY2PG-1)) & ~(BY2PG-1);
	bssend = t + fhdr.datsz + fhdr.bsssz;
	b = (bssend + (BY2PG-1)) & ~(BY2PG-1);

	s = &memory.seg[Text];
	s->type = Text;
	s->base = fhdr.txtaddr - fhdr.hdrsz;
	s->end = (fhdr.txtaddr+fhdr.txtsz+(BY2PG-1)) & ~(BY2PG-1);
	s->fileoff = fhdr.txtoff - fhdr.hdrsz;
	s->fileend = s->fileoff + fhdr.txtsz;
	s->table = emalloc(((s->end-s->base)/BY2PG)*sizeof(uchar*));

	iprofsize = (s->end-s->base)/PROFGRAN;
	iprof = emalloc(iprofsize*sizeof(long));
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

	// TODO(aram): enable once we have it in libmach.
	// extern Machdata arm64mach;

	seek(fd, 0, 0);
	if (!crackhdr(fd, &fhdr))
		fatal(0, "read text header");

	if(fhdr.type != FARM64)
		fatal(0, "bad magic number");

	if(syminit(fd, &fhdr) < 0)
		fatal(0, "%r\n");
	symmap = loadmap(symmap, fd, &fhdr);
	if (mach->sbreg && lookup(0, mach->sbreg, &s))
		mach->sb = s.value;
	// TODO(aram): enable once we have it in libmach.
	// machdata = &arm64mach;
}

uvlong
greg(int f, ulong off)
{
	int n;
	uvlong l;
	uchar wd[BY2WD];

	n = pread(f, wd, BY2WD, off);
	if(n != BY2WD)
		fatal(1, "read register");

	l  = (uvlong)wd[0]<<56;
	l |= (uvlong)wd[1]<<48;
	l |= (uvlong)wd[2]<<40;
	l |= (uvlong)wd[3]<<32;
	l |= (uvlong)wd[4]<<24;
	l |= (uvlong)wd[5]<<16;
	l |= (uvlong)wd[6]<<8;
	l |= (uvlong)wd[7];
	return l;
}

ulong
roff[] = {
	REGOFF(r0),
	REGOFF(r1),	REGOFF(r2),	REGOFF(r3),
	REGOFF(r4),	REGOFF(r5),	REGOFF(r6),
	REGOFF(r7),	REGOFF(r8),	REGOFF(r9),
	REGOFF(r10),	REGOFF(r11),	REGOFF(r12),
	REGOFF(r13),	REGOFF(r14),	REGOFF(r15),
	REGOFF(r16),	REGOFF(r17),	REGOFF(r18),
	REGOFF(r19),	REGOFF(r20),	REGOFF(r21),
	REGOFF(r22),	REGOFF(r23),	REGOFF(r24),
	REGOFF(r25),	REGOFF(r26),	REGOFF(r27),
	REGOFF(r28),	REGOFF(r29),	REGOFF(link),
};

void
seginit(int fd, Segment *s, int idx, ulong vastart, ulong vaend)
{
	int n;

	while(vastart < vaend) {
		s->table[idx] = emalloc(BY2PG);
		n = pread(fd, s->table[idx], BY2PG, vastart);
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
	reg.r[31] = greg(m, REGOFF(sp));
	reg.r[2] = greg(m, REGOFF(r2));
	reg.r[30] = greg(m, REGOFF(link));

	for(i = 0; i < 31; i++)
		reg.r[i] = greg(m, roff[i-1]);

	s = &memory.seg[Stack];
	vastart = reg.r[31] & ~(BY2PG-1);
	seginit(m, s, (vastart-s->base)/BY2PG, vastart, STACKTOP);
	close(m);
	Bprint(bioout, "7i\n"); 
}

void
reset(void)
{
	int i, l, m;
	Segment *s;
	Breakpoint *b;

	memset(&reg, 0, sizeof(Registers));
	fpreginit();
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
	uvlong size, sp, ap, tos;
	int i;
	char *p;

	initmap();
	tos = STACKTOP - sizeof(Tos)*2;	/* we'll assume twice the host's is big enough */
	sp = tos;
	for (i = 0; i < sizeof(Tos)*2; i++)
		putmem_b(tos + i, 0);

	/*
	 * pid is second word from end of tos and needs to be set for nsec().
	 * we know arm64 is a 64-bit cpu, so we'll assume knowledge of the Tos
	 * struct for now, and use our pid.
	 */
	putmem_w(tos + 4*8 + 2*sizeof(ulong) + 3*sizeof(uvlong), getpid());

	/* Build exec stack */
	size = strlen(file)+1+BY2WD+BY2WD+(BY2WD*2);	
	for(i = 0; i < argc; i++)
		size += strlen(argv[i])+BY2WD+1;

	sp -= size;
	sp &= ~7;
	reg.r[31] = sp;
	reg.r[0] = tos;		/* Plan 9 profiling clock, etc. */

	/* Push argc */
	putmem_v(sp, argc+1);
	sp += BY2WD;

	/* Compute sizeof(argv) and push argv[0] */
	ap = sp+((argc+1)*BY2WD)+BY2WD;
	putmem_v(sp, ap);
	sp += BY2WD;
	
	/* Build argv[0] string into stack */
	for(p = file; *p; p++)
		putmem_b(ap++, *p);

	putmem_b(ap++, '\0');

	/* Loop through pushing the arguments */
	for(i = 0; i < argc; i++) {
		putmem_v(sp, ap);
		sp += BY2WD;
		for(p = argv[i]; *p; p++)
			putmem_b(ap++, *p);
		putmem_b(ap++, '\0');
	}
	/* Null terminate argv */
	putmem_v(sp, 0);

}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRMAX], *s;
	va_list ap;

	va_start(ap, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, ap);
	va_end(ap);
	s = "7i: %s\n";
	if(syserr)
		s = "7i: %s: %r\n";
	fprint(2, s, buf);
	exits(buf);
}

void
itrace(char *fmt, ...)
{
	char buf[128];
	va_list ap;

	va_start(ap, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, ap);
	va_end(ap);
	Bprint(bioout, "%8lux %.8lux %s\n", reg.pc, reg.ir, buf);
Bflush(bioout);
}

void
dumpreg(void)
{
	int i;

	Bprint(bioout, "PC  #%-6lux (#%-6lux) LR  #%-16llux NZCV %d%d%d%d\n",
				reg.pc, reg.prevpc, reg.r[30], reg.N, reg.Z, reg.C, reg.V);

	for(i = 0; i < 31; i++) {
		if((i%3) == 0 && i != 0)
			Bprint(bioout, "\n");
		Bprint(bioout, "R%-2d #%-16llux ", i, reg.r[i]);
	}
	Bprint(bioout, "SP  #%-16llux\n", reg.r[31]);
}

void
dumpfreg(void)
{
	dumpdreg();
}

void
dumpdreg(void)
{
	int i;
	char buf[64];
	FPdbleword d;

	i = 0;
	while(i < 32) {
		d.x = reg.fd[i];
		ieeedftos(buf, sizeof(buf), d.hi, d.lo);
		Bprint(bioout, "F%-2d %s\t", i, buf);
		i++;
		d.x = reg.fd[i];
		ieeedftos(buf, sizeof(buf), d.hi, d.lo);
		Bprint(bioout, "\tF%-2d %s\n", i, buf);
		i++;
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
