/*
 * Interface to raw PS/2 aux port.
 * Used by user-level mouse daemon.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#define Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

/*
 * BUG: we ignore shift here.
 * we need a more general solution,
 * one that will also work for serial mice.
 */
Queue *psauxq;

static void
psauxputc(int c, int)
{
	uchar uc;

	uc = c;
	qproduce(psauxq, &uc, 1);
}

static long
psauxread(Chan*, void *a, long n, vlong)
{
	return qread(psauxq, a, n);
}

static long
psauxwrite(Chan*, void *a, long n, vlong)
{
	return i8042auxcmds(a, n);
}

void
psauxlink(void)
{
	psauxq = qopen(1024, 0, 0, 0);
	if(psauxq == nil)
		panic("psauxlink");
	qnoblock(psauxq, 1);
	i8042auxenable(psauxputc);
	addarchfile("psaux", DMEXCL|0660, psauxread, psauxwrite);
}
