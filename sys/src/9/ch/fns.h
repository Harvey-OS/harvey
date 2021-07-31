#include "../port/portfns.h"

void	addclock0link(void (*)(void));
void	arginit(void);
int	busprobe(ulong);
void	cleancache(void);
void	clearmmucache(void);
void	clock(Ureg*);
void	clockinit(void);
#define coherence()
void	dcflush(void*, ulong);
void	epcenable(ulong);
void	epcinit(int, int);
void	evenaddr(ulong);
void	faultmips(Ureg*, int, int);
ulong	fcr31(void);
void	firmware(int);
#define	flushpage(s)	icflush((void*)(s), BY2PG)
void	fptrap(Ureg*);
void	gettlb(int, ulong*);
int	gettlbp(ulong, ulong*);
ulong	gettlbvirt(int);
void	gotopc(ulong);
void	hinv(void);
void	icdirty(void *, ulong);
void	icflush(void *, ulong);
#define	idlehands()			/* nothing to do in the runproc */
void	intr(Ureg*);
void	ioinit(void);
int	iprint(char*, ...);
int	kbdinit(void);
int	kbdintr(void);
void	kfault(Ureg*);
KMap*	kmap(Page*);
void	kmapinit(void);
void	kmapinval(void);
void	kunmap(KMap*);
void	launchinit(void);
void	launch(int);
void	links(void);
ulong	kmapwired(ulong);
ulong	machstatus(void);
void	mmunewpage(Page*);
void	newstart(void);
int	newtlbpid(Proc*);
void	online(void);
ulong	prid(void);
#define	procrestore(p)
#define	procsave(p)
#define	procsetup(p)	((p)->fpstate = FPinit)
void	purgetlb(int);
Softtlb*	putstlb(ulong, ulong);
int	puttlb(ulong, ulong, ulong);
void	puttlbx(int, ulong, ulong, ulong, int);
ulong	rdcount(void);
long*	reg(Ureg*, int);
void	restfpregs(FPsave*, ulong);
#define screenputs(x, y)
void	sethandler(int, void(*)(void));
void	setleveldest(int, int, uvlong*);
long	syscall(Ureg*);
int	tas(ulong*);
void	tlbinit(void);
ulong	tlbvirt(void);
void	touser(void*);
void	updatefastclock(ulong);
ulong	uvmove(uvlong*, uvlong*);
void	vecinit(void);
void	vector0(void);
void	vector100(void);
void	vector180(void);
void	wbflush(void);
void	wrcompare(ulong);
void	wrcount(ulong);
void	z8530intr(int);
void	z8530setup(uchar*, uchar*, uchar*, uchar*, ulong, int);
void	z8530special(int, int, Queue**, Queue**, int (*)(Queue*, int));
ulong	wiredpte(vlong);
void	machwire(void);

#define	waserror()	setlabel(&up->errlab[up->nerrlab++])
#define KADDR(a)	((void*)((ulong)(a)|KSEG0))
#define KADDR1(a)	((void*)((ulong)(a)|KSEG1))
#define PADDR(a)	((ulong)(a)&~KSEGM)

void	uvld(ulong, uvlong*);
void	uvst(ulong, uvlong*);
