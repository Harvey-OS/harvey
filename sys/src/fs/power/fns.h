#include "../port/portfns.h"

#define vme2sysmap(bus, addr, size)	MP2VME(addr)
#define vme2sysfree(bus, addr, size)	if(0);else
#define vmeflush(bus, addr, size)	if(0);else
#define wbackcache(addr, size)		if(0);else
#define invalcache(addr, size)		if(0);else

void	vmeinit(void);
int	vmeintr(int);
void	vector80(void);
int	probe(void*, int);
void	nlights(int);
void	puttlbx(int, ulong, ulong);

void	scsiintr(int);
void	scsiinit(void);
