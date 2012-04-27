#include "headers.h"

int
smbmailslotsend(NbDgramSendParameters *p, SmbBuffer *msg, char **errmsgp)
{
	ushort setup[3];
	int rv;
	SmbTransaction transaction;
	SmbBuffer *b;
	SmbHeader h;
	setup[0] = 1;
	setup[1] = 0;
	setup[2] = 0;
	memset(&transaction, 0, sizeof(transaction));
	transaction.in.name = smbglobals.mailslotbrowse;
	transaction.in.scount = 3;
	transaction.in.setup = setup;
	transaction.in.tdcount = smbbufferreadspace(msg);
	transaction.in.data = smbbufferreadpointer(msg);
	b = smbbuffernew(NbDgramMaxLen);
	memset(&h, 0, sizeof(h));
	rv = smbtransactionexecute(&transaction, &h, nil, b, &smbtransactionmethoddgram, p, nil, errmsgp);
	smbbufferfree(&b);
	return rv;
}

int
smbbrowsesendhostannouncement(char *name, ulong periodms, ulong type, char *comment, char **errmsgp)
{
	NbDgramSendParameters p;
	SmbBuffer *b;
	int rv;
//	NbName msbrowse;

//	msbrowse[0] = 1;
//	msbrowse[1] = 2;
//	memcpy(msbrowse + 2, "__MSBROWSE__", 12);
//	msbrowse[14] = 2;
//	msbrowse[15] = 1;
//	nbnamecpy(p.to, msbrowse);
	nbmknamefromstringandtype(p.to, smbglobals.primarydomain, 0x1d);
	p.type = NbDgramDirectUnique;
	b = smbbuffernew(NbDgramMaxLen);
	smbbufferputb(b, 1);
	smbbufferputb(b, 0);
	smbbufferputl(b, periodms);
	smbbufferputstrn(b, name, 16, 1);
	smbbufferputb(b, 4);
	smbbufferputb(b, 0);
	smbbufferputl(b, type);
	smbbufferputl(b, 0xaa55011f);
	smbbufferputstring(b, nil, 0, comment);
	rv = smbmailslotsend(&p, b, errmsgp);
	smbbufferfree(&b);
	return rv;
}
