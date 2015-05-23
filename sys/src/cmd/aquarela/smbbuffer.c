/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

#define BUFFER 1
#define STRUCT 2
#define PUSHED 4

struct SmbBuffer {
	uint8_t *buf;
	uint32_t realmaxlen;
	uint32_t maxlen;
	uint32_t rn;
	uint32_t wn;
	uint32_t savewn;
	int flags;
};

void
smbbufferreset(SmbBuffer *s)
{
	if (s == nil)
		return;
	s->rn = 0;
	s->wn = 0;
	s->flags &= ~PUSHED;
}

void
smbbuffersetbuf(SmbBuffer *s, void *p, uint32_t maxlen)
{
	s->realmaxlen = s->maxlen = maxlen;
	if (s->buf) {
		if (s->flags & BUFFER)
			free(s->buf);
		s->buf = nil;
	}
	s->flags &= ~BUFFER;
	if (p)
		s->buf = p;
	else {
		s->buf = smbemalloc(maxlen);
		s->flags |= BUFFER;
	}
	smbbufferreset(s);
}

SmbBuffer *
smbbufferinit(void *base, void *bdata, uint32_t blen)
{
	SmbBuffer *b;
	b = smbemalloc(sizeof(*b));
	b->buf = base;
	b->flags = STRUCT;	
	b->rn = (uint8_t *)bdata - (uint8_t *)base;
	b->wn = b->rn + blen;
	b->realmaxlen = b->maxlen = b->wn;
	return b;
}

int
smbbufferalignl2(SmbBuffer *s, int al2)
{
	uint32_t mask, newn;
	mask = (1 << al2) - 1;
	newn = (s->wn + mask) & ~mask;
	if (newn != s->wn) {
		if (newn > s->maxlen)
			return 0;
		s->wn = newn;
	}
	return 1;
}

int
smbbufferputb(SmbBuffer *s, uint8_t b)
{
	if (s->wn >= s->maxlen)
		return 0;
	s->buf[s->wn++] = b;
	return 1;
}

uint32_t
smbbufferspace(SmbBuffer *sess)
{
	return sess->maxlen - sess->wn;
}

int
smbbufferoffsetputs(SmbBuffer *sess, uint32_t offset, uint16_t s)
{
	if (offset + 2 > sess->wn)
		return 0;
	smbhnputs(sess->buf + offset, s);
	return 1;
}

int
smbbufferputs(SmbBuffer *sess, uint16_t s)
{
	if (sess->wn + sizeof(uint16_t) > sess->maxlen)
		return 0;
	smbhnputs(sess->buf + sess->wn, s);
	sess->wn += sizeof(uint16_t);
	return 1;
}

int
smbbufferputl(SmbBuffer *s, uint32_t l)
{
	if (s->wn + sizeof(uint32_t) > s->maxlen)
		return 0;
	smbhnputl(s->buf + s->wn, l);
	s->wn += sizeof(uint32_t);
	return 1;
}

int
smbbufferputv(SmbBuffer *s, int64_t v)
{
	if (s->wn + sizeof(int64_t) > s->maxlen)
		return 0;
	smbhnputv(s->buf + s->wn, v);
	s->wn += sizeof(int64_t);
	return 1;
}

int
smbbufferputbytes(SmbBuffer *s, void *data, uint32_t datalen)
{
	if (s->wn + datalen > s->maxlen)
		return 0;
	if (data)
		memcpy(s->buf + s->wn, data, datalen);
	s->wn += datalen;
	return 1;
}

int
smbbufferputstring(SmbBuffer *b, SmbPeerInfo *p, uint32_t flags,
		   char *string)
{
	int n = smbstringput(p, flags, b->buf, b->wn, b->maxlen, string);
	if (n <= 0)
		return 0;
	b->wn += n;
	return 1;
}

int
smbbufferputstrn(SmbBuffer *s, char *string, int size, int upcase)
{
	int n = smbstrnput(s->buf, s->wn, s->maxlen, string, size, upcase);
	if (n <= 0)
		return 0;
	s->wn += n;
	return 1;
}

uint32_t
smbbufferwriteoffset(SmbBuffer *s)
{
	return s->wn;
}

uint32_t
smbbufferwritemaxoffset(SmbBuffer *s)
{
	return s->maxlen;
}

uint32_t
smbbufferreadoffset(SmbBuffer *s)
{
	return s->rn;
}

void *
smbbufferreadpointer(SmbBuffer *s)
{
	return s->buf + s->rn;
}

void *
smbbufferwritepointer(SmbBuffer *s)
{
	return s->buf + s->wn;
}

uint32_t
smbbufferwritespace(SmbBuffer *b)
{
	return b->maxlen - b->wn;
}

SmbBuffer *
smbbuffernew(uint32_t maxlen)
{
	SmbBuffer *b;
	b = smbemalloc(sizeof(SmbBuffer));
	b->buf = smbemalloc(maxlen);
	b->realmaxlen = b->maxlen = maxlen;
	b->rn = 0;
	b->wn = 0;
	b->flags = STRUCT | BUFFER;
	return b;
}

void
smbbufferfree(SmbBuffer **bp)
{
	SmbBuffer *b = *bp;
	if (b) {
		if (b->flags & BUFFER) {
			free(b->buf);
			b->buf = nil;
			b->flags &= ~BUFFER;
		}
		if (b->flags & STRUCT)
			free(b);
		*bp = nil;
	}
}

uint8_t *
smbbufferbase(SmbBuffer *b)
{
	return b->buf;
}

int
smbbuffergetbytes(SmbBuffer *b, void *buf, uint32_t len)
{
	if (b->rn + len > b->wn)
		return 0;
	if (buf)
		memcpy(buf, b->buf + b->rn, len);
	b->rn += len;
	return 1;
}

void
smbbuffersetreadlen(SmbBuffer *b, uint32_t len)
{
	b->wn = b->rn + len;
}

int
smbbuffertrimreadlen(SmbBuffer *b, uint32_t len)
{
	if (b->rn + len > b->wn)
		return 0;
	else if (b->rn + len < b->wn)
		b->wn = b->rn + len;
	return 1;
}

int
smbbuffergets(SmbBuffer *b, uint16_t *sp)
{
	if (b->rn + 2 > b->wn)
		return 0;
	*sp = smbnhgets(b->buf + b->rn);
	b->rn += 2;
	return 1;
}

int
smbbuffergetstrn(SmbBuffer *b, uint16_t size, char **sp)
{
	uint8_t *np;
	if (size > b->wn - b->rn)
		return 0;
	np = memchr(b->buf + b->rn, 0, size);
	if (np == nil)
		return 0;
	*sp = strdup((char *)b->buf + b->rn);
	b->rn += size;
	return 1;
}

int
smbbuffergetstr(SmbBuffer *b, uint32_t flags, char **sp)
{
	int c;
	char *p;
	uint8_t *np;

	np = memchr(b->buf + b->rn, 0, b->wn - b->rn);
	if (np == nil)
		return 0;
	*sp = strdup((char *)b->buf + b->rn);
	for (p = *sp; *p != 0; p++) {
		c = *p;
		if (c >= 'a' && c <= 'z' && (flags & SMB_STRING_UPCASE))
			*p = toupper(c);
		else if (c == '/' && (flags & SMB_STRING_REVPATH))
			*p = '\\';
		else if (c == '\\' && (flags & SMB_STRING_PATH))
			*p = '/';
		else if (smbglobals.convertspace){
			if (c == 0xa0 && (flags & SMB_STRING_REVPATH))
				*p = ' ';
			else if (c == ' ' && (flags & SMB_STRING_PATH))
				*p = 0xa0;
		}
	}
	b->rn = np - b->buf + 1;
	return 1;
}

int
smbbuffergetstrinline(SmbBuffer *b, char **sp)
{
	uint8_t *np;
	np = memchr(b->buf + b->rn, 0, b->wn - b->rn);
	if (np == nil)
		return 0;
	*sp = (char *)b->buf + b->rn;
	b->rn = np - b->buf + 1;
	return 1;
}

int
smbbuffergetucs2(SmbBuffer *b, uint32_t flags, char **sp)
{
	uint8_t *bdata = b->buf + b->rn;
	uint8_t *edata = b->buf + b->wn;
	Rune r;
	int l;
	char *p, *q;
	uint8_t *savebdata;
	int first;

	l = 0;
	if ((flags & SMB_STRING_UNALIGNED) == 0 && (bdata - b->buf) & 1)
		bdata++;
	savebdata = bdata;
	first = 1;
	do {
		if (bdata + 2 > edata) {
			l++;
			break;
		}
		r = smbnhgets(bdata); bdata += 2;
		if (first && (flags & SMB_STRING_PATH) && r != '\\')
			l++;
		first = 0;
		if (flags & SMB_STRING_CONVERT_MASK)
			r = smbruneconvert(r, flags);
		l += runelen(r);
	} while (r != 0);
	p = smbemalloc(l);
	bdata = savebdata;
	q = p;
	first = 1;
	do {
		if (bdata + 2 > edata) {
			*q = 0;
			break;
		}
		r = smbnhgets(bdata); bdata += 2;
		if (first && (flags & SMB_STRING_PATH) && r != '\\')
			*q++ = '/';
		first = 0;
		if (flags & SMB_STRING_CONVERT_MASK)
			r = smbruneconvert(r, flags);
		q += runetochar(q, &r);
	} while (r != 0);
	b->rn = bdata - b->buf;
	*sp = p;
	return 1;
}

int
smbbuffergetstring(SmbBuffer *b, SmbHeader *h, uint32_t flags, char **sp)
{
	if (flags & SMB_STRING_UNICODE)
		return smbbuffergetucs2(b, flags, sp);
	else if (flags & SMB_STRING_ASCII)
		return smbbuffergetstr(b, flags, sp);
	else if (h->flags2 & SMB_FLAGS2_UNICODE)
		return smbbuffergetucs2(b, flags, sp);
	else
		return smbbuffergetstr(b, flags, sp);
}

void *
smbbufferpointer(SmbBuffer *b, uint32_t offset)
{
	return b->buf + offset;
}

int
smbbuffergetb(SmbBuffer *b, uint8_t *bp)
{
	if (b->rn < b->wn) {
		*bp = b->buf[b->rn++];
		return 1;
	}
	return 0;
}

int
smbbuffergetl(SmbBuffer *b, uint32_t *lp)
{
	if (b->rn + 4 <= b->wn) {
		*lp = smbnhgetl(b->buf + b->rn);
		b->rn += 4;
		return 1;
	}
	return 0;
}

int
smbbuffergetv(SmbBuffer *b, int64_t *vp)
{
	if (b->rn + 8 <= b->wn) {
		*vp = smbnhgetv(b->buf + b->rn);
		b->rn += 8;
		return 1;
	}
	return 0;
}

uint32_t
smbbufferreadspace(SmbBuffer *b)
{
	return b->wn - b->rn;
}

void
smbbufferwritelimit(SmbBuffer *b, uint32_t limit)
{
	if (b->rn + limit < b->maxlen)
		b->maxlen = b->rn + limit;
}

int
smbbufferreadskipto(SmbBuffer *b, uint32_t offset)
{
	if (offset < b->rn || offset >= b->wn)
		return 0;
	b->rn = offset;
	return 1;
}

int
smbbufferpushreadlimit(SmbBuffer *b, uint32_t limit)
{
	if (b->flags & PUSHED)
		return 0;
	if (limit > b->wn || limit < b->rn)
		return 0;
	b->savewn = b->wn;
	b->wn = limit;
	b->flags |= PUSHED;
	return 1;
}

int
smbbufferpopreadlimit(SmbBuffer *b)
{
	if ((b->flags & PUSHED) == 0)
		return 0;
	b->wn = b->savewn;
	b->flags &= ~PUSHED;
	return 1;
}

int
smbbufferwritebackup(SmbBuffer *b, uint32_t offset)
{
	if (offset >= b->rn && offset <= b->wn) {
		b->wn = offset;
		return 1;
	}
	return 0;
}

int
smbbufferreadbackup(SmbBuffer *b, uint32_t offset)
{
	if (offset <= b->rn) {
		b->rn = offset;
		return 1;
	}
	return 0;
}

int
smbbufferfixuprelatives(SmbBuffer *b, uint32_t fixupoffset)
{
	uint32_t fixval;
	if (fixupoffset < b->rn || fixupoffset > b->wn - 2)
		return 0;
	fixval = b->wn - fixupoffset - 2;
	if (fixval > 65535)
		return 0;
	smbhnputs(b->buf + fixupoffset, fixval);
	return 1;
}

int
smbbufferfixuprelativel(SmbBuffer *b, uint32_t fixupoffset)
{
	uint32_t fixval;
	if (fixupoffset < b->rn || fixupoffset > b->wn - 4)
		return 0;
	fixval = b->wn - fixupoffset - 4;
	smbhnputl(b->buf + fixupoffset, fixval);
	return 1;
}

int
smbbufferfixupabsolutes(SmbBuffer *b, uint32_t fixupoffset)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 2)
		return 0;
	if (b->wn > 65535)
		return 0;
	smbhnputs(b->buf + fixupoffset, b->wn);
	return 1;
}

int
smbbufferfixupl(SmbBuffer *b, uint32_t fixupoffset, uint32_t fixupval)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 4)
		return 0;
	smbhnputl(b->buf + fixupoffset, fixupval);
	return 1;
}

int
smbbufferfixupabsolutel(SmbBuffer *b, uint32_t fixupoffset)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 2)
		return 0;
	smbhnputl(b->buf + fixupoffset, b->wn);
	return 1;
}

int
smbbufferfixuprelativeinclusivel(SmbBuffer *b, uint32_t fixupoffset)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 4)
		return 0;
	smbhnputl(b->buf + fixupoffset, b->wn - fixupoffset);
	return 1;
}

int
smbbufferfill(SmbBuffer *b, uint8_t val, uint32_t len)
{
	if (b->maxlen - b->wn < len)
		return 0;
	memset(b->buf + b->wn, val, len);
	b->wn += len;
	return 1;
}

int
smbbufferoffsetgetb(SmbBuffer *b, uint32_t offset, uint8_t *bp)
{
	if (offset >= b->rn && offset + 1 <= b->wn) {
		*bp = b->buf[b->rn + offset];
		return 1;
	}
	return 0;
}

int
smbbuffercopy(SmbBuffer *to, SmbBuffer *from, uint32_t amount)
{
	if (smbbufferreadspace(from) < amount)
		return 0;
	if (smbbufferputbytes(to, smbbufferreadpointer(from), amount)) {
		assert(smbbuffergetbytes(from, nil, amount));
		return 1;
	}
	return 0;
}

int
smbbufferoffsetcopystr(SmbBuffer *b, uint32_t offset, char *buf,
		       int buflen,
		       int *lenp)
{
	uint8_t *np;
	if (offset < b->rn || offset >= b->wn)
		return 0;
	np = memchr(b->buf + offset, 0, b->wn - offset);
	if (np == nil)
		return 0;
	*lenp = np - (b->buf + offset) + 1;
	if (*lenp > buflen)
		return 0;
	memcpy(buf, b->buf + offset, *lenp);
	return 1;
}
