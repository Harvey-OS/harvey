#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

Apminfo apm;

void
apminit(void)
{
	if(getconf("apm0") && apm.haveinfo)
		changeconf("apm0=ax=%x ebx=%x cx=%x dx=%x di=%x esi=%x\n",
			apm.ax, apm.ebx, apm.cx, apm.dx, apm.di, apm.esi);
}
