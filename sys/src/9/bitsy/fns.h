#include "../port/portfns.h"

void	audiopower(int);
void	cacheflush(void);
void	cachewb(void);
void	cachewbaddr(void*);
void	cachewbregion(ulong, int);
void	dcacheinvalidate(void);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
void	clockinit(void);
#define	coherence()
#define	dcflush(a, b)
void	delay(int);
void	µdelay(int);
void	dmainit(void);
void	_doze(void);
void	(*doze)(void);
void	egpiobits(ulong, int);
void	evenaddr(ulong);
void	exppackpower(int);
void	flashprogpower(int);
void	flushmmu(void);
int	fpiarm(Ureg *ur);
char*	getconf(char*);
ulong	getcpuid(void);
ulong	getfar(void);
ulong	getfsr(void);
#define	getpgcolor(a)	0
void	gpiointrenable(ulong, int, void (*)(Ureg*, void*), void*, char*);
void	h3650uartsetup(void);
void	icacheinvalidate(void);
void	idle(void);
void	idlehands(void);
uchar	inb(ulong);
ushort	ins(ulong);
void	inss(ulong, void*, int);
ulong	inl(ulong);
void	intrenable(int, int, void (*)(Ureg*, void*), void*, char*);
int	iprint(char*, ...);
void	irpower(int);
void	lcdpower(int);
void	links(void);
void*	mapmem(ulong, int, int);
void	mappedIvecEnable(void);
void	mappedIvecDisable(void);
void*	mapspecial(ulong, int);
void	meminit(void);
void	mmuinit(void);
void	mmuenable(void);
void	mmudisable(void);
void	mmuinvalidate(void);
void	mmuinvalidateaddr(ulong);
ulong	mmu_kaddr(ulong);
ulong	mmu_paddr(ulong);
int	µcputc(Queue*, int);
void	noted(Ureg*, ulong);
int	notify(Ureg*);
void	outb(ulong, uchar);
void	outs(ulong, ushort);
void	outss(ulong, void*, int);
void	outl(ulong, ulong);
void	pcmcisread(PCMslot*);
int	pcmcistuple(int, int, int, void*, int);
PCMmap*	pcmmap(int, ulong, int, int);
void	pcmunmap(int, PCMmap*);
void	penbutton(int, int);
void	pentrackxy(int x, int y);
#define	procrestore(p)
void	procsave(Proc*);
void	procsetup(Proc*);
void	putdac(ulong);
void	putttb(ulong);
void	putpid(ulong);
void	qpanic(char *, ...);
void	rs232power(int);
Uart*	uartsetup(PhysUart*, void*, ulong, char*);
void	uartspecial(Uart*, int, Queue**, Queue**, int (*)(Queue*, int));
void	sa1100_uartsetup(int);
void	screeninit(void);
int	screenprint(char*, ...);			/* debugging */
void	screenputs(char*, int);
void	serialµcputs(uchar *str, int n);
void	setr13(int, ulong*);
uchar*	tarlookup(uchar*, char*, int*);
void	touser(void*);
void	trapdump(char *tag);
void	trapinit(void);
int	tas(void*);
int	uartstageoutput(Uart*);
void	uartkick(void*);
void	uartrecv(Uart*, char);
int	unsac(uchar*, uchar*, int, int);
#define	userureg(ur)	(((ur)->psr & PsrMask) == PsrMusr)
void	vectors(void);
void	vtable(void);
void	wbflush(void);
#define KADDR(a) (void*)mmu_kaddr((ulong)(a))
#define PADDR(a) mmu_paddr((ulong)(a))

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
