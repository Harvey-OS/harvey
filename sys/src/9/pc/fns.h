#include "../port/portfns.h"

void	aamloop(int);
void	addconf(char*, char*);
void	bbinit(void);
void	bootargs(ulong);
int	cistrcmp(char*, char*);
#define	clearmmucache()		/* 386 doesn't have one */
void	clockinit(void);
void	config(int);
int	cpuspeed(int);
void	delay(int);
void	dmaend(int);
void	dmainit(void);
long	dmasetup(int, void*, long, int);
#define	evenaddr(x)		/* 386 doesn't care */
void	faultinit(void);
void	fclock(Ureg*);
void	fclockinit(void);
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
void	fpenv(FPsave*);
void	fpinit(void);
void	fpoff(void);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
ulong	fpstatus(void);
ulong	getcr0(void);
ulong	getcr2(void);
char*	getconf(char*);
void	hardclock(void);
void	i8042a20(void);
void	i8042reset(void);
void	ident(void);
void	idlefunc(void);
ulong	ilputl(ulong*, ulong);
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
int	isaconfig(char*, int, ISAConf*);
ulong	getisa(ulong, int, int);
void	kbdinit(void);
void*	l0update(uchar*, uchar*, long);
void*	l1update(uchar*, uchar*, long);
void*	l2update(uchar*, uchar*, long);
long*	mapaddr(ulong);
void	mathinit(void);
void	meminit(void);
void	microdelay(int);
void	mmuinit(void);
#define	mmunewpage(x)
int	modem(int);
void	mousectl(char*);
uchar	nvramread(int);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
void	pcicreset(void);
PCMmap*	pcmmap(int, ulong, int, int);
int	pcmspecial(char*, ISAConf*);
void	pcmspecialclose(int);
void	pcmunmap(int, PCMmap*);
void	prhex(ulong);
void	printcpufreq(void);
void	procrestore(Proc*);
void	procsave(Proc*);
void	procsetup(Proc*);
void	ps2poll(void);
void	putgdt(Segdesc*, int);
void	putidt(Segdesc*, int);
void	putcr3(ulong);
void	putisa(ulong, int);
void	puttr(ulong);
long	rtctime(void);
void	screeninit(void);
void	screenputc(int);
void	screenputs(char*, int);
int	serial(int);
void	setvec(int, void (*)(Ureg*, void*), void*);
long	syscall(Ureg*, void*);
void	systrap(void);
void	toscreen(void*);
void	touser(void*);
void	trapinit(void);
int	tas(void*);
void	uartclock(void);
void	uartspecial(int, IOQ*, IOQ*, int);
void	vgainit(void);
int	x86(void);
int	x86cpuid(int*, int*);

#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define	kmapperm(x)	kmap(x)
#define getcallerpc(x)	(*(ulong*)(x))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
