#include "../port/portfns.h"

void	bootargs(ulong);
void	cacheinit(void);
ulong	call(void*, ...);
void	clearftt(ulong);
#define	clearmmucache()
void	clockinit(void);
void	compile(void);
void	disabfp(void);
void	enabfp(void);
void	evenaddr(ulong);
char*	excname(ulong);
void	faultasync(Ureg*);
void	faultsparc(Ureg*);
void	flushcpucache(void);
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
void	intrinit(void);
void	ioinit(void);
int	kbdstate(IOQ*, int);
void	kbdclock(void);
void	kbdrepeat(int);
KMap*	kmap(Page*);
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
void	kmapinit(void);
KMap*	kmappa(ulong, ulong);
KMap*	kmapperm(Page*);
ulong	kmapregion(ulong, ulong, ulong);
int	kprint(char*, ...);
void	kproftimer(ulong);
void	kunmap(KMap*);
void	lanceintr(void);
void	lancesetup(Lance*);
void	lancetoggle(void);
void	mmuinit(void);
void	mousebuttons(int);
void	printinit(void);
#define	procrestore(p)
#define	procsave(p)
#define	procsetup(x)	((p)->fpstate = FPinit)
void	putcxsegm(int, ulong, int);
void	putpmeg(ulong, ulong);
void	putsegm(ulong, int);
void	putstr(char*);
void	putstrn(char*, long);
void	puttbr(ulong);
void	putw4(ulong, ulong);	/* needed only at boot time */
void	systemreset(void);
void	restfpregs(FPsave*, ulong);
void	screeninit(char*, int);
void	screenputc(int);
void	screenputs(char*, int);
void	scsiintr(void);
void	setpsr(ulong);
void	spldone(void);
ulong	tas(ulong*);
void	touser(ulong);
void	trap(Ureg*);
void	trapinit(void);
#define	wbflush()	/* mips compatibility */
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define getcallerpc(x)	_getcallerpc()
ulong	_getcallerpc(void);

/*
 * compiled entry points
 */
extern ulong	(*getcontext)(void);
extern void	(*putcontext)(ulong);
extern void	(*putenab)(ulong);
extern ulong	(*getenab)(void);
extern ulong	(*getsysspace)(ulong);
extern void	(*putsysspace)(ulong, ulong);
extern uchar	(*getsysspaceb)(ulong);
extern void	(*putsysspaceb)(ulong, uchar);
extern ulong	(*getsegspace)(ulong);
extern void	(*putsegspace)(ulong, ulong);
extern ulong	(*getpmegspace)(ulong);
extern void	(*putpmegspace)(ulong, ulong);
extern ulong	(*flushpg)(ulong);
