#include "headers.h"

SmbProcessResult
smbcomreadandx(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar andxcommand;
	ushort andxoffset;
	ulong andxoffsetfixup;
	ulong datafixup;
	ulong bytecountfixup;
	ushort fid;
	SmbTree *t;
	SmbFile *f;
	vlong offset;
	ushort maxcount;
	long toread;
	long nb;

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
		offset |= (vlong)smbnhgetl(pdata) << 32;

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
