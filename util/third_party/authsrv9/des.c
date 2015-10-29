typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
#include "des.h"


static uchar initperm[] = {
58, 50, 42, 34, 26, 18, 10, 2,
60, 52, 44, 36, 28, 20, 12, 4,
62, 54, 46, 38, 30, 22, 14, 6,
64, 56, 48, 40, 32, 24, 16, 8,
57, 49, 41, 33, 25, 17, 9,  1,
59, 51, 43, 35, 27, 19, 11, 3,
61, 53, 45, 37, 29, 21, 13, 5,
63, 55, 47, 39, 31, 23, 15, 7,
};

static uchar finalperm[] = {
40, 8, 48, 16, 56, 24, 64, 32,
39, 7, 47, 15, 55, 23, 63, 31,
38, 6, 46, 14, 54, 22, 62, 30,
37, 5, 45, 13, 53, 21, 61, 29,
36, 4, 44, 12, 52, 20, 60, 28,
35, 3, 43, 11, 51, 19, 59, 27,
34, 2, 42, 10, 50, 18, 58, 26,
33, 1, 41,  9, 49, 17, 57, 25,
};

/* expand, for round function */
static uchar eperm[] = {
32,  1,  2,  3,  4,  5,
 4,  5,  6,  7,  8,  9,
 8,  9, 10, 11, 12, 13,
12, 13, 14, 15, 16, 17,
16, 17, 18, 19, 20, 21,
20, 21, 22, 23, 24, 25,
24, 25, 26, 27, 28, 29,
28, 29, 30, 31, 32,  1,
};

/* permute, for round function */
static uchar pperm[] = {
16,  7, 20, 21,
29, 12, 28, 17,
 1, 15, 23, 26,
 5, 18, 31, 10,
 2,  8, 24, 14,
32, 27,  3,  9,
19, 13, 30,  6,
22, 11,  4, 25,
};

static uchar sbox[8][64] = {
{ /* s1 */
	14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
	 3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
	 4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
	15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13,
},
{ /* s2 */
	15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
	 9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
	 0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
	 5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9,
},
{ /* s3 */
	10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
	 1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
	13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
	11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12,
},
{ /* s4 */
	 7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
	 1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
	10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
	15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14,
},
{ /* s5 */
	 2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
	 8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
	 4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
	15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3,
},
{ /* s6 */
	12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
	 0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
	 9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
	 7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13,
},
{ /* s7 */
	 4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
	 3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
	 1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
	10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12,
},
{ /* s8 */
	13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
	10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
	 7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
	 0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11,
},
};

/* key permutation 1 for c */
static uchar pc1perm0[] = {
57, 49, 41, 33, 25, 17,  9,
 1, 58, 50, 42, 34, 26, 18,
10,  2, 59, 51, 43, 35, 27,
19, 11,  3, 60, 52, 44, 36,
};

/* key permutation 1 for d */
static uchar pc1perm1[] = {
63, 55, 47, 39, 31, 23, 15,
 7, 62, 54, 46, 38, 30, 22,
14,  6, 61, 53, 45, 37, 29,
21, 13,  5, 28, 20, 12,  4,
};

/* key permutation 2 */
static uchar pc2perm[] = {
14, 17, 11, 24,  1,  5,
 3, 28, 15,  6, 21, 10,
23, 19, 12,  4, 26,  8,
16,  7, 27, 20, 13,  2,
41, 52, 31, 37, 47, 55,
30, 40, 51, 45, 33, 48,
44, 49, 39, 56, 34, 53,
46, 42, 50, 36, 29, 32,
};

static uchar keyshifts[] = {
1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1,
};

enum {
	Mask6 = (1<<6)-1,
	Mask7 = (1<<7)-1,
	Mask28 = (1<<28)-1,
};

static ulong
g32(uchar *p)
{
	ulong v;

	v = 0;
	v = (v<<8)|*p++;
	v = (v<<8)|*p++;
	v = (v<<8)|*p++;
	v = (v<<8)|*p++;
	return v;
}

static void
p32(uchar *p, ulong v)
{
	*p++ = v>>24;
	*p++ = v>>16;
	*p++ = v>>8;
	*p++ = v>>0;
}


/* input 32 bit, output 6 bit */
static uchar
perm6(ulong r, uchar *index)
{
	uchar v;
	int i;

	v = 0;
	for(i = 0; i < 6; i++)
		v |= ((r>>(32-index[i]))&1)<<(6-1-i);
	return v;
}

/* input 32 bit, output 32 bit */
static ulong
perm32(ulong r, uchar *index)
{
	ulong v;
	int i;

	v = 0;
	for(i = 0; i < 32; i++)
		v |= ((r>>(32-index[i]))&1)<<(32-1-i);
	return v;
}

/* input 64 bit, output 28 bit */
static ulong
perm28(uvlong r, uchar *index)
{
	ulong v;
	int i;

	v = 0;
	for(i = 0; i < 28; i++)
		v |= ((r>>(64-index[i]))&1)<<(28-1-i);
	return v;
}

/* input 56 bit, output 48 bit */
static uvlong
perm56(uvlong r, uchar *index)
{
	uvlong v;
	int i;

	v = 0;
	for(i = 0; i < 48; i++)
		v |= ((r>>(56-index[i]))&1)<<(48-1-i);
	return v;
}

/* input 64 bit, output 64 bit */
static uvlong
perm64(uvlong r, uchar *index)
{
	uvlong v;
	int i;

	v = 0;
	for(i = 0; i < 64; i++)
		v |= ((r>>(64-index[i]))&1)<<(64-1-i);
	return v;
}

static ulong
f(ulong r, uvlong k)
{
	ulong p;
	uchar ei, ki;
	int i, j;

	p = 0;
	for(i = 0; i < 8; i++) {
		j = 7-i;
		ei = perm6(r, eperm+i*6);
		ki = (k>>(j*6))&Mask6;
		p |= (ulong)sbox[i][ei^ki]<<(j*4);
	}
	return perm32(p, pperm);
}

static ulong
rotateleft28(ulong v, int i)
{
	ulong left, right;
	left = (v<<i)&Mask28;
	right = (v>>(28-i))&((1<<i)-1);
	return left|right;
}

static void
deskey(Desks *ks, uvlong k64)
{
	ulong c, d;
	int i;

	c = perm28(k64, pc1perm0);
	d = perm28(k64, pc1perm1);
	for(i = 0; i < 16; i++) {
		c = rotateleft28(c, keyshifts[i]);
		d = rotateleft28(d, keyshifts[i]);
		ks->keys[i] = perm56(((uvlong)c<<28)|d, pc2perm);
	}
}

static void
descrypt(Desks *ks, void *buf, int s, int e, int delta)
{
	ulong l, r, tmp;
	uvlong v;
	int i;

	v = g32(buf);
	v = (v<<32)|g32(buf+4);
	v = perm64(v, initperm);
	l = v>>32;
	r = v;
	for(i = s; i != e; i += delta) {
		tmp = r;
		r = l^f(r, ks->keys[i]);
		l = tmp;
	}
	v = r;
	v = (v<<32)|l;
	v = perm64(v, finalperm);
	p32(buf, v>>32);
	p32(buf+4, v);
}


void
deskey56(Desks *ks, uchar *k56)
{
	uvlong t, k;
	int i, j;

	t = 0;
	for(i = 0; i < 7; i++)
		t = (t<<8)|*k56++;
	k = 0;
	for(i = 0; i < 8; i++) {
		j = 8-1-i;
		k |= ((t>>(j*7))&Mask7)<<(j*8+1);
	}
	deskey(ks, k);
}

void
deskey64(Desks *ks, uchar *k64)
{
	uvlong k;
	int i;

	k = 0;
	for(i = 0; i < 8; i++)
		k = (k<<8)|*k64++;
	deskey(ks, k);
}

void
desenc(Desks *ks, void *buf)
{
	descrypt(ks, buf, 0, 16, 1);
}

void
desdec(Desks *ks, void *buf)
{
	descrypt(ks, buf, 15, -1, -1);
}
