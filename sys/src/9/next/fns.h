#include "../port/portfns.h"

void	Xdelay(int);
void	addportintr(int (*)(void));
int	atoi(char*);
void	bootargs(ulong);
#define	clearmmucache()
void	clock(Ureg*);
void	crankfifo(int);
void	etherdmarintr(void);
void	etherdmaxintr(void);
void	etherrintr(void);
void	etherxintr(void);
#define	evenaddr(x)	/* 68040 doesn't care */
void	fault68040(Ureg*, FFrame*);
void	floppyintr(void);
void	flushatcpage(ulong);
void	flushatc(void);
void	flushcpucache(void);
void	flushpage(ulong);
int	fpcr(int);
void	fpregrestore(char*);
void	fpregsave(char*);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
int	fptrap(Ureg*);
ulong	getsr(void);
int	getcc(int);
void	incontoggle(void);
void	initmmureg(ulong*);
void	ioinit(void);
void	kbdmouseintr(void);
int	kernaddr(ulong);
ulong	kmappa(ulong);
ulong	kmappba(ulong);
void	kproftimer(ulong);
void	mmuinit(void);
#define	mmunewpage(x)
int	mmusr(ulong, int);
void	mousebuttons(int);
void	mousedelta(int, int, int);
long	muldiv(long, long, long);
int	portprobe(char*, int, int, int, long);
void	poweroff(void);
void	procrestore(Proc*);
void	procsave(Proc*);
void	procsetup(Proc*);
void	putcc(int, int);
void	putstr(char*);
void	putstrn(char*, long);
void	restfpregs(FPsave*);
long	rtctime(void);
void	screeninit(void);
void	screenputc(int);
void	screenputs(char*, int);
void	scsiintr(int);
uchar*	scsirecv(uchar *);
void	scsirun(void);
long	syscall(Ureg*);
void	audiodmaintr(void);
int	spl1(void);
void	spldone(void);
ulong	systimer(void);
int	tas(char*);
void	timerinit(void);
void	touser(void*);
void	trapinit(void);
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define wbflush()
#define getcallerpc(x)	(*(ulong*)(x))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
