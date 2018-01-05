/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

Rune
smbruneconvert(Rune r, uint32_t flags)
{
	if (r >= 'a' && r <= 'z' && (flags & SMB_STRING_UPCASE))
		r = toupper(r);
	else if (r == '/' && (flags & SMB_STRING_REVPATH))
		r = '\\';
	else if (r == '\\' && (flags & SMB_STRING_PATH))
		r = '/';
	else if (r == 0xa0 && (flags & SMB_STRING_REVPATH) && smbglobals.convertspace)
		r = ' ';
	else if (r == ' ' && (flags & SMB_STRING_PATH) && smbglobals.convertspace)
		r = 0xa0;
	return r;
}

int
smbucs2len(char *string)
{
	return (string ? utflen(string) : 0) * 2 + 2;
}

int
smbstrlen(char *string)
{
	return (string ? strlen(string) : 0) + 1;
}

int
smbstringlen(SmbPeerInfo *i, char *string)
{
	if (smbglobals.unicode && (i == nil || (i->capabilities & CAP_UNICODE) != 0))
		return smbucs2len(string);
	return  smbstrlen(string);
}

char *
smbstrinline(uint8_t **bdatap, uint8_t *edata)
{
	char *p;
	uint8_t *np;
	np = memchr(*bdatap, 0, edata - *bdatap);
	if (np == nil)
		return nil;
	p = (char *)*bdatap;
	*bdatap = np + 1;
	return p;
}

char *
smbstrdup(uint8_t **bdatap, uint8_t *edata)
{
	char *p;
	uint8_t *np;
	np = memchr(*bdatap, 0, edata - *bdatap);
	if (np == nil)
		return nil;
	p = smbestrdup((char *)(*bdatap));
	*bdatap = np + 1;
	return p;
}

char *
smbstringdup(SmbHeader *h, uint8_t *base, uint8_t **bdatap, uint8_t *edata)
{
	char *p;
	if (h && h->flags2 & SMB_FLAGS2_UNICODE) {
		uint8_t *bdata = *bdatap;
		uint8_t *savebdata;
		Rune r;
		int l;
		char *q;

		l = 0;
		if ((bdata - base) & 1)
			bdata++;
		savebdata = bdata;
		do {
			if (bdata + 2 > edata)
				return nil;
			r = smbnhgets(bdata); bdata += 2;
			l += runelen(r);
		} while (r != 0);
		p = smbemalloc(l);
		bdata = savebdata;
		q = p;
		do {
			r = smbnhgets(bdata); bdata += 2;
			q += runetochar(q, &r);
		} while (r != 0);
		*bdatap = bdata;
		return p;
	}
	return smbstrdup(bdatap, edata);
}

int
smbstrnput(uint8_t *buf, uint16_t n, uint16_t maxlen, char *string,
	   uint16_t size, int upcase)
{
	uint8_t *p = buf + n;
	int l;
	l = strlen(string);
	if (l + 1 > size)
		l = size - 1;
	if (n + l + 1 > maxlen)
		return 0;
	if (upcase) {
		int x;
		for (x = 0; x < l; x++)
			p[x] = toupper(string[x]);
	}
	else
		memcpy(p, string, l);

	p += l;
	while (l++ < size)
		*p++ = 0;
	return size;
}

int
smbstrput(uint32_t flags, uint8_t *buf, uint16_t n, uint16_t maxlen,
	  char *string)
{
	uint8_t *p = buf + n;
	int l;
	l = string ? strlen(string) : 0;
	if (n + l + ((flags & SMB_STRING_UNTERMINATED) == 0 ? 1 : 0) > maxlen)
		return 0;
	memcpy(p, string, l);
	if (flags & (SMB_STRING_UPCASE | SMB_STRING_PATH | SMB_STRING_REVPATH)) {
		uint8_t *q;
		for (q = p; q < p + l; q++)
			if (*q >= 'a' && *q <= 'z' && (flags & SMB_STRING_UPCASE))
				*q = toupper(*q);
			else if (*q == '/' && (flags & SMB_STRING_REVPATH))
				*q = '\\';
			else if (*q == '\\' && (flags & SMB_STRING_PATH))
				*q = '/';
	}
	p += l;
	if ((flags & SMB_STRING_UNTERMINATED) == 0)
		*p++ = 0;
	return p - (buf + n);
}

int
smbucs2put(uint32_t flags, uint8_t *buf, uint16_t n, uint16_t maxlen,
	   char *string)
{
	uint8_t *p = buf + n;
	int l;
	int align;
	align = (flags & SMB_STRING_UNALIGNED) == 0 && (n & 1) != 0;
	l = string ? utflen(string) * 2 : 0;
	if (n + l + ((flags & SMB_STRING_UNTERMINATED) ? 0 : 2) + align > maxlen)
		return 0;
	if (align)
		*p++ = 0;
	while (string && *string) {
		Rune r;
		int i;
		i = chartorune(&r, string);
		if (flags & SMB_STRING_CONVERT_MASK)
			r = smbruneconvert(r, flags);
		smbhnputs(p, r);
		p += 2;
		string += i;
	}
	if ((flags & SMB_STRING_UNTERMINATED) == 0) {
		smbhnputs(p, 0);
		p += 2;
	}
	assert(p <= buf + maxlen);
	return p - (buf + n);
}

int
smbstringput(SmbPeerInfo *p, uint32_t flags, uint8_t *buf, uint16_t n,
	     uint16_t maxlen, char *string)
{
	if (flags & SMB_STRING_UNICODE)
		return smbucs2put(flags, buf, n, maxlen, string);
	if (flags & SMB_STRING_ASCII)
		return smbstrput(flags, buf, n, maxlen, string);
	if (p && (p->capabilities & CAP_UNICODE) != 0)
		return smbucs2put(flags, buf, n, maxlen, string);
	return smbstrput(flags, buf, n, maxlen, string);
}

void
smbstringprint(char **p, char *fmt, ...)
{
	va_list arg;
	if (*p) {
		free(*p);
		*p = nil;
	}
	va_start(arg, fmt);
	*p = vsmprint(fmt, arg);
	va_end(arg);
}
