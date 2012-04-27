#include "headers.h"

SmbProcessResult
smbcomclose(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *)
{
	SmbTree *t;
	SmbFile *f;
	ushort fid;
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
