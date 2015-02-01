/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "common.h"
#include "send.h"

/*
 *  Run a command to authorize or refuse entry.  Return status 0 means
 *  authorize, -1 means refuse.
 */
void
authorize(dest *dp)
{
	process *pp;
	String *errstr;

	dp->authorized = 1;
	pp = proc_start(s_to_c(dp->repl1), (stream *)0, (stream *)0, outstream(), 1, 0);
	if (pp == 0){
		dp->status = d_noforward;
		return;
	}
	errstr = s_new();
	while(s_read_line(pp->std[2]->fp, errstr))
		;
	if ((dp->pstat = proc_wait(pp)) != 0) {
		dp->repl2 = errstr;
		dp->status = d_noforward;
	} else
		s_free(errstr);
	proc_free(pp);
}
