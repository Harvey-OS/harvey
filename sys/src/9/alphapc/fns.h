#include "../port/portfns.h"

Dirtab*	addarchfile(char*, int, long(*)(Chan*,void*,long,vlong), long(*)(Chan*,void*,long,vlong));
void	archinit(void);
void	arginit(void);
void	arith(void);
ulong	cankaddr(ulong);
void	clock(Ureg*);
void	clockinit(void);
void	clockintrsched(void);
#define coherence 	mb
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
int	cmpswap(long*, long, long);
void	cpuidprint(void);
void	cserve(ulong, ulong);
#define cycles(x)	do{}while(0)
void	timeradd(Timer *);
void	timerdel(Timer *);
int	dmacount(int);
int	dmadone(int);
void	dmaend(int);
int	dmainit(int, int);
long	dmasetup(int, void*, long, int);
void	_dumpstack(Ureg *);
void	evenaddr(ulong);
void	fataltrap(Ureg *, char *);
void	fault0(void);
void	faultalpha(Ureg*);
ulong	fcr31(void);
void	firmware(void);
void	fpenab(int);
void	fptrap(Ureg*);
int	getcfields(char*, char**, int, char*);
char	*getconf(char*);
int	havetimer(void);
int	i8042auxcmd(int);
void	i8042auxenable(void (*)(int, int));
void	i8042reset(void);
void	i8250console(void);
void	i8250mouse(char*, int(*)(Queue*,int), int);
void	i8250setmouseputc(char*, int (*)(Queue*, int));
void	i8259init(void);
int	i8259enable(int, int, Vctl*);
#define	idlehands()		/* nothing to do in the runproc */
void	icflush(void);
void	illegal0(void);
void	intr0(void);
void	intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
int	intrdisable(int, void (*)(Ureg *, void *), void*, int, char*);
int	ioalloc(int, int, int, char*);
void	iofree(int);
void	ioinit(void);
int	iounused(int, int);
int	irqallocread(char*, long, vlong);
int	isaconfig(char*, int, ISAConf*);
void	kbdinit(void);
#define kexit(a)
#define	kmapinval()
void	*kmapv(uvlong, int);
int	kprint(char*, ...);
void	links(void);
void	mb(void);
void 	memholes(void);
ulong 	meminit(void);
void	mmudump(void);
void	mmuinit(void);
void	mmupark(void);
#define mtrr(a, b, c)
ulong	pcibarsize(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
void	pcihinv(Pcidev*);
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
void	pcireset(void);
void	pcisetbme(Pcidev*);
int	pcmspecial(char*, ISAConf*);
int	(*_pcmspecial)(char *, ISAConf *);
void	pcmspecialclose(int);
void	(*_pcmspecialclose)(int);
void	prflush(void);
void	printinit(void);
#define	procrestore(p)
void	procsave(Proc*);
void	procsetup(Proc*);
void	restfpregs(FPsave*);
uvlong	rpcc(uvlong*);
void	screeninit(void);
void	(*screenputs)(char*, int);
void 	setpcb(PCB *);
PCB	*swpctx(PCB *);
void	syscall0(void);
int	tas(ulong*);
void	tlbflush(int, ulong);
void	touser(void*);
void	trapinit(void);
void	unaligned(void);
ulong	upaalloc(int, int);
void	upafree(ulong, int);
#define	userureg(ur) ((ur)->status & UMODE)
void*	vmap(ulong, int);
void	wrent(int, void*);
void	wrvptptr(uvlong);
void	vunmap(void*, int);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)

#define	inb(p)	(arch->_inb)(p)
#define	ins(p)	(arch->_ins)(p)
#define	inl(p)	(arch->_inl)(p)
#define	outb(p, x)	(arch->_outb)((p), (x))
#define	outs(p, x)	(arch->_outs)((p), (x))
#define	outl(p, x)	(arch->_outl)((p), (x))

#define	insb(p, buf, len)	(arch->_insb)((p), (buf), (len))
#define	inss(p, buf, len)	(arch->_inss)((p), (buf), (len))
#define	insl(p, buf, len)	(arch->_insl)((p), (buf), (len))
#define	outsb(p, buf, len)	(arch->_outsb)((p), (buf), (len))
#define	outss(p, buf, len)	(arch->_outss)((p), (buf), (len))
#define	outsl(p, buf, len)	(arch->_outsl)((p), (buf), (len))
