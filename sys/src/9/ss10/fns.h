#include "../port/portfns.h"

void	bootargs(ulong);
void	cacheinit(void);
ulong	call(void*, ...);
void	clearftt(ulong);
#define	clearmmucache()
void	clock(Ureg*);
void	clockinit(void);
void	compile(void);
#define dcflush() putdflush(0, 0)
void	disabfp(void);
void	dmaend(int);
long	dmasetup(int, void*, long, int);
void	enabfp(void);
void	evenaddr(ulong);
char*	excname(ulong);
void	faultasync(Ureg*);
void	faultsparc(Ureg*);
int	floppycmd(void);
void	floppyeject(FDrive*);
int	floppyexec(char*, long, int);
void floppyintr(Ureg*);
void	floppyon(FDrive*);
void	floppyoff(FDrive*);
int	floppyresult(void);
void	floppysetdef(FDrive*);
void floppysetup0(FController*);
void floppysetup1(FController*);
void	flushcpucache(void);
#define	flushpage(pa)	icflush()
#define	flushtlb() putflushprobe(0x400, 0)
#define	flushtlbctx() putflushprobe(0x300, 0)
#define	flushtlbpage(addr) putflushprobe((addr)&~(BY2PG-1), 0)
int	fpcr(int);
int	fpquiet(void);
void	fpregrestore(char*);
void	fpregsave(char*);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
int	fptrap(void);
int	getfpq(ulong*);
ulong	getfsr(void);
ulong	getpsr(void);
#define icflush() putiflush(0, 0)
/* void	icflush(void); */
uchar	inb(int);
void	intrinit(void);
void	ioinit(void);
int	kbdstate(IOQ*, int);
void	kbdclock(void);
void	kbdrepeat(int);
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
void	kmapinit(void);
ulong	kmapdma(ulong, ulong);
KMap*	kmappas(uchar, ulong, ulong);
ulong	kmapsbus(int);
int	kprint(char*, ...);
void	kproftimer(ulong);
void	lanceintr(void);
void	lancesetup(Lance*);
void	lancetoggle(void);
void	mmuinit(void);
#define	mmunewpage(x)
void	mousebuttons(int);
void	outb(int, uchar);
void	printinit(void);
#define	procrestore(p)
#define	procsave(p)
#define	procsetup(x)	((p)->fpstate = FPinit)
void	putpmeg(ulong, ulong);
void	putstr(char*);
void	putstrn(char*, long);
void	puttbr(ulong);
void	restfpregs(FPsave*, ulong);
long	rtctime(void);
void	screeninit(char*, int);
void	screenputc(int);
void	screenputs(char*, int);
void	scsiintr(void);
void	setpsr(ulong);
void	spldone(void);
long	syscall(Ureg*);
void	systemreset(void);
ulong	tas(ulong*);
void	touser(ulong);
void	trap(Ureg*);
void	trapinit(void);
#define	wbflush()	/* mips compatibility */
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define getcallerpc(x)	_getcallerpc()
ulong	_getcallerpc(void);

/* 
 * Debugging stuff
 */
void mychandevreset(void);
 
/*
 * compiled entry points
 */
ulong	(*getflushprobe)(ulong);
void	(*putflushprobe)(ulong, ulong);
ulong	(*getrmmu)(ulong);
void	(*putrmmu)(ulong, ulong);
void	(*putrmmuflush)(ulong, ulong);
ulong	(*getphys)(ulong);
void	(*putphys)(ulong, ulong);
ulong	(*getsbus)(ulong);
void	(*putsbus)(ulong, ulong);
ulong	(*getio)(ulong);
void	(*putio)(ulong, ulong);
void	(*putiflush)(ulong, ulong);
void	(*putdflush)(ulong, ulong);
ulong	(*getaction)(ulong);
void	(*putaction)(ulong, ulong);

#define getpcr() getrmmu(0)
#define setpcr(new) putrmmuflush(0, (new))
/* 
ulong	getpcr(void);
void	setpcr(ulong);
*/ 

void	getmxccd(ulong, ulong*);
void	putmxccd(ulong, ulong*);

