#include "headers.h"

int
smbclientopen(SmbClient *c, ushort mode, char *name, uchar *errclassp, ushort *errorp,
	ushort *fidp, ushort *attrp, ulong *mtimep, ulong *sizep, ushort *accessallowedp, char **errmsgp)
{
	SmbBuffer *b;
	SmbHeader h;
	ulong bytecountfixup;
	long n;
	uchar *pdata;
	ushort bytecount;

	b = smbbuffernew(65535);
	h = c->protoh;
	h.tid = c->sharetid;
	h.command = SMB_COM_OPEN;
	h.wordcount = 2;
	smbbufferputheader(b, &h, &c->peerinfo);
	smbbufferputs(b, mode);
	smbbufferputs(b, 0);
	bytecountfixup = smbbufferwriteoffset(b);
	smbbufferputs(b, 0);
	smbbufferputb(b, 4);
	smbbufferputstring(b, &c->peerinfo, SMB_STRING_REVPATH, name);
	smbbufferfixuprelatives(b, bytecountfixup);
	nbsswrite(c->nbss, smbbufferreadpointer(b), smbbufferwriteoffset(b));
	smbbufferreset(b);
	n = nbssread(c->nbss, smbbufferwritepointer(b), smbbufferwritespace(b));
	if (n < 0) {
		smbstringprint(errmsgp, "read error: %r");
		smbbufferfree(&b);
		return 0;
	}
	smbbuffersetreadlen(b, n);
	if (!smbbuffergetandcheckheader(b, &h, h.command, 7, &pdata, &bytecount, errmsgp)) {
		smbbufferfree(&b);
		return 0;
	}
	if (h.errclass) {
		*errclassp = h.errclass;
		*errorp = h.error;
		smbbufferfree(&b);
		return 0;
	}
	*fidp = smbnhgets(pdata); pdata += 2;
	*attrp = smbnhgets(pdata); pdata += 2;
	*mtimep = smbnhgetl(pdata); pdata += 4;
	*sizep = smbnhgets(pdata); pdata += 4;
	*accessallowedp = smbnhgets(pdata);
	return 1;
}

