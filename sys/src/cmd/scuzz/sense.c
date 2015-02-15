/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include "scsireq.h"

extern Biobuf bout;

static char* key[16] = {
	"no sense",
	"recovered error",
	"not ready",
	"medium error",
	"hardware error",
	"illegal request",
	"unit attention",
	"data protect",
	"blank check",
	"vendor specific",
	"copy aborted",
	"aborted command",
	"equal",
	"volume overflow",
	"miscompare",
	"reserved",
};

/*
 * use libdisk to read /sys/lib/scsicodes
 */
void
makesense(ScsiReq *rp)
{
	char *s;
	int i;

	Bprint(&bout, "sense data: %s", key[rp->sense[2] & 0x0F]);
	if(rp->sense[7] >= 5 && (s = scsierror(rp->sense[0xc], rp->sense[0xd])))
		Bprint(&bout, ": %s", s);
	Bprint(&bout, "\n\t");
	for(i = 0; i < 8+rp->sense[7]; i++)
		Bprint(&bout, " %2.2ux", rp->sense[i]);
	Bprint(&bout, "\n");
}
