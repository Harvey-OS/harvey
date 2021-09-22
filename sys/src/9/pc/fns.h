#include "../port/portfns.h"

int	aadd(int *, int);
void	aamloop(int);
void	acpiscan(void (*func)(uchar *));
void	addabus(int type, int bno);
Dirtab*	addarchfile(char*, int, long(*)(Chan*,void*,long,vlong), long(*)(Chan*,void*,long,vlong));
void	apueccon(void);
void	archinit(void);
void	archreset(void);
void	archrevert(void);
#ifdef DREGS					/* BIOS32 */
int	bios32call(BIOS32ci*, u16int[3]);
int	bios32ci(BIOS32si*, BIOS32ci*);
void	bios32close(BIOS32si*);
BIOS32si* bios32open(char*);
#endif
uchar*	bootargs(void*);
int	bsr(Clzuint);
ulong	cankaddr(ulong);
void	cgapost(int);
void	clockintr(Ureg*, void*);
#define cmpswap cmpswap486
int	cmpswap486(long*, long, long);
void	(*coherence)(void);
void	cpuid(int, ulong regs[]);
int	cpuidentify(void);
void	cpuidprint(void);
#define cpuisa386() 0		/* couldn't buy a 386 now if you wanted to */
// #define cpuisa386() (X86FAMILY(conf.cpuidax) == 3)
#define cpuispost486() 1	/* Pentium arrived in 1993 */
// #define cpuispost486() (X86FAMILY(conf.cpuidax) >= 5)
void	(*cycles)(uvlong*);
int	dmacount(int);
int	dmadone(int);
void	dmaend(int);
int	dmainit(int, int);
long	dmasetup(int, void*, long, int);
int	etherfmt(Fmt* fmt);
void	fpclear(void);
void	fpenv(FPsave*);
void	fpinit(void);
void	fpoff(void);
void	fpon(void);
void	(*fprestore)(FPsave*);
void	(*fpsave)(FPsave*);
void	fpsavealloc(void);
void	fpsserestore(FPsave*);
void	fpsserestore0(FPsave*);
void	fpssesave(FPsave*);
void	fpssesave0(FPsave*);
ulong	fpstatus(void);
void	fpx87restore(FPsave*);
void	fpx87save(FPsave*);
ulong	getcr0(void);
ulong	getcr2(void);
ulong	getcr3(void);
ulong	getcr4(void);
char*	getconf(char*);
void	guesscpuhz(int);
void	halt(void);
int	i8042auxcmd(int);
int	i8042auxcmds(uchar*, int);
void	i8042auxenable(void (*)(int, int));
void	i8042reset(void);
void	i8250console(void);
void	i8250config(char *);
void*	i8250alloc(int, int, int);
void	i8250mouse(char*, int (*)(Queue*, int), int);
void	i8250setmouseputc(char*, int (*)(Queue*, int));
void	i8253enable(void);
void	i8253init(void);
void	i8253link(void);
uvlong	i8253read(uvlong*);
void	i8253timerset(uvlong);
int	i8259disable(int);
int	i8259enable(Vctl*);
void	i8259init(void);
int	i8259isr(int);
void	i8259on(void);
void	i8259off(void);
int	i8259vecno(int);
void	idle(void);
void	(*idlehands)(void);
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
int	intrdisable(int, int (*)(Ureg *, void *), void*, int, char*);
int	intrenable(int, int (*)(Ureg*, void*), void*, int, char*);
void	introff(void);
void	intron(void);
void	invlpg(ulong);
void	iofree(int);
void	ioinit(void);
int	iounused(int, int);
int	ioalloc(int, int, int, char*);
int	ioreserve(int, int, int, char*);
int	iprint(char*, ...);
int	isa20on(int);
int	isaconfig(char*, int, ISAConf*);
void*	kaddr(ulong);
void	kbdenable(void);
void	kbdinit(void);
#define	kmapinval()
uint	lapicid(uintptr);
void	lgdt(ushort[3]);
void	lidt(ushort[3]);
void	links(void);
void	ltr(ulong);
void	mach0init(void);
void	mathinit(void);
void	mb386(void);
void	mb586(void);
void	meminit(void);
void	memorysummary(void);
void	mfence(void);
void	mkmultiboot(void);
#define mmuflushtlb(pdb) putcr3(pdb)
void	mmuinit(void);
ulong*	mmuwalk(ulong*, ulong, int, int);
void	monitor(void* address, ulong extensions, ulong hints);
void	mpresetothers(void);
int	mtrr(uvlong, uvlong, char *);
void	mtrrclock(void);
int	mtrrprint(char *, long);
void	mwait(ulong extensions, ulong hints);
uchar	nvramread(int);
void	nvramwrite(int, uchar);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
ulong	paddr(void*);
#define pause()
void	pciaddbuses(Pcidev*);
ulong	pcibarsize(Pcidev*, int);
void	pcibussize(Pcidev*, ulong*, ulong*);
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
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
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
int	pciusebios(void);		/* in optional pcibios.c */
int	pdbmap(ulong*, ulong, ulong, int);
void	procfpusave(Proc *p);
void	procrestore(Proc*);
void	procsave(Proc*);
void	procsetup(Proc*);
void	putcr0(ulong);
void	putcr3(ulong);
void	putcr4(ulong);
void*	rampage(void);
void	rdmsr(int, vlong*);
void	readlsconf(uintptr e820);
void	realmode(Ureg*);
void	runoncpu(int cpu);
void	screeninit(void);
void	(*screenputs)(char*, int);
#define sdfree(p)	free(p)
/* force page alignment for NVME buffers at least */
enum { Sdalign = 8*KB };
#define sdmalloc(n)	mallocalign((n), Sdalign, 0, 0)
void*	sigsearch(char*);
void	syncclock(void);
void	syscallfmt(int syscallno, ulong pc, va_list list);
void	sysretfmt(int syscallno, va_list list, long ret, uvlong start, uvlong stop);
void*	tmpmap(Page*);
void	tmpunmap(void*);
void	touser(void*);
void	trapenable(int, int (*)(Ureg*, void*), void*, char*);
void	trapinit(void);
void	trapinit0(void);
int	tas(void*);
uvlong	tscticks(uvlong*);
void	trim(char *s, int len);
ulong	umbmalloc(ulong, int, int);
void	umbfree(ulong, int);
ulong	umbrwmalloc(ulong, int, int);
void	umbrwfree(ulong, int);
ulong	upaalloc(int, int);
void	upafree(ulong, int);
void	upareserve(ulong, int);
#define	userureg(ur) (((ur)->cs & 0xFFFF) == UESEL)
#define validalign(a, b)
void	vectortable(void);
void*	vmap(ulong, int);
int	vmapsync(ulong);
void	vmbotch(ulong, char *);
void	vunmap(void*, int);
void	wbinvd(void);
void	wrmsr(int, vlong);
int	xchgw(ushort*, int);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define	KADDR(a)	kaddr(a)
#define PADDR(a)	paddr((void*)(a))

#define	dcflush(a, b)

#define BIOSSEG(a)	KADDR(((uint)(a))<<4)	/* 1st mb address as segment */

#define L16GET(p)	(((p)[1]<<8)|(p)[0])
#define L32GET(p)	(((uint)L16GET((p)+2)<<16)|L16GET(p))
