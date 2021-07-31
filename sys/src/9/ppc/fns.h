#include "../port/portfns.h"

ulong	cankaddr(ulong);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	clockinit(void);
void	clockintr(Ureg*);
int	cmpswap(long*, long, long);
void	cpuidprint(void);
void	cycles(uvlong*);
void	dbgputc(int c);
void	dcacheenb(void);
void	dcflush(void*, ulong);
void	dczap(void*, ulong);
void	delay(int);
void	delayloopinit(void);
void	dmiss(void);
void	dumpregs(Ureg*);
void	eieio(void);
void	evenaddr(ulong);
void	faultpower(Ureg*, ulong addr, int read);
void	flashprogpower(int);
void	fpgareset(void);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
void	fptrap(Ureg*);
char*	getconf(char*);
ulong	getdar(void);
ulong	getdcmp(void);
ulong	getdec(void);
ulong	getdmiss(void);
ulong	getdsisr(void);
ulong	gethash1(void);
ulong	gethash2(void);
ulong	gethid0(void);
ulong	gethid1(void);
ulong	geticmp(void);
ulong	geticmp(void);
ulong	getimiss(void);
ulong	getmsr(void);
ulong	getpvr(void);
ulong	getrpa(void);
ulong	getsdr1(void);
ulong	getsr(int);
ulong	getsrr1(void);
void	gotopc(ulong);
void	hwintrinit(void);
void	icacheenb(void);
void	icflush(void*, ulong);
void	idle(void);
#define	idlehands()		/* nothing to do in the runproc */
void	imiss(void);
int	inb(int);
void	intr(Ureg*);
void	intrenable(int, void (*)(Ureg*, void*), void*, char*);
void	intrdisable(int, void (*)(Ureg*, void*), void*, char*);
int	ioalloc(int, int, int, char*);
void	iofree(int);
int	iprint(char*, ...);
int	isaconfig(char*, int, ISAConf*);
void	kfpinit(void);
#define	kmapinval()
void	links(void);
void	vectordisable(Vctl *);
int	vectorenable(Vctl *);
void	intack(void);
void	intend(int);
int	intvec(void);
void	l2disable(void);
void	mmuinit(void);
void	mmusweep(void*);
int	newmmupid(void);
void	outb(int, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pcicfgw8(Pcidev*, int, int);
void	procsave(Proc*);
void	procsetup(Proc*);
void	putdcmp(ulong);
void	putdec(ulong);
void	puthash1(ulong);
void	puthash2(ulong);
void	puthid0(ulong);
void	puthid2(ulong);
void	puticmp(ulong);
void	puticmp(ulong);
void	putmsr(ulong);
void	putrpa(ulong);
void	putsdr1(ulong);
void	putsdr1(ulong);
void	putsr(int, ulong);
void	putsrr1(ulong);
void	sethvec(int, void (*)(void));
void	setmvec(int, void (*)(void), void (*)(void));
void	sharedseginit(void);
void	console(void);
void	sync(void);
int	tas(void*);
void	timeradd(Timer *);
void	timerdel(Timer *);
void	timerinit(void);
void	tlbflush(ulong);
void	tlbflushall(void);
void	tlbld(ulong);
void	tlbli(ulong);
void	tlbvec(void);
void	touser(void*);
void	trapinit(void);
void	trapvec(void);
#define	userureg(ur) (((ur)->status & MSR_PR) != 0)
#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((((ulong)(a)&0xf0000000)==0xf0000000)?(ulong)(a):((ulong)(a)&~KZERO))
#define coherence()	eieio()
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
void	procrestore(Proc*);

#ifdef ucuconf
extern ulong getpll(void);
extern ulong getl2cr(void);
extern void putl2cr(ulong);
#endif
