/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

SmbProcessResult
smbnegotiate(SmbSession *s, SmbHeader *h, uint8_t *, SmbBuffer *b)
{
	uint16_t index;
	int i;
	uint8_t bufferformat;

	if (!smbcheckwordcount("negotiate", h, 0))
		return SmbProcessResultFormat;
	if (s->state != SmbSessionNeedNegotiate) {
		/* this acts as a complete session reset */
		smblogprint(-1, "smbnegotiate: called when already negotiated\n");
		return SmbProcessResultUnimp;
	}
	i = 0;
	index = 0xffff;
	while (smbbuffergetb(b, &bufferformat)) {
		char *s;
		if (bufferformat != 0x02) {
			smblogprint(-1, "smbnegotiate: unrecognised buffer format 0x%.2x\n", bufferformat);
			return SmbProcessResultFormat;
		}
		if (!smbbuffergetstr(b, 0, &s)) {
			smblogprint(-1, "smbnegotiate: no null found\n");
			return SmbProcessResultFormat;
		}
		smblogprint(h->command, "smbnegotiate: '%s'\n", s);
		if (index == 0xffff && strcmp(s, "NT LM 0.12") == 0)
			index = i;
		i++;
		free(s);
	}
	if (index != 0xffff) {
		Tm *tm;
		uint32_t capabilities;
		uint32_t bytecountfixupoffset;

		h->wordcount = 17;
		if (!smbbufferputheader(s->response, h, nil)
			|| !smbbufferputs(s->response, index)
			|| !smbbufferputb(s->response, 3)			/* user security, encrypted */
			|| !smbbufferputs(s->response, 1)			/* max mux */
			|| !smbbufferputs(s->response, 1)			/* max vc */
			|| !smbbufferputl(s->response, smbglobals.maxreceive)		/* max buffer size */
			|| !smbbufferputl(s->response, 0x10000)		/* max raw */
			|| !smbbufferputl(s->response, threadid()))	/* session key */
			goto die;
		/* <= Win2k insist upon this being set to ensure that they observe the prototol (!) */
		capabilities = CAP_NT_SMBS;
		if (smbglobals.unicode)
			capabilities |= CAP_UNICODE;
		tm = localtime(time(nil));
		s->tzoff = tm->tzoff;
		if (!smbbufferputl(s->response, capabilities)
			|| !smbbufferputv(s->response, nsec() / 100 + (int64_t)10000000 * 11644473600LL)
			|| !smbbufferputs(s->response, -s->tzoff / 60)
			|| !smbbufferputb(s->response, 8))	/* crypt len */
			goto die;
		bytecountfixupoffset = smbbufferwriteoffset(s->response);
		if (!smbbufferputs(s->response, 0))
			goto die;
		s->cs = auth_challenge("proto=mschap role=server");
		if (s->cs == nil) {
			smblogprint(h->command, "smbnegotiate: couldn't get mschap challenge\n");
			return SmbProcessResultMisc;
		}
		if (s->cs->nchal != 8) {
			smblogprint(h->command, "smbnegotiate: nchal %d\n", s->cs->nchal);
			return SmbProcessResultMisc;
		}
		if (!smbbufferputbytes(s->response, s->cs->chal, s->cs->nchal)
			|| !smbbufferputstring(s->response, nil, SMB_STRING_UNICODE, smbglobals.primarydomain)
			|| !smbbufferfixuprelatives(s->response, bytecountfixupoffset))
			goto die;
	}
	else {
		h->wordcount = 1;
		if (!smbbufferputheader(s->response, h, nil)
			|| !smbbufferputs(s->response, index)
			|| !smbbufferputs(s->response, 0))
			goto die;
	}
	s->state = SmbSessionNeedSetup;
	return SmbProcessResultReply;
die:
	return SmbProcessResultDie;
}
