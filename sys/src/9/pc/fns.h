#include "../port/portfns.h"

void	meminit(void);
void	bigcursor(void);
void	bootargs(ulong);
#define	clearmmucache()		/* 386 doesn't have one */
void	clock(Ureg*);
void	clockinit(void);
void	config(int);
int	cpuspeed(int);
void	delay(int);
void	dmaend(int);
long	dmasetup(int, void*, long, int);
#define	evenaddr(x)		/* 386 doesn't care */
void	fault386(Ureg*);
void	faultinit(void);
void	fclock(Ureg*);
void	fclockinit(void);
void	fpenv(FPsave*);
void	fpinit(void);
void	fpoff(void);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
ulong	fpstatus(void);
ulong	getcr0(void);
ulong	getcr2(void);
void	heada20(void);
void	headreset(void);
void	i8042a20(void);
void	i8042reset(void);
void	ident(void);
void	idle(void);
int	inb(int);
void	inss(int, void*, int);
void	kbdinit(void);
void	mathinit(void);
void	mmuinit(void);
int	modem(int);
void	mouseserial(int);
void	mouseps2(void);
void	mouseaccelerate(int);
void	mouseres(int);
void	mousespeed(int);
void	outb(int, int);
void	outss(int, void*, int);
void	pmubuzz(int, int);
int	pmucpuspeed(int);
void	pmulights(int);
int	pmumodem(int);
int	pmuserial(int);
void	prhex(ulong);
void	procrestore(Proc*);
void	procsave(Proc*);
void	procsetup(Proc*);
void	putgdt(Segdesc*, int);
void	putidt(Segdesc*, int);
void	putcr3(ulong);
void	puttr(ulong);
void	screeninit(void);
void	screenputc(int);
void	screenputs(char*, int);
int	serial(int);
void	setvec(int, void (*)(Ureg*));
void	systrap(void);
void	toscreen(void*);
void	touser(void*);
void	trapinit(void);
int	tas(void*);
void	uartclock(void);
void	uartintr0(Ureg*);
void	uartspecial(int, IOQ*, IOQ*, int);
void	vgainit(void);
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define	kmapperm(x)	kmap(x)
#define getcallerpc(x)	(*(ulong*)(x))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
