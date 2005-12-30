#include "headers.h"

SmbProcessResult
smbcomecho(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	ushort echocount, e;
	if (!smbcheckwordcount("comecho", h, 1))
		return SmbProcessResultFormat;
	echocount = smbnhgets(pdata);
	for (e = 0; e < echocount; e++) {
		ulong bytecountfixupoffset;
		SmbProcessResult pr;
		if (!smbbufferputheader(s->response, h, &s->peerinfo)
			|| !smbbufferputs(s->response, e))
			return SmbProcessResultMisc;
		bytecountfixupoffset = smbbufferwriteoffset(s->response);
		if (!smbbufferputbytes(s->response, smbbufferreadpointer(b), smbbufferreadspace(b))
			|| !smbbufferfixuprelatives(s->response, bytecountfixupoffset))
			return SmbProcessResultMisc;
		pr = smbresponsesend(s);
		if (pr != SmbProcessResultOk)
			return SmbProcessResultDie;
	}
	return SmbProcessResultOk;
}
