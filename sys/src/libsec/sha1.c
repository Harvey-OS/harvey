#include "os.h"
#include <mp.h>
#include <libsec.h>

static void encode(uchar*, u32int*, ulong);
static void decode(u32int*, uchar*, ulong);



static void
sha1block(uchar *p, ulong len, SHAstate *s)
{
	u32int a, b, c, d, e, tmp;
	uchar *end;
	u32int *wp, *wend;
	u32int w[80];

	/* at this point, we have a multple of 64 bytes */
	for(end = p+len; p < end;){
		/* pack message */
		decode(w, p, 64);
		p += 64;

		/* expand */
		wend = w + 80;
		for(wp = w + 16; wp < wend; wp++){
			a = *(wp-3) ^ *(wp-8) ^ *(wp-14) ^ *(wp-16);
			*wp = (a<<1) | (a>>31);
		}

		a = s->state[0];
		b = s->state[1];
		c = s->state[2];
		d = s->state[3];
		e = s->state[4];

		/* 4 rounds */
		wend = w + 20;
		for(wp = w; wp < wend; wp++){
			tmp = ((a<<5) | (a>>27)) + e + *wp;
			tmp += 0x5a827999 + ((b&c)|(d&~b));
			e = d;
			d = c;
			c = (b<<30)|(b>>2);
			b = a;
			a = tmp;
		}
		wend = w + 40;
		for(; wp < wend; wp++){
			tmp = ((a<<5) | (a>>27)) + e + *wp;
			tmp += 0x6ed9eba1 + (b^c^d);
			e = d;
			d = c;
			c = (b<<30)|(b>>2);
			b = a;
			a = tmp;
		}
		wend = w + 60;
		for(; wp < wend; wp++){
			tmp = ((a<<5) | (a>>27)) + e + *wp;
			tmp += 0x8f1bbcdc + ((b&c)|(b&d)|(c&d));
			e = d;
			d = c;
			c = (b<<30)|(b>>2);
			b = a;
			a = tmp;
		}
		wend = w + 80;
		for(; wp < wend; wp++){
			tmp = ((a<<5) | (a>>27)) + e + *wp;
			tmp += 0xca62c1d6 + (b^c^d);
			e = d;
			d = c;
			c = (b<<30)|(b>>2);
			b = a;
			a = tmp;
		}

		/* save state */
		s->state[0] += a;
		s->state[1] += b;
		s->state[2] += c;
		s->state[3] += d;
		s->state[4] += e;

		s->len += 64;
	}
}

/*
 *  we require len to be a multiple of 64 for all but
 *  the last call.  There must be room in the input buffer
 *  to pad.
 */
SHAstate*
sha1(uchar *p, ulong len, uchar *digest, SHAstate *s)
{
	uchar buf[128];
	u32int x[16];
	int i;
	uchar *e;

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
			sha1block(s->buf, s->blen, s);
			s->blen = 0;
		}
	}

	/* do 64 byte blocks */
	i = len & ~0x3f;
	if(i){
		sha1block(p, i, s);
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
	sha1block(p, len+8, s);

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
encode(uchar *output, u32int *input, ulong len)
{
	u32int x;
	uchar *e;

	for(e = output + len; output < e;) {
		x = *input++;
		*output++ = x >> 24;
		*output++ = x >> 16;
		*output++ = x >> 8;
		*output++ = x;
	}
}

/*
 *	decodes input (uchar) into output (ulong). Assumes len is
 *	a multiple of 4.
 */
static void
decode(u32int *output, uchar *input, ulong len)
{
	uchar *e;

	for(e = input+len; input < e; input += 4)
		*output++ = input[3] | (input[2] << 8) |
			(input[1] << 16) | (input[0] << 24);
}
