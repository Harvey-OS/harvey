/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

static int
sendresponse(void *magic, SmbBuffer *, char **errmsgp)
{
	int rv;
	SmbSession *s = magic;
	rv = smbresponsesend(s);
	if (rv < 0) {
		smbstringprint(errmsgp, "sendresponse failed");
		return 0;
	}
	return 1;
}

SmbTransactionMethod smbtransactionmethod = {
	.encoderesponse = smbtransactionencoderesponse,
	.sendresponse = sendresponse,
};

SmbTransactionMethod smbtransactionmethod2 = {
	.encoderesponse = smbtransactionencoderesponse2,
	.sendresponse = sendresponse,
};

int
smbcomtransaction(SmbSession *s, SmbHeader *h, uint8_t *pdata, SmbBuffer *b)
{
	int rv;
	char *errmsg;
	SmbProcessResult pr = SmbProcessResultDie;
	errmsg = nil;
	rv = smbtransactiondecodeprimary(&s->transaction, h, pdata, b, &errmsg);
	if (rv < 0) {
		pr = SmbProcessResultFormat;
		goto done;
	}
	if (rv == 0) {
		h->wordcount = 0;
		if (smbbufferputack(s->response, h, &s->peerinfo)) {
			pr = SmbProcessResultReply;
			s->nextcommand = SMB_COM_TRANSACTION_SECONDARY;
		}
		goto done;
	}
	smblogprint(h->command, "smbcomtransaction: %s scount %u tpcount %lu tdcount %lu maxscount %lu maxpcount %lu maxdcount %lu\n",
		s->transaction.in.name, s->transaction.in.scount, s->transaction.in.tpcount, s->transaction.in.tdcount,
		s->transaction.in.maxscount, s->transaction.in.maxpcount, s->transaction.in.maxdcount);
	smbbufferfree(&s->transaction.out.parameters);
	smbbufferfree(&s->transaction.out.data);
	s->transaction.out.parameters = smbbuffernew(s->transaction.in.maxpcount);
	s->transaction.out.data = smbbuffernew(s->transaction.in.maxdcount);
	if (strcmp(s->transaction.in.name, smbglobals.pipelanman) == 0)
		pr = smbrap2(s);
	else {
		smbseterror(s, ERRDOS, ERRbadpath);
		pr = SmbProcessResultError;
		goto done;
	}
	if (pr == SmbProcessResultReply) {
		char *errmsg;
		errmsg = nil;
		rv = smbtransactionrespond(&s->transaction, h, &s->peerinfo, s->response, &smbtransactionmethod, s, &errmsg);
		if (!rv) {
			smblogprint(h->command, "smbcomtransaction: failed: %s\n", errmsg);
			pr = SmbProcessResultMisc;
		}
		else
			pr = SmbProcessResultOk;
	}
done:
	free(errmsg);
	return pr;
}

int
smbcomtransaction2(SmbSession *s, SmbHeader *h, uint8_t *pdata, SmbBuffer *b)
{
	int rv;
	char *errmsg;
	SmbProcessResult pr = SmbProcessResultDie;
	uint16_t op;

	errmsg = nil;
	rv = smbtransactiondecodeprimary2(&s->transaction, h, pdata, b, &errmsg);
	if (rv < 0) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}
	if (rv == 0) {
		h->wordcount = 0;
		if (smbbufferputack(s->response, h, &s->peerinfo)) {
			pr = SmbProcessResultReply;
			s->nextcommand = SMB_COM_TRANSACTION2_SECONDARY;
		}
		goto done;
	}
	smblogprint(h->command, "smbcomtransaction2: scount %u tpcount %lu tdcount %lu maxscount %lu maxpcount %lu maxdcount %lu\n",
		s->transaction.in.scount, s->transaction.in.tpcount, s->transaction.in.tdcount,
		s->transaction.in.maxscount, s->transaction.in.maxpcount, s->transaction.in.maxdcount);
	smbbufferfree(&s->transaction.out.parameters);
	smbbufferfree(&s->transaction.out.data);
	s->transaction.out.parameters = smbbuffernew(s->transaction.in.maxpcount);
	s->transaction.out.data = smbbuffernew(s->transaction.in.maxdcount);
	if (s->transaction.in.scount != 1)
		goto fmtfail;
	op = s->transaction.in.setup[0];
	if (op >= smbtrans2optablesize || smbtrans2optable[op].name == nil) {
		smblogprint(-1, "smbcomtransaction2: function %d unknown\n", op);
		pr = SmbProcessResultUnimp;
		goto done;
	}
	if (smbtrans2optable[op].process == nil) {
		smblogprint(-1, "smbcomtransaction2: %s unimplemented\n", smbtrans2optable[op].name);
		pr = SmbProcessResultUnimp;
		goto done;
	}
	pr = (*smbtrans2optable[op].process)(s, h);
	if (pr == SmbProcessResultReply) {
		char *errmsg;
		errmsg = nil;
		rv = smbtransactionrespond(&s->transaction, h, &s->peerinfo, s->response, &smbtransactionmethod2, s, &errmsg);
		if (!rv) {
			smblogprint(h->command, "smbcomtransaction2: failed: %s\n", errmsg);
			pr = SmbProcessResultMisc;
		}
		else
			pr = SmbProcessResultOk;
	}
done:
	free(errmsg);
	return pr;
}



