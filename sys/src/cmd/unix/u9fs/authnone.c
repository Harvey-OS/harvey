/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>

static char*
noneauth(Fcall *rx, Fcall *tx)
{
	USED(rx);
	USED(tx);
	return "u9fs authnone: no authentication required";
}

static char*
noneattach(Fcall *rx, Fcall *tx)
{
	USED(rx);
	USED(tx);
	return nil;
}

Auth authnone = {
	"none",
	noneauth,
	noneattach,
};
