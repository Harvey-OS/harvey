/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

void
nbdgramdump(NbDgram *s)
{
	print("type 0x%.2ux flags 0x%.2ux id 0x%.4ux srcip %I port %d\n",
		s->type, s->flags, s->id, s->srcip, s->srcport);
	switch (s->type) {
	case NbDgramError:
		print("\terror.code 0x%.2ux\n", s->error.code);
		break;
	case NbDgramDirectUnique:
	case NbDgramDirectGroup:
	case NbDgramBroadcast:
		print("\tlength %ud offset %ud srcname %B dstname %B\n",
			s->datagram.length, s->datagram.offset, s->datagram.srcname, s->datagram.dstname);
		break;
	}
}
