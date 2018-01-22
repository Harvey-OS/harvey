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
smbcomclose(SmbSession *s, SmbHeader *h, uint8_t *pdata, SmbBuffer *sb)
{
	SmbTree *t;
	SmbFile *f;
	uint16_t fid;
	if (!smbcheckwordcount("comclose", h, 3))
		return SmbProcessResultFormat;
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	fid = smbnhgets(pdata);
	f = smbidmapfind(s->fidmap, fid);
	if (f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		return SmbProcessResultError;
	}
	smbfileclose(s, f);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
