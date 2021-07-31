#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>
#include <stdlib.h>	/* for random stuff */

typedef struct	Ticket		Ticket;
typedef struct	Ticketreq	Ticketreq;
typedef struct	Authenticator	Authenticator;

enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	DESKEYLEN=	7,		/* length of a des key for encrypt/decrypt */
	CHALLEN=	8		/* length of a challenge */
};

/* encryption numberings (anti-replay) */
enum
{
	AuthTreq=1,	/* ticket request */
	AuthChal=2,	/* challenge box request */
	AuthPass=3,	/* change password */
	AuthOK=4,	/* fixed length reply follows */
	AuthErr=5,	/* error follows */
	AuthMod=6,	/* modify user */
	AuthApop=7,	/* apop authentication for pop3 */
	AuthOKvar=9,	/* variable length reply follows */
	AuthChap=10,	/* chap authentication for ppp */
	AuthMSchap=11,	/* MS chap authentication for ppp */
	AuthCram=12,	/* CRAM verification for IMAP (RFC2195 & rfc2104) */
	AuthHttp=13,	/* http domain login */
	AuthVNC=14,	/* http domain login */


	AuthTs=64,	/* ticket encrypted with server's key */
	AuthTc,		/* ticket encrypted with client's key */
	AuthAs,		/* server generated authenticator */
	AuthAc,		/* client generated authenticator */
	AuthTp,		/* ticket encrypted with client's key for password change */
	AuthHr		/* http reply */
};

struct Ticketreq
{
	char	type;
	char	authid[NAMELEN];	/* server's encryption id */
	char	authdom[DOMLEN];	/* server's authentication domain */
	char	chal[CHALLEN];		/* challenge from server */
	char	hostid[NAMELEN];	/* host's encryption id */
	char	uid[NAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*NAMELEN+CHALLEN+DOMLEN+1)

struct Ticket
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];		/* server challenge */
	char	cuid[NAMELEN];		/* uid on client */
	char	suid[NAMELEN];		/* uid on server */
	char	key[DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*NAMELEN+DESKEYLEN+1)

struct Authenticator
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];
	ulong	id;			/* authenticator id, ++'d with each auth */
};
#define	AUTHENTLEN	(CHALLEN+4+1)

static	int	convT2M(Ticket*, char*, char*);
static	void	convM2T(char*, Ticket*, char*);
static	void	convM2Tnoenc(char*, Ticket*);
static	int	convA2M(Authenticator*, char*, char*);
static	void	convM2A(char*, Authenticator*, char*);
static	int	convTR2M(Ticketreq*, char*);
static	void	convM2TR(char*, Ticketreq*);
static	int	passtokey(char*, char*);

/*
 *	Data Encryption Standard
 *	D.P.Mitchell  83/06/08.
 *
 *	block_cipher(key, block, decrypting)
 */

static	long	ip_low(char [8]);
static	long	ip_high(char [8]);
static	void	fp(long, long, char[8]);
static	void	key_setup(char[DESKEYLEN], char[128]);
static	void	block_cipher(char[128], char[8], int);

/*
 * destructively encrypt the buffer, which
 * must be at least 8 characters long.
 */
static int
encrypt9p(void *key, void *vbuf, int n)
{
	char ekey[128], *buf;
	int i, r;

	if(n < 8)
		return 0;
	key_setup(key, ekey);
	buf = vbuf;
	n--;
	r = n % 7;
	n /= 7;
	for(i = 0; i < n; i++){
		block_cipher(ekey, buf, 0);
		buf += 7;
	}
	if(r)
		block_cipher(ekey, buf - 7 + r, 0);
	return 1;
}

/*
 * destructively decrypt the buffer, which
 * must be at least 8 characters long.
 */
static int
decrypt9p(void *key, void *vbuf, int n)
{
	char ekey[128], *buf;
	int i, r;

	if(n < 8)
		return 0;
	key_setup(key, ekey);
	buf = vbuf;
	n--;
	r = n % 7;
	n /= 7;
	buf += n * 7;
	if(r)
		block_cipher(ekey, buf - 7 + r, 1);
	for(i = 0; i < n; i++){
		buf -= 7;
		block_cipher(ekey, buf, 1);
	}
	return 1;
}

/*
 *	Tables for Combined S and P Boxes
 */

static long  s0p[] = {
0x00410100,0x00010000,0x40400000,0x40410100,0x00400000,0x40010100,0x40010000,0x40400000,
0x40010100,0x00410100,0x00410000,0x40000100,0x40400100,0x00400000,0x00000000,0x40010000,
0x00010000,0x40000000,0x00400100,0x00010100,0x40410100,0x00410000,0x40000100,0x00400100,
0x40000000,0x00000100,0x00010100,0x40410000,0x00000100,0x40400100,0x40410000,0x00000000,
0x00000000,0x40410100,0x00400100,0x40010000,0x00410100,0x00010000,0x40000100,0x00400100,
0x40410000,0x00000100,0x00010100,0x40400000,0x40010100,0x40000000,0x40400000,0x00410000,
0x40410100,0x00010100,0x00410000,0x40400100,0x00400000,0x40000100,0x40010000,0x00000000,
0x00010000,0x00400000,0x40400100,0x00410100,0x40000000,0x40410000,0x00000100,0x40010100,
};

static long  s1p[] = {
0x08021002,0x00000000,0x00021000,0x08020000,0x08000002,0x00001002,0x08001000,0x00021000,
0x00001000,0x08020002,0x00000002,0x08001000,0x00020002,0x08021000,0x08020000,0x00000002,
0x00020000,0x08001002,0x08020002,0x00001000,0x00021002,0x08000000,0x00000000,0x00020002,
0x08001002,0x00021002,0x08021000,0x08000002,0x08000000,0x00020000,0x00001002,0x08021002,
0x00020002,0x08021000,0x08001000,0x00021002,0x08021002,0x00020002,0x08000002,0x00000000,
0x08000000,0x00001002,0x00020000,0x08020002,0x00001000,0x08000000,0x00021002,0x08001002,
0x08021000,0x00001000,0x00000000,0x08000002,0x00000002,0x08021002,0x00021000,0x08020000,
0x08020002,0x00020000,0x00001002,0x08001000,0x08001002,0x00000002,0x08020000,0x00021000,
};

static long  s2p[] = {
0x20800000,0x00808020,0x00000020,0x20800020,0x20008000,0x00800000,0x20800020,0x00008020,
0x00800020,0x00008000,0x00808000,0x20000000,0x20808020,0x20000020,0x20000000,0x20808000,
0x00000000,0x20008000,0x00808020,0x00000020,0x20000020,0x20808020,0x00008000,0x20800000,
0x20808000,0x00800020,0x20008020,0x00808000,0x00008020,0x00000000,0x00800000,0x20008020,
0x00808020,0x00000020,0x20000000,0x00008000,0x20000020,0x20008000,0x00808000,0x20800020,
0x00000000,0x00808020,0x00008020,0x20808000,0x20008000,0x00800000,0x20808020,0x20000000,
0x20008020,0x20800000,0x00800000,0x20808020,0x00008000,0x00800020,0x20800020,0x00008020,
0x00800020,0x00000000,0x20808000,0x20000020,0x20800000,0x20008020,0x00000020,0x00808000,
};

static long  s3p[] = {
0x00080201,0x02000200,0x00000001,0x02080201,0x00000000,0x02080000,0x02000201,0x00080001,
0x02080200,0x02000001,0x02000000,0x00000201,0x02000001,0x00080201,0x00080000,0x02000000,
0x02080001,0x00080200,0x00000200,0x00000001,0x00080200,0x02000201,0x02080000,0x00000200,
0x00000201,0x00000000,0x00080001,0x02080200,0x02000200,0x02080001,0x02080201,0x00080000,
0x02080001,0x00000201,0x00080000,0x02000001,0x00080200,0x02000200,0x00000001,0x02080000,
0x02000201,0x00000000,0x00000200,0x00080001,0x00000000,0x02080001,0x02080200,0x00000200,
0x02000000,0x02080201,0x00080201,0x00080000,0x02080201,0x00000001,0x02000200,0x00080201,
0x00080001,0x00080200,0x02080000,0x02000201,0x00000201,0x02000000,0x02000001,0x02080200,
};

static long  s4p[] = {
0x01000000,0x00002000,0x00000080,0x01002084,0x01002004,0x01000080,0x00002084,0x01002000,
0x00002000,0x00000004,0x01000004,0x00002080,0x01000084,0x01002004,0x01002080,0x00000000,
0x00002080,0x01000000,0x00002004,0x00000084,0x01000080,0x00002084,0x00000000,0x01000004,
0x00000004,0x01000084,0x01002084,0x00002004,0x01002000,0x00000080,0x00000084,0x01002080,
0x01002080,0x01000084,0x00002004,0x01002000,0x00002000,0x00000004,0x01000004,0x01000080,
0x01000000,0x00002080,0x01002084,0x00000000,0x00002084,0x01000000,0x00000080,0x00002004,
0x01000084,0x00000080,0x00000000,0x01002084,0x01002004,0x01002080,0x00000084,0x00002000,
0x00002080,0x01002004,0x01000080,0x00000084,0x00000004,0x00002084,0x01002000,0x01000004,
};

static long  s5p[] = {
0x10000008,0x00040008,0x00000000,0x10040400,0x00040008,0x00000400,0x10000408,0x00040000,
0x00000408,0x10040408,0x00040400,0x10000000,0x10000400,0x10000008,0x10040000,0x00040408,
0x00040000,0x10000408,0x10040008,0x00000000,0x00000400,0x00000008,0x10040400,0x10040008,
0x10040408,0x10040000,0x10000000,0x00000408,0x00000008,0x00040400,0x00040408,0x10000400,
0x00000408,0x10000000,0x10000400,0x00040408,0x10040400,0x00040008,0x00000000,0x10000400,
0x10000000,0x00000400,0x10040008,0x00040000,0x00040008,0x10040408,0x00040400,0x00000008,
0x10040408,0x00040400,0x00040000,0x10000408,0x10000008,0x10040000,0x00040408,0x00000000,
0x00000400,0x10000008,0x10000408,0x10040400,0x10040000,0x00000408,0x00000008,0x10040008,
};

static long  s6p[] = {
0x00000800,0x00000040,0x00200040,0x80200000,0x80200840,0x80000800,0x00000840,0x00000000,
0x00200000,0x80200040,0x80000040,0x00200800,0x80000000,0x00200840,0x00200800,0x80000040,
0x80200040,0x00000800,0x80000800,0x80200840,0x00000000,0x00200040,0x80200000,0x00000840,
0x80200800,0x80000840,0x00200840,0x80000000,0x80000840,0x80200800,0x00000040,0x00200000,
0x80000840,0x00200800,0x80200800,0x80000040,0x00000800,0x00000040,0x00200000,0x80200800,
0x80200040,0x80000840,0x00000840,0x00000000,0x00000040,0x80200000,0x80000000,0x00200040,
0x00000000,0x80200040,0x00200040,0x00000840,0x80000040,0x00000800,0x80200840,0x00200000,
0x00200840,0x80000000,0x80000800,0x80200840,0x80200000,0x00200840,0x00200800,0x80000800,
};

static long  s7p[] = {
0x04100010,0x04104000,0x00004010,0x00000000,0x04004000,0x00100010,0x04100000,0x04104010,
0x00000010,0x04000000,0x00104000,0x00004010,0x00104010,0x04004010,0x04000010,0x04100000,
0x00004000,0x00104010,0x00100010,0x04004000,0x04104010,0x04000010,0x00000000,0x00104000,
0x04000000,0x00100000,0x04004010,0x04100010,0x00100000,0x00004000,0x04104000,0x00000010,
0x00100000,0x00004000,0x04000010,0x04104010,0x00004010,0x04000000,0x00000000,0x00104000,
0x04100010,0x04004010,0x04004000,0x00100010,0x04104000,0x00000010,0x00100010,0x04004000,
0x04104010,0x00100000,0x04100000,0x04000010,0x00104000,0x00004010,0x04004010,0x04100000,
0x00000010,0x04104000,0x00104010,0x00000000,0x04000000,0x04100010,0x00004000,0x00104010,
};

/*
 *	DES electronic codebook encryption of one block
 */
static void
block_cipher(char expanded_key[128], char text[8], int decrypting)
{
	char *key;
	long crypto, temp, right, left;
	int i, key_offset;

	key = expanded_key;
	left = ip_low(text);
	right = ip_high(text);
	if (decrypting) {
		key_offset = 16;
		key = key + 128 - 8;
	} else
		key_offset = 0;
	for (i = 0; i < 16; i++) {
		temp = (right << 1) | ((right >> 31) & 1);
		crypto  = s0p[(temp         & 0x3f) ^ *key++];
		crypto |= s1p[((temp >>  4) & 0x3f) ^ *key++];
		crypto |= s2p[((temp >>  8) & 0x3f) ^ *key++];
		crypto |= s3p[((temp >> 12) & 0x3f) ^ *key++];
		crypto |= s4p[((temp >> 16) & 0x3f) ^ *key++];
		crypto |= s5p[((temp >> 20) & 0x3f) ^ *key++];
		crypto |= s6p[((temp >> 24) & 0x3f) ^ *key++];
		temp = ((right & 1) << 5) | ((right >> 27) & 0x1f);
		crypto |= s7p[temp ^ *key++];
		temp = left;
		left = right;
		right = temp ^ crypto;
		key -= key_offset;
	}
	/*
	 *	standard final permutation (IPI)
	 *	left and right are reversed here
	 */
	fp(right, left, text);
}

/*
 *	Initial Permutation
 */
static long iptab[] = {
	0x00000000, 0x00008000, 0x00000000, 0x00008000,
	0x00000080, 0x00008080, 0x00000080, 0x00008080
};

static long
ip_low(char block[8])
{
	int i;
	long l;

	l = 0;
	for(i = 0; i < 8; i++){
		l |= iptab[(block[i] >> 4) & 7] >> i;
		l |= iptab[block[i] & 7] << (16 - i);
	}
	return l;
}

static long
ip_high(char block[8])
{
	int i;
	long l;

	l = 0;
	for(i = 0; i < 8; i++){
		l |= iptab[(block[i] >> 5) & 7] >> i;
		l |= iptab[(block[i] >> 1) & 7] << (16 - i);
	}
	return l;
}

/*
 *	Final Permutation
 */
static unsigned long	fptab[] = {
0x00000000,0x80000000,0x00800000,0x80800000,0x00008000,0x80008000,0x00808000,0x80808000,
0x00000080,0x80000080,0x00800080,0x80800080,0x00008080,0x80008080,0x00808080,0x80808080,
};

static void
fp(long left, long right, char text[8])
{
	unsigned long ta[2], t, v[2];
	int i, j, sh;

	ta[0] = right;
	ta[1] = left;
	v[0] = v[1] = 0;
	for(i = 0; i < 2; i++){
		t = ta[i];
		sh = i;
		for(j = 0; j < 4; j++){
			v[1] |= fptab[t & 0xf] >> sh;
			t >>= 4;
			v[0] |= fptab[t & 0xf] >> sh;
			t >>= 4;
			sh += 2;
		}
	}
	for(i = 0; i < 2; i++)
		for(j = 0; j < 4; j++){
			*text++ = v[i];
			v[i] >>= 8;
		}
}

/*
 *	Key set-up
 */
static uchar keyexpand[][15][2] = {
	{   3,  2,   9,  8,  18,  8,  27, 32,  33,  2,  42, 16,  48,  8,  65, 16, 
	   74,  2,  80,  2,  89,  4,  99, 16, 104,  4, 122, 32,   0,  0, },
	{   1,  4,   8,  1,  18,  4,  25, 32,  34, 32,  41,  8,  50,  8,  59, 32, 
	   64, 16,  75,  4,  90,  1,  97, 16, 106,  2, 112,  2, 123,  1, },
	{   2,  1,  19,  8,  35,  1,  40,  1,  50,  4,  57, 32,  75,  2,  80, 32, 
	   89,  1,  96, 16, 107,  4, 120,  8,   0,  0,   0,  0,   0,  0, },
	{   4, 32,  20,  2,  31,  4,  37, 32,  47,  1,  54,  1,  63,  2,  68,  1, 
	   78,  4,  84,  8, 101, 16, 108,  4, 119, 16, 126,  8,   0,  0, },
	{   5,  4,  15,  4,  21, 32,  31,  1,  38,  1,  47,  2,  53,  2,  68,  8, 
	   85, 16,  92,  4, 103, 16, 108, 32, 118, 32, 124,  2,   0,  0, },
	{  15,  2,  21,  2,  39,  8,  46, 16,  55, 32,  61,  1,  71, 16,  76, 32, 
	   86, 32,  93,  4, 102,  2, 108, 16, 117,  8, 126,  1,   0,  0, },
	{  14, 16,  23, 32,  29,  1,  38,  8,  52,  2,  63,  4,  70,  2,  76, 16, 
	   85,  8, 100,  1, 110,  4, 116,  8, 127,  8,   0,  0,   0,  0, },
	{   1,  8,   8, 32,  17,  1,  24, 16,  35,  4,  50,  1,  57, 16,  67,  8, 
	   83,  1,  88,  1,  98,  4, 105, 32, 114, 32, 123,  2,   0,  0, },
	{   0,  1,  11, 16,  16,  4,  35,  2,  40, 32,  49,  1,  56, 16,  65,  2, 
	   74, 16,  80,  8,  99,  8, 115,  1, 121,  4,   0,  0,   0,  0, },
	{   9, 16,  18,  2,  24,  2,  33,  4,  43, 16,  48,  4,  66, 32,  73,  8, 
	   82,  8,  91, 32,  97,  2, 106, 16, 112,  8, 122,  1,   0,  0, },
	{  14, 32,  21,  4,  30,  2,  36, 16,  45,  8,  60,  1,  69,  2,  87,  8, 
	   94, 16, 103, 32, 109,  1, 118,  8, 124, 32,   0,  0,   0,  0, },
	{   7,  4,  14,  2,  20, 16,  29,  8,  44,  1,  54,  4,  60,  8,  71,  8, 
	   78, 16,  87, 32,  93,  1, 102,  8, 116,  2, 125,  4,   0,  0, },
	{   7,  2,  12,  1,  22,  4,  28,  8,  45, 16,  52,  4,  63, 16,  70,  8, 
	   84,  2,  95,  4, 101, 32, 111,  1, 118,  1,   0,  0,   0,  0, },
	{   6, 16,  13, 16,  20,  4,  31, 16,  36, 32,  46, 32,  53,  4,  62,  2, 
	   69, 32,  79,  1,  86,  1,  95,  2, 101,  2, 119,  8,   0,  0, },
	{   0, 32,  10,  8,  19, 32,  25,  2,  34, 16,  40,  8,  59,  8,  66,  2, 
	   72,  2,  81,  4,  91, 16,  96,  4, 115,  2, 121,  8,   0,  0, },
	{   3, 16,  10,  4,  17, 32,  26, 32,  33,  8,  42,  8,  51, 32,  57,  2, 
	   67,  4,  82,  1,  89, 16,  98,  2, 104,  2, 113,  4, 120,  1, },
	{   1, 16,  11,  8,  27,  1,  32,  1,  42,  4,  49, 32,  58, 32,  67,  2, 
	   72, 32,  81,  1,  88, 16,  99,  4, 114,  1,   0,  0,   0,  0, },
	{   6, 32,  12,  2,  23,  4,  29, 32,  39,  1,  46,  1,  55,  2,  61,  2, 
	   70,  4,  76,  8,  93, 16, 100,  4, 111, 16, 116, 32,   0,  0, },
	{   6,  2,  13, 32,  23,  1,  30,  1,  39,  2,  45,  2,  63,  8,  77, 16, 
	   84,  4,  95, 16, 100, 32, 110, 32, 117,  4, 127,  4,   0,  0, },
	{   4,  1,  13,  2,  31,  8,  38, 16,  47, 32,  53,  1,  62,  8,  68, 32, 
	   78, 32,  85,  4,  94,  2, 100, 16, 109,  8, 127,  2,   0,  0, },
	{   5, 16,  15, 32,  21,  1,  30,  8,  44,  2,  55,  4,  61, 32,  68, 16, 
	   77,  8,  92,  1, 102,  4, 108,  8, 126, 16,   0,  0,   0,  0, },
	{   2,  8,   9,  1,  16, 16,  27,  4,  42,  1,  49, 16,  58,  2,  75,  1, 
	   80,  1,  90,  4,  97, 32, 106, 32, 113,  8, 120, 32,   0,  0, },
	{   2,  4,   8,  4,  27,  2,  32, 32,  41,  1,  48, 16,  59,  4,  66, 16, 
	   72,  8,  91,  8, 107,  1, 112,  1, 123, 16,   0,  0,   0,  0, },
	{   3,  8,  10,  2,  16,  2,  25,  4,  35, 16,  40,  4,  59,  2,  65,  8, 
	   74,  8,  83, 32,  89,  2,  98, 16, 104,  8, 121, 16,   0,  0, },
	{   4,  2,  13,  4,  22,  2,  28, 16,  37,  8,  52,  1,  62,  4,  79,  8, 
	   86, 16,  95, 32, 101,  1, 110,  8, 126, 32,   0,  0,   0,  0, },
	{   5, 32,  12, 16,  21,  8,  36,  1,  46,  4,  52,  8,  70, 16,  79, 32, 
	   85,  1,  94,  8, 108,  2, 119,  4, 126,  2,   0,  0,   0,  0, },
	{   5,  2,  14,  4,  20,  8,  37, 16,  44,  4,  55, 16,  60, 32,  76,  2, 
	   87,  4,  93, 32, 103,  1, 110,  1, 119,  2, 124,  1,   0,  0, },
	{   7, 32,  12,  4,  23, 16,  28, 32,  38, 32,  45,  4,  54,  2,  60, 16, 
	   71,  1,  78,  1,  87,  2,  93,  2, 111,  8, 118, 16, 125, 16, },
	{   1,  1,  11, 32,  17,  2,  26, 16,  32,  8,  51,  8,  64,  2,  73,  4, 
	   83, 16,  88,  4, 107,  2, 112, 32, 122,  8,   0,  0,   0,  0, },
	{   0,  4,   9, 32,  18, 32,  25,  8,  34,  8,  43, 32,  49,  2,  58, 16, 
	   74,  1,  81, 16,  90,  2,  96,  2, 105,  4, 115, 16, 122,  4, },
	{   2,  2,  19,  1,  24,  1,  34,  4,  41, 32,  50, 32,  57,  8,  64, 32, 
	   73,  1,  80, 16,  91,  4, 106,  1, 113, 16, 123,  8,   0,  0, },
	{   3,  4,  10, 16,  16,  8,  35,  8,  51,  1,  56,  1,  67, 16,  72,  4, 
	   91,  2,  96, 32, 105,  1, 112, 16, 121,  2,   0,  0,   0,  0, },
	{   4, 16,  15,  1,  22,  1,  31,  2,  37,  2,  55,  8,  62, 16,  69, 16, 
	   76,  4,  87, 16,  92, 32, 102, 32, 109,  4, 118,  2, 125, 32, },
	{   6,  4,  23,  8,  30, 16,  39, 32,  45,  1,  54,  8,  70, 32,  77,  4, 
	   86,  2,  92, 16, 101,  8, 116,  1, 125,  2,   0,  0,   0,  0, },
	{   4,  4,  13,  1,  22,  8,  36,  2,  47,  4,  53, 32,  63,  1,  69,  8, 
	   84,  1,  94,  4, 100,  8, 117, 16, 127, 32,   0,  0,   0,  0, },
	{   3, 32,   8, 16,  19,  4,  34,  1,  41, 16,  50,  2,  56,  2,  67,  1, 
	   72,  1,  82,  4,  89, 32,  98, 32, 105,  8, 114,  8, 121,  1, },
	{   1, 32,  19,  2,  24, 32,  33,  1,  40, 16,  51,  4,  64,  8,  83,  8, 
	   99,  1, 104,  1, 114,  4, 120,  4,   0,  0,   0,  0,   0,  0, },
	{   8,  2,  17,  4,  27, 16,  32,  4,  51,  2,  56, 32,  66,  8,  75, 32, 
	   81,  2,  90, 16,  96,  8, 115,  8, 122,  2,   0,  0,   0,  0, },
	{   2, 16,  18,  1,  25, 16,  34,  2,  40,  2,  49,  4,  59, 16,  66,  4, 
	   73, 32,  82, 32,  89,  8,  98,  8, 107, 32, 113,  2, 123,  4, },
	{   7,  1,  13,  8,  28,  1,  38,  4,  44,  8,  61, 16,  71, 32,  77,  1, 
	   86,  8, 100,  2, 111,  4, 117, 32, 124, 16,   0,  0,   0,  0, },
	{  12,  8,  29, 16,  36,  4,  47, 16,  52, 32,  62, 32,  68,  2,  79,  4, 
	   85, 32,  95,  1, 102,  1, 111,  2, 117,  2, 126,  4,   0,  0, },
	{   5,  1,  15, 16,  20, 32,  30, 32,  37,  4,  46,  2,  52, 16,  61,  8, 
	   70,  1,  79,  2,  85,  2, 103,  8, 110, 16, 119, 32, 124,  4, },
	{   0, 16,   9,  2,  18, 16,  24,  8,  43,  8,  59,  1,  65,  4,  75, 16, 
	   80,  4,  99,  2, 104, 32, 113,  1, 123, 32,   0,  0,   0,  0, },
	{  10, 32,  17,  8,  26,  8,  35, 32,  41,  2,  50, 16,  56,  8,  66,  1, 
	   73, 16,  82,  2,  88,  2,  97,  4, 107, 16, 112,  4, 121, 32, },
	{   0,  2,  11,  1,  16,  1,  26,  4,  33, 32,  42, 32,  49,  8,  58,  8, 
	   65,  1,  72, 16,  83,  4,  98,  1, 105, 16, 114,  2,   0,  0, },
	{   8,  8,  27,  8,  43,  1,  48,  1,  58,  4,  64,  4,  83,  2,  88, 32, 
	   97,  1, 104, 16, 115,  4, 122, 16,   0,  0,   0,  0,   0,  0, },
	{   5,  8,  14,  1,  23,  2,  29,  2,  47,  8,  54, 16,  63, 32,  68,  4, 
	   79, 16,  84, 32,  94, 32, 101,  4, 110,  2, 116, 16, 127,  1, },
	{   4,  8,  15,  8,  22, 16,  31, 32,  37,  1,  46,  8,  60,  2,  69,  4, 
	   78,  2,  84, 16,  93,  8, 108,  1, 118,  4,   0,  0,   0,  0, },
	{   7, 16,  14,  8,  28,  2,  39,  4,  45, 32,  55,  1,  62,  1,  76,  1, 
	   86,  4,  92,  8, 109, 16, 116,  4, 125,  1,   0,  0,   0,  0, },
	{   1,  2,  11,  4,  26,  1,  33, 16,  42,  2,  48,  2,  57,  4,  64,  1, 
	   74,  4,  81, 32,  90, 32,  97,  8, 106,  8, 115, 32, 120, 16, },
	{   2, 32,  11,  2,  16, 32,  25,  1,  32, 16,  43,  4,  58,  1,  75,  8, 
	   91,  1,  96,  1, 106,  4, 113, 32,   0,  0,   0,  0,   0,  0, },
	{   3,  1,   9,  4,  19, 16,  24,  4,  43,  2,  48, 32,  57,  1,  67, 32, 
	   73,  2,  82, 16,  88,  8, 107,  8, 120,  2,   0,  0,   0,  0, },
	{   0,  8,  10,  1,  17, 16,  26,  2,  32,  2,  41,  4,  51, 16,  56,  4, 
	   65, 32,  74, 32,  81,  8,  90,  8,  99, 32, 105,  2, 114, 16, },
	{   6,  1,  20,  1,  30,  4,  36,  8,  53, 16,  60,  4,  69,  1,  78,  8, 
	   92,  2, 103,  4, 109, 32, 119,  1, 125,  8,   0,  0,   0,  0, },
	{   7,  8,  21, 16,  28,  4,  39, 16,  44, 32,  54, 32,  61,  4,  71,  4, 
	   77, 32,  87,  1,  94,  1, 103,  2, 109,  2, 124,  8,   0,  0, },
	{   6,  8,  12, 32,  22, 32,  29,  4,  38,  2,  44, 16,  53,  8,  71,  2, 
	   77,  2,  95,  8, 102, 16, 111, 32, 117,  1, 127, 16,   0,  0, }
};

static void
key_setup(char key[DESKEYLEN], char *ek)
{
	int i, j, k, mask;
	uchar (*x)[2];

	memset(ek, 0, 128);
	x = keyexpand[0];
	for(i = 0; i < 7; i++){
		k = key[i];
		for(mask = 0x80; mask; mask >>= 1){
			if(k & mask)
				for(j = 0; j < 15; j++)
					ek[x[j][0]] |= x[j][1];
			x += 15;
		}
	}
}

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	VLONG(q)	p[0] = (q); p[1] = (q)>>8; p[2] = (q)>>16; p[3] = (q)>>24; p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(p, f->x, n); p += n

static int
convT2M(Ticket *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	STRING(cuid, NAMELEN);
	STRING(suid, NAMELEN);
	STRING(key, DESKEYLEN);
	n = p - (uchar*)ap;
	if(key)
		encrypt9p(key, ap, n);
	return n;
}

int
convA2M(Authenticator *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	LONG(id);
	n = p - (uchar*)ap;
	if(key)
		encrypt9p(key, ap, n);
	return n;
}

#undef CHAR
#undef SHORT
#undef VLONG
#undef LONG
#undef STRING

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	VLONG(q)	q = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(f->x, p, n); p += n

void
convM2A(char *ap, Authenticator *f, char *key)
{
	uchar *p;

	if(key)
		decrypt9p(key, ap, AUTHENTLEN);
	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	LONG(id);
	USED(p);
}

void
convM2T(char *ap, Ticket *f, char *key)
{
	uchar *p;

	if(key)
		decrypt9p(key, ap, TICKETLEN);
	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	STRING(cuid, NAMELEN);
	f->cuid[NAMELEN-1] = 0;
	STRING(suid, NAMELEN);
	f->suid[NAMELEN-1] = 0;
	STRING(key, DESKEYLEN);
	USED(p);
}

#undef CHAR
#undef SHORT
#undef LONG
#undef VLONG
#undef STRING

static int
passtokey(char *key, char *p)
{
	uchar buf[NAMELEN], *t;
	int i, n;

	n = strlen(p);
	if(n >= NAMELEN)
		n = NAMELEN-1;
	memset(buf, ' ', 8);
	t = buf;
	strncpy((char*)t, p, n);
	t[n] = 0;
	memset(key, 0, DESKEYLEN);
	for(;;){
		for(i = 0; i < DESKEYLEN; i++)
			key[i] = (t[i] >> i) + (t[i+1] << (8 - (i+1)));
		if(n <= 8)
			return 1;
		n -= 8;
		t += 8;
		if(n < 8){
			t -= 8 - n;
			n = 8;
		}
		encrypt9p(key, t, 8);
	}
	return 1;	/* not reached */
}

static char authkey[DESKEYLEN];
static char *authid;
static char *authdom;
static char cchal[CHALLEN];
static char schal[CHALLEN];

static void
a9p1init(void)
{
	int n, fd;
	static char abuf[200];
	char *af, *f[4];

	af = autharg;
	if(af == nil)
		af = "/etc/u9fs.key";

	if((fd = open(af, OREAD)) < 0)
		sysfatal("can't open key file '%s'", af);

	if((n = readn(fd, abuf, sizeof(abuf)-1)) < 0)
		sysfatal("can't read key file '%s'", af);
	abuf[n] = '\0';

	if(getfields(abuf, f, nelem(f), 0, "\n") != 3)
		sysfatal("key file '%s' not exactly 3 lines", af);

	passtokey(authkey, f[0]);
	authid = f[1];
	authdom = f[2];
}

static int
a9p1session(Fcall *rx, Fcall *tx)
{
	if(rx->nchal != CHALLEN)
		return -1;

	memmove(cchal, rx->chal, CHALLEN);
	randombytes((uchar*)schal, CHALLEN);
	memset(tx->chal, 0, CHALLEN+NAMELEN+DOMLEN);
	memmove(tx->chal, schal, CHALLEN);
	tx->nchal = CHALLEN;
	tx->authid = authid;
	tx->authdom = authdom;
	return 0;
}

static int
a9p1attach(Fcall *rx, Fcall *tx)
{
	static Ticket t;
	Authenticator a;

	if(rx->nauth != TICKETLEN+AUTHENTLEN){
		fprint(2, "bad length in attach\n");
		return -1;
	}

	convM2T((char*)rx->auth, &t, authkey);
	if(t.num != AuthTs){
		fprint(2, "bad AuthTs in attach\n");
		return -1;
	}
	if(memcmp(t.chal, schal, CHALLEN) != 0){
		fprint(2, "bad challenge in attach\n");
		return -1;
	}

	if(strcmp(t.cuid, rx->uname) != 0){
		fprint(2, "bad uid %s %s\n", t.cuid, rx->uname);
		return -1;
	}

	rx->uname = t.suid;

	convM2A((char*)rx->auth+TICKETLEN, &a, t.key);
	if(a.num != AuthAc)
		return -1;
	if(memcmp(a.chal, schal, CHALLEN) != 0)
		return -1;

	a.num = AuthAs;
	memmove(a.chal, cchal, CHALLEN);
	convA2M(&a, (char*)tx->rauth, t.key);
	memset(t.key, 0, sizeof t.key);
	tx->nrauth = AUTHENTLEN;
	return 0;
}

Auth auth9p1 = {
	"9p1",
	a9p1session,
	a9p1attach,
	a9p1init,
};
