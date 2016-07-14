/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

static char *s9p2000 = "9p2000";

SmbProcessResult
smbcomtreeconnectandx(SmbSession *s, SmbHeader *h, uint8_t *pdata,
		      SmbBuffer *b)
{
	uint8_t andxcommand;
	uint16_t andxoffset;
	char *path = nil;
	char *service = nil;
	uint16_t flags;
	uint16_t passwordlength;
//	ushort bytecount;
	uint8_t errclass;
	uint16_t error;
	SmbService *serv;
	SmbTree *tree;
	uint32_t andxfixupoffset, bytecountfixup;
	SmbProcessResult pr;

	if (!smbcheckwordcount("comtreeconnectandx", h, 4)) {
	fmtfail:
		pr = SmbProcessResultFormat;
		goto done;
	}

	switch (s->state) {
	case SmbSessionNeedNegotiate:
		smblogprint(-1, "smbcomtreeconnectandx: called when negotiate expected\n");
		return SmbProcessResultUnimp;
	case SmbSessionNeedSetup:
		smbseterror(s, ERRDOS, ERRbadpw);
		return SmbProcessResultError;
	}

	andxcommand = *pdata++;
	switch (andxcommand) {
	case SMB_COM_OPEN:
	case SMB_COM_CREATE_NEW:
	case SMB_COM_DELETE_DIRECTORY:
	case SMB_COM_FIND_UNIQUE:
	case SMB_COM_CHECK_DIRECTORY:
	case SMB_COM_GET_PRINT_QUEUE:
	case SMB_COM_TRANSACTION:
	case SMB_COM_SET_INFORMATION:
	case SMB_COM_OPEN_ANDX:
	case SMB_COM_CREATE_DIRECTORY:
	case SMB_COM_FIND:
	case SMB_COM_RENAME:
	case SMB_COM_QUERY_INFORMATION:
	case SMB_COM_OPEN_PRINT_FILE:
	case SMB_COM_NO_ANDX_COMMAND:
	case SMB_COM_NT_RENAME:
	case SMB_COM_CREATE:
	case SMB_COM_DELETE:
	case SMB_COM_COPY:
		break;
	default:
		smblogprint(h->command, "smbcomtreeconnectandx: invalid andxcommand %s (0x%.2x)\n",
			smboptable[andxcommand].name, andxcommand);
		goto fmtfail;
	}
	pdata++;
	andxoffset = smbnhgets(pdata); pdata += 2;
	flags = smbnhgets(pdata); pdata += 2;
	passwordlength = smbnhgets(pdata); //pdata += 2;
//	bytecount = smbnhgets(pdata); pdata += 2;
smblogprint(h->command, "passwordlength: %u\n", passwordlength);
smblogprint(h->command, "flags: 0x%.4x\n", flags);
	if (!smbbuffergetbytes(b, nil, passwordlength)) {
		smblogprint(h->command, "smbcomtreeconnectandx: not enough bytes for password\n");
		goto fmtfail;
	}
smblogprint(h->command, "offset %lu limit %lu\n", smbbufferreadoffset(b), smbbufferwriteoffset(b));
	if (!smbbuffergetstring(b, h, SMB_STRING_PATH, &path)
		|| !smbbuffergetstr(b, 0, &service)) {
		smblogprint(h->command, "smbcomtreeconnectandx: not enough bytes for strings\n");
		goto fmtfail;
	}
smblogprint(h->command, "path: %s\n", path);
smblogprint(h->command, "service: %s\n", service);
	if (flags & 1)
		smbtreedisconnectbyid(s, h->tid);
	serv = smbservicefind(s, path, service, &errclass, &error);
	if (serv == nil) {
		pr = SmbProcessResultError;
		smbseterror(s, errclass, error);
		goto done;
	}
	tree = smbtreeconnect(s, serv);
	h->tid = tree->id;
	h->wordcount = 3;
	if (!smbresponseputandxheader(s, h, andxcommand, &andxfixupoffset)
		|| !smbresponseputs(s, 1)) {
	misc:
		pr = SmbProcessResultMisc;
		goto done;
	}
	bytecountfixup = smbresponseoffset(s);
	if (!smbresponseputs(s, 0)
		|| !smbresponseputstr(s, serv->type)
		|| !smbresponseputstring(s, 1, s9p2000))
		goto misc;
	if (!smbbufferfixuprelatives(s->response, bytecountfixup))
		goto misc;
	if (andxcommand != SMB_COM_NO_ANDX_COMMAND) {
		pr = smbchaincommand(s, h, andxfixupoffset, andxcommand, andxoffset, b);
	}
	else
		pr = SmbProcessResultReply;
done:
	free(path);
	free(service);
	return pr;
}
