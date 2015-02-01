/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

extern int debug;

enum {
	DBGSERVER	= 0x01,
	DBGCONTROL	= 0x02,
	DBGCTLGRP	= 0x04,
	DBGPICKLE	= 0x08,
	DBGSTATE	= 0x10,
	DBGPLAY		= 0x20,
	DBGPUMP		= 0x40,
	DBGTHREAD	= 0x80,
	DBGFILEDUMP	= 0x100,
};
