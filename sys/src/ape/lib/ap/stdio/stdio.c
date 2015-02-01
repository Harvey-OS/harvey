/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * pANS stdio -- data
 */
#include "iolib.h"
FILE _IO_stream[]={
/*	fd	flags	state	buf	rp	wp	lp	bufl	unbuf */
	0,	0,	OPEN,	0,	0,	0,	0,	0,	0,
	1,	0,	OPEN,	0,	0,	0,	0,	0,	0,
	2,	0,	OPEN,	0,	0,	0,	0,	0,	0,
};
