#include "../port/portfns.h"

int	aadd(int*, int);
void	aamloop(int);
//int	acpiinit(void);
Dirtab*	addarchfile(char*, int, long(*)(Chan*,void*,long,vlong), long(*)(Chan*,void*,long,vlong));
void	apicinitipi(int apicno);
void	apueccon(void);
void	archfmtinstall(void);
vlong	archhz(void);
void	archinit(void);
int	archmmu(void);
void	archreset(void);
uvlong	asmalloc(uvlong, uvlong, int, int);
int	asmfree(uvlong, uvlong, int);
void	asminit(void);
void	asmmapinit(u64int, u64int, int);
void	asmmodinit(u32int, u32int, char*);
int	bsr(Clzuint n);
void	cgaconsputs(char*, int);
void	cgapost(int);
void	(*coherence)(void);
#define CONSREGS()	uart0regs
int	corecolor(int);
u32int	cpuid(u32int, u32int, u32int[4]);
int	cpuidinfo(u32int eax, u32int ecx, u32int info[4]);
#define	cycles(t) (*(t) = rdtsc())
int	dbgprint(char*, ...);
int	decref(Ref *r);
//#define decref(r)	adec(&(r)->ref)
#define delay(ms) millidelay(ms)
int	etherfmt(Fmt* fmt);
#define	evenaddr(x)				/* x86 doesn't care */
void	fpsts2ureg(Ureg *ureg);
int	fpudevprocio(Proc*, void*, long, uintptr, int);
void	fpuinit(void);
void	fpunoted(void);
void	fpunotify(Ureg*);
void	fpuprocrestore(Proc*);
void	fpuprocsave(Proc*);
void	fpusysprocsetup(Proc*);
void	fpusysrfork(Ureg*);
void	fpusysrforkchild(Proc*, Proc*);
char*	getconf(char*);
void	halt(void);
#ifdef unused
int	i8042auxcmd(int);
int	i8042auxcmds(uchar*, int);
void	i8042auxenable(void (*)(int, int));
void	i8042reset(void);
#endif
void*	i8250alloc(int, int, int);
Uart*	i8250console(char*);
vlong	i8254hz(u32int[2][4]);
void	i8254set(int port, int hz);
void	idlehands(void);
void	idthandlers(void);
int	inb(int);
int	incref(Ref *r);
//#define incref(r)	ainc(&(r)->ref)
ulong	inl(int);
ushort	ins(int);
#ifdef SDATA
void	inss(int, void*, int);
#endif
int	intrdisable(void*);
void*	intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
void	invlpg(uintptr);
int	ioalloc(int, int, int, char*);
void	iofree(int);
void	ioinit(void);
int	iounused(int, int);
int	iprint(char*, ...);
int	isaconfig(char*, int, ISAConf*);
int	k10waitfor(int*, int);
void	kbdenable(void);
void	kbdinit(void);
void	kexit(Ureg*);
#define	kmapinval()
void	kmesginit(void);
void	lapicnmidisable(void);
void	lapicnmienable(void);
void	links(void);
int	memcolor(uintmem, uintmem*);
void	meminit(void);
void	mfence(void);
void	mmuflushtlb(u64int);
void	mmuidentitymap(void);
void	mmuinit(void);
u64int	mmuphysaddr(uintptr);
int	mmuwalk(uintptr, int, PTE**, u64int (*)(uintptr));
void	monitor(void* address, u32int extensions, u32int hints);
void	mpshutdown(void);
int	multiboot(u32int, u32int, int);
void	mwait(u32int extensions, u32int hints);
void	(*mwaitforlock)(Lock*, ulong);
void	ndnr(void);
void	nmienable(void);
void	nop(void);
int	notify(Ureg*);
uchar	nvramread(int);
void	nvramwrite(int, uchar);
void	mboptinit(char*);
void	outb(int, int);
void	outl(int, ulong);
void	outs(int, ushort);
#ifdef SDATA
void	outss(int, void*, int);
#endif
void	pause(void);
ulong	pcibarsize(Pcidev*, int);
void	pcicf9reset(void);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pcicfgw8(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
int	pciclrcfgbit(Pcidev *p, int reg, ulong bit, char *offmsg);
void	pciclrioe(Pcidev*);
void	pciclrmwi(Pcidev*);
int	pcigetmsi(Pcidev *p, Msi *msi);
int	pcigetmsixcap(Pcidev *p);
int	pcigetpciecap(Pcidev *p);
int	pcigetpms(Pcidev*);
void	pcihinv(Pcidev*);
void	pciintrs(Pcidev*);
uchar	pciipin(Pcidev*, uchar);
Pcidev*	pcimatch(Pcidev*, int, int);
Pcidev*	pcimatchtbdf(int);
void	pcimsioff(Vctl*, Pcidev*);
void	pcinointrs(Pcidev*);
void	pcireset(void);
int	pciscan(int, Pcidev**);
void	pcisetbme(Pcidev*);
int	pcisetcfgbit(Pcidev *p, int reg, ulong bit, char *onmsg);
void	pcisetioe(Pcidev*);
int	pcisetmsi(Pcidev *p, Msi *msi);
void	pcisetmwi(Pcidev*);
int	pcisetpms(Pcidev*, int);
/*
 *  performance measurement ticks.  must be low overhead.
 *  doesn't have to count over a second.
 */
#define	perfticks() ((ulong)rdtsc())
void	(*pmcupdate)(void);
void	popcpu0sipi(Sipireboot *, Sipi *, void *, void *, void *, int);
void	portmwaitforlock(Lock *, ulong);
void	prsnolock(char *s);
void	realmwaitforlock(Lock*, ulong);
void	runoncpu(int cpu);
uvlong	sampletimer(int tmrport, int *cntp);
void	screeninit(void);
int	screenprint(char*, ...);			/* debugging */
void	(*screenputs)(char*, int);
void	setsipihandler(uintmem);
void*	sigsearch(char* signature);
void*	sysexecregs(uintptr, ulong, ulong);
uintptr	sysexecstack(uintptr, int);
void	sysprocsetup(Proc*);
void	trapenable(int, void (*)(Ureg*, void*), void*, char*);
void	trapinit(void);
void	tssrsp0(u64int);
int	userureg(Ureg*);
void*	vmap(uintmem, uintptr);
void	vmbotch(ulong, char *);
void	vsvminit(int);
void	vunmap(void*, uintptr);
int	(*waitfor)(int*, int);
void	wbinvd(void);
void	writeconf(void);

extern Mreg cr0get(void);
extern void cr0put(Mreg);
extern Mreg cr2get(void);
extern Mreg cr3get(void);
extern void cr3put(Mreg);
extern Mreg cr4get(void);
extern void cr4put(Mreg);
extern void gdtget(void*);
extern void gdtput(int, u64int, u16int);
extern void idtput(int, u64int);
extern u64int rdmsr(u32int);
extern u64int rdtsc(void);
extern void trput(u64int);
extern void wrmsr(u32int, u64int);

extern int islo(void);
extern void spldone(void);
extern Mreg splhi(void);
extern Mreg spllo(void);
extern void splx(Mreg);

/* libc atomics */
int	_tas(int*);
int	cas(uint*, int, int);

#define CASW		cas
#define TAS		_tas			/* use the libc atomic */

void	touser(uintptr);
void	syscallentry(void);
void	syscallreturn(void);
void	sysrforkret(void);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

void*	KADDR(uintptr);
uintptr	PADDR(void*);

#define BIOSSEG(a)	KADDR(((uint)(a))<<4)

/*
 * apic.c
 */
extern int apiceoi(int);
extern void apicinit(int, uintptr, int);
extern int apicisr(int);
extern int apiconline(void);
extern void apicresetothers(void);
extern void apicsipi(int, uintptr);
extern void apictimerdisable(void);
extern void apictimerenable(void);
extern void apictimerintr(Ureg*, void*);
extern void apictprput(int);

extern void ioapicinit(int, uintmem);
extern void ioapicintrinit(int, int, int, int, u32int);
extern int  ioapicintrtbloff(uint vno);
extern void ioapiconline(void);

/*
 * archk10.c
 */
extern void millidelay(int);

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
extern void mpsinit(void);

/*
 * sipi.c
 */
extern void sipi(void);
