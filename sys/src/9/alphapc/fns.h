#include "../port/portfns.h"

void		addclock0link(void (*)(void));
void		archinit(void);
void		arginit(void);
void		arith(void);
void		clearmmucache(void);
void		clock(Ureg*);
void		clockinit(void);
#define 	coherence 	mb
int		cistrcmp(char*, char*);
int		cistrncmp(char*, char*, int);
void		cpuidprint(void);
void		cserve(ulong, ulong);
int	dmacount(int);
int	dmadone(int);
void	dmaend(int);
int	dmainit(int, int);
long	dmasetup(int, void*, long, int);
void		evenaddr(ulong);
void		fataltrap(Ureg *, char *);
void		fault0(void);
void		faultalpha(Ureg*);
ulong	fcr31(void);
void		firmware(void);
#define	flushpage(s)	icflush()
void		fpenab(int);
void		fptrap(Ureg*);
int		getcfields(char*, char**, int, char*);
char		*getconf(char*);
ulong	getfcr(void);
ulong	getstatus(void);
void		gotopc(ulong);
int		i8042auxcmd(int);
void		i8042auxenable(void (*)(int, int));
void		i8042reset(void);
void		i8259init(void);
int		i8259enable(int, int, Vctl*);
#define	idlehands()			/* nothing to do in the runproc */
void		icflush(void);
void		illegal0(void);
void		intr0(void);
void		intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
int		ioalloc(int, int, int, char*);
void		iofree(int);
void		ioinit(void);
int		iounused(int, int);
int		iprint(char*, ...);
int		irqallocread(char*, long, vlong);
int		isaconfig(char*, int, ISAConf*);
void		kbdinit(void);
void		*kmapv(uvlong, int);
int		kprint(char*, ...);
void		launchinit(void);
void		launch(int);
void		links(void);
void		mb(void);
void 		memholes(void);
ulong 	meminit(void);
void		mmuinit(void);
#define	mmunewpage(x)
void		mntdump(void);
void		ns16552install(void);
void		ns16552special(int, int, Queue**, Queue**, int (*)(Queue*, int));
ulong	pcibarsize(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pcihinv(Pcidev*);
Pcidev* pcimatch(Pcidev*, int, int);
void	pcireset(void);
void	pcisetbme(Pcidev*);
void		prflush(void);
void		printinit(void);
#define	procrestore(p)
void	procsave(Proc*);
void	procsetup(Proc*);
void		restfpregs(FPsave*);
uvlong		rpcc(uvlong*);
void		screeninit(void);
void		(*screenputs)(char*, int);
void 		setpcb(PCB *);
PCB		*swpctx(PCB *);
void		syscall0(void);
int		tas(ulong*);
void		tlbflush(int, ulong);
void		touser(void*);
void		trapinit(void);
void		unaligned(void);
ulong	upamalloc(ulong, int, int);
void	upafree(ulong, int);
#define		userureg(ur) ((ur)->status & UMODE)
void		wrent(int, void*);
void		wrvptptr(uvlong);

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define	kmapperm(x)	kmap(x)
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)

#define	inb(p)	(arch->_inb)(p)
#define	ins(p)	(arch->_ins)(p)
#define	inl(p)	(arch->_inl)(p)
#define	outb(p, x)	(arch->_outb)((p), (x))
#define	outs(p, x)	(arch->_outs)((p), (x))
#define	outl(p, x)	(arch->_outl)((p), (x))

#define	insb(p, buf, len)	(arch->_insb)((p), (buf), (len))
#define	inss(p, buf, len)		(arch->_inss)((p), (buf), (len))
#define	insl(p, buf, len)		(arch->_insl)((p), (buf), (len))
#define	outsb(p, buf, len)	(arch->_outsb)((p), (buf), (len))
#define	outss(p, buf, len)	(arch->_outss)((p), (buf), (len))
#define	outsl(p, buf, len)	(arch->_outsl)((p), (buf), (len))
