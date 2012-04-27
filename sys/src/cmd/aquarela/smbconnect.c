#include "headers.h"

SmbClient *
smbconnect(char *to, char *share, char **errmsgp)
{
	NbSession *nbs;
	SmbBuffer *b;
	SmbHeader h, rh;
	long n;
	ushort bytecountfixupoffset;
	ushort andxfixupoffset;
	uchar *pdata;
	SmbPeerInfo peerinfo;
	ushort index;
	vlong utcintenthsofaus;
	ulong secssince1970;
	ushort bytecount;
	int x;
	MSchapreply mschapreply;
	NbName nbto;
	SmbClient *c;
	char namebuf[100];
	ushort ipctid, sharetid;

	nbmknamefromstringandtype(nbto, to, 0x20);

	peerinfo.encryptionkey = nil;
	peerinfo.oemdomainname = nil;
	assert(smbglobals.nbname[0] != 0);
	nbs = nbssconnect(nbto, smbglobals.nbname);
	if (nbs == nil)
		return nil;
print("netbios session established\n");
	b = smbbuffernew(65535);
	memset(&h, 0, sizeof(h));
	h.command = SMB_COM_NEGOTIATE;
	h.flags2 = SMB_FLAGS2_KNOWS_LONG_NAMES | SMB_FLAGS2_IS_LONG_NAME | SMB_FLAGS2_UNICODE;
	h.wordcount = 0;
	h.pid = 42;
	smbbufferputheader(b, &h, &peerinfo);
	bytecountfixupoffset = smbbufferwriteoffset(b);
	smbbufferputbytes(b, nil, 2);
	smbbufferputb(b, 2);
	smbbufferputstring(b, nil, SMB_STRING_ASCII, "NT LM 0.12");
	smbbufferoffsetputs(b, bytecountfixupoffset, smbbufferwriteoffset(b) - bytecountfixupoffset - 2);
	nbdumpdata(smbbufferreadpointer(b), smbbufferwriteoffset(b));
	nbsswrite(nbs, smbbufferreadpointer(b), smbbufferwriteoffset(b));
	/*
	 * now receive a reply
	 */
	smbbufferreset(b);
	n = nbssread(nbs, smbbufferwritepointer(b), smbbufferwritespace(b));
	if (n < 0) {
		smbstringprint(errmsgp, "smbconnect: read error: %r");
		goto fail;
	}
	smbbuffersetreadlen(b, n);
	nbdumpdata(smbbufferreadpointer(b), smbbufferwriteoffset(b));
	if (!smbbuffergetandcheckheader(b, &rh, h.command, 1, &pdata, &bytecount, errmsgp))
		goto fail;
	if (!smbsuccess(&rh, errmsgp))
		goto fail;
	if (rh.wordcount == 0) {
		smbstringprint(errmsgp, "no parameters in negotiate response");
		goto fail;
	}
	index = smbnhgets(pdata); pdata += 2;
	if (index != 0) {
		smbstringprint(errmsgp, "no agreement on protocol");
		goto fail;
	}
	if (rh.wordcount != 17) {
		smbstringprint(errmsgp, "wrong number of parameters for negotiate response");
		goto fail;
	}
	peerinfo.securitymode = *pdata++;
	peerinfo.maxmpxcount = smbnhgets(pdata); pdata += 2;
	peerinfo.maxnumbervcs = smbnhgets(pdata); pdata += 2;
	peerinfo.maxbuffersize = smbnhgetl(pdata); pdata += 4;
	peerinfo.maxrawsize = smbnhgetl(pdata); pdata += 4;
	peerinfo.sessionkey = smbnhgets(pdata); pdata += 4;
	peerinfo.capabilities = smbnhgets(pdata); pdata += 4;
	utcintenthsofaus = smbnhgetv(pdata); pdata += 8;
	secssince1970 = utcintenthsofaus / 10000000 - 11644473600LL;
	peerinfo.utc =  (vlong)secssince1970 * (vlong)1000000000 + (utcintenthsofaus % 10000000) * 100;
	peerinfo.tzoff = -smbnhgets(pdata) * 60; pdata += 2;
	peerinfo.encryptionkeylength = *pdata++;
	print("securitymode: 0x%.2ux\n", peerinfo.securitymode);
	print("maxmpxcount: 0x%.4ux\n", peerinfo.maxmpxcount);
	print("maxnumbervcs: 0x%.4ux\n", peerinfo.maxnumbervcs);
	print("maxbuffersize: 0x%.8lux\n", peerinfo.maxbuffersize);
	print("maxrawsize: 0x%.8lux\n", peerinfo.maxrawsize);
	print("sessionkey: 0x%.8lux\n", peerinfo.sessionkey);
	print("capabilities: 0x%.8lux\n", peerinfo.capabilities);
	print("utc: %s(and %lld μs)\n", asctime(gmtime(peerinfo.utc / 1000000000)), peerinfo.utc % 1000000000);
	print("tzoff: %d\n", peerinfo.tzoff);
	print("encryptionkeylength: %d\n", peerinfo.encryptionkeylength);
	smberealloc(&peerinfo.encryptionkey, peerinfo.encryptionkeylength);
	if (!smbbuffergetbytes(b, peerinfo.encryptionkey, peerinfo.encryptionkeylength)) {
		smbstringprint(errmsgp, "not enough data for encryption key");
		goto fail;
	}
	print("encryptionkey: ");
	for (x = 0; x < peerinfo.encryptionkeylength; x++)
		print("%.2ux", peerinfo.encryptionkey[x]);
	print("\n");
	if (!smbbuffergetucs2(b, 0, &peerinfo.oemdomainname)) {
		smbstringprint(errmsgp, "not enough data for oemdomainname");
		goto fail;
	}
	print("oemdomainname: %s\n", peerinfo.oemdomainname);
	if (peerinfo.capabilities & CAP_EXTENDED_SECURITY) {
		smbstringprint(errmsgp, "server wants extended security");
		goto fail;
	}
	/*
	 * ok - now send SMB_COM_SESSION_SETUP_ANDX
	 * fix the flags to reflect what the peer can do
	 */
	smbbufferreset(b);
	h.command = SMB_COM_SESSION_SETUP_ANDX;
	h.wordcount = 13;
	h.flags2 &= ~SMB_FLAGS2_UNICODE;
	if (smbsendunicode(&peerinfo))
		h.flags2 |= SMB_FLAGS2_UNICODE;
	smbbufferputheader(b, &h, &peerinfo);
	smbbufferputb(b, SMB_COM_TREE_CONNECT_ANDX);
	smbbufferputb(b, 0);
	andxfixupoffset = smbbufferwriteoffset(b);
	smbbufferputs(b, 0);
	smbbufferputs(b, 0xffff);
	smbbufferputs(b, 1);
	smbbufferputs(b, 0);
	smbbufferputl(b, peerinfo.sessionkey);
	smbbufferputs(b, sizeof(mschapreply.LMresp));
	smbbufferputs(b, sizeof(mschapreply.NTresp));
	smbbufferputl(b, 0);
	smbbufferputl(b, CAP_UNICODE | CAP_LARGE_FILES);
	bytecountfixupoffset = smbbufferwriteoffset(b);
	smbbufferputs(b, 0);
	if (auth_respond(peerinfo.encryptionkey, peerinfo.encryptionkeylength,
		nil, 0,
		&mschapreply, sizeof(mschapreply), auth_getkey,
		"proto=mschap role=client server=%s", "cher") != sizeof(mschapreply)) {
		print("auth_respond failed: %r\n");
		goto fail;
	}
	smbbufferputbytes(b, &mschapreply, sizeof(mschapreply));
	smbbufferputstring(b, &peerinfo, 0, smbglobals.accountname);
	smbbufferputstring(b, &peerinfo, 0, smbglobals.primarydomain);
	smbbufferputstring(b, &peerinfo, 0, smbglobals.nativeos);
	smbbufferputstring(b, &peerinfo, 0, "");
	smbbufferoffsetputs(b, bytecountfixupoffset, smbbufferwriteoffset(b) - bytecountfixupoffset - 2);
	smbbufferalignl2(b, 2);
	smbbufferoffsetputs(b, andxfixupoffset, smbbufferwriteoffset(b));
	smbbufferputb(b, 4);
	smbbufferputb(b, SMB_COM_NO_ANDX_COMMAND);
	smbbufferputb(b, 0);
	smbbufferputs(b, 0);
	smbbufferputs(b, 0);
	smbbufferputs(b, 0);
	bytecountfixupoffset = smbbufferwriteoffset(b);
	smbbufferputs(b, 0);
	strcpy(namebuf, "\\\\");
	strcat(namebuf, to);
	strcat(namebuf, "\\IPC$");
	smbbufferputstring(b, &peerinfo, SMB_STRING_UPCASE, namebuf);
	smbbufferputstring(b, nil, SMB_STRING_ASCII, "?????");
	smbbufferoffsetputs(b, bytecountfixupoffset, smbbufferwriteoffset(b) - bytecountfixupoffset - 2);
	nbdumpdata(smbbufferreadpointer(b), smbbufferwriteoffset(b));
	nbsswrite(nbs, smbbufferreadpointer(b), smbbufferwriteoffset(b));
	smbbufferreset(b);
	n = nbssread(nbs, smbbufferwritepointer(b), smbbufferwritespace(b));
	if (n < 0) {
		smbstringprint(errmsgp, "read error: %r");
		goto fail;
	}
	smbbuffersetreadlen(b, n);
	nbdumpdata(smbbufferreadpointer(b), smbbufferwriteoffset(b));
	if (!smbbuffergetandcheckheader(b, &rh, h.command, 1, &pdata, &bytecount, errmsgp))
		goto fail;
	if (!smbsuccess(&rh, errmsgp))
		goto fail;
	h.uid = rh.uid;
	ipctid = rh.tid;
	/*
	 * now do another TREE_CONNECT if needed
	 */
	if (share) {
		smbbufferreset(b);
		h.command = SMB_COM_TREE_CONNECT_ANDX;
		h.wordcount = 4;
		h.tid = 0;
		smbbufferputheader(b, &h, &peerinfo);
		smbbufferputb(b, SMB_COM_NO_ANDX_COMMAND);
		smbbufferputb(b, 0);
		smbbufferputs(b, 0);
		smbbufferputs(b, 0);
		smbbufferputs(b, 0);
		bytecountfixupoffset = smbbufferwriteoffset(b);
		smbbufferputs(b, 0);
		strcpy(namebuf, "\\\\");
		strcat(namebuf, to);
		strcat(namebuf, "\\");
		strcat(namebuf, share);
		smbbufferputstring(b, &peerinfo, SMB_STRING_UPCASE, namebuf);
		smbbufferputstring(b, nil, SMB_STRING_ASCII, "A:");
		smbbufferoffsetputs(b, bytecountfixupoffset, smbbufferwriteoffset(b) - bytecountfixupoffset - 2);
		nbdumpdata(smbbufferreadpointer(b), smbbufferwriteoffset(b));
		nbsswrite(nbs, smbbufferreadpointer(b), smbbufferwriteoffset(b));
		smbbufferreset(b);
		n = nbssread(nbs, smbbufferwritepointer(b), smbbufferwritespace(b));
		if (n < 0) {
			smbstringprint(errmsgp, "read error: %r");
			goto fail;
		}
		smbbuffersetreadlen(b, n);
		nbdumpdata(smbbufferreadpointer(b), smbbufferwriteoffset(b));
		if (!smbbuffergetandcheckheader(b, &rh, h.command, 3, &pdata, &bytecount, errmsgp))
			goto fail;
		if (!smbsuccess(&rh, errmsgp))
			goto fail;
		sharetid = rh.tid;
	}
	else
		sharetid = -2;
	c = smbemalloc(sizeof(*c));
	c->peerinfo = peerinfo;
	c->ipctid = ipctid;
	c->sharetid = sharetid;
	c->b = b;
	c->protoh = h;
	c->nbss = nbs;
	return c;
fail:
	smbbufferfree(&b);
	free(peerinfo.encryptionkey);
	free(peerinfo.oemdomainname);
	return nil;
}

void
smbclientfree(SmbClient *c)
{
	if (c) {
		free(c->peerinfo.encryptionkey);
		free(c->peerinfo.oemdomainname);
		free(c);
		smbbufferfree(&c->b);
	}
}

int
smbtransactionclientsend(void *magic, SmbBuffer *ob, char **)
{
	SmbClient *c = magic;
smblogprint(-1, "sending:\n");
smblogdata(-1, smblogprint, smbbufferreadpointer(ob), smbbufferwriteoffset(ob), 256);
	return nbsswrite(c->nbss, smbbufferreadpointer(ob), smbbufferwriteoffset(ob)) == 0;
}

int
smbtransactionclientreceive(void *magic, SmbBuffer *ib, char **)
{
	long n; 
	SmbClient *c = magic;
	smbbufferreset(ib);
	n = nbssread(c->nbss, smbbufferwritepointer(ib), smbbufferwritespace(ib));
	if (n >= 0) {
		assert(smbbufferputbytes(ib, nil, n));
		return 1;
	}
	return 0;
}

