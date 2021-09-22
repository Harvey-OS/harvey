#define checkmmu(a, b)
#define countpagerefs(a, b)

#include "../port/portfns.h"

typedef struct Ether Ether;
struct Ether;

#pragma	varargck argpos	_uartprint 1

long	adec(long *);
long	ainc(long *);
void	allcacheinfo(Memcache *);
void	allcachesinfo(Memcache *);
void	allcachesinvse(void *va, int bytes);
void	allcachesinv(void);
void	allcachesoff(void);
void	allcacheson(void);
void	allcacheswbinvse(void *va, int bytes);
void	allcacheswbinv(void);
void	allcacheswbse(void *va, int bytes);
void	allcacheswb(void);
int	archether(unsigned, Ether *);
void	archreboot(void);
void	archreset(void);
void	awaitstablel1pt(void);
void	cachedinvse(void*, int);
void	cachedinv(void);
void	cachedon(void);
void	cachedwbinvse(void*, int);
void	cachedwbinv(void);
void	cachedwbse(void*, int);
void	cachedwb(void);
void	cacheiinv(void);
void	cacheinit(void);
void	cacheuwbinv(void);
uintptr	cankaddr(uintptr pa);
void	chkmissing(void);
ulong	ckcpurst(void);
void	ckstack(Ureg **uregp);
void	clockenable(int cpu);
void	clockprod(Ureg *);
void	clockshutdown(void);
void	clockson(void);
void	clrex(void);
int	cmpswap(long*, long, long);
void	coherence(void);
int	condok(int cc, int c);
void	confirmup(void);
u32int	controlget(void);
void	cortexa9cachecfg(void);
u32int	cpctget(void);
u32int	cpidget(void);
ulong	cpmpidget(void);
ulong	cprd(int cp, int op1, int crn, int crm, int op2);
ulong	cprdsc(int op1, int crn, int crm, int op2);
void	cpucleanup(void);
void	cpuidprint(void);
int	cpusinreset(void);
char	*cputype2name(char *buf, int size);
void	cpuwfi(void);
void	cpwr(int cp, int op1, int crn, int crm, int op2, ulong val);
void	cpwrsc(int op1, int crn, int crm, int op2, ulong val);
#define cycles(vlp) *(vlp) = (ulong)lcycles()
u32int	dacget(void);
void	dacput(u32int);
void	datasegok(void);
void	delay(int);
void	dmainit(void);
int	dmastart(void *, int, void *, int, uint, Rendez *, int *);
void	dmatest(void);
void	dsb(void);
void	dumpintrpend(void);
void	dumpscustate(void);
void	dump(void *vaddr, int words);
void	errata(void);
void	exceptinit(void);
u32int	farget(void);
u32int	fariget(void);
void	forcedown(uint want);
void	forkret(void);
void	fpclear(void);
void	fpoff(void);
void	fpon(void);
ulong	fprd(int fpreg);
void	fprestreg(int fpreg, uvlong val);
ulong	fpsavereg(int fpreg, uvlong *fpp);
void	fpwr(int fpreg, ulong val);
u32int	fsrget(void);
u32int	fsriget(void);
ulong	getauxctl(void);
ulong	getclvlid(void);
ulong	getcyc(void);
int	getfpinst(uintptr pc, Fpinst *);
Inst	getinst(uintptr pc);
ulong	getmmfr0(void);
ulong	getmmfr1(void);
ulong	getmmfr2(void);
ulong	getmmfr3(void);
int	getncpus(void);
u32int	getpc(void);
u32int	getpsr(void);
u32int	getscr(void);
ulong	getwayssets(void);
int	havefp(void);
void	hivecs(void);
int	inkzeroseg(void);
void	intcmask(uint);
void	intcunmask(uint);
void	intrcpu(int cpu, int dolock);
void	intrcpushutdown(void);
#define intrdisable(i, f, a, b, n)	irqdisable((i), (f), (a), (n))
#define intrenable(i, f, a, b, n)	irqenable((i), (f), (a), (n))
void	intrshutdown(void);
void	intrsoff(void);
int	irqdisable(uint, void (*)(Ureg*, void*), void*, char*);
int	irqenable(uint, void (*)(Ureg*, void*), void*, char*);
void	irqreassign(void);
int	isaconfig(char*, int, ISAConf*);
int	iscpureset(uint cpu);
int	isdmadone(int);
void	kbdenable(void);
void	kexit(Ureg*);
void	l2pl310init(void);
void	l2pl310off(void);
void	l2pl310sync(void);
void	machoff(uint cpu);
void	machon(uint cpu);
void	mmudisable(void);
void	mmudoublemap(void);
void	mmudump(PTE *);
void	mmuenable(void);
void	mmuinvalidateaddr(u32int);		/* 'mmu' or 'tlb'? */
void	mmuinvalidate(void);			/* 'mmu' or 'tlb'? */
void	mmul1copy(void *dest, void *src);
void	mmuon(uintptr);
void	mmusetnewl1(Mach *mm, PTE *l1);
void	mmuzerouser(void);
void	mousectl(Cmdbuf *cb);
void	notcpu0(void);
ulong	pcibarsize(Pcidev*, int);
void	pcibussize(Pcidev*, ulong*, ulong*);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pcicfgw8(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
void	pciclrioe(Pcidev*);
void	pciclrmwi(Pcidev*);
void	pcieintrdone(void);
int	pcigetpms(Pcidev*);
void	pcihinv(Pcidev*);
uchar	pciipin(Pcidev*, uchar);
Pcidev*	pcimatch(Pcidev*, int, int);
Pcidev*	pcimatchtbdf(int);
void	pcireset(void);
void	pcisetbme(Pcidev*);
void	pcisetioe(Pcidev*);
void	pcisetmwi(Pcidev*);
int	pcisetpms(Pcidev*, int);
void	periphreset(void);
u32int	pidget(void);
void	pidput(u32int);
void	poweron(void);
void	prcachecfg(void);
vlong	probeaddr(uintptr);
void	procrestore(Proc *);
void	procsave(Proc*);
void	procsetup(Proc*);
void	prstkuse(void);
void	putauxctl(ulong);
void	putpsr(u32int);
void	warpregs(ulong);
void	resetcpu(uint);
void	_reset(void);
void	restartwatchdog(void);
void	screenclockson(void);
void	screeninit(void);
void	scuinval(void);
void	scuoff(void);
void	scuon(void);
#define sdmalloc(n)	mallocalign((n), CACHELINESZ, 0, 0)
void	setcachelvl(int);
void	setr13(int, u32int*);
void	setsp(uintptr);
void	sgienable(void);
void	signall1ptstable(void);
ulong	smpcoheroff(void);
ulong	smpcoheron(void);
int	startcpu(uint);
void	stopcpu(uint);
int	tas(void *);
void	tegclock0init(void);
void	tegclockinit(void);
void	tegclockintr(void);
void	tegclockshutdown(void);
void	tegwdogintr(Ureg *, void *);
void	tellscuup(void);
u32int	ttbget(void);
void	ttbput(u32int);
int	_uartprint(char*, ...);
void	_uartputs(char*, int);
void	unclamp(void);
int	userureg(Ureg*);
void	vectors(void);
int	vfyintrs(void);
void*	vmap(uintptr, usize);
void	_vrst(void);
void	vunmap(void*, usize);
void	wakewfi(void);
void	watchdogoff(void);
void	_wfiloop(void);
void	wfi(void);
void	zerolow(void);

/*
 * Things called in main.
 */
void	archconfinit(void);
void	clockinit(void);
int	i8250console(void);
void	links(void);
void	mmuinit(void);
void	mmuinit0(void);
void	touser(uintptr);
void	trapinit(void);

int	fpiarm(Ureg*, Fpinst*);
int	fpudevprocio(Proc*, void*, long, uintptr, int);
void	fpuinit(void);
void	fpunoted(void);
void	fpunotify(Ureg*);
void	fpuprocrestore(Proc*);
void	fpuprocsave(Proc*);
void	fpusysprocsetup(Proc*);
void	fpusysrfork(Ureg*);
void	fpusysrforkchild(Proc*, Ureg*, Proc*);
int	fpuemu(Ureg*);

/*
 * Miscellaneous machine dependent stuff.
 */
int	cas(int *, int, int);
char*	getenv(char*, char*, int);
char*	getconf(char*);
uintptr mmukmap(uintptr, uintptr, usize);
uintptr mmukunmap(uintptr, uintptr, usize);
void*	mmuuncache(void*, usize);
void*	ucalloc(usize);
Block*	ucallocb(int);
void*	ucallocalign(usize size, int align, int span);
void	ucfree(void*);
void	ucfreeb(Block*);

/*
 * Things called from port.
 */
void	delay(int);
void	idlehands(void);
int	islo(void);
void	microdelay(int);
void	setkernur(Ureg*, Proc*);		/* only devproc.c */
void	syscallfmt(int syscallno, ulong pc, va_list list);
void*	sysexecregs(uintptr, ulong, int);
void	sysprocsetup(Proc*);
void	sysretfmt(int syscallno, va_list list, long ret, uvlong start, uvlong stop);
void	validalign(ulong, ulong);

/* libc */
long	labs(long);

#define	getpgcolor(a)	0
#define	kmapinval()

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))

/* these are only guaranteed to work on DRAM addresses */
#define KADDR(pa)	(void *) (KZERO    | ((uintptr)(pa) & ~KSEGM))
#define PADDR(va)	(uintptr)(PHYSDRAM | ((uintptr)(va) & ~KSEGM))
#define DRAMADDR(a) (inkzeroseg()? (uintptr)KADDR(a): PADDR(a))
