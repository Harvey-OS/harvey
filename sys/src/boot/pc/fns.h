void	aamloop(int);
Alarm*	alarm(int, void (*)(Alarm*), void*);
void	alarminit(void);
int	bootp(int, char*);
void	cancel(Alarm*);
void	cgainit(void);
void	cgaputs(IOQ*, char*, int);
void	checkalarms(void);
void	clockinit(void);
void	consinit(void);
int	conschar(void);
void	delay(int);
uchar*	etheraddr(int);
int	etherinit(void);
int	etherrxpkt(int, Etherpkt*, int);
int	ethertxpkt(int, Etherpkt*, int, int);
#define	evenaddr(x)		/* 386 doesn't care */
int	floppyinit(void);
long	floppyread(int, void*, long);
long	floppyseek(int, long);
char*	getconf(char*);
ulong	getcr0(void);
ulong	getcr2(void);
ulong	getcr3(void);
int	getfields(char*, char**, int, char);
int	getstr(char*, char*, int, char*, int);
int	hardinit(void);
long	hardread(int, void*, long);
long	hardseek(int, long);
long	hardwrite(int, void*, long);
void	i8042a20(void);
void	i8042reset(void);
void*	ialloc(ulong, int);
void	idle(void);
int	inb(int);
ushort	ins(int);
ulong	inl(int);
void	insb(int, void*, int);
void	inss(int, void*, int);
void	insl(int, void*, int);
int	isaconfig(char*, int, ISAConf*);
void	kbdinit(void);
void	kbdchar(int);
void	machinit(void);
void	meminit(void);
void	mmuinit(void);
uchar	nvramread(int);
void	outb(int, int);
void	outs(int, ushort);
void	outl(int, ulong);
void	outsb(int, void*, int);
void	outss(int, void*, int);
void	outsl(int, void*, int);
int	plan9boot(int, long (*)(int, long), long (*)(int, void*, int));
void	panic(char*, ...);
void	putcr3(ulong);
void	putidt(Segdesc*, int);
void	qinit(IOQ*);
int	scsiexec(Scsi*, int);
int	scsiinit(void);
long	scsiread(int, void*, long);
long	scsiseek(int, long);
Partition* sethardpart(int, char*);
Partition* setscsipart(int, char*);
void	setvec(int, void (*)(Ureg*, void*), void*);
int	splhi(void);
int	spllo(void);
void	splx(int);
void	trapinit(void);
void	uartspecial(int, void (*)(int), int (*)(void), int);
void	uartputs(IOQ*, char*, int);
int	x86(void);

#define	GSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GLONG(p)	((GSHORT(p+2)<<16)|GSHORT(p))
#define	GLSHORT(p)	(((p)[0]<<8)|(p)[1])
#define	GLLONG(p)	((GLSHORT(p)<<16)|GLSHORT(p+2))

#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
