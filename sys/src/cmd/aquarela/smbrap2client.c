#include "headers.h"

static SmbTransactionMethod smbtransactionmethodrap = {
	.encodeprimary = smbtransactionencodeprimary,
	.sendrequest = smbtransactionclientsend,
	.receiveresponse = smbtransactionclientreceive,
	.decoderesponse = smbtransactiondecoderesponse,
};

int
smbclientrap(SmbClient *c, SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata, char **errmsgp)
{
	SmbTransaction transaction;
	SmbHeader h;
	memset(&transaction, 0, sizeof(transaction));
	transaction.in.name = smbglobals.pipelanman;
	transaction.in.parameters = smbbufferreadpointer(inparam);
	transaction.in.tpcount = smbbufferreadspace(inparam);
	transaction.in.maxpcount = smbbufferwritespace(outparam);
	transaction.in.maxdcount = smbbufferwritespace(outdata);
	transaction.out.parameters = outparam;
	transaction.out.data = outdata;
	h = c->protoh;
	h.tid = c->ipctid;
	h.mid = 0;
	return smbtransactionexecute(&transaction, &h, &c->peerinfo, c->b, &smbtransactionmethodrap, c, nil, errmsgp);
}

int
smbnetserverenum2(SmbClient *c, ulong stype, char *domain, int *entriesp, SmbRapServerInfo1 **sip, char **errmsgp)
{
	int rv;
	ushort ec, entries, total, converter;
	SmbRapServerInfo1 *si = nil;
	SmbBuffer *ipb = smbbuffernew(512);
	SmbBuffer *odb = smbbuffernew(65535);
	SmbBuffer *opb = smbbuffernew(8);
	smbbufferputs(ipb, 104);
	smbbufferputstring(ipb, nil, SMB_STRING_ASCII, "WrLehDz");
	smbbufferputstring(ipb, nil, SMB_STRING_ASCII, "B16BBDz");
	smbbufferputs(ipb, 1);
	smbbufferputs(ipb, smbbufferwritespace(odb));
	smbbufferputl(ipb, stype);
	smbbufferputstring(ipb, nil, SMB_STRING_ASCII, domain);
	rv = !smbclientrap(c, ipb, opb, odb, errmsgp);
	smbbufferfree(&ipb);
	if (rv == 0) {
		char *remark, *eremark;
		int remarkspace;
		int i;
		if (!smbbuffergets(opb, &ec)
			|| !smbbuffergets(opb, &converter)
			|| !smbbuffergets(opb, &entries)
			|| !smbbuffergets(opb, &total)) {
			smbstringprint(errmsgp, "smbnetserverenum2: not enough return parameters");
			rv = -1;
			goto done;
		}
		if (ec != 0) {
			rv = ec;
			goto done;
		}
		if (smbbufferreadspace(odb) < entries * 26) {
			smbstringprint(errmsgp, "smbnetserverenum2: not enough return data");
			rv = -1;
			goto done;
		}
		remarkspace = smbbufferreadspace(odb) - entries * 26;
		si = smbemalloc(entries * sizeof(SmbRapServerInfo1) + remarkspace);
		remark = (char *)&si[entries];
		eremark = remark + remarkspace;
		for (i = 0; i < entries; i++) {
			ulong offset;
			int remarklen;
			assert(smbbuffergetbytes(odb, si[i].name, 16));
			assert(smbbuffergetb(odb, &si[i].vmaj));
			assert(smbbuffergetb(odb, &si[i].vmin));
			assert(smbbuffergetl(odb, &si[i].type));
			assert(smbbuffergetl(odb, &offset));
			offset -= converter;
			if (!smbbufferoffsetcopystr(odb, offset, remark, eremark - remark, &remarklen)) {
				smbstringprint(errmsgp, "smbnetserverenum2: invalid string offset");
				rv = -1;
				goto done;
			}
			si[i].remark = remark;
			remark += remarklen;
		}
		*sip = si;
		si = nil;
		*entriesp = entries;
	}
	else
		rv = -1;	
done:
	free(si);
	smbbufferfree(&opb);
	smbbufferfree(&odb);
	return rv;
}
