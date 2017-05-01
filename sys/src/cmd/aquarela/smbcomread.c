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
smbcomreadandx(SmbSession *s, SmbHeader *h, uint8_t *pdata, SmbBuffer *b)
{
	uint8_t andxcommand;
	uint16_t andxoffset;
	uint32_t andxoffsetfixup;
	uint32_t datafixup;
	uint32_t bytecountfixup;
	uint16_t fid;
	SmbTree *t;
	SmbFile *f;
	int64_t offset;
	uint16_t maxcount;
	int32_t toread;
	int32_t nb;

	if (h->wordcount != 10 && h->wordcount != 12)
		return SmbProcessResultFormat;

	andxcommand = *pdata++;
	pdata++;
	andxoffset = smbnhgets(pdata); pdata += 2;
	fid = smbnhgets(pdata); pdata += 2;
	offset = smbnhgetl(pdata); pdata += 4;
	maxcount = smbnhgets(pdata); pdata += 2;
	pdata += 2;	// mincount
	pdata += 4;	// timeout ?
	pdata += 2;	// remaining
	if (h->wordcount == 12)
		offset |= (int64_t)smbnhgetl(pdata) << 32;

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		return SmbProcessResultError;
	}
	f = smbidmapfind(s->fidmap, fid);
	if (f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		return SmbProcessResultError;
	}

	if (!f->ioallowed) {
		smbseterror(s, ERRDOS, ERRbadaccess);
		return SmbProcessResultError;
	}

	h->wordcount = 12;
	if (!smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand, &andxoffsetfixup))
		return SmbProcessResultMisc;
	if (!smbbufferputs(s->response, -1)			// remaining
		|| !smbbufferputs(s->response, 0)		// datacompactionmode
		|| !smbbufferputs(s->response, 0))		// reserved
		return SmbProcessResultMisc;
	datafixup = smbbufferwriteoffset(s->response);
	if (!smbbufferputbytes(s->response, nil, 6)
		|| !smbbufferfill(s->response, 0, 8))		// reserved
		return SmbProcessResultMisc;
	bytecountfixup = smbbufferwriteoffset(s->response);
	if (!smbbufferputs(s->response, 0)
		|| !smbbufferputb(s->response, 0))
		return SmbProcessResultMisc;
	smbbufferwritelimit(s->response, smbbufferwriteoffset(s->response) + 65535);
	smbbufferoffsetputs(s->response, datafixup + 2, smbbufferwriteoffset(s->response));
	seek(f->fd, offset, 0);
	toread = smbbufferwritespace(s->response);
	if (toread > maxcount)
		toread = maxcount;
	nb = readn(f->fd, smbbufferwritepointer(s->response), toread);
	if (nb < 0) {
		smbseterror(s, ERRDOS, ERRbadaccess);
		return SmbProcessResultError;
	}
	if (!smbbufferputbytes(s->response, nil, nb)
		|| !smbbufferfixuprelatives(s->response, bytecountfixup)
		|| !smbbufferoffsetputs(s->response, datafixup, nb)
		|| !smbbufferoffsetputs(s->response, datafixup + 4, nb >> 16))
		return SmbProcessResultMisc;
	if (andxcommand != SMB_COM_NO_ANDX_COMMAND)
		return smbchaincommand(s, h, andxoffsetfixup, andxcommand, andxoffset, b);
	return SmbProcessResultReply;
}
