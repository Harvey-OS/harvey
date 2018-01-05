/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <libsec.h>

static void encode(uint8_t*, uint32_t*, uint32_t);

extern void _sha1block(uint8_t*, uint32_t, uint32_t*);

/*
 *  we require len to be a multiple of 64 for all but
 *  the last call.  There must be room in the input buffer
 *  to pad.
 */
SHA1state*
sha1(uint8_t *p, uint32_t len, uint8_t *digest, SHA1state *s)
{
	uint8_t buf[128];
	uint32_t x[16];
	int i;
	uint8_t *e;

	if(s == nil){
		s = malloc(sizeof(*s));
		if(s == nil)
			return nil;
		memset(s, 0, sizeof(*s));
		s->malloced = 1;
	}

	if(s->seeded == 0){
		/* seed the state, these constants would look nicer big-endian */
		s->state[0] = 0x67452301;
		s->state[1] = 0xefcdab89;
		s->state[2] = 0x98badcfe;
		s->state[3] = 0x10325476;
		s->state[4] = 0xc3d2e1f0;
		s->seeded = 1;
	}

	/* fill out the partial 64 byte block from previous calls */
	if(s->blen){
		i = 64 - s->blen;
		if(len < i)
			i = len;
		memmove(s->buf + s->blen, p, i);
		len -= i;
		s->blen += i;
		p += i;
		if(s->blen == 64){
			_sha1block(s->buf, s->blen, s->state);
			s->len += s->blen;
			s->blen = 0;
		}
	}

	/* do 64 byte blocks */
	i = len & ~0x3f;
	if(i){
		_sha1block(p, i, s->state);
		s->len += i;
		len -= i;
		p += i;
	}

	/* save the left overs if not last call */
	if(digest == 0){
		if(len){
			memmove(s->buf, p, len);
			s->blen += len;
		}
		return s;
	}

	/*
	 *  this is the last time through, pad what's left with 0x80,
	 *  0's, and the input count to create a multiple of 64 bytes
	 */
	if(s->blen){
		p = s->buf;
		len = s->blen;
	} else {
		memmove(buf, p, len);
		p = buf;
	}
	s->len += len;
	e = p + len;
	if(len < 56)
		i = 56 - len;
	else
		i = 120 - len;
	memset(e, 0, i);
	*e = 0x80;
	len += i;

	/* append the count */
	x[0] = s->len>>29;
	x[1] = s->len<<3;
	encode(p+len, x, 8);

	/* digest the last part */
	_sha1block(p, len+8, s->state);
	s->len += len+8;

	/* return result and free state */
	encode(digest, s->state, SHA1dlen);
	if(s->malloced == 1)
		free(s);
	return nil;
}

/*
 *	encodes input (ulong) into output (uchar). Assumes len is
 *	a multiple of 4.
 */
static void
encode(uint8_t *output, uint32_t *input, uint32_t len)
{
	uint32_t x;
	uint8_t *e;

	for(e = output + len; output < e;) {
		x = *input++;
		*output++ = x >> 24;
		*output++ = x >> 16;
		*output++ = x >> 8;
		*output++ = x;
	}
}

DigestState*
hmac_sha1(uint8_t *p, uint32_t len, uint8_t *key, uint32_t klen,
	  uint8_t *digest,
	DigestState *s)
{
	return hmac_x(p, len, key, klen, digest, s, sha1, SHA1dlen);
}
