#include "lib9.h"

ulong
getcallerpc(void *p)
{
	ulong *lp;

	lp = p;

	return lp[-1];
}
