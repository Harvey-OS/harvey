#include "../port/portfns.h"

void	clockinit(void);
void	clockintr(int, Ureg*);
void	eiaclock(void);
void	eiasetup(int, void*);
void	eiaspecial(int, IOQ*, IOQ*, int);
void	evenaddr(ulong);
void	flushcpucache(void);
ulong	getcpureg(int);
void	gotopc(ulong);
void	kproftimer(ulong);
void	mmuflushpte(ulong);
void	mmuflushtlb(void);
void	mmuinit(void);
int	mmuiomap(ulong, ulong);
void	mmusetstb(ulong);
ulong*	mmuwalk(Proc*, ulong, int);
void	screeninit(int);
void	screenputs(char*, int);
void	setvector(Vector*);
void	softreset(void);
int	tas(ulong*);
void	touser(void);

#define buzz(freq, duration)
#define lights(a)
#define procrestore(p)
#define procsave(p)
#define procsetup(p)
#define savefpregs(initfp)
#define	waserror()	(u->nerrlab++, setlabel(&u->errlab[u->nerrlab-1]))
#define getcallerpc(x)	(*(ulong*)(x))

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

#define QUADALIGN(x)	(ROUNDUP((x), 16))
