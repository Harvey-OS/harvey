#include "../port/portfns.h"

void	DEBUG(void);

void	arginit(void);
int	busprobe(ulong);
void	cdirtyexcl(void*, ulong);
void	cleancache(void);
void	clearmmucache(void);
void	clock(Ureg*);
void	clockinit(void);
ulong	confeval(char*);
void	confprint(void);
void	confread(void);
void	confset(char*);
int	conschar(void);
void	consoff(void);
int	consputc(int);
void	dcflush(void*, ulong);
void	evenaddr(ulong);
void	faultmips(Ureg*, int, int);
ulong	fcr31(void);
void	firmware(int);
#define	flushpage(s)	icflush((void*)(s), BY2PG)
void	fptrap(Ureg*);
ulong	getcallerpc(void*);
void	getnveaddr(void*);
int	getnvram(ulong, void *, int);
ulong	getstatus(void);
void	gettlb(int, ulong*);
int	gettlbp(ulong, ulong*);
ulong	gettlbvirt(int);
void	gotopc(ulong);
void	icdirty(void *, ulong);
void	icflush(void *, ulong);
void	intr(Ureg*);
void	ioinit(void);
KMap*	kmap(Page*);
void	kmapinit(void);
int	kprint(char*, ...);
void	kproftimer(ulong);
void	kunmap(KMap*);
void	l2intr(Ureg*, void*);
void	launchinit(void);
void	launch(int);
void	lightbits(int, int);
void	lptintr(void);
void	mmunewpage(Page*);
void	mntdump(void);
void	newstart(void);
int	newtlbpid(Proc*);
void	nonettoggle(void);
void	novme(int);
void	online(void);
Block*	prepend(Block*, int);
void	prflush(void);
ulong	prid(void);
void	printinit(void);
#define	procrestore(p)
#define	procsave(p)
#define	procsetup(p)	((p)->fpstate = FPinit)
void	purgetlb(int);
int	putnvram(ulong, void*, int);
Softtlb*	putstlb(ulong, ulong);
void	puttlb(ulong, ulong, ulong);
void	puttlbx(int, ulong, ulong, ulong);
void*	pxalloc(ulong);
void*	pxspanalloc(ulong, int, ulong);
ulong	rdcount(void);
int	readlog(ulong, char*, ulong);
void	restfpregs(FPsave*, ulong);
#define	screenputs
void	screeninit(int);
void	scsiintr(void);
void	seqintr(Ureg*, void*);
void	setatvec(int, void (*)(int));
void	setled(int);
void	setvector(int, void (*)(Ureg*, void*), void*);
long	syscall(Ureg*);
void	syslog(char*, int);
void	sysloginit(void);
int	tas(ulong*);
void	tlbinit(void);
ulong	tlbvirt(void);
void	touser(void*);
void	vecinit(void);
void	vector0(void);
void	vector100(void);
void	vector180(void);
void	wbflush(void);
void	wrcompare(ulong);
void	Xdelay(int);

#define	waserror()	setlabel(&u->errlab[u->nerrlab++])
#define	kmapperm(x)	kmap(x)
#define KADDR(a)	((void*)((ulong)(a)|KSEG0))
#define KADDR1(a)	((void*)((ulong)(a)|KSEG1))
#define PADDR(a)	((ulong)(a)&~KSEGM)
