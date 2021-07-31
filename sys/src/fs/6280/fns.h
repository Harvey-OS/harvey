#include "../port/portfns.h"

extern	ulong	vme2sysmap(int, ulong, ulong);
extern	void	vme2sysfree(int, ulong, ulong);
extern	void	vmeflush(int, ulong, ulong);
extern	void	wbackcache(void*, unsigned);
extern	void	invalcache(void*, unsigned);

extern	void	cacheflush(void);
extern	void	invaldline(void*);
extern	void	inval2cache(void*, unsigned);
extern	void	wback2cache(void*, unsigned);
extern	void	vmeinit(void);
extern	void	ioaintr(void);
extern	void	vmeintr(int);
extern	int	tas(Lock*);
extern	void	vector80(void);
extern	void	intrclr(ulong);
extern	int	probe(void *, unsigned);
extern	void	setioavector(ulong *, ulong);
extern	void	hrodstart(void);
