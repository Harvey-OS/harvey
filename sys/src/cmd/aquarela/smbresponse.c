#include "headers.h"

void
smbresponsereset(SmbSession *s)
{
	smbbufferreset(s->response);
}

void
smbresponseinit(SmbSession *s, ushort maxlen)
{
	smbbufferfree(&s->response);
	s->response = smbbuffernew(maxlen);
}

int
smbresponsealignl2(SmbSession *s, int l2a)
{
	return smbbufferalignl2(s->response, l2a);
}

int
smbresponseputheader(SmbSession *s, SmbHeader *h, uchar errclass, ushort error)
{
	h->errclass = errclass;
	h->error = error;
	return smbbufferputheader(s->response, h, &s->peerinfo);
}

int
smbresponseputb(SmbSession *s, uchar b)
{
	return smbbufferputb(s->response, b);
}

ushort
smbresponsespace(SmbSession *sess)
{
	return smbbufferwritespace(sess->response);
}

int
smbresponseskip(SmbSession *sess, ushort amount)
{
	return smbbufferputbytes(sess->response, nil, amount);
}

int
smbresponseoffsetputs(SmbSession *sess, ushort offset, ushort s)
{
	return smbbufferoffsetputs(sess->response, offset, s);
}

int
smbresponseputs(SmbSession *sess, ushort s)
{
	return smbbufferputs(sess->response, s);
}

int
smbresponseputl(SmbSession *s, ulong l)
{
	return smbbufferputl(s->response, l);
}

int
smbresponsecpy(SmbSession *s, uchar *data, ushort datalen)
{
	return smbbufferputbytes(s->response, data, datalen);
}

int
smbresponseputstring(SmbSession *s, int mustalign, char *string)
{
	return smbbufferputstring(s->response, &s->peerinfo, mustalign ? 0 : SMB_STRING_UNALIGNED, string);
}

int
smbresponseputstr(SmbSession *s, char *string)
{
	return smbbufferputstring(s->response, nil, SMB_STRING_ASCII, string);
}

ushort
smbresponseoffset(SmbSession *s)
{
	return smbbufferwriteoffset(s->response);
}

SmbProcessResult
smbresponsesend(SmbSession *s)
{
	uchar cmd;
	SmbProcessResult pr;

	assert(smbbufferoffsetgetb(s->response, 4, &cmd));
smbloglock();
smblogprint(cmd, "sending:\n");
smblogdata(cmd, smblogprint, smbbufferreadpointer(s->response), smbbufferreadspace(s->response), 256);
smblogunlock();
	if (s->nbss) {
		NbScatterGather a[2];
		a[0].p = smbbufferreadpointer(s->response);
		a[0].l = smbbufferreadspace(s->response);
		a[1].p = nil;
		nbssgatherwrite(s->nbss, a);
		pr = SmbProcessResultOk;
	}
	else if (s->cifss) {
		ulong l = smbbufferreadspace(s->response);
		uchar nl[4];
		hnputl(nl, l);
		write(s->cifss->fd, nl, 4);
		write(s->cifss->fd, smbbufferreadpointer(s->response), l);
		pr = SmbProcessResultOk;
	}
	else
		pr = SmbProcessResultDie;
	smbbufferreset(s->response);
	return pr;
}

int
smbresponseputandxheader(SmbSession *s, SmbHeader *h, ushort andxcommand, ulong *andxoffsetfixupp)
{
	return smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand, andxoffsetfixupp);
}

int
smbresponseputerror(SmbSession *s, SmbHeader *h, uchar errclass, ushort error)
{
	h->wordcount = 0;
	return smbresponseputheader(s, h, errclass, error)
		&& smbresponseputs(s, 0);
}
