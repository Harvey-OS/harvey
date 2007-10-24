void	aamloop(int);
void	addconf(char*, ...);
Alarm*	alarm(int, void (*)(Alarm*), void*);
void	alarminit(void);
Block*	allocb(int);
void	apminit(void);
int	biosboot(int dev, char *file, Boot *b);
void*	biosgetfspart(int i, char *name, int chatty);
void	biosinitdev(int i, char *name);
int	biosinit(void);
void	biosprintbootdevs(int dev);
void	biosprintdevs(int i);
int	bootpboot(int, char*, Boot*);
int	bootpass(Boot*, void*, int);
void	cancel(Alarm*);
int	cdinit(void);
void	check(char*);
void	cgascreenputs(char*, int);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	changeconf(char*, ...);
void	checkalarms(void);
void	clockinit(void);
#define coherence()	mb386()
void	consdrain(void);
void	consinit(char*, char*);
void	consputs(char*, int);
void	delay(int);
uchar*	etheraddr(int);
int	etherinit(void);
void	etherinitdev(int, char*);
void	etherprintdevs(int);
int	etherrxflush(int);
int	etherrxpkt(int, Etherpkt*, int);
int	ethertxpkt(int, Etherpkt*, int, int);
#define	evenaddr(x)		/* 386 doesn't care */
int	floppyboot(int, char*, Boot*);
int	floppyinit(void);
void	floppyinitdev(int, char*);
void	floppyprintdevs(int);
void*	floppygetfspart(int, char*, int);
void	freeb(Block*);
char*	getconf(char*);
ulong	getcr0(void);
ulong	getcr2(void);
ulong	getcr3(void);
ulong	getcr4(void);
int	getfields(char*, char**, int, char);
int	getstr(char*, char*, int, char*, int);
int	gunzip(uchar*, int, uchar*, int);
void	i8042a20(void);
void	i8042init(void);
void	i8042reset(void);
void*	ialloc(ulong, int);
void	idle(void);
void	ilock(Lock*);
int	inb(int);
ushort	ins(int);
ulong	inl(int);
void	insb(int, void*, int);
void	inss(int, void*, int);
void	insl(int, void*, int);
#define ioalloc(addr, len, align, name)	(addr)
#define iofree(addr)
void	iunlock(Lock*);
int	isaconfig(char*, int, ISAConf*);
void	kbdinit(void);
void	kbdchar(int);
void	machinit(void);
void	mb386(void);
void	meminit(ulong);
void	microdelay(int);
void	mmuinit(void);
#define	nelem(x)	(sizeof(x)/sizeof(x[0]))
char*	nextelem(char*, char*);
uchar	nvramread(int);
void	outb(int, int);
void	outs(int, ushort);
void	outl(int, ulong);
void	outsb(int, void*, int);
void	outss(int, void*, int);
void	outsl(int, void*, int);
void	panic(char*, ...);
ulong	pcibarsize(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
void	pciclrioe(Pcidev*);
void	pciclrmwi(Pcidev*);
int	pcigetpms(Pcidev*);
void	pcihinv(Pcidev*);
Pcidev*	pcimatch(Pcidev*, int, int);
uchar	pciintl(Pcidev *);
uchar	pciipin(Pcidev *, uchar);
void	pcireset(void);
void	pcisetbme(Pcidev*);
void	pcisetioe(Pcidev*);
void	pcisetmwi(Pcidev*);
int	pcisetpms(Pcidev*, int);
void	pcmcisread(PCMslot*);
int	pcmcistuple(int, int, int, void*, int);
PCMmap*	pcmmap(int, ulong, int, int);
int	pcmspecial(char*, ISAConf*);
void	pcmspecialclose(int);
void	pcmunmap(int, PCMmap*);
void	ptcheck(char*);
void	putcr3(ulong);
void	putidt(Segdesc*, int);
void*	pxegetfspart(int, char*, int);
void	qinit(IOQ*);
void	readlsconf(void);
void	sdaddconf(int);
int	sdboot(int, char*, Boot*);
void	sdcheck(char*);
void*	sdgetfspart(int, char*, int);
int	sdinit(void);
void	sdinitdev(int, char*);
void	sdprintdevs(int);
int	sdsetpart(int, char*);
void	setvec(int, void (*)(Ureg*, void*), void*);
int	splhi(void);
int	spllo(void);
void	splx(int);
void	trapinit(void);
void	trapdisable(void);
void	trapenable(void);
void	uartdrain(void);
void	uartspecial(int, void (*)(int), int (*)(void), int);
void	uartputs(IOQ*, char*, int);
ulong	umbmalloc(ulong, int, int);
void	umbfree(ulong, int);
ulong	umbrwmalloc(ulong, int, int);
void	upafree(ulong, int);
ulong	upamalloc(ulong, int, int);
void	warp86(char*, ulong);
void	warp9(ulong);
int	x86cpuid(int*, int*);
void*	xspanalloc(ulong, int, ulong);

#define malloc(n)	ialloc(n, 0)
#define mallocz(n, c)	ialloc(n, 0)
#define free(v)		while(0)

#define	GSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GLONG(p)	((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLSHORT(p)	(((p)[0]<<8)|(p)[1])
#define	GLLONG(p)	(((ulong)GLSHORT(p)<<16)|GLSHORT(p+2))
#define	PLLONG(p,v)	(p)[3]=(v);(p)[2]=(v)>>8;(p)[1]=(v)>>16;(p)[0]=(v)>>24

#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~0xF0000000)

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))


#define xalloc(n)	ialloc(n, 0)
#define xfree(v)	while(0)
#define lock(l)		if(l){/* nothing to do */;}else{/* nothing to do */;}
#define unlock(l)	if(l){/* nothing to do */;}else{/* nothing to do */;}

int	dmacount(int);
int	dmadone(int);
void	dmaend(int);
void	dmainit(int);
long	dmasetup(int, void*, long, int);

extern int (*_pcmspecial)(char *, ISAConf *);
extern void (*_pcmspecialclose)(int);
extern void devi82365link(void);
extern void devpccardlink(void);
