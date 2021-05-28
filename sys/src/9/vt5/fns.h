#define checkmmu(a, b)
#define countpagerefs(a, b)

#include "../port/portfns.h"

typedef struct Dma Dma;
typedef struct Ether Ether;
#pragma incomplete Dma

void	addconf(char*, char*);
long	adec(long*);
long	ainc(long*);
void	archetherspeed(int, int);
void	archinit(void);
uint	archmiiport(int);
void	archreboot(void);
void	archreset(void);
ulong	archuartclock(int, int);
void	barriers(void);
uintptr	cankaddr(uintptr pa);
Block*	clallocb(void);
void	clockinit(void);
void	clockintr(Ureg*);
void	clrmchk(void);
int	cmpswap(long*, long, long);
#define coherence()	mbar()
char	*cputype2name(char *, int);
void	cpuidprint(void);
void	critintrvec(void);
#define cycles(ip) *(ip) = lcycles()
void	dcbi(uintptr, usize);
void	dcbst(uintptr, usize);
void	dcflush(uintptr, usize);
void	dcrcompile(void);
long	decref(Ref*);
void	delay(int);
void	dtlbmiss(void);
void	dump(void *, int);
void	dumpmal(void);
void	dumpregs(Ureg*);
void	delayloopinit(void);
Dev*	devtabget(int, int);
void	devtabinit(void);
void	devtabreset(void);
long	devtabread(Chan*, void*, long, vlong);
void	devtabshutdown(void);
void	dump(void *, int);
void	eieio(void);
void	etherclock(void);
void	fifoinit(Ether *);
void	firmware(int);
int	fpipower(Ureg*);
int	fpuavail(Ureg*);
int	fpudevprocio(Proc*, void*, long, uintptr, int);
int	fpuemu(Ureg*);
void	fpuinit(void);
void	fpunoted(void);
void	fpunotify(Ureg*);
void	fpuprocresetore(Proc*);
#define	fpuprocresetore(p) USED(p)
void	fpuprocsave(Proc*);
#define	fpuprocsave(p) USED(p)
void	fpusysprocsetup(Proc*);
void	fpusysrfork(Ureg*);
void	fpusysrforkchild(Proc*, Ureg*, Proc*);
void	fputrap(Ureg*, int);
char*	getconf(char*);
u32int	getccr0(void);
u32int	getdar(void);
u32int	getdcr(int);
u32int	getdear(void);
u32int	getdec(void);
u32int	getesr(void);
u32int	getmcsr(void);
u32int	getmsr(void);
u32int	getpid(void);
u32int	getpir(void);
u32int	getpit(void);
u32int	getpvr(void);
u32int	getstid(void);
u32int	gettbl(void);
u32int	gettsr(void);
void	gotopc(uintptr);
int	gotsecuremem(void);
int	havetimer(void);
void	iccci(void);
void	icflush(uintptr, usize);
void	idlehands(void);
int	inb(int);
long	incref(Ref*);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
void	intr(Ureg*);
void	intrdisable(int, void (*)(Ureg*, void*), void*, int, char*);
void	intrfmtcounts(char *s, char *se);
void	intrinit(void);
void	intrshutdown(void);
int	ioalloc(int, int, int, char*);
void	iofree(int);
void	ioinit(void);
int	iprint(char*, ...);
void	isync(void);
void	itlbmiss(void);
void	kexit(Ureg*);
void*	kmapphys(uintptr, uintptr, ulong, ulong, ulong);
uchar	lightbitoff(int);
uchar	lightbiton(int);
uchar	lightstate(int);
void	links(void);
void	malinit(void);
void	mbar(void);
void	meminit(void);
void	mmuinit(void);
void*	mmucacheinhib(void*, ulong);
ulong	mmumapsize(ulong);
void	mutateproc(void *);
int	mutatetrigger(void);
int	newmmupid(void);
int	notify(Ureg*);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
u32int	pidget(void);
void	pidput(u32int);
vlong	pokeaddr(uintptr addr, uint, uint);
void	ppc405console(void);
vlong	probeaddr(uintptr addr);
#define procrestore(p)
void	procsave(Proc*);
void	procsetup(Proc*);
void	putdbsr(ulong);
void	putdcr(int, u32int);
void	putdec(ulong);
void	putesr(ulong);
void	putevpr(ulong);
void	putmcsr(u32int);
void	putmsr(u32int);
void	putpid(u32int);
void	putpit(u32int);
void	putsdr1(u32int);
void	puttcr(u32int);
void	puttsr(u32int);
ulong	qtmborder(void);
void	qtmclock(void);
void	qtmerr(void);
void	qtmerrs(char *);
void	qtminit(void);
void	shutdown(int ispanic);
void	spldone(void);
int	splhi(void);
int	spllo(void);
void	splx(int);
void	splxpc(int);
void	startcpu(int);
u32int	stidget(void);
u32int	stidput(u32int);
void	sync(void);
void	syscall(Ureg*);
uintptr	sysexecstack(uintptr, int);
void	sysprocsetup(Proc*);
#define tas tas32
void	temactransmit(Ether *);
void	touser(uintptr);
void	trapinit(void);
void	trapcritvec(void);
void	trapvec(void);
void	trapmvec(void);
void	tlbdump(char *s);
u32int	tlbrehi(int);
u32int	tlbrelo(int);
u32int	tlbremd(int);
int	tlbsxcc(uintptr);
void	tlbwrx(int, u32int, u32int, u32int);
void	uartliteconsole(void);
void	uartlputc(int);
void	uartlputs(char *s);
void	uncinit(void);
void	uncinitwait(void);
#define	userureg(ur)	(((ur)->status & MSR_PR) != 0)
void	validalign(uintptr, unsigned);
void	verifyproc(Proc *);
void	verifymach(Mach *);
#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
void	watchreset(void);
void	whackether(Ether *);
void	writeconf(void);

/*
 * dma.c
 */
void	dma0init(void);
void	dmainit(void);
int	dmastart(int, void *dest, void *src, ulong len, ulong flags, void (*f)(int));
void	dmawait(int);

/*
 * intr.c
 */
void	intrack(ulong);
void	intrenable(ulong bit, int (*func)(ulong), char *);
void	intrinit(void);

int	cas32(void*, u32int, u32int);
int	tas32(void*);

#define CASU(p, e, n)	cas32((p), (u32int)(e), (u32int)(n))
#define CASV(p, e, n)	cas32((p), (u32int)(e), (u32int)(n))
#define CASW(addr, exp, new)	cas32((addr), (exp), (new))
#define TAS(addr)	tas32(addr)

void	forkret(void);

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

#define	isphys(a)	(((ulong)(a)&KSEGM)!=KSEG0 && ((ulong)(a)&KSEGM)!=KSEG1)
#define KADDR(a)	((void*)((ulong)(a) | KZERO))
#define PADDR(a)	(isphys(a)? (ulong)(a): ((ulong)(a) & ~KSEGM))

/*
 * this low-level printing stuff is ugly,
 * but there appears to be no other way to
 * print until after #t is populated.
 */

#define wave(c) { \
	barriers(); \
	*(ulong *)Uartlite = (c); \
	barriers(); \
}
