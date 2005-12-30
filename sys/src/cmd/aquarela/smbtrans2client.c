#include "headers.h"

static SmbTransactionMethod method = {
	.encodeprimary = smbtransactionencodeprimary2,
	.sendrequest = smbtransactionclientsend,
	.receiveresponse = smbtransactionclientreceive,
	.decoderesponse = smbtransactiondecoderesponse2,
};

int
smbclienttrans2(SmbClient *c, uchar scount, ushort *setup, SmbBuffer *inparam, SmbBuffer *outparam, SmbBuffer *outdata, SmbHeader *rh, char **errmsgp)
{
	SmbTransaction transaction;
	SmbHeader h;
	memset(&transaction, 0, sizeof(transaction));
	transaction.in.scount = scount;
	transaction.in.setup = setup;
	transaction.in.parameters = smbbufferreadpointer(inparam);
	transaction.in.tpcount = smbbufferreadspace(inparam);
	transaction.in.maxpcount = smbbufferwritespace(outparam);
	transaction.in.maxdcount = smbbufferwritespace(outdata);
	transaction.out.parameters = outparam;
	transaction.out.data = outdata;
	h = c->protoh;
	h.tid = c->sharetid;
	h.mid = 0;
	return smbtransactionexecute(&transaction, &h, &c->peerinfo, c->b, &method, c, rh, errmsgp);
}

int
smbclienttrans2findfirst2(SmbClient *c, ushort searchcount, char *filename,
	ushort *sidp, ushort *searchcountp, ushort *endofsearchp,SmbFindFileBothDirectoryInfo *ip, char **errmsgp)
{
	int rv;
	ushort setup;
	SmbBuffer *inparam;
	SmbBuffer *outparam;
	SmbBuffer *outdata;
	SmbHeader rh;
	setup = SMB_TRANS2_FIND_FIRST2;
	inparam = smbbuffernew(512);
	smbbufferputs(inparam, 0x16);
	smbbufferputs(inparam, searchcount);
	smbbufferputs(inparam, 7);
	smbbufferputs(inparam, SMB_FIND_FILE_BOTH_DIRECTORY_INFO);
	smbbufferputl(inparam, 0);
	smbbufferputstring(inparam, &c->peerinfo, 0, filename);
	outparam = smbbuffernew(10);
	outdata = smbbuffernew(65535);
	rv = smbclienttrans2(c, 1, &setup, inparam, outparam, outdata, &rh, errmsgp);
	smbbufferfree(&inparam);
	if (rv) {
		ushort eaerroroffset, lastnameoffset;
		ulong nextentry;
		int i;

		if (!smbbuffergets(outparam, sidp)
			|| !smbbuffergets(outparam, searchcountp)
			|| !smbbuffergets(outparam, endofsearchp)
			|| !smbbuffergets(outparam, &eaerroroffset)
			|| !smbbuffergets(outparam, &lastnameoffset)) {
			smbstringprint(errmsgp, "smbclienttrans2findfirst2: not enough parameters returned");
			rv = 0;
			goto done;
		}
		nextentry = 0;
smblogprint(-1, "returned data:\n");
smblogdata(-1, smblogprint, smbbufferreadpointer(outdata), smbbufferreadspace(outdata), 256);
		for (i = 0; i < *searchcountp; i++) {
			SmbFindFileBothDirectoryInfo *info = ip + i;
			ulong neo, filenamelength, easize;
			uchar shortnamelength;
			if (i && !smbbufferreadskipto(outdata, nextentry)) {
			underflow:
				smbstringprint(errmsgp, "smbclientrans2findfirst2: not enough data returned");
				rv = 0;
				goto done;
			}
			if (!smbbuffergetl(outdata, &neo))
				goto underflow;
			nextentry = smbbufferreadoffset(outdata) + neo - 4;
print("neo 0x%.8lux\n", neo);
			if (!smbbuffergetl(outdata, &info->fileindex)
				|| !smbbuffergetv(outdata, &info->creationtime)
				|| !smbbuffergetv(outdata, &info->lastaccesstime)
				|| !smbbuffergetv(outdata, &info->lastwritetime)
				|| !smbbuffergetv(outdata, &info->changetime)
				|| !smbbuffergetv(outdata, &info->endoffile)
				|| !smbbuffergetv(outdata, &info->allocationsize))
				goto underflow;
print("got here\n");
			if (!smbbuffergetl(outdata, &info->extfileattributes)
				|| !smbbuffergetl(outdata, &filenamelength)
				|| !smbbuffergetl(outdata, &easize)
				|| !smbbuffergetb(outdata, &shortnamelength)
				|| !smbbuffergetbytes(outdata, nil, 1)
				|| !smbbuffergetbytes(outdata, nil, 24)
				|| !smbbuffergetstring(outdata, &rh, SMB_STRING_REVPATH, &info->filename))
				goto underflow;
print("got here as well\n");
		}
	}
done:
	smbbufferfree(&outparam);
	smbbufferfree(&outdata);
	return rv;
}

