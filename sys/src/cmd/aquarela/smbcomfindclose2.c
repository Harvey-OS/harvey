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
