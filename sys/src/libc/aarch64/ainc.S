// This file is part of the Harvey operating system.  It is subject to the
// license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
// directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
//
// No part of Harvey operating system, including this file, may be copied,
// modified, propagated, or distributed except according to the terms
// contained in the LICENSE.gpl file.

.text

.globl ainc
ainc:	LDXR	x1, [x0]
	ADD	x1, x1, #1
	STLXR	w1, x1, [x0]
	CBNZ	w1, ainc
	DMB	ISH
	RET

.globl adec
adec:	LDXR	x1, [x0]
	SUB	x1, x1, #1
	STLXR	w1, x1, [x0]
	CBNZ	w1, adec
	DMB	ISH
	RET
