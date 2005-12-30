#include "headers.h"

SmbProcessResult
smbcomtreedisconnect(SmbSession *s, SmbHeader *h, uchar *, SmbBuffer *)
{
	if (!smbcheckwordcount("comtreedisconnect", h, 0))
		return SmbProcessResultFormat;
	smbtreedisconnectbyid(s, h->tid);
	return smbbufferputack(s->response, h, &s->peerinfo);
}
