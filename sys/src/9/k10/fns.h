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
Dirtab*	addarchfile(char*, int, long(*)(Chan*,void*,long,vlong), long(*)(Chan*,void*,long,vlong));
void	acmmuswitch(void);
void	acmodeset(int);
void	archfmtinstall(void);
void	archidle(void);
int	archmmu(void);
int	asmfree(uintmem, uintmem, int);
uvlong	asmalloc(uintmem, uintmem, int, int);
void	asminit(void);
void	asmmapinit(uintmem, uintmem, int);
extern void asmmodinit(u32int, u32int, char*);
void	noerrorsleft(void);
void	archinit(void);
void	archreset(void);
vlong	archhz(void);
void	cgaconsputs(char*, int);
void	cgainit(void);
void	cgapost(int);
void	checkpa(char*, uintmem);
#define	clearmmucache()				/* x86 doesn't have one */
void	(*coherence)(void);
int	corecolor(int);
u32int	cpuid(u32int, u32int, u32int[4]);
int	dbgprint(char*, ...);
int	decref(Ref*);
void	delay(int);
void	dumpmmu(Proc*);
void	dumpmmuwalk(u64int pa);
void	dumpptepg(int lvl,uintptr pa);
#define	evenaddr(x)				/* x86 doesn't care */
int	fpudevprocio(Proc*, void*, long, uintptr, int);
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
void	halt(void);
void	hardhalt(void);
int	i8042auxcmd(int);
int	i8042auxcmds(uchar*, int);
void	i8042auxenable(void (*)(int, int));
void	i8042reset(void);
Uart*	i8250console(char*);
void*	i8250alloc(int, int, int);
vlong	i8254hz(u32int[2][4]);
void	idlehands(void);
void	acidthandlers(void);
void	idthandlers(void);
int	inb(int);
int	incref(Ref*);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
uint32_t	inl(int);
void	insl(int, void*, int);
int	intrdisable(void*);
void*	intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
void	invlpg(uintptr);
void	iofree(int);
void	ioinit(void);
int	iounused(int, int);
int	ioalloc(int, int, int, char*);
int	ioreserve(int, int, int, char*);
int	iprint(char*, ...);
int	isaconfig(char*, int, ISAConf*);
void	kbdenable(void);
void	kbdinit(void);
void	kexit(Ureg*);
#define	kmapinval()
void	lfence(void);
void	links(void);
void	machinit(void);
void	mach0init(void);
void	mapraminit(uvlong, uvlong);
void	mapupainit(uvlong, uint32_t);
void	meminit(void);
void	mfence(void);
void	mmuflushtlb(u64int);
void	mmuinit(void);
uintptr	mmukmap(uintptr, uintptr, usize);
int	mmukmapsync(uvlong);
uintmem	mmuphysaddr(uintptr);
int	mmuwalk(PTE*, uintptr, int, PTE**, PTE (*)(usize));
int	multiboot(u32int, u32int, int);
void	ndnr(void);
uchar	nvramread(int);
void	nvramwrite(int, uchar);
void	optionsinit(char*);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
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
u64int	splhi(void);
u64int	spllo(void);
void	splx(u64int);
void	splxpc(u64int);
void	stopac(void);
void	syncclock(void);
void	syscall(int scallnr, Ureg* ureg);
void*	sysexecregs(uintptr, uint32_t, uint32_t);
uintptr	sysexecstack(uintptr, int);
void	sysprocsetup(Proc*);
void	tssrsp0(u64int);
void	trapenable(int, void (*)(Ureg*, void*), void*, char*);
void	trapinit(void);
void	trap(Ureg*);
void	umeminit(void);
void	upafree(uintptr, usize);
uintptr	upamalloc(uintptr, usize, usize);
void	upareserve(uintptr, usize);
int	userureg(Ureg*);
void*	vmap(uintptr, usize);
void	vsvminit(int, int);
void	vunmap(void*, usize);

extern u64int cr0get(void);
extern void cr0put(u64int);
extern u64int cr2get(void);
extern u64int cr3get(void);
extern void cr3put(u64int);
extern u64int cr4get(void);
extern void cr4put(u64int);
extern void gdtget(void*);
extern void gdtput(int, u64int, u16int);
extern void idtput(int, u64int);
extern u64int rdmsr(u32int);
extern u64int rdtsc(void);
extern void trput(u64int);
extern void wrmsr(u32int, u64int);

extern int islo(void);
extern void spldone(void);
extern Mpl splhi(void);
extern Mpl spllo(void);
extern void splx(Mpl);

int	cas32(void*, u32int, u32int);
int	cas64(void*, u64int, u64int);
int	tas32(void*);
u64int	fas64(u64int*, u64int);

#define CASU(p, e, n)	cas64((p), (u64int)(e), (u64int)(n))
#define CASV(p, e, n)	cas64((p), (u64int)(e), (u64int)(n))
#define CASP(p, e, n)	cas64((p), (u64int)(e), (u64int)(n))
#define CASW(p, e, n)	cas32((p), (e), (n))
#define TAS(addr)	tas32((addr))
#define	FASP(p, v)	((void*)fas64((u64int*)(p), (u64int)(v)))

void	touser(uintptr);
void	syscallentry(void);
void	acsyscallentry(void);
void	syscallreturn(void);
void	sysrforkret(void);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))

#define	dcflush(a, b)

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

void*	KADDR(uintptr);
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
extern void ioapicintrinit(int, int, int, int, u32int);
extern void ioapiconline(void);

/*
 * archk10.c
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
