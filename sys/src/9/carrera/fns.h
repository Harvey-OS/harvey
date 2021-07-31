#include "../port/portfns.h"

void		addclock0link(void (*)(void));
void		arginit(void);
int		busprobe(ulong);
int		cistrcmp(char*, char*);
void		cleancache(void);
void		clearmmucache(void);
void		clock(Ureg*);
void		clockinit(void);
#define coherence()
void		dcflush(void*, ulong);
void		dcinvalidate(void*, ulong);
void		dmaend(int);
long		dmasetup(int, void*, long, int);
void		evenaddr(ulong);
void		faultmips(Ureg*, int, int);
ulong		fcr31(void);
void		firmware(int);
#define		flushpage(s)		icflush((void*)(s), BY2PG)
void		fptrap(Ureg*);
ulong		getstatus(void);
void		gettlb(int, ulong*);
int		gettlbp(ulong, ulong*);
ulong		gettlbvirt(int);
void		icflush(void *, ulong);
#define		idlehands()
void		intr(Ureg*);
void		ioinit(int);
int		iprint(char*, ...);
int		isaconfig(char*, int, ISAConf*);
int		kbdinit(void);
void		kbdintr(void);
void		kfault(Ureg*);
KMap*		kmap(Page*);
void		kmapinit(void);
void		kmapinval(void);
void		kunmap(KMap*);
void		links(void);
void		mmunewpage(Page*);
void		mouseintr(void);
int		newtlbpid(Proc*);
void		ns16552install(void);
#define		procrestore(p)
#define		procsave(p)
#define		procsetup(p)		((p)->fpstate = FPinit)
void		purgetlb(int);
Softtlb*	putstlb(ulong, ulong);
int		puttlb(ulong, ulong, ulong);
void		puttlbx(int, ulong, ulong, ulong, int);
void		rdbginit(void);
ulong		rdcompare(void);
ulong		rdcount(void);
ulong*		reg(Ureg*, int);
void		restfpregs(FPsave*, ulong);
void		screeninit(void);
void		screenputs(char*, int);
void		seteisadma(int, void(*)(void));
long		syscall(Ureg*);
int		tas(ulong*);
void		tlbinit(void);
ulong		tlbvirt(void);
void		touser(void*);
#define userureg(ur) ((ur)->status & KUSER)
void		vecinit(void);
void		vector0(void);
void		vector100(void);
void		vector180(void);
void		vmereset(void);
void		uartclock(void);
void		wbflush(void);
void		wrcompare(ulong);

void		serialinit(void);
void		ns16552special(int, int, Queue**, Queue**, int (*)(Queue*, int));
void		ns16552setup(ulong, ulong, char*, int);
void		ns16552intr(int);
void		etherintr(void);
void		iomapinit(void);
void		enetaddr(uchar*);

#define	waserror()		setlabel(&up->errlab[up->nerrlab++])
#define	kmapperm(x)		kmap(x)

#define KADDR(a)	((void*)((ulong)(a)|KSEG0))
#define KADDR1(a)	((void*)((ulong)(a)|KSEG1))
#define PADDR(a)	((ulong)(a)&~KSEGM)

void	audiosbintr(void);
void	audiodmaintr(void);
void	mpegintr(void);
