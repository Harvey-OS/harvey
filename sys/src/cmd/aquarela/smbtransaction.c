#include "headers.h"

typedef struct Args Args;

struct Args {
	ulong pcount;
	ulong poffset;
	ulong pdisplacement;
	ulong dcount;
	ulong doffset;
	ulong ddisplacement;
	uchar scount;
};

int
_smbtransactiondecodeprimary(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, int hasname, char **errmsgp)
{
	ushort poffset, doffset;

	if (h->wordcount < 14) {
		smbstringprint(errmsgp, "word count less than 14");
		return -1;
	}
	t->in.scount = pdata[13 * 2];
	if (h->wordcount != 14 + t->in.scount) {
		smbstringprint(errmsgp, "smbcomtransaction: word count invalid\n");
		return -1;
	}
	t->in.tpcount = smbnhgets(pdata); pdata += 2;
	t->in.tdcount = smbnhgets(pdata); pdata += 2;
	t->in.maxpcount = smbnhgets(pdata); pdata += 2;
	t->in.maxdcount = smbnhgets(pdata); pdata += 2;
	t->in.maxscount = *pdata++;
	pdata++;
	t->in.flags = smbnhgets(pdata); pdata += 2;
	pdata += 4; /* timeout */
	pdata += 2;
	t->in.pcount = smbnhgets(pdata); pdata += 2;
	poffset = smbnhgets(pdata); pdata += 2;
	t->in.dcount = smbnhgets(pdata); pdata += 2;
	doffset = smbnhgets(pdata); pdata += 2;
	pdata++; /* scount */
	pdata++; /* reserved */
	smbfree(&t->in.setup);
	if (t->in.scount) {
		int x;
		t->in.setup = smbemalloc(t->in.scount * sizeof(ushort));
		for (x = 0; x < t->in.scount; x++) {
			t->in.setup[x] = smbnhgets(pdata);
			pdata += 2;
		}
	}
	smbfree(&t->in.name);
	if (hasname && !smbbuffergetstring(b, h, SMB_STRING_PATH, &t->in.name)) {
		smbstringprint(errmsgp, "not enough bdata for name");
		return -1;
	}
	if (poffset + t->in.pcount > smbbufferwriteoffset(b)) {
		smbstringprint(errmsgp, "not enough bdata for parameters");
		return -1;
	}
	if (t->in.pcount > t->in.tpcount) {
		smbstringprint(errmsgp, "too many parameters");
		return -1;
	}
	smbfree(&t->in.parameters);
	t->in.parameters = smbemalloc(t->in.tpcount);
	memcpy(t->in.parameters, smbbufferpointer(b, poffset), t->in.pcount);
	if (doffset + t->in.dcount > smbbufferwriteoffset(b)) {
		smbstringprint(errmsgp, "not enough bdata for data");
		return -1;
	}
	if (t->in.dcount > t->in.tdcount) {
		smbstringprint(errmsgp, "too much data");
		return -1;
	}
	smbfree(&t->in.data);
	t->in.data = smbemalloc(t->in.tdcount);
	memcpy(t->in.data, smbbufferpointer(b, doffset), t->in.dcount);
	if (t->in.dcount < t->in.tdcount || t->in.pcount < t->in.tpcount)
		return 0;
	return 1;
}

int
decoderesponse(SmbTransaction *t, Args *a, SmbBuffer *b, char **errmsgp)
{
	if (t->out.tpcount > smbbufferwritemaxoffset(t->out.parameters)) {
		smbstringprint(errmsgp, "decoderesponse: too many parameters for buffer");
		return 0;
	}
	if (t->out.tdcount > smbbufferwritemaxoffset(t->out.data)) {
		smbstringprint(errmsgp, "decoderesponse: too much data for buffer");
		return 0;
	}
	if (a->pdisplacement + a->pcount > t->out.tpcount) {
		smbstringprint(errmsgp, "decoderesponse: more parameters than tpcount");
		return 0;
	}
	if (a->pdisplacement != smbbufferwriteoffset(t->out.parameters)) {
		smbstringprint(errmsgp, "decoderesponse: parameter displacement inconsistent");
		return 0;
	}
	if (a->ddisplacement + a->dcount > t->out.tdcount) {
		smbstringprint(errmsgp, "decoderesponse: more data than tdcount");
		return 0;
	}
	if (a->ddisplacement != smbbufferwriteoffset(t->out.data)) {
		smbstringprint(errmsgp, "decoderesponse: data displacement inconsistent");
		return 0;
	}
	assert(a->scount == 0);
	if (a->pcount) {
		if (!smbbufferreadskipto(b, a->poffset)) {
			smbstringprint(errmsgp, "smbtransactiondecoderesponse: invalid parameter offset");
			return 0;
		}
		if (!smbbuffercopy(t->out.parameters, b, a->pcount)) {
			smbstringprint(errmsgp, "smbtransactiondecoderesponse: not enough data for parameters");
			return 0;
		}
	}
	if (a->dcount) {
		if (!smbbufferreadskipto(b, a->doffset)) {
			smbstringprint(errmsgp, "smbtransactiondecoderesponse: invalid data offset");
			return 0;
		}
		if (!smbbuffercopy(t->out.data, b, a->dcount)) {
			smbstringprint(errmsgp, "smbtransactiondecoderesponse: not enough data for data");
			return 0;
		}
	}
	return 1;
}

int
smbtransactiondecoderesponse(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp)
{
	Args a;

	if (h->command != SMB_COM_TRANSACTION) {
		smbstringprint(errmsgp, "smbtransactiondecoderesponse: not an SMB_COM_TRANSACTION");
		return 0;
	}
	if (h->wordcount < 10) {
		smbstringprint(errmsgp, "smbtransactiondecoderesponse: word count less than 10");
		return -1;
	}
	t->out.tpcount = smbnhgets(pdata); pdata += 2;
	t->out.tdcount = smbnhgets(pdata); pdata += 2;
	pdata += 2;
	a.pcount = smbnhgets(pdata); pdata += 2;
	a.poffset =  smbnhgets(pdata); pdata += 2;
	a.pdisplacement = smbnhgets(pdata); pdata += 2;
	a.dcount = smbnhgets(pdata); pdata += 2;
	a.doffset =  smbnhgets(pdata); pdata += 2;
	a.ddisplacement = smbnhgets(pdata); pdata += 2;
	a.scount = *pdata;
	if (a.scount != h->wordcount - 10) {
		smbstringprint(errmsgp, "smbtransactiondecoderesponse: scount inconsistent");
		return 0;
	}
	return decoderesponse(t, &a, b, errmsgp);
}

int
smbtransactiondecoderesponse2(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp)
{
	Args a;

	if (h->command != SMB_COM_TRANSACTION2) {
		smbstringprint(errmsgp, "smbtransactiondecoderesponse2: not an SMB_COM_TRANSACTION2");
		return 0;
	}
	if (h->wordcount < 10) {
		smbstringprint(errmsgp, "smbtransactiondecoderesponse2: word count less than 10");
		return -1;
	}
	t->out.tpcount = smbnhgets(pdata); pdata += 2;
	t->out.tdcount = smbnhgets(pdata); pdata += 2;
	pdata += 2;
	a.pcount = smbnhgets(pdata); pdata += 2;
	a.poffset =  smbnhgets(pdata); pdata += 2;
	a.pdisplacement = smbnhgets(pdata); pdata += 2;
	a.dcount = smbnhgets(pdata); pdata += 2;
	a.doffset =  smbnhgets(pdata); pdata += 2;
	a.ddisplacement = smbnhgets(pdata); pdata += 2;
	a.scount = *pdata;
	if (a.scount != h->wordcount - 10) {
		smbstringprint(errmsgp, "smbtransactiondecoderesponse2: scount inconsistent");
		return 0;
	}
	return decoderesponse(t, &a, b, errmsgp);
}

int
smbtransactiondecodeprimary(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp)
{
	return _smbtransactiondecodeprimary(t, h, pdata, b, 1, errmsgp);
}

int
smbtransactiondecodeprimary2(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp)
{
	return _smbtransactiondecodeprimary(t, h, pdata, b, 0, errmsgp);
}

void
smbtransactionfree(SmbTransaction *t)
{
	free(t->in.parameters);
	free(t->in.data);
	free(t->in.setup);
	free(t->in.name);
	smbbufferfree(&t->out.parameters);
	smbbufferfree(&t->out.data);
}

static int
_transactionencodeprimary(SmbTransaction *t, uchar cmd, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	uchar *wordcountp, ushort *bytecountp, char **errmsgp)
{
	SmbHeader mh;
	ulong countsfixupoffset, bytecountfixupoffset;
	int x;
	mh = *h;
	*wordcountp = mh.wordcount = 14 + t->in.scount;
	mh.flags &= ~SMB_FLAGS_SERVER_TO_REDIR;
	mh.command = cmd;
	if (!smbbufferputheader(ob, &mh, p)) {
	toosmall:
		smbstringprint(errmsgp, "output buffer too small");
		return 0;
	}
	if (t->in.tpcount > 65535 || t->in.tdcount > 65535 || t->in.maxpcount > 65535 || t->in.maxdcount > 65535) {
		smbstringprint(errmsgp, "counts too big");
		return 0;
	}
	if (!smbbufferputs(ob, t->in.tpcount)
		|| !smbbufferputs(ob, t->in.tdcount)
		|| !smbbufferputs(ob, t->in.maxpcount)
		|| !smbbufferputs(ob, t->in.maxdcount)
		|| !smbbufferputb(ob, t->in.maxscount)
		|| !smbbufferputb(ob, 0)
		|| !smbbufferputs(ob, t->in.flags)
		|| !smbbufferputl(ob, 0)
		|| !smbbufferputs(ob, 0))
		goto toosmall;
	countsfixupoffset = smbbufferwriteoffset(ob);
	if (!smbbufferputs(ob, 0)
		|| !smbbufferputs(ob, 0)
		|| !smbbufferputs(ob, 0)
		|| !smbbufferputs(ob, 0))
		goto toosmall;
	if (!smbbufferputb(ob, t->in.scount)
		|| !smbbufferputb(ob, 0))
		goto toosmall;
	for (x = 0; x < t->in.scount; x++)
		if (!smbbufferputs(ob, t->in.setup[x]))
			goto toosmall;
	bytecountfixupoffset = smbbufferwriteoffset(ob);
	if (!smbbufferputs(ob, 0))
		goto toosmall;
	smbbufferwritelimit(ob, smbbufferwriteoffset(ob) + 65535);
	if (!smbbufferputstring(ob, p, SMB_STRING_UPCASE, t->in.name))
		goto toosmall;
	if (t->in.pcount < t->in.tpcount) {
		ulong align = smbbufferwriteoffset(ob) & 1;
		ulong pthistime;
		pthistime = smbbufferwritespace(ob) - align;
		if (pthistime > t->in.tpcount - t->in.pcount)
			pthistime = t->in.tpcount - t->in.pcount;
		if (pthistime > 65535)
			pthistime = 65535;
		if (smbbufferwriteoffset(ob) > 65535)
			pthistime = 0;
		if (pthistime) {
			assert(smbbufferalignl2(ob, 0));
			assert(smbbufferoffsetputs(ob, countsfixupoffset, pthistime));
			assert(smbbufferoffsetputs(ob, countsfixupoffset + 2, smbbufferwriteoffset(ob)));
			assert(smbbufferputbytes(ob, t->in.parameters + t->in.pcount, pthistime));
		}
		t->in.pcount += pthistime;
	}
	if (t->in.dcount < t->in.tdcount) {
		ulong align = smbbufferwriteoffset(ob) & 1;
		ulong dthistime;
		dthistime = smbbufferwritespace(ob) - align;
		if (dthistime > t->in.tdcount - t->in.dcount)
			dthistime = t->in.tdcount - t->in.dcount;
		if (dthistime > 65535)
			dthistime = 65535;
		if (smbbufferwriteoffset(ob) > 65535)
			dthistime = 0;
		if (dthistime) {
			assert(smbbufferalignl2(ob, 0));
			assert(smbbufferoffsetputs(ob, countsfixupoffset + 4, dthistime));
			assert(smbbufferoffsetputs(ob, countsfixupoffset + 6, smbbufferwriteoffset(ob)));
			assert(smbbufferputbytes(ob, t->in.data + t->in.dcount, dthistime));
		}
		t->in.dcount += dthistime;
	}
	*bytecountp = smbbufferwriteoffset(ob) - bytecountfixupoffset - 2;
	assert(smbbufferoffsetputs(ob, bytecountfixupoffset, *bytecountp));
	return 1;
}

int
smbtransactionencodeprimary(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	uchar *wordcountp, ushort *bytecountp, char **errmsgp)
{
	return _transactionencodeprimary(t, SMB_COM_TRANSACTION, h, p,ob, wordcountp, bytecountp, errmsgp);
};

int
smbtransactionencodeprimary2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob,
	uchar *wordcountp, ushort *bytecountp, char **errmsgp)
{
	return _transactionencodeprimary(t, SMB_COM_TRANSACTION2, h, p,ob, wordcountp, bytecountp, errmsgp);
};

int
_transactionencoderesponse(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, uchar cmd,
	char **errmsgp)
{
	SmbHeader mh;
	ulong countsfixupoffset, bytecountfixupoffset;
	int palign, dalign;
	ulong pbytecount, dbytecount;
	ulong poffset, doffset;

	if (t->in.maxpcount > 65535 || t->in.maxdcount > 65535) {
		smbstringprint(errmsgp, "counts too big");
		return 0;
	}
	mh = *h;
	mh.wordcount = 10;
	mh.flags &= ~SMB_FLAGS_SERVER_TO_REDIR;
	mh.command = cmd;
	mh.errclass = SUCCESS;
	mh.error = SUCCESS;
	if (!smbbufferputheader(ob, &mh, p)
		|| !smbbufferputs(ob, smbbufferwriteoffset(t->out.parameters))
		|| !smbbufferputs(ob, smbbufferwriteoffset(t->out.data))
		|| !smbbufferputs(ob, 0)) {
	toosmall:
		smbstringprint(errmsgp, "output buffer too small");
		goto toosmall;
	}
	countsfixupoffset = smbbufferwriteoffset(ob);
	if (!smbbufferputbytes(ob, nil, 6 * sizeof(ushort))
		|| !smbbufferputb(ob, 0)	// scount == 0
		|| !smbbufferputb(ob, 0))	// reserved2
		goto toosmall;
	/* now the byte count */
	bytecountfixupoffset = smbbufferwriteoffset(ob);
	if (!smbbufferputs(ob, 0))
		goto toosmall;
	smbbufferwritelimit(ob, smbbufferwriteoffset(ob) + 65535);
	palign = bytecountfixupoffset & 1;
	if (palign && !smbbufferputb(ob, 0))
		goto toosmall;
	pbytecount = smbbufferreadspace(t->out.parameters);
	if (pbytecount > smbbufferwritespace(ob))
		pbytecount = smbbufferwritespace(ob);
	poffset = smbbufferwriteoffset(ob);
	if (poffset > 65535)
		goto toosmall;
	if (!smbbufferputbytes(ob, smbbufferreadpointer(t->out.parameters), pbytecount))
		goto toosmall;
	dalign = smbbufferwritespace(ob) > 0 && (smbbufferwriteoffset(ob) & 1) != 0;
	if (dalign && !smbbufferputb(ob, 0))
		goto toosmall;
	dbytecount = smbbufferreadspace(t->out.data);
	if (dbytecount > smbbufferwritespace(ob))
		dbytecount = smbbufferwritespace(ob);
	doffset = smbbufferwriteoffset(ob);
	if (doffset > 65535)
		goto toosmall;
	if (!smbbufferputbytes(ob, smbbufferreadpointer(t->out.data), dbytecount))
		goto toosmall;
	if (!smbbufferoffsetputs(ob, bytecountfixupoffset, palign + pbytecount + dalign + dbytecount)
		|| !smbbufferoffsetputs(ob, countsfixupoffset, pbytecount)
		|| !smbbufferoffsetputs(ob, countsfixupoffset + 2, poffset)
		|| !smbbufferoffsetputs(ob, countsfixupoffset + 4, smbbufferreadoffset(t->out.parameters))
		|| !smbbufferoffsetputs(ob, countsfixupoffset + 6, dbytecount)
		|| !smbbufferoffsetputs(ob, countsfixupoffset + 8, doffset)
		|| !smbbufferoffsetputs(ob, countsfixupoffset + 10, smbbufferreadoffset(t->out.data)))
		goto toosmall;
	assert(smbbufferoffsetputs(ob, bytecountfixupoffset, smbbufferwriteoffset(ob) - bytecountfixupoffset - 2));
	smbbuffergetbytes(t->out.parameters, nil, pbytecount);
	smbbuffergetbytes(t->out.data, nil, dbytecount);
	return 1;
}

int
smbtransactionencoderesponse(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp)
{
	return _transactionencoderesponse(t, h, p, ob, SMB_COM_TRANSACTION, errmsgp);
}

int
smbtransactionencoderesponse2(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, char **errmsgp)
{
	return _transactionencoderesponse(t, h, p, ob, SMB_COM_TRANSACTION2, errmsgp);
}

int
smbtransactionrespond(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *ob, SmbTransactionMethod *method, void *magic, char **errmsgp)
{
	/* generate one or more responses */
	while (smbbufferreadspace(t->out.parameters) || smbbufferreadspace(t->out.data)) {
		assert(method->encoderesponse);
		if (!(*method->encoderesponse)(t, h, p, ob, errmsgp))
			return 0;
		assert(method->sendresponse);
		if (!(*method->sendresponse)(magic, ob, errmsgp))
			return 0;
	}
	return 1;
}

int
smbtransactionnbdgramsend(void *magic, SmbBuffer *ob, char **errmsgp)
{
	NbDgramSendParameters *p = magic;
//print("sending to %B\n", p->to);
//nbdumpdata(smbbufferreadpointer(ob), smbbufferreadspace(ob));
	if (!nbdgramsend(p, smbbufferreadpointer(ob), smbbufferreadspace(ob))) {
		smbstringprint(errmsgp, "dgram send failed");
		return 0;
	}
	return 1;
}

SmbTransactionMethod smbtransactionmethoddgram = {
	.encodeprimary = smbtransactionencodeprimary,
	.sendrequest = smbtransactionnbdgramsend,
	.encoderesponse = smbtransactionencoderesponse,
};

int
smbtransactionexecute(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p, SmbBuffer *iob, SmbTransactionMethod *method, void *magic, SmbHeader *rhp, char **errmsgp)
{
	uchar sentwordcount;
	ushort sentbytecount;
	SmbHeader rh;
	smbbufferreset(iob);
	if (!(*method->encodeprimary)(t, h, p, iob, &sentwordcount, &sentbytecount, errmsgp))
		return 0;
//	smblogprint(-1, "sent...\n");
//	smblogdata(-1, smblogprint, smbbufferreadpointer(iob), smbbufferreadspace(iob));
	if (!(*method->sendrequest)(magic, iob, errmsgp))
		return 0;
	if (t->in.pcount < t->in.tpcount || t->in.dcount < t->in.tdcount) {
		uchar wordcount;
		ushort bytecount;
		/* secondary needed */
		if (method->encodesecondary == nil || method->receiveintermediate == nil) {
			smbstringprint(errmsgp, "buffer too small and secondaries not allowed");
			return 0;
		}
		if (!(*method->receiveintermediate)(magic, &wordcount, &bytecount, errmsgp))
			return 0;
		if (sentwordcount != wordcount || sentbytecount != bytecount) {
			smbstringprint(errmsgp, "server intermediate reply counts differ");
			return 0;
		}
		do {
			if (!(*method->encodesecondary)(t, h, iob, errmsgp))
				return 0;
			if (!(*method->sendrequest)(magic, iob, errmsgp))
				return 0;
		} while (t->in.pcount < t->in.tpcount || t->in.dcount < t->in.tdcount);
	}
	if (method->receiveresponse == nil || method->decoderesponse == nil)
		return 1;
	do {
		uchar *pdata;
		ushort bytecount;

		if (!(*method->receiveresponse)(magic, iob, errmsgp))
			return 0;
		if (!smbbuffergetheader(iob, &rh, &pdata, &bytecount)) {
			smbstringprint(errmsgp, "smbtransactionexecute: invalid response header");
			return 0;
		}
		if (!smbcheckheaderdirection(&rh, 1, errmsgp))
			return 0;
		if (rh.errclass != SUCCESS) {
			smbstringprint(errmsgp, "smbtransactionexecute: remote error %d/%d", rh.errclass, rh.error);
			return 0;
		}
		if (!smbbuffertrimreadlen(iob, bytecount)) {
			smbstringprint(errmsgp, "smbtransactionexecute: invalid bytecount");
			return 0;
		}
//		smblogprint(-1, "received...\n");
//		smblogdata(-1, smblogprint, smbbufferreadpointer(iob), smbbufferreadspace(iob));
		if (!(*method->decoderesponse)(t, &rh, pdata, iob, errmsgp))
			return 0;
	} while (smbbufferwriteoffset(t->out.parameters) < t->out.tpcount || smbbufferwriteoffset(t->out.data) < t->out.tdcount);
	if (rhp)
		*rhp = rh;
	return 1;
}

