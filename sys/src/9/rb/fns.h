#include "../port/portfns.h"

void	arginit(void);
int	busprobe(ulong);
ulong	cankaddr(ulong);
void	cleancache(void);
void	clearmmucache(void);
void	clock(Ureg*);
void	clockinit(void);
void	clockshutdown(void);
int	cmpswap(long*, long, long);
void	coherence(void);
void	cycles(uvlong *);
void	dcflush(void*, ulong);
void	evenaddr(ulong);
void	faultmips(Ureg*, int, int);
ulong	fcr31(void);
void	firmware(int);
void	fpclear(void);
void	fpsave(FPsave *);
void	fptrap(Ureg*);
int	fpuemu(Ureg *);
void	fpwatch(Ureg *);
ulong	getcause(void);
char*	getconf(char*);
ulong	getconfig(void);
ulong	getconfig1(void);
ulong	getconfig2(void);
ulong	getconfig3(void);
ulong	getconfig7(void);
ulong	gethwreg3(void);
ulong	getpagemask(void);
ulong	getstatus(void);
int	gettlbp(ulong, ulong*);
ulong	gettlbvirt(int);
void	gotopc(ulong);
void	hinv(void);
int	i8250console(void);
void	icflush(void *, ulong);
void	idle(void);
void	idlehands(void);
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
void	ioinit(void);
void	introff(int);
void	intron(int);
void	intrshutdown(void);
void	kfault(Ureg*);
KMap*	kmap(Page*);
void	kmapinit(void);
void	kmapinval(void);
void	kunmap(KMap*);
void	launchinit(void);
void	launch(int);
void	links(void);
ulong	machstatus(void);
void	newstart(void);
int	newtlbpid(Proc*);
void	online(void);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
ulong	pcibarsize(Pcidev*, int);
void	pcibussize(Pcidev*, ulong*, ulong*);
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
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
void	pcireset(void);
int	pciscan(int, Pcidev**);
void	pcisetbme(Pcidev*);
void	pcisetioe(Pcidev*);
void	pcisetmwi(Pcidev*);
int	pcisetpms(Pcidev*, int);
ulong	prid(void);
void	procrestore(Proc *);
void	procsave(Proc *);
#define	procsetup(p)	((p)->fpstate = FPinit)
void	purgetlb(int);
Softtlb*	putstlb(ulong, ulong);
int	puttlb(ulong, ulong, ulong);
void	puttlbx(int, ulong, ulong, ulong, int);
ulong	rdcompare(void);
ulong	rdcount(void);
ulong*	reg(Ureg*, int);
void	restfpregs(FPsave*, ulong);
void	intrenable(int, void(*)(void *), void *);
void	setleveldest(int, int, uvlong*);
void	setpagemask(ulong);
void	setsp(ulong);
void	setstatus(ulong);
void	setwatchhi0(ulong);
void	setwatchlo0(ulong);
void	setwired(ulong);
void	sicwdog(void);
void	silencewdog(void);
ulong	stlbhash(ulong);
void	stopwdog(void);
void	syncclock(void);
long	syscall(Ureg*);
void	syscallfmt(int syscallno, ulong pc, va_list list);
void	sysretfmt(int syscallno, va_list list, long ret, uvlong start, uvlong stop);
int	tas(ulong*);
void	tlbinit(void);
ulong	tlbvirt(void);
void	touser(uintptr);
void	unleash(void);
#define	userureg(ur) ((ur)->status & KUSER)
void	vecinit(void);
void	vector0(void);
void	vector100(void);
void	vector180(void);
void	wdogreset(void);
void	wrcompare(ulong);
void	wrcount(ulong);
ulong	wiredpte(vlong);
void	machwire(void);
void	_uartputs(char*, int);
int	_uartprint(char*, ...);

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))

#define KADDR(a)	((void*)((ulong)(a)|KSEG0))
#define PADDR(a)	((ulong)(a)&~KSEGM)

#define KSEG1ADDR(a)	((void*)((ulong)(a)|KSEG1))
