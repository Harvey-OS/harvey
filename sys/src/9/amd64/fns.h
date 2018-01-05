/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "../port/portfns.h"
void	intrac(Proc*);
void	acinit(void);
int	acpiinit(void);
void	actrapenable(int, char* (*)(Ureg*, void*), void*, char*);
void	apicipi(int);
void	apicpri(int);
void	acsysret(void);
void	actouser(void);
void		runacore(void);
void	aamloop(int);
Dirtab*	addarchfile(char*, int,
			   int32_t(*)(Chan*,void*,int32_t,int64_t),
			   int32_t(*)(Chan*,void*,int32_t,int64_t));
void	acmmuswitch(void);
void	acmodeset(int);
void	archfmtinstall(void);
void	archidle(void);
int	archmmu(void);
int	asmfree(uintmem, uintmem, int);
uint64_t	asmalloc(uintmem, uintmem, int, int);
void	asminit(void);
void	asmmapinit(uintmem, uintmem, int);
extern void asmmodinit(uint32_t, uint32_t, char*);
void	noerrorsleft(void);
void	archinit(void);
void	archreset(void);
int64_t	archhz(void);
int	cgaprint(int off, char *fmt, ...);
int	cgaclearln(int off, int c);
void	cgaconsputs(char*, int);
void	cgainit(void);
void	cgapost(int);
void	checkpa(char*, uintmem);
#define	clearmmucache()				/* x86 doesn't have one */
void	(*coherence)(void);
int	corecolor(int);
uint32_t	cpuid(uint32_t, uint32_t, uint32_t[4]);
int	dbgprint(char*, ...);
int	decref(Ref*);
void	delay(int);
void	dumpmmu(Proc*);
void	dumpmmuwalk(uint64_t pa);
void	dumpptepg(int lvl,uintptr_t pa);
#define	evenaddr(x)				/* x86 doesn't care */
int	fpudevprocio(Proc*, void*, int32_t, uintptr_t, int);
void	fpuinit(void);
void	fpunoted(void);
void	fpunotify(Ureg*);
void	fpuprocrestore(Proc*);
void	fpuprocsave(Proc*);
void	fpusysprocsetup(Proc*);
void	fpusysrfork(Ureg*);
void	fpusysrforkchild(Proc*, Proc*);
Mach*	getac(Proc*, int);
char*	getconf(char*);
void    gdb2ureg(uintptr_t *g, Ureg *u);
void	halt(void);
void	hardhalt(void);
void	mouseenable(void);
void	i8042systemreset(void);
int	mousecmd(int);
void	mouseenable(void);
void	i8250console(char*);
void*	i8250alloc(int, int, int);
int64_t	i8254hz(uint32_t *info0, uint32_t *info1);
void	idlehands(void);
void	acidthandlers(void);
void	idthandlers(void);
int	inb(int);
int	incref(Ref*);
void	insb(int, void*, int);
uint16_t	ins(int);
void	inss(int, void*, int);
uint32_t	inl(int);
void	insl(int, void*, int);
int	intrdisable(void*);
void*	intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
void	invlpg(uintptr_t);
void	iofree(int);
void	ioinit(void);
int	iounused(int, int);
int	ioalloc(int, int, int, char*);
int	ioreserve(int, int, int, char*);
int	iprint(char*, ...);
int	isaconfig(char*, int, ISAConf*);
void	kexit(Ureg*);
void	keybenable(void);
void	keybinit(void);
#define	kmapinval()
void	lfence(void);
void	links(void);
void	machinit(void);
void	mach0init(void);
void	mapraminit(uint64_t, uint64_t);
void	mapupainit(uint64_t, uint32_t);
void	meminit(void);
void	mfence(void);
void	mmuflushtlb(uint64_t);
void	mmuinit(void);
uintptr_t	mmukmap(uintptr_t, uintptr_t, usize);
int	mmukmapsync(uint64_t);
uintmem	mmuphysaddr(uintptr_t);
int	mmuwalk(PTE*, uintptr_t, int, PTE**, PTE (*)(usize));
int	multiboot(uint32_t, uint32_t, int);
void	ndnr(void);
unsigned char	nvramread(int);
void	nvramwrite(int, unsigned char);
void	optionsinit(char*);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, uint16_t);
void	outss(int, void*, int);
void	outl(int, uint32_t);
void	outsl(int, void*, int);
int	pcicap(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
void	pciclrmwi(Pcidev*);
int	pcigetpms(Pcidev*);
void	pcihinv(Pcidev*);
Pcidev*	pcimatch(Pcidev*, int, int);
Pcidev*	pcimatchtbdf(int);
void	pcireset(void);
void	pcisetbme(Pcidev*);
void	pcisetmwi(Pcidev*);
int	pcisetpms(Pcidev*, int);
int	pickcore(int, int);
void	printcpufreq(void);
void	putac(Mach*);
void	runapcore(int);
int	screenprint(char*, ...);			/* debugging */
void	sfence(void);
void	spldone(void);
uint64_t	splhi(void);
uint64_t	spllo(void);
void	splx(uint64_t);
void	splxpc(uint64_t);
void	kstackok(void); /* panic if kstack guards garbaged, works with and without externup */
Stackframe	*stackframe(void); /* l64v.S */
void	stacksnippet(void);
void	stopac(void);
void	syncclock(void);
void	syscall(unsigned int scallnr, Ureg *ureg);
void*	sysexecregs(uintptr_t, uint32_t, void*);
uintptr_t	sysexecstack(uintptr_t, int);
void	sysprocsetup(Proc*);
void	tssrsp0(Mach *, uint64_t);
void	trapenable(int, void (*)(Ureg*, void*), void*, char*);
void	trapinit(void);
void	trap(Ureg*);
void	umeminit(void);
void    ureg2gdb(Ureg *u, uintptr_t *g);
int	userureg(Ureg*);
void*	vmap(uintptr_t, usize);
void	vsvminit(int, int, Mach *);
void	vunmap(void*, usize);

extern uint64_t cr0get(void);
extern void cr0put(uint64_t);
extern uint64_t cr2get(void);
extern uint64_t cr3get(void);
extern void cr3put(uint64_t);
extern uint64_t cr4get(void);
extern void cr4put(uint64_t);
extern void gdtget(void*);
extern void gdtput(int, uint64_t, uint16_t);
extern void idtput(int, uint64_t);
extern uint64_t rdmsr(uint32_t);
extern uint64_t rdtsc(void);
extern void trput(uint64_t);
extern void wrmsr(uint32_t, uint64_t);

// TODO(aki): once we figure this out, these will go.
extern int infected_with_std(void);
extern void disinfect_std(void);

extern int islo(void);
extern void spldone(void);
extern Mpl splhi(void);
extern Mpl spllo(void);
extern void splx(Mpl);

int	cas32(void*, uint32_t, uint32_t);
int	cas64(void*, uint64_t, uint64_t);
int	tas32(void*);
uint64_t	fas64(uint64_t*, uint64_t);

#define CASU(p, e, n)	cas64((p), (uint64_t)(e), (uint64_t)(n))
#define CASV(p, e, n)	cas64((p), (uint64_t)(e), (uint64_t)(n))
#define CASP(p, e, n)	cas64((p), (uint64_t)(e), (uint64_t)(n))
#define CASW(p, e, n)	cas32((p), (e), (n))
#define TAS(addr)	tas32((addr))
#define	FASP(p, v)	((void*)fas64((uint64_t*)(p), (uint64_t)(v)))

void	touser(uintptr_t);
void	syscallentry(void);
void	acsyscallentry(void);
void	syscallreturn(void);
void	sysrforkret(void);

#define	waserror()	setlabel(&up->errlab[up->nerrlab++])
#define	poperror()	up->nerrlab--

#define	dcflush(a, b)

#define PTR2UINT(p)	((uintptr_t)(p))
#define UINT2PTR(i)	((void*)(i))

void*	KADDR(uintptr_t);
uintmem	PADDR(void*);

#define BIOSSEG(a)	KADDR(((uint)(a))<<4)

/*
 * apic.c
 */
extern int apiceoi(int);
extern void apicipi(int);
extern void apicinit(int, uintmem, int);
extern int apicisr(int);
extern int apiconline(void);
extern void apicpri(int);
extern void apicsipi(int, uintmem);

extern void ioapicinit(int, uintmem);
extern void ioapicintrinit(int, int, int, int, uint32_t);
extern void ioapiconline(void);

/*
 * archamd64.c
 */
extern void millidelay(int);
extern void k10mwait(void*);

/*
 * i8259.c
 */
extern int i8259init(int);
extern int i8259irqdisable(int);
extern int i8259irqenable(int);
extern int i8259isr(int);

/*
 * mp.c
 */
extern void mpsinit(int);

/*
 * sipi.c
 */
extern void sipi(void);

/*
 * debug
 */
void HERE(void);
void DONE(void);

/* all these go to 0x3f8 */
void hi(char *s);

Mach *machp(void);
Proc *externup(void);

/* temporary. */
void die(char *);
void dumpgpr(Ureg* ureg);

/* debug support. */
int backtrace_list(uintptr_t pc, uintptr_t fp, uintptr_t *pcs, size_t nr_slots);

/* horror */
static inline void __clobber_callee_regs(void)
{
	asm volatile ("" : : : "rbx", "r12", "r13", "r14", "r15");
}

int slim_setlabel(Label*) __attribute__((returns_twice));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#define setlabel(label) ({int err;                                                 \
                    __clobber_callee_regs();                               \
                    err = slim_setlabel(label);                                     \
                    err;})

#pragma GCC diagnostic pop
