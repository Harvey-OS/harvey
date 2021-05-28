/* portable clock code */
#include "include.h"

ulong intrcount[MAXMACH];

void
hzclock(void)
{
	m->ticks++;
	dcflush(PTR2UINT(&m->ticks), sizeof m->ticks);
}

void
timerintr(Ureg *)
{
	intrcount[m->machno]++;
	hzclock();
}

void
timersinit(void)
{
}
