#include "../port/portfns.h"

extern	void	invalicache(void*, unsigned);
extern	void	invaldcache(void*, unsigned);
extern	int	tas(Lock*);
extern	ulong	getstatus(void);
extern	void	vector80(void);
extern	void	puttlbx(int, ulong, ulong);

extern	void	lancesetup(Lance*);
extern	void	sccsetup(void *, ulong);
extern	void	sccspecial(int, void (*)(int), int (*)(void), int);
extern	void	sccintr(void);
extern	int	sccgetc(int);
extern	void	sccputc(int, int);
extern	void	scsiinit(int);
extern	void	scsiintr(int);
extern	int	setsimmtype(int);

extern	void*	iallocspan(ulong, int, ulong);
extern	void	trapinit(void);
