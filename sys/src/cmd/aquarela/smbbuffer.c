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
	u8 *buf;
	u32 realmaxlen;
	u32 maxlen;
	u32 rn;
	u32 wn;
	u32 savewn;
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
smbbuffersetbuf(SmbBuffer *s, void *p, u32 maxlen)
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
smbbufferinit(void *base, void *bdata, u32 blen)
{
	SmbBuffer *b;
	b = smbemalloc(sizeof(*b));
	b->buf = base;
	b->flags = STRUCT;
	b->rn = (u8 *)bdata - (u8 *)base;
	b->wn = b->rn + blen;
	b->realmaxlen = b->maxlen = b->wn;
	return b;
}

int
smbbufferalignl2(SmbBuffer *s, int al2)
{
	u32 mask, newn;
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
smbbufferputb(SmbBuffer *s, u8 b)
{
	if (s->wn >= s->maxlen)
		return 0;
	s->buf[s->wn++] = b;
	return 1;
}

u32
smbbufferspace(SmbBuffer *sess)
{
	return sess->maxlen - sess->wn;
}

int
smbbufferoffsetputs(SmbBuffer *sess, u32 offset, u16 s)
{
	if (offset + 2 > sess->wn)
		return 0;
	smbhnputs(sess->buf + offset, s);
	return 1;
}

int
smbbufferputs(SmbBuffer *sess, u16 s)
{
	if (sess->wn + sizeof(u16) > sess->maxlen)
		return 0;
	smbhnputs(sess->buf + sess->wn, s);
	sess->wn += sizeof(u16);
	return 1;
}

int
smbbufferputl(SmbBuffer *s, u32 l)
{
	if (s->wn + sizeof(u32) > s->maxlen)
		return 0;
	smbhnputl(s->buf + s->wn, l);
	s->wn += sizeof(u32);
	return 1;
}

int
smbbufferputv(SmbBuffer *s, i64 v)
{
	if (s->wn + sizeof(i64) > s->maxlen)
		return 0;
	smbhnputv(s->buf + s->wn, v);
	s->wn += sizeof(i64);
	return 1;
}

int
smbbufferputbytes(SmbBuffer *s, void *data, u32 datalen)
{
	if (s->wn + datalen > s->maxlen)
		return 0;
	if (data)
		memcpy(s->buf + s->wn, data, datalen);
	s->wn += datalen;
	return 1;
}

int
smbbufferputstring(SmbBuffer *b, SmbPeerInfo *p, u32 flags,
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

u32
smbbufferwriteoffset(SmbBuffer *s)
{
	return s->wn;
}

u32
smbbufferwritemaxoffset(SmbBuffer *s)
{
	return s->maxlen;
}

u32
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

u32
smbbufferwritespace(SmbBuffer *b)
{
	return b->maxlen - b->wn;
}

SmbBuffer *
smbbuffernew(u32 maxlen)
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

u8 *
smbbufferbase(SmbBuffer *b)
{
	return b->buf;
}

int
smbbuffergetbytes(SmbBuffer *b, void *buf, u32 len)
{
	if (b->rn + len > b->wn)
		return 0;
	if (buf)
		memcpy(buf, b->buf + b->rn, len);
	b->rn += len;
	return 1;
}

void
smbbuffersetreadlen(SmbBuffer *b, u32 len)
{
	b->wn = b->rn + len;
}

int
smbbuffertrimreadlen(SmbBuffer *b, u32 len)
{
	if (b->rn + len > b->wn)
		return 0;
	else if (b->rn + len < b->wn)
		b->wn = b->rn + len;
	return 1;
}

int
smbbuffergets(SmbBuffer *b, u16 *sp)
{
	if (b->rn + 2 > b->wn)
		return 0;
	*sp = smbnhgets(b->buf + b->rn);
	b->rn += 2;
	return 1;
}

int
smbbuffergetstrn(SmbBuffer *b, u16 size, char **sp)
{
	u8 *np;
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
smbbuffergetstr(SmbBuffer *b, u32 flags, char **sp)
{
	int c;
	char *p;
	u8 *np;

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
	u8 *np;
	np = memchr(b->buf + b->rn, 0, b->wn - b->rn);
	if (np == nil)
		return 0;
	*sp = (char *)b->buf + b->rn;
	b->rn = np - b->buf + 1;
	return 1;
}

int
smbbuffergetucs2(SmbBuffer *b, u32 flags, char **sp)
{
	u8 *bdata = b->buf + b->rn;
	u8 *edata = b->buf + b->wn;
	Rune r;
	int l;
	char *p, *q;
	u8 *savebdata;
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
smbbuffergetstring(SmbBuffer *b, SmbHeader *h, u32 flags, char **sp)
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
smbbufferpointer(SmbBuffer *b, u32 offset)
{
	return b->buf + offset;
}

int
smbbuffergetb(SmbBuffer *b, u8 *bp)
{
	if (b->rn < b->wn) {
		*bp = b->buf[b->rn++];
		return 1;
	}
	return 0;
}

int
smbbuffergetl(SmbBuffer *b, u32 *lp)
{
	if (b->rn + 4 <= b->wn) {
		*lp = smbnhgetl(b->buf + b->rn);
		b->rn += 4;
		return 1;
	}
	return 0;
}

int
smbbuffergetv(SmbBuffer *b, i64 *vp)
{
	if (b->rn + 8 <= b->wn) {
		*vp = smbnhgetv(b->buf + b->rn);
		b->rn += 8;
		return 1;
	}
	return 0;
}

u32
smbbufferreadspace(SmbBuffer *b)
{
	return b->wn - b->rn;
}

void
smbbufferwritelimit(SmbBuffer *b, u32 limit)
{
	if (b->rn + limit < b->maxlen)
		b->maxlen = b->rn + limit;
}

int
smbbufferreadskipto(SmbBuffer *b, u32 offset)
{
	if (offset < b->rn || offset >= b->wn)
		return 0;
	b->rn = offset;
	return 1;
}

int
smbbufferpushreadlimit(SmbBuffer *b, u32 limit)
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
smbbufferwritebackup(SmbBuffer *b, u32 offset)
{
	if (offset >= b->rn && offset <= b->wn) {
		b->wn = offset;
		return 1;
	}
	return 0;
}

int
smbbufferreadbackup(SmbBuffer *b, u32 offset)
{
	if (offset <= b->rn) {
		b->rn = offset;
		return 1;
	}
	return 0;
}

int
smbbufferfixuprelatives(SmbBuffer *b, u32 fixupoffset)
{
	u32 fixval;
	if (fixupoffset < b->rn || fixupoffset > b->wn - 2)
		return 0;
	fixval = b->wn - fixupoffset - 2;
	if (fixval > 65535)
		return 0;
	smbhnputs(b->buf + fixupoffset, fixval);
	return 1;
}

int
smbbufferfixuprelativel(SmbBuffer *b, u32 fixupoffset)
{
	u32 fixval;
	if (fixupoffset < b->rn || fixupoffset > b->wn - 4)
		return 0;
	fixval = b->wn - fixupoffset - 4;
	smbhnputl(b->buf + fixupoffset, fixval);
	return 1;
}

int
smbbufferfixupabsolutes(SmbBuffer *b, u32 fixupoffset)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 2)
		return 0;
	if (b->wn > 65535)
		return 0;
	smbhnputs(b->buf + fixupoffset, b->wn);
	return 1;
}

int
smbbufferfixupl(SmbBuffer *b, u32 fixupoffset, u32 fixupval)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 4)
		return 0;
	smbhnputl(b->buf + fixupoffset, fixupval);
	return 1;
}

int
smbbufferfixupabsolutel(SmbBuffer *b, u32 fixupoffset)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 2)
		return 0;
	smbhnputl(b->buf + fixupoffset, b->wn);
	return 1;
}

int
smbbufferfixuprelativeinclusivel(SmbBuffer *b, u32 fixupoffset)
{
	if (fixupoffset < b->rn || fixupoffset > b->wn - 4)
		return 0;
	smbhnputl(b->buf + fixupoffset, b->wn - fixupoffset);
	return 1;
}

int
smbbufferfill(SmbBuffer *b, u8 val, u32 len)
{
	if (b->maxlen - b->wn < len)
		return 0;
	memset(b->buf + b->wn, val, len);
	b->wn += len;
	return 1;
}

int
smbbufferoffsetgetb(SmbBuffer *b, u32 offset, u8 *bp)
{
	if (offset >= b->rn && offset + 1 <= b->wn) {
		*bp = b->buf[b->rn + offset];
		return 1;
	}
	return 0;
}

int
smbbuffercopy(SmbBuffer *to, SmbBuffer *from, u32 amount)
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
smbbufferoffsetcopystr(SmbBuffer *b, u32 offset, char *buf,
		       int buflen,
		       int *lenp)
{
	u8 *np;
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
