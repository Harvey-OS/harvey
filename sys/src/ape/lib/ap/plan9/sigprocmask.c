/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <signal.h>
#include <errno.h>

sigset_t _psigblocked;

int
sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	if(oset)
		*oset = _psigblocked;
	if(how==SIG_BLOCK)
		_psigblocked |= *set;
	else if(how==SIG_UNBLOCK)
		_psigblocked &= ~*set;
	else if(how==SIG_SETMASK)
		_psigblocked = *set;
	else{
		errno = EINVAL;
		return -1;
	}
	return 0;
}
