/*
 * Stub for the WD9[56]A chips on the Challenge.
 * There's only enough here to stop the initialisation
 * code in main from complaining.
 * To make this real, sdwd95a.old is a working driver
 * using the old SCSI driver interface.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "sd.h"
extern SDifc sdwd95aifc;

int
wd95acheck(ulong devaddr)
{
	USED(devaddr);
	return 1;
}

void
wd95aboot(int ctlrno, S1chan *s1p, ulong dev)
{
	USED(s1p, dev);
	panic("wd95aintr %d\n", ctlrno);
}

void
wd95aintr(int ctlrno)
{
	panic("wd95aintr %d\n", ctlrno);
}

SDifc sdwd95aifc = {
	"wd95a",			/* name */

	nil,				/* pnp */
	nil,				/* legacy */
	nil,				/* id */
	nil,				/* enable */
	nil,				/* disable */

	nil,				/* verify */
	nil,				/* online */
	nil,				/* rio */
	nil,				/* rctl */
	nil,				/* wctl */

	nil,				/* bio */
};
