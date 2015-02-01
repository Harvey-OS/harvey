/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef enum NandEccError {
	NandEccErrorBad,
	NandEccErrorGood,
	NandEccErrorOneBit,
	NandEccErrorOneBitInEcc,
} NandEccError;

ulong nandecc(uchar buf[256]);
NandEccError nandecccorrect(uchar buf[256], ulong calcecc, ulong *storedecc,
	int reportbad);

