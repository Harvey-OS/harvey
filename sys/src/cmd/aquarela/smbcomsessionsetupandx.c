#include "headers.h"
#include <mp.h>
#include <libsec.h>

SmbProcessResult
smbcomsessionsetupandx(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar andxcommand;
	ushort andxoffset;
	ulong andxfixupoffset;
	ushort vcnumber;
	ulong sessionkey;
	ushort caseinsensitivepasswordlength;
	ushort casesensitivepasswordlength;
	ushort bytecountfixup, offset;
	uchar *mschapreply;
	AuthInfo *ai;
	char *sp;
	SmbProcessResult pr;
	char *accountname = nil;
	char *primarydomain = nil;
	char *nativeos = nil;
	char *nativelanman = nil;

	if (!smbcheckwordcount("comsessionsetupandx", h, 13)) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}

	andxcommand = *pdata++;
	switch (andxcommand) {
	case SMB_COM_TREE_CONNECT_ANDX:
	case SMB_COM_OPEN_ANDX:
	case SMB_COM_CREATE_NEW:
	case SMB_COM_DELETE:
	case SMB_COM_FIND:
	case SMB_COM_COPY:
	case SMB_COM_NT_RENAME:
	case SMB_COM_QUERY_INFORMATION:
	case SMB_COM_NO_ANDX_COMMAND:
	case SMB_COM_OPEN:
	case SMB_COM_CREATE:
	case SMB_COM_CREATE_DIRECTORY:
	case SMB_COM_DELETE_DIRECTORY:
	case SMB_COM_FIND_UNIQUE:
	case SMB_COM_RENAME:
	case SMB_COM_CHECK_DIRECTORY:
	case SMB_COM_SET_INFORMATION:
	case SMB_COM_OPEN_PRINT_FILE:
		break;
	default:
		smblogprint(h->command, "smbcomsessionsetupandx: invalid andxcommand %s (0x%.2ux)\n",
			smboptable[andxcommand].name, andxcommand);
		goto fmtfail;
	}
	pdata++;
	andxoffset = smbnhgets(pdata); pdata += 2;
	s->peerinfo.maxlen = smbnhgets(pdata); pdata += 2;
	smbresponseinit(s, s->peerinfo.maxlen);
	s->client.maxmpxcount = smbnhgets(pdata); pdata += 2;
	vcnumber = smbnhgets(pdata); pdata += 2;
	sessionkey = smbnhgetl(pdata); pdata += 4;
	caseinsensitivepasswordlength = smbnhgets(pdata); pdata += 2;
	casesensitivepasswordlength = smbnhgets(pdata); pdata += 2;
	pdata += 4;
	s->peerinfo.capabilities = smbnhgetl(pdata); /*pdata += 4;*/
smbloglock();
smblogprint(h->command, "andxcommand: %s offset %ud\n", smboptable[andxcommand].name, andxoffset);
smblogprint(h->command, "client.maxbuffersize: %ud\n", s->peerinfo.maxlen);
smblogprint(h->command, "client.maxmpxcount: %ud\n", s->client.maxmpxcount);
smblogprint(h->command, "vcnumber: %ud\n", vcnumber);
smblogprint(h->command, "sessionkey: 0x%.8lux\n", sessionkey);
smblogprint(h->command, "caseinsensitivepasswordlength: %ud\n", caseinsensitivepasswordlength);
smblogprint(h->command, "casesensitivepasswordlength: %ud\n", casesensitivepasswordlength);
smblogprint(h->command, "clientcapabilities: 0x%.8lux\n", s->peerinfo.capabilities);
smblogunlock();

	mschapreply = smbbufferreadpointer(b);

	if (!smbbuffergetbytes(b, nil, caseinsensitivepasswordlength + casesensitivepasswordlength)) {
		smblogprint(h->command, "smbcomsessionsetupandx: not enough bdata for passwords\n");
		goto fmtfail;
	}
	if (!smbbuffergetstring(b, h, 0, &accountname)
		|| !smbbuffergetstring(b, h, 0, &primarydomain)
		|| !smbbuffergetstring(b, h, 0, &nativeos)
		|| !smbbuffergetstring(b, h, 0, &nativelanman)) {
		smblogprint(h->command, "smbcomsessionsetupandx: not enough bytes for strings\n");
		goto fmtfail;
	}

	for (sp = accountname; *sp; sp++)
		*sp = tolower(*sp);

smblogprint(h->command, "account: %s\n", accountname);
smblogprint(h->command, "primarydomain: %s\n", primarydomain);
smblogprint(h->command, "nativeos: %s\n", nativeos);
smblogprint(h->command, "nativelanman: %s\n", nativelanman);

	if (s->client.accountname && accountname[0] && strcmp(s->client.accountname, accountname) != 0) {
		smblogprint(h->command, "smbcomsessionsetupandx: more than one user on VC (before %s, now %s)\n",
			s->client.accountname, accountname);
		smbseterror(s, ERRSRV, ERRtoomanyuids);
	errordone:
		pr = SmbProcessResultError;
		goto done;
	}

	if (s->client.accountname == nil) {
		/* first time */
		if (accountname[0] == 0) {
			smbseterror(s, ERRSRV, ERRbaduid);
			goto errordone;
		}
		if ((casesensitivepasswordlength != 24 || caseinsensitivepasswordlength != 24)) {
			smblogprint(h->command,
				"smbcomsessionsetupandx: case sensitive/insensitive password length not 24\n");
			smbseterror(s, ERRSRV, ERRbadpw);
			goto errordone;
		}
		memcpy(&s->client.mschapreply, mschapreply, sizeof(s->client.mschapreply));
		if(s->cs == nil){
			smbseterror(s, ERRSRV, ERRerror);
			goto errordone;
		}
		s->cs->user = accountname;
		s->cs->resp = &s->client.mschapreply;
		s->cs->nresp = sizeof(MSchapreply);
		ai = auth_response(s->cs);
		if (ai == nil) {
			smblogprint(h->command, "authentication failed\n");
			smbseterror(s, ERRSRV, ERRbadpw);
			goto errordone;
		}
		smblogprint(h->command, "authentication succeeded\n");
		if (auth_chuid(ai, nil) < 0) {
			smblogprint(h->command, "smbcomsessionsetupandx: chuid failed: %r\n");
			auth_freeAI(ai);
		miscerror:
			pr = SmbProcessResultMisc;
			goto done;
		}
		auth_freeAI(ai);
		h->uid = 1;
		s->client.accountname = accountname;
		s->client.primarydomain = primarydomain;
		s->client.nativeos = nativeos;
		s->client.nativelanman = nativelanman;
		accountname = nil;
		primarydomain = nil;
		nativeos = nil;
		nativelanman = nil;
	}
	else {
		if (caseinsensitivepasswordlength == 24 && casesensitivepasswordlength == 24
			&& memcmp(&s->client.mschapreply, mschapreply, sizeof(MSchapreply)) != 0) {
			smblogprint(h->command, "second time authentication failed\n");
			smbseterror(s, ERRSRV, ERRbadpw);
			goto errordone;
		}
	}

	/* CIFS says 4 with or without extended security, samba/ms says 3 without */
	h->wordcount = 3;
	if (!smbresponseputandxheader(s, h, andxcommand, &andxfixupoffset))
		goto miscerror;
	if (!smbresponseputs(s, 0))
		goto miscerror;
	bytecountfixup = smbresponseoffset(s);
	if (!smbresponseputs(s, 0))
		goto miscerror;
	if (!smbresponseputstring(s, 1, smbglobals.nativeos)
		|| !smbresponseputstring(s, 1, smbglobals.serverinfo.nativelanman)
		|| !smbresponseputstring(s, 1, smbglobals.primarydomain))
		goto miscerror;
	offset = smbresponseoffset(s);
	smbresponseoffsetputs(s, bytecountfixup, offset - bytecountfixup - 2);
	s->state = SmbSessionEstablished;
	if (andxcommand != SMB_COM_NO_ANDX_COMMAND)
		pr = smbchaincommand(s, h, andxfixupoffset, andxcommand, andxoffset, b);
	else
		pr = SmbProcessResultReply;
done:
	free(accountname);
	free(primarydomain);
	free(nativeos);
	free(nativelanman);
	return pr;
}
