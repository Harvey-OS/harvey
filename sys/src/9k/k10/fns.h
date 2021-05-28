#include "../port/portfns.h"

void	aamloop(int);
int	acpiinit(void);
Dirtab*	addarchfile(char*, int, long(*)(Chan*,void*,long,vlong), long(*)(Chan*,void*,long,vlong));
void	archfmtinstall(void);
void	archinit(void);
int	archmmu(void);
void	archreset(void);
vlong	archhz(void);
int	asmfree(uvlong, uvlong, int);
uvlong	asmalloc(uvlong, uvlong, int, int);
void	asminit(void);
void	asmmapinit(u64int, u64int, int);
void	asmmodinit(u32int, u32int, char*);
void	cgaconsputs(char*, int);
void	cgainit(void);
void	cgapost(int);
#define	clearmmucache()				/* x86 doesn't have one */
void	(*coherence)(void);
int	corecolor(int);
u32int	cpuid(u32int, u32int, u32int[4]);
int	dbgprint(char*, ...);
#define decref(r)	adec(&(r)->ref)
void	delay(int);
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
char*	getconf(char*);
void	halt(void);
int	i8042auxcmd(int);
int	i8042auxcmds(uchar*, int);
void	i8042auxenable(void (*)(int, int));
void	i8042reset(void);
Uart*	i8250console(char*);
void*	i8250alloc(int, int, int);
void	i8250mouse(char*, int (*)(Queue*, int), int);
void	i8250setmouseputc(char*, int (*)(Queue*, int));
vlong	i8254hz(u32int[2][4]);
void	idlehands(void);
void	idthandlers(void);
int	inb(int);
#define incref(r)	ainc(&(r)->ref)
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
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
void	mapupainit(uvlong, ulong);
int	memcolor(uintmem, uintmem*);
void	meminit(void);
void	mfence(void);
void	mmucachectl(Page*, uint);
void	mmuflushtlb(u64int);
void	mmuinit(void);
u64int	mmuphysaddr(uintptr);
int	mmuwalk(uintptr, int, PTE**, u64int (*)(usize));
int	multiboot(u32int, u32int, int);
void	ndnr(void);
uchar	nvramread(int);
void	nvramwrite(int, uchar);
void	optionsinit(char*);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
void	pause(void);
int	pciscan(int, Pcidev**);
ulong	pcibarsize(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
void	pciclrioe(Pcidev*);
void	pciclrmwi(Pcidev*);
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
void	(*pmcupdate)(void);
void	printcpufreq(void);
int	screenprint(char*, ...);			/* debugging */
void	sfence(void);
void	spldone(void);
u64int	splhi(void);
u64int	spllo(void);
void	splx(u64int);
void	splxpc(u64int);
void	syncclock(void);
void*	sysexecregs(uintptr, ulong, ulong);
uintptr	sysexecstack(uintptr, int);
void	sysprocsetup(Proc*);
void	tssrsp0(u64int);
void	trapenable(int, void (*)(Ureg*, void*), void*, char*);
void	trapinit(void);
int	userureg(Ureg*);
void	umeminit(void);
void*	vmap(uintmem, usize);
void	vsvminit(int);
void	vunmap(void*, usize);
int	(*waitfor)(int*, int);

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

int	cas32(void*, u32int, u32int);
int	cas64(void*, u64int, u64int);
int	tas32(void*);

#define CASU(p, e, n)	cas64((p), (u64int)(e), (u64int)(n))
#define CASV(p, e, n)	cas64((p), (u64int)(e), (u64int)(n))
#define CASW(p, e, n)	cas32((p), (e), (n))
#define TAS(addr)	tas32((addr))

void	touser(uintptr);
void	syscallentry(void);
void	syscallreturn(void);
void	sysrforkret(void);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))

#define	dcflush(a, b)

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

//#define KADDR(a)	UINT2PTR(kseg0+((uintptr)(a)))
void*	KADDR(uintptr);
//#define PADDR(a)	PTR2UINT(((uintptr)(a)) - kseg0)
uintptr	PADDR(void*);

#define BIOSSEG(a)	KADDR(((uint)(a))<<4)

/*
 * apic.c
 */
extern int apiceoi(int);
extern void apicinit(int, uintptr, int);
extern int apicisr(int);
extern int apiconline(void);
extern void apicsipi(int, uintptr);
extern void apictimerdisable(void);
extern void apictimerenable(void);
extern void apictimerintr(Ureg*, void*);
extern void apictprput(int);

extern void ioapicinit(int, uintmem);
extern void ioapicintrinit(int, int, int, int, int, u32int);
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
 * mpacpi.c
 */
extern void mpacpi(void);

/*
 * sipi.c
 */
extern void sipi(void);
