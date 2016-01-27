/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>

#include "ip.h"

static void
nullbind(Ipifc *ipifc, int i, char **argv)
{
	error("cannot bind null device");
}

static void
nullunbind(Ipifc *ipifc)
{
}

static void
nullbwrite(Ipifc *ipifc, Block *block, int i, uint8_t *c)
{
	error("nullbwrite");
}

Medium nullmedium =
{
.name=		"null",
.bind=		nullbind,
.unbind=	nullunbind,
.bwrite=	nullbwrite,
};

void
nullmediumlink(void)
{
	addipmedium(&nullmedium);
}
