#include "../port/portfns.h"

void	archinit(void);
int	brgalloc(void);
void	brgfree(int);
ulong	baudgen(int, int);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	clockcheck(void);
void	clockinit(void);
void	clockintr(Ureg*);
void	clrfptrap(void);
#define coherence()
void	cpminit(void);
int	cpuidentify(void);
void	cpuidprint(void);
void	dcflush(void*, ulong);
void	delay(int);
ulong	draminit(ulong*);
void	dtlbmiss(void);
void	dtlberror(void);
void	dumpregs(Ureg*);
void	delayloopinit(void);
void	eieio(void);
//#define	eieio()
void	evenaddr(ulong);
void	faultpower(Ureg*, ulong addr, int read);
void	firmware(int);
void	fpinit(void);
int	fpipower(Ureg*);
void	fpoff(void);
ulong	fpstatus(void);
char*	getconf(char*);
ulong	getdar(void);
ulong	getdec(void);
ulong	getdepn(void);
ulong	getdsisr(void);
ulong	getimmr(void);
ulong	getmsr(void);
ulong	getpvr(void);
ulong	gettbl(void);
ulong	gettbu(void);
void	gotopc(ulong);
void	icflush(void*, ulong);
void	idle(void);
#define	idlehands()			/* nothing to do in the runproc */
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
void	intr(Ureg*);
void	intrenable(int, void (*)(Ureg*, void*), void*, int);
int	intrstats(char*, int);
void	intrvec(void);
int	iprint(char*, ...);
void	itlbmiss(void);
int	isaconfig(char*, int, ISAConf*);
void	kbdinit(void);
void	kbdreset(void);
void	kernelmmu(void);
void	links(void);
void	mapfree(RMap*, ulong, int);
void	mapinit(RMap*, Map*, int);
void	mathinit(void);
void	mmuinit(void);
ulong*	mmuwalk(ulong*, ulong, int);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
PCMmap*	pcmmap(int, ulong, int, int);
void	pcmunmap(int, PCMmap*);
int		pcmspecial(char*, ISAConf*);
void	pcmspecialclose(int);
#define	procrestore(p)
void	powerdownled(void);
void	powerupled(void);
void	procsave(Proc*);
void	procsetup(Proc*);
void	putdec(ulong);
void	putmsr(ulong);
void	putcasid(ulong);
ulong	rmapalloc(RMap*, ulong, int, int);
void	screeninit(void);
void	setpanic(void);
int	screenprint(char*, ...);			/* debugging */
#define screenputs(x, y)
ulong	sdraminit(ulong*);
int	segflush(void*, ulong);
void	spireset(void);
long	spioutin(void*, long, void*);
int	tas(void*);
uchar*	tarlookup(uchar *addr, char *file, int *dlen);
void	touser(void*);
void	trapinit(void);
void	trapvec(void);
void	tlbflush(ulong);
void	tlbflushall(void);
void	uartinstall(void);
void	uartwait(void);	/* debugging */
int unsac(uchar *dst, uchar *src, int n, int nsrc);
#define	userureg(ur) ((ur)->status & KUSER)
void	wbflush(void);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)

/* IBM bit field order */
#define	IBIT(b)		((ulong)1<<(31-(b)))
#define	SIBIT(n)	((ushort)1<<(15-(n)))

/* compatibility with inf2.1 */
#define	bpenumenv(n)	((char*)0)

#define IOREGS(x, T)	((T*)((char*)m->iomem+(x)))
