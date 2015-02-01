/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

SmbProcessResult
smbcomfindclose2(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *)
{
	ushort sid;
	if (!smbcheckwordcount("comfindclose2", h, 1))
		return SmbProcessResultFormat;
	sid = smbnhgets(pdata);
	smbsearchclosebyid(s, sid);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
