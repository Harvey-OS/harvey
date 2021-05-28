#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"io.h"
#include	"../port/netif.h"
#include	"etherif.h"
#include	"../ip/ip.h"

Mutstats mutstats;
Ureg *gureg;

int
gotsecuremem(void)
{
	return 0;
}

ulong
qtmborder(void)
{
	return 0;
}

void
qtmerrs(char *)
{
}

void
qtmerr(void)
{
}

void
qtminit(void)
{
}

int
mutatetrigger(void)
{
	return 0;
}

void
mutateproc(void *)
{
}

void
qtmclock(void)
{
}
