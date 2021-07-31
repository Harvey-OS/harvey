#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

static ulong bufdatasize;

void
scsibufreset(ulong datasize)
{
	bufdatasize = datasize;
}

Scsibuf *
scsibuf(void)
{
	Scsibuf *b;

	b = smalloc(sizeof(*b));
	b->virt = smalloc(bufdatasize);
	b->phys = (void *)(PADDR(b->virt));
	return b;
}

void
scsifree(Scsibuf *b)
{
	free(b->virt);
	free(b);
}

/*
 * Hack for devvid
 */
Scsibuf *
scsialloc(ulong n)
{
	Scsibuf *b;

	b = smalloc(sizeof(*b));
	b->virt = smalloc(n);
	b->phys = (void *)(PADDR(b->virt));
	return b;
}
