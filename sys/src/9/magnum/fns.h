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
void	dcflush(void *, int);
void	evenaddr(ulong);
void	faultmips(Ureg*, int, int);
ulong	fcr31(void);
void	firmware(int);
#define	flushpage(s)	icflush((void*)(s), BY2PG)
void	fptrap(Ureg*);
ulong	getcallerpc(void*);
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
void	lanceintr(void);
void	lanceparity(void);
void	lancesetup(Lance*);
void	lancetoggle(void);
void	launchinit(void);
void	launch(int);
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
void	putstlb(ulong, ulong);
void	puttlb(ulong, ulong);
void	puttlbx(int, ulong, ulong);
int	readlog(ulong, char*, ulong);
void	restfpregs(FPsave*, ulong);
void	screeninit(int);
void	screenputs(char*, int);
void	scsiintr(void);
void	setatvec(int, void (*)(int));
int	setsimmtype(int);
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
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
