#include "../port/portfns.h"

extern	void	compile(void);
extern	ulong	getpsr(void);
extern	ulong	getpte(ulong);
extern	void	invalcache(ulong, ulong);
extern	KMap*	kmappa(ulong, ulong);
extern	void	kunmap(KMap*);
extern	ulong	kmapregion(ulong, ulong, ulong);
extern	void	putcxsegm(int, ulong, int);
extern	void	putpte(ulong, ulong, int);
extern	void	puttbr(ulong);
extern	void	putw4(ulong, ulong);	/* needed only at boot time */
extern	void	setpsr(ulong);
extern	int	tas(Lock*);
extern	void	trapinit(void);
extern	void	wbackcache(ulong, ulong);
extern	void*	xialloc(ulong, int, ulong);

#define VADDR(a)	(((ulong)(a))|KZERO)
#define PADDR(a)	((ulong)(a)&~KZERO)

extern	void	lancesetup(Lance*);
extern	void	sccsetup(void *, ulong);
extern	void	sccspecial(int, void (*)(int), int (*)(void), int);
extern	void	sccintr(void);
extern	int	sccgetc(int);
extern	void	sccputc(int, int);
extern	void	scsiinit(int);
extern	void	scsiintr(int);
extern	void	toyinit(void);

/*
 * compiled entry points
 */
extern	void	compile(void);
extern	void	(*putcontext)(ulong);
extern	void	(*putenab)(ulong);
extern	ulong	(*getenab)(void);
extern	ulong	(*getsysspace)(ulong);
extern	void	(*putsysspace)(ulong, ulong);
extern	uchar	(*getsysspaceb)(ulong);
extern	void	(*putsysspaceb)(ulong, uchar);
extern	ulong	(*getsegspace)(ulong);
extern	void	(*putsegspace)(ulong, ulong);
extern	ulong	(*getpmegspace)(ulong);
extern	void	(*putpmegspace)(ulong, ulong);
extern	ulong	(*flushcx)(ulong);
extern	ulong	(*flushpg)(ulong);
