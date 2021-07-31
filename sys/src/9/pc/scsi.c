#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

/*
 * Known to devscsi.c.
int scsidebugs[8];
int scsiownid = CtlrID;
 */

void
initscsi(void)
{
}

/*
 * Quick hack. Need to do a better job of dynamic initialisation
 * for machines with peculiar memory/cache restictions.
 * Also, what about 16Mb address limit on the Adaptec?
 */
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

extern int (*aha1542reset(void))(Scsi*, int);
extern int (*ultra14freset(void))(Scsi*, int);

static int (*exec)(Scsi*, int);

int
scsiexec(Scsi *p, int rflag)
{
	if(exec == 0)
		error(Enonexist);
	return (*exec)(p, rflag);
}

void
resetscsi(void)
{
	if(exec = aha1542reset())
		return;
	exec = ultra14freset();
}
