#include "../port/portfns.h"

void	DEBUG(void);

void	arginit(void);
void	clearmmucache(void);
void	clockinit(void);
ulong	confeval(char*);
void	confprint(void);
void	confread(void);
void	confset(char*);
int	conschar(void);
void	consoff(void);
int	consputc(int);
void	dcflush(void *, ulong);
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
ulong	gettlbvirt(int);
void	gotopc(ulong);
void	icflush(void *, ulong);
void	intr(Ureg*);
void	ioinit(void);
int	kbdinit(void);
int	kbdintr(void);
int	kprint(char*, ...);
void	kproftimer(ulong);
void	launchinit(void);
void	launch(int);
void	lightbits(int, int);
void	lptintr(void);
void	mntdump(void);
void	newstart(void);
int	newtlbpid(Proc*);
void	nonettoggle(void);
void	novme(int);
void	online(void);
Block*	prepend(Block*, int);
void	prflush(void);
void	printinit(void);
#define	procrestore(p)
#define	procsave(p)
#define	procsetup(p)	((p)->fpstate = FPinit)
void	purgetlb(int);
int	putnvram(ulong, void*, int);
void	putstlb(ulong, ulong);
void	puttlb(ulong, ulong);
void	puttlbx(int, ulong, ulong);
int	readlog(ulong, char*, ulong);
void	restfpregs(FPsave*, ulong);
void	screeninit(int);
void	screenputs(char*, int);
void	scsiintr(void);
void	seqintr(void);
void	setatvec(int, void (*)(int));
void	setled(int);
void	setvmevec(int, void (*)(int));
void	sinit(void);
uchar*	smap(int, uchar*);
void	sunmap(int, uchar*);
void	syslog(char*, int);
void	sysloginit(void);
int	tas(ulong*);
void	tlbinit(void);
void	touser(void*);
void	vecinit(void);
void	vector0(void);
void	vector80(void);
void	vmereset(void);
void	wbflush(void);
void	Xdelay(int);

#define	waserror()	setlabel(&u->errlab[u->nerrlab++])
#define	kmapperm(x)	kmap(x)
#define KADDR(a)	((void*)((ulong)(a)|KSEG0))
#define KADDR1(a)	((void*)((ulong)(a)|KSEG1))
#define PADDR(a)	((ulong)(a)&~KSEGM)
