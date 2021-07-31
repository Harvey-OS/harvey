#include "../port/portfns.h"

void	archinit(void);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	clockcheck(void);
void	clockinit(void);
void	clockintr(Ureg*);
void	clockintrsched(void);
#define coherence()	eieio()
void	timeradd(Timer *);
void	timerdel(Timer *);
void	cpuidentify(void);
void	cpuidprint(void);
void	dcflush(void*, ulong);
void	delay(int);
void	dumpregs(Ureg*);
void	delayloopinit(void);
void	eieio(void);
void	evenaddr(ulong);
void	faultpower(Ureg*, ulong addr, int read);
void	fpinit(void);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
void	fptrap(Ureg*);
char*	getconf(char*);
ulong	getdar(void);
ulong	getdec(void);
ulong	getdepn(void);
ulong	getdsisr(void);
ulong	gethid0(void);
ulong	gethid1(void);
ulong	getimmr(void);
ulong	getmsr(void);
ulong	getpvr(void);
ulong	gettbl(void);
ulong	gettbu(void);
void	gotopc(ulong);
int	havetimer(void);
void	hwintrinit(void);
void	i8250console(void);
void	i8259init(void);
int	i8259intack(void);
int	i8259enable(Vctl*);
int	i8259vecno(int);
int	i8259disable(int);
void	icflush(void*, ulong);
void	idle(void);
#define	idlehands()			/* nothing to do in the runproc */
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
void	intr(Ureg*);
void	intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
int	ioalloc(int, int, int, char*);
void	iofree(int);
void	ioinit(void);
int	iprint(char*, ...);
int	isaconfig(char*, int, ISAConf*);
void	kbdinit(void);
void	kbdreset(void);
void	kernelmmu(void);
#define	kmapinval()
void	links(void);
void	mathinit(void);
void	mmuinit(void);
void	mmusweep(void*);
void	mpicdisable(int);
void	mpicenable(int, Vctl*);
int	mpiceoi(int);
int	mpicintack(void);
int	newmmupid(void);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
int	pciscan(int, Pcidev **);
ulong pcibarsize(Pcidev *, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void pciclrbme(Pcidev*);
void	pcihinv(Pcidev*);
uchar pciipin(Pcidev *, uchar);
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
void	pcireset(void);
void	pcisetbme(Pcidev*);
#define procrestore(p)
void	procsave(Proc*);
void	procsetup(Proc*);
void	putdec(ulong);
void	puthid0(ulong);
void	puthid1(ulong);
void	putmsr(ulong);
void	putsdr1(ulong);
void	putsr(int, ulong);
void	raveninit(void);
void	screeninit(void);
int	screenprint(char*, ...);			/* debugging */
int	segflush(void*, ulong);
void	sync(void);
int	tas(void*);
void	touser(void*);
void	trapinit(void);
void	trapvec(void);
void	tlbflush(ulong);
void	tlbflushall(void);
void	uartinstall(void);
void	uartwait(void);	/* debugging */
#define	userureg(ur) (((ur)->status & MSR_PR) != 0)
void	watchreset(void);
void	wbflush(void);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
