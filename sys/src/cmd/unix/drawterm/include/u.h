/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "dtos.h"

/* avoid name conflicts */
#undef accept
#undef listen

/* sys calls */
#undef bind
#undef chdir
#undef close
#undef create
#undef dup
#undef export
#undef fstat
#undef fwstat
#undef mount
#undef open
#undef start
#undef read
#undef remove
#undef seek
#undef stat
#undef write
#undef wstat
#undef unmount
#undef pipe
