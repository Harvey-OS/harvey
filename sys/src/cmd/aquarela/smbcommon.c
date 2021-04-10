/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

int
smbsendunicode(SmbPeerInfo *i)
{
	return smbglobals.unicode && (i == nil || (i->capabilities & CAP_UNICODE) != 0);
}

int
smbcheckwordcount(char *name, SmbHeader *h, u16 wordcount)
{
	if (h->wordcount != wordcount) {
		smblogprint(-1, "smb%s: word count not %u\n", name, wordcount);
		return 0;
	}
	return 1;
}

int
smbcheckwordandbytecount(char *name, SmbHeader *h, u16 wordcount,
			 u8 **bdatap, u8 **edatap)
{
	u16 bytecount;
	u8 *bdata;
	if (h->wordcount != wordcount) {
		smblogprint(-1, "smb%s: word count not %u\n", name, wordcount);
		return 0;
	}
	bdata = *bdatap;
	if (bdata + 2 > *edatap) {
		smblogprint(-1, "smb%s: not enough data for byte count\n", name);
		return 0;
	}
	bytecount = smbnhgets(bdata); bdata += 2;
	if (bdata + bytecount > *edatap) {
		smblogprint(-1, "smb%s: not enough data for bytes\n", name);
		return 0;
	}
	*edatap = bdata + bytecount;
	*bdatap = bdata;
	return 1;
}

int
smbchaincommand(SmbSession *s, SmbHeader *h, u32 andxoffsetfixup,
		u8 cmd, u16 offset, SmbBuffer *b)
{
	SmbOpTableEntry *ote;
	u8 *pdata;
	u16 bytecount;

	h->command = cmd;
	ote = smboptable + cmd;
	if (ote->process == nil) {
		smblogprint(-1, "smbchaincommand: %s (0x%.2x) not implemented\n", ote->name, cmd);
		return SmbProcessResultUnimp;
	}
	if (!smbresponsealignl2(s, 2)
		|| !smbresponseoffsetputs(s, andxoffsetfixup, smbresponseoffset(s))
		|| !smbbufferpopreadlimit(b))
		return SmbProcessResultMisc;
	if (!smbbufferreadskipto(b, offset)) {
		smblogprint(-1, "smbchaincommand: illegal offset\n");
		return SmbProcessResultFormat;
	}
	if (!smbbuffergetb(b, &h->wordcount)) {
		smblogprint(-1, "smbchaincommand: not enough space for wordcount\n");
		return SmbProcessResultFormat;
	}
	pdata = smbbufferreadpointer(b);
	if (!smbbuffergetbytes(b, nil, h->wordcount * 2)) {
		smblogprint(-1, "smbchaincommand: not enough space for parameters\n");
		return SmbProcessResultFormat;
	}
	if (!smbbuffergets(b, &bytecount)) {
		smblogprint(-1, "smbchaincommand: not enough space for bytecount\n");
		return SmbProcessResultFormat;
	}
	if (!smbbufferpushreadlimit(b, smbbufferreadoffset(b) + bytecount)) {
		smblogprint(-1, "smbchaincommand: not enough space for bytes\n");
		return SmbProcessResultFormat;
	}
smblogprint(cmd, "chaining to %s\n", ote->name);
	return (*ote->process)(s, h, pdata, b);
}

int
smbbuffergetheader(SmbBuffer *b, SmbHeader *h, u8 **parametersp,
		   u16 *bytecountp)
{
	SmbOpTableEntry *ote;
	SmbRawHeader *rh;
	rh = (SmbRawHeader *)smbbufferreadpointer(b);
	if (!smbbuffergetbytes(b, nil, (i32)offsetof(SmbRawHeader, parameterwords[0]))) {
		smblogprint(-1, "smbgetheader: short packet\n");
		return 0;
	}
	if (rh->protocol[0] != 0xff || memcmp(rh->protocol + 1, "SMB", 3) != 0) {
		smblogprint(-1, "smbgetheader: invalid protocol\n");
		return 0;
	}
	h->command = rh->command;
	ote = smboptable + h->command;
	if (ote->name == nil) {
		smblogprint(-1, "smbgetheader: illegal opcode 0x%.2x\n", h->command);
		return 0;
	}
	h->errclass = rh->status[0];
	h->error = smbnhgets(rh->status + 2);
	h->flags = rh->flags;
	h->flags2 = smbnhgets(rh->flags2);
	if (h->flags & ~(SmbHeaderFlagCaseless | SMB_FLAGS_SERVER_TO_REDIR | SmbHeaderFlagReserved | SmbHeaderFlagServerIgnore))
		smblogprint(-1, "smbgetheader: warning: unexpected flags 0x%.2x\n", h->flags);
	h->wordcount = rh->wordcount;
	if (parametersp)
		*parametersp = smbbufferreadpointer(b);
	if (!smbbuffergetbytes(b, nil, h->wordcount * 2)) {
		smblogprint(-1, "smbgetheader: not enough data for parameter words\n");
		return 0;
	}
	h->tid = smbnhgets(rh->tid);
	h->pid = smbnhgets(rh->pid);
	h->uid = smbnhgets(rh->uid);
	h->mid = smbnhgets(rh->mid);
	if (!smbbuffergets(b, bytecountp))
		*bytecountp = 0;
	if (!smbbufferpushreadlimit(b, smbbufferreadoffset(b) + *bytecountp))
		return 0;

smblogprint(h->command, "%s %s: tid 0x%.4x pid 0x%.4x uid 0x%.4x mid 0x%.4x\n", ote->name,
	(h->flags & SMB_FLAGS_SERVER_TO_REDIR) ? "response" : "request", h->tid, h->pid, h->uid, h->mid);
	return 1;
}

int
smbcheckheaderdirection(SmbHeader *h, int response, char **errmsgp)
{
	if (((h->flags & SMB_FLAGS_SERVER_TO_REDIR) == 0) == response) {
		smbstringprint(errmsgp, "unexpected %s", response ? "request" : "response");
		return 0;
	}
	return 1;
}

int
smbcheckheader(SmbHeader *h, u8 command, int response, char **errmsgp)
{
	if (response && h->command != command) {
		smbstringprint(errmsgp, "sent %.2uc request, got %.2x response", command, h->command);
		return 0;
	}
	if (!smbcheckheaderdirection(h, response, errmsgp))
		return 0;
	return 1;
}

int
smbbuffergetandcheckheader(SmbBuffer *b, SmbHeader *h, u8 command,
			   int response, u8 **pdatap,
			   u16 *bytecountp, char **errmsgp)
{
	if (!smbbuffergetheader(b, h, pdatap, bytecountp)) {
		smbstringprint(errmsgp, "smbbuffergetandcheckheader: not enough data for header");
		return 0;
	}
	return smbcheckheader(h, command, response, errmsgp);
}

int
smbsuccess(SmbHeader *h, char **errmsgp)
{
	if (h->errclass != SUCCESS) {
		smbstringprint(errmsgp, "%s returned error %d/%d", smboptable[h->command].name, h->errclass, h->error);
		return 0;
	}
	return 1;
}

#define BASE_FLAGS (0)

int
smbbufferputheader(SmbBuffer *b, SmbHeader *h, SmbPeerInfo *p)
{
	SmbRawHeader *rh;
	if (offsetof(SmbRawHeader, parameterwords[0]) > smbbufferwritespace(b))
		return 0;
	if (smbbufferwriteoffset(b) == 0) {
		rh = (SmbRawHeader *)smbbufferwritepointer(b);
		rh->protocol[0] = 0xff;
		memcpy(rh->protocol + 1, "SMB", 3);
		rh->flags = SMB_FLAGS_SERVER_TO_REDIR | SmbHeaderFlagCaseless;
		rh->command = h->command;
		smbhnputs(rh->flags2, BASE_FLAGS | (smbsendunicode(p) ? SMB_FLAGS2_UNICODE : 0));
		memset(rh->extra, 0, sizeof(rh->extra));
		if (!smbbufferputbytes(b, nil, offsetof(SmbRawHeader, parameterwords[0])))
			return 0;
		rh->wordcount = h->wordcount;
	}
	else {
		rh = (SmbRawHeader *)smbbufferreadpointer(b);
		smbbufferputb(b, h->wordcount);
	}
	rh->status[0] = h->errclass;
	rh->status[1] = 0;
	smbhnputs(rh->status + 2, h->error);
	smbhnputs(rh->tid, h->tid);
	smbhnputs(rh->pid, h->pid);
	smbhnputs(rh->uid, h->uid);
	smbhnputs(rh->mid, h->mid);
	return 1;
}

int
smbbufferputerror(SmbBuffer *s, SmbHeader *h, SmbPeerInfo *p,
		  u8 errclass,
		  u16 error)
{
	h->errclass = errclass;
	h->error = error;
	return smbbufferputheader(s, h, p);
}

int
smbbufferputandxheader(SmbBuffer *b, SmbHeader *h, SmbPeerInfo *p,
		       u8 andxcommand,
		       u32 *andxoffsetfixupp)
{
	if (!smbbufferputheader(b, h, p)
		|| !smbbufferputb(b, andxcommand)
		|| !smbbufferputb(b, 0))
		return 0;
	*andxoffsetfixupp = smbbufferwriteoffset(b);
	return smbbufferputbytes(b, nil, 2);
}

void
smbseterror(SmbSession *s, u8 errclass, u16 error)
{
	s->errclass = errclass;
	s->error = error;
}

int
smbbufferputack(SmbBuffer *b, SmbHeader *h, SmbPeerInfo *p)
{
	h->wordcount = 0;
	return smbbufferputheader(b, h, p) && smbbufferputs(b, 0) ? SmbProcessResultReply : SmbProcessResultMisc;
}

u16
smbplan9mode2dosattr(u32 mode)
{
	if (mode & DMDIR)
		return SMB_ATTR_DIRECTORY;
	return SMB_ATTR_NORMAL;
}

u32
smbdosattr2plan9mode(u16 attr)
{
	u32 mode = 0444;
	if ((attr & SMB_ATTR_READ_ONLY) == 0)
		mode |= 0222;
	if (attr & SMB_ATTR_DIRECTORY) {
		mode |= DMDIR | 0711;
		mode &= DMDIR | 0755;
	}
	else
		mode &= 0744;
	return mode;
}

u32
smbdosattr2plan9wstatmode(u32 oldmode, u16 attr)
{
	u32 mode;
	if (oldmode & DMDIR)
		attr |= SMB_ATTR_DIRECTORY;
	else
		attr &= ~SMB_ATTR_DIRECTORY;
	mode = smbdosattr2plan9mode(attr);
	if (oldmode & 0444)
		mode = (mode & ~0444) | (mode & 0444);
	if ((attr & SMB_ATTR_READ_ONLY) == 0)
		mode |= oldmode & 0222;
	if (mode == oldmode)
		mode = 0xffffffff;
	return mode;
}

u32
smbplan9length2size32(i64 length)
{
	if (length > 0xffffffff)
		return 0xffffffff;
	return length;
}

i64
smbl2roundupi64(i64 v, int l2)
{
	u64 mask;
	mask = (1 << l2) - 1;
	return (v + mask) & ~mask;
}

SmbSlut smbsharemodeslut[] = {
	{ "compatibility", SMB_OPEN_MODE_SHARE_COMPATIBILITY },
	{ "exclusive", SMB_OPEN_MODE_SHARE_EXCLUSIVE },
	{ "denywrite", SMB_OPEN_MODE_SHARE_DENY_WRITE },
	{ "denyread", SMB_OPEN_MODE_SHARE_DENY_READOREXEC },
	{ "denynone", SMB_OPEN_MODE_SHARE_DENY_NONE },
	{ 0 }
};

SmbSlut smbopenmodeslut[] = {
	{ "oread", OREAD },
	{ "owrite", OWRITE },
	{ "ordwr", ORDWR },
	{ "oexec", OEXEC },
	{ 0 }
};

int
smbslut(SmbSlut *s, char *pat)
{
	while (s->name) {
		if (cistrcmp(s->name, pat) == 0)
			return s->val;
		s++;
	}
	return -1;
}

char *
smbrevslut(SmbSlut *s, int val)
{
	while (s->name) {
		if (s->val == val)
			return s->name;
		s++;
	}
	return nil;
}
