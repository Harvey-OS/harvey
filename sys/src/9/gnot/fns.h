#include "../port/portfns.h"

void	addportintr(int (*)(void));
void	bootargs(ulong);
void	clearmmucache(void);
void	clock(Ureg*);
void	dspclock(void);
void	duartclock(void);
void	duartinit(void);
void	duartstarttimer(void);
void	duartstoptimer(void);
#define	evenaddr(x)	/* 68020 doesn't care */
void	fault68020(Ureg*, FFrame*);
void	firmware(void);
#define	flushapage(x)
void	flushcpucache(void);
#define	flushpage(x) if(x)
int	fpcr(int);
ulong	fpiar(void);
void	fpregrestore(char*);
void	fpregsave(char*);
void	fprestore(FPsave*);
void	fpsave(FPsave*);
ulong	getsr(void);
#define icflush(a, b)
void	incontoggle(void);
KMap*	kmap(Page*);
void	kmapinit(void);
void	kunmap(KMap*);
void	mmuinit(void);
#define	mmunewpage(x)
void	mousebuttons(int);
void	mouseclock(void);
int	portprobe(char*, int, int, int, long);
void	procrestore(Proc*);
void	procsave(Proc*);
void	procsetup(Proc*);
void	putkmmu(ulong, ulong);
void	restfpregs(FPsave*);
void	screeninit(void);
void	screenputc(int);
void	screenputs(char*, int);
void	scsictrlintr(void);
void	scsidmaintr(void);
uchar*	scsirecv(uchar *);
void	scsirun(void);
uchar*	scsixmit(uchar *);
int	spl1(void);
void	spldone(void);
int	splduart(void);
long	syscall(Ureg*);
int	tas(char*);
void	touser(void*);
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define	kmapperm(x)	kmap(x)
#define getcallerpc(x)	(*(ulong*)(x))
#define KADDR(a)	((void*)((ulong)(a)|KZERO))
#define PADDR(a)	((ulong)(a)&~KZERO)
