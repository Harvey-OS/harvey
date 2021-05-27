/*
Adapted from chacha-merged.c version 20080118
D. J. Bernstein
Public domain.

modified for use in Plan 9 and Inferno (no algorithmic changes),
and including the changes to block number and nonce defined in RFC7539
*/

#include <u.h>
#include <libc.h>
#include <libsec.h>

enum{
	Blockwords=	ChachaBsize/sizeof(u32int)
};

/* little-endian data order */
#define GET4(p)	((((((p)[3]<<8) | (p)[2])<<8) | (p)[1])<<8 | (p)[0])
#define PUT4(p, v)	(((p)[0]=v), (v>>=8), ((p)[1]=v), (v>>=8), ((p)[2]=v), (v>>=8), ((p)[3]=v))

#define ROTATE(v,c) ((u32int)((v) << (c)) | ((v) >> (32 - (c))))

#define QUARTERROUND(ia,ib,ic,id) { \
	u32int a, b, c, d, t;\
	a = x[ia]; b = x[ib]; c = x[ic]; d = x[id]; \
	a += b; t = d^a; d = ROTATE(t,16); \
	c += d; t = b^c; b = ROTATE(t,12); \
	a += b; t = d^a; d = ROTATE(t, 8); \
	c += d; t = b^c; b = ROTATE(t, 7); \
	x[ia] = a; x[ib] = b; x[ic] = c; x[id] = d; \
}

#define ENCRYPT(s, x, y, d) {\
	u32int v; \
	uchar *sp, *dp; \
	sp = (s); \
	v = GET4(sp); \
	v ^= (x)+(y); \
	dp = (d); \
	PUT4(dp, v); \
}

static uchar sigma[16] = "expand 32-byte k";
static uchar tau[16] = "expand 16-byte k";

static void
load(u32int *d, uchar *s, int nw)
{
	int i;

	for(i = 0; i < nw; i++, s+=4)
		d[i] = GET4(s);
}

void
setupChachastate(Chachastate *s, uchar *key, usize keylen, uchar *iv, int rounds)
{
	if(keylen != 256/8 && keylen != 128/8)
		sysfatal("invalid chacha key length");
	if(rounds == 0)
		rounds = 20;
	s->rounds = rounds;
	if(keylen == 256/8) { /* recommended */
		load(&s->input[0], sigma, 4);
		load(&s->input[4], key, 8);
	}else{
		load(&s->input[0], tau, 4);
		load(&s->input[4], key, 4);
		load(&s->input[8], key, 4);
	}
	s->input[12] = 0;
	if(iv == nil){
		s->input[13] = 0;
		s->input[14] = 0;
		s->input[15] = 0;
	}else
		load(&s->input[13], iv, 3);
}

void
chacha_setblock(Chachastate *s, u32int blockno)
{
	s->input[12] = blockno;
}

static void
encryptblock(Chachastate *s, uchar *src, uchar *dst)
{
	u32int x[Blockwords];
	int i, rounds;

	rounds = s->rounds;
	x[0] = s->input[0];
	x[1] = s->input[1];
	x[2] = s->input[2];
	x[3] = s->input[3];
	x[4] = s->input[4];
	x[5] = s->input[5];
	x[6] = s->input[6];
	x[7] = s->input[7];
	x[8] = s->input[8];
	x[9] = s->input[9];
	x[10] = s->input[10];
	x[11] = s->input[11];
	x[12] = s->input[12];
	x[13] = s->input[13];
	x[14] = s->input[14];
	x[15] = s->input[15];

	for(i = rounds; i > 0; i -= 2) {
		QUARTERROUND(0, 4, 8,12)
		QUARTERROUND(1, 5, 9,13)
		QUARTERROUND(2, 6,10,14)
		QUARTERROUND(3, 7,11,15)

		QUARTERROUND(0, 5,10,15)
		QUARTERROUND(1, 6,11,12)
		QUARTERROUND(2, 7, 8,13)
		QUARTERROUND(3, 4, 9,14)
	}

#ifdef FULL_UNROLL
	ENCRYPT(src+0*4, x[0], s->input[0], dst+0*4);
	ENCRYPT(src+1*4, x[1], s->input[1], dst+1*4);
	ENCRYPT(src+2*4, x[2], s->input[2], dst+2*4);
	ENCRYPT(src+3*4, x[3], s->input[3], dst+3*4);
	ENCRYPT(src+4*4, x[4], s->input[4], dst+4*4);
	ENCRYPT(src+5*4, x[5], s->input[5], dst+5*4);
	ENCRYPT(src+6*4, x[6], s->input[6], dst+6*4);
	ENCRYPT(src+7*4, x[7], s->input[7], dst+7*4);
	ENCRYPT(src+8*4, x[8], s->input[8], dst+8*4);
	ENCRYPT(src+9*4, x[9], s->input[9], dst+9*4);
	ENCRYPT(src+10*4, x[10], s->input[10], dst+10*4);
	ENCRYPT(src+11*4, x[11], s->input[11], dst+11*4);
	ENCRYPT(src+12*4, x[12], s->input[12], dst+12*4);
	ENCRYPT(src+13*4, x[13], s->input[13], dst+13*4);
	ENCRYPT(src+14*4, x[14], s->input[14], dst+14*4);
	ENCRYPT(src+15*4, x[15], s->input[15], dst+15*4);
#else
	for(i=0; i<nelem(x); i+=4){
		ENCRYPT(src, x[i], s->input[i], dst);
		ENCRYPT(src+4, x[i+1], s->input[i+1], dst+4);
		ENCRYPT(src+8, x[i+2], s->input[i+2], dst+8);
		ENCRYPT(src+12, x[i+3], s->input[i+3], dst+12);
		src += 16;
		dst += 16;
	}
#endif

	s->input[12]++;
}

void
chacha_encrypt2(uchar *src, uchar *dst, usize bytes, Chachastate *s)
{
	uchar tmp[ChachaBsize];

	for(; bytes >= ChachaBsize; bytes -= ChachaBsize){
		encryptblock(s, src, dst);
		src += ChachaBsize;
		dst += ChachaBsize;
	}
	if(bytes > 0){
		memmove(tmp, src, bytes);
		encryptblock(s, tmp, tmp);
		memmove(dst, tmp, bytes);
	}
}

void
chacha_encrypt(uchar *buf, usize bytes, Chachastate *s)
{
	chacha_encrypt2(buf, buf, bytes, s);
}
