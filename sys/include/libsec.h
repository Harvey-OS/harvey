/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * AES definitions
 */

enum
{
	AESbsize=	16,
	AESmaxkey=	32,
	AESmaxrounds=	14
};

typedef struct AESstate AESstate;
struct AESstate
{
	u32	setup;
	int	rounds;
	int	keybytes;
	u32	ctrsz;
	u8	key[AESmaxkey];			/* unexpanded key */
	u32	ekey[4*(AESmaxrounds + 1)];	/* encryption key */
	u32	dkey[4*(AESmaxrounds + 1)];	/* decryption key */
	u8	ivec[AESbsize];			/* initialization vector */
	u8	mackey[3 * AESbsize];		/* 3 XCBC mac 96 keys */
};

/* block ciphers */
void	aes_encrypt(u32 rk[], int Nr, u8 pt[16],
			u8 ct[16]);
void	aes_decrypt(u32 rk[], int Nr, u8 ct[16],
			u8 pt[16]);

void	setupAESstate(AESstate *s, u8 key[], int keybytes,
			  u8 *ivec);
void	aesCBCencrypt(u8 *p, int len, AESstate *s);
void	aesCBCdecrypt(u8 *p, int len, AESstate *s);
void	aesCTRdecrypt(u8 *p, int len, AESstate *s);
void	aesCTRencrypt(u8 *p, int len, AESstate *s);

void	setupAESXCBCstate(AESstate *s);
u8 *	aesXCBCmac(u8 *p, int len, AESstate *s);

/*
 * Blowfish Definitions
 */

enum
{
	BFbsize	= 8,
	BFrounds= 16
};

/* 16-round Blowfish */
typedef struct BFstate BFstate;
struct BFstate
{
	u32	setup;

	u8	key[56];
	u8	ivec[8];

	u32 	pbox[BFrounds+2];
	u32	sbox[1024];
};

void	setupBFstate(BFstate *s, u8 key[], int keybytes,
			 u8 *ivec);
void	bfCBCencrypt(u8*, int, BFstate*);
void	bfCBCdecrypt(u8*, int, BFstate*);
void	bfECBencrypt(u8*, int, BFstate*);
void	bfECBdecrypt(u8*, int, BFstate*);

/*
 * DES definitions
 */

enum
{
	DESbsize=	8
};

/* single des */
typedef struct DESstate DESstate;
struct DESstate
{
	u32	setup;
	u8	key[8];		/* unexpanded key */
	u32	expanded[32];	/* expanded key */
	u8	ivec[8];	/* initialization vector */
};

void	setupDESstate(DESstate *s, u8 key[8], u8 *ivec);
void	des_key_setup(u8[8], u32[32]);
void	block_cipher(u32[32], u8[8], int);
void	desCBCencrypt(u8*, int, DESstate*);
void	desCBCdecrypt(u8*, int, DESstate*);
void	desECBencrypt(u8*, int, DESstate*);
void	desECBdecrypt(u8*, int, DESstate*);

/* for backward compatibility with 7-byte DES key format */
void	des56to64(u8 *k56, u8 *k64);
void	des64to56(u8 *k64, u8 *k56);
void	key_setup(u8[7], u32[32]);

/* triple des encrypt/decrypt orderings */
enum {
	DES3E=		0,
	DES3D=		1,
	DES3EEE=	0,
	DES3EDE=	2,
	DES3DED=	5,
	DES3DDD=	7
};

typedef struct DES3state DES3state;
struct DES3state
{
	u32	setup;
	u8	key[3][8];		/* unexpanded key */
	u32	expanded[3][32];	/* expanded key */
	u8	ivec[8];		/* initialization vector */
};

void	setupDES3state(DES3state *s, u8 key[3][8], u8 *ivec);
void	triple_block_cipher(u32 keys[3][32], u8[8], int);
void	des3CBCencrypt(u8*, int, DES3state*);
void	des3CBCdecrypt(u8*, int, DES3state*);
void	des3ECBencrypt(u8*, int, DES3state*);
void	des3ECBdecrypt(u8*, int, DES3state*);

/*
 * digests
 */

enum
{
	SHA1dlen=	20,	/* SHA digest length */
	SHA2_224dlen=	28,	/* SHA-224 digest length */
	SHA2_256dlen=	32,	/* SHA-256 digest length */
	SHA2_384dlen=	48,	/* SHA-384 digest length */
	SHA2_512dlen=	64,	/* SHA-512 digest length */
	MD4dlen=	16,	/* MD4 digest length */
	MD5dlen=	16,	/* MD5 digest length */
	AESdlen=	16,	/* TODO: see rfc */

	Hmacblksz	= 64,	/* in bytes; from rfc2104 */
};

typedef struct DigestState DigestState;
struct DigestState
{
	u64	len;
	union {
		u32	state[8];
		u64	bstate[8];
	};
	u8	buf[256];
	int	blen;
	char	malloced;
	char	seeded;
};
typedef struct DigestState SHAstate;	/* obsolete name */
typedef struct DigestState SHA1state;
typedef struct DigestState SHA2_224state;
typedef struct DigestState SHA2_256state;
typedef struct DigestState SHA2_384state;
typedef struct DigestState SHA2_512state;
typedef struct DigestState MD5state;
typedef struct DigestState MD4state;
typedef struct DigestState AEShstate;

DigestState*	md4(u8*, u32, u8*, DigestState*);
DigestState*	md5(u8*, u32, u8*, DigestState*);
DigestState*	sha1(u8*, u32, u8*, DigestState*);
DigestState*	sha2_224(u8*, u32, u8*, DigestState*);
DigestState*	sha2_256(u8*, u32, u8*, DigestState*);
DigestState*	sha2_384(u8*, u32, u8*, DigestState*);
DigestState*	sha2_512(u8*, u32, u8*, DigestState*);
DigestState*	aes(u8*, u32, u8*, DigestState*);
DigestState*	hmac_x(u8 *p, u32 len, u8 *key,
			   u32 klen,
			   u8 *digest, DigestState *s,
			   DigestState*(*x)(u8*, u32, u8*, DigestState*),
			   int xlen);
DigestState*	hmac_md5(u8*, u32, u8*, u32,
			     u8*,
			     DigestState*);
DigestState*	hmac_sha1(u8*, u32, u8*, u32,
			      u8*,
			      DigestState*);
DigestState*	hmac_sha2_224(u8*, u32, u8*, u32,
				  u8*, DigestState*);
DigestState*	hmac_sha2_256(u8*, u32, u8*, u32,
				  u8*, DigestState*);
DigestState*	hmac_sha2_384(u8*, u32, u8*, u32,
				  u8*, DigestState*);
DigestState*	hmac_sha2_512(u8*, u32, u8*, u32,
				  u8*, DigestState*);
DigestState*	hmac_aes(u8*, u32, u8*, u32,
			     u8*,
			     DigestState*);
char*		md5pickle(MD5state*);
MD5state*	md5unpickle(char*);
char*		sha1pickle(SHA1state*);
SHA1state*	sha1unpickle(char*);

/*
 * random number generation
 */
void	genrandom(u8 *buf, int nbytes);
void	prng(u8 *buf, int nbytes);
u32	fastrand(void);
u32	nfastrand(u32);

/*
 * primes
 */
void	genprime(mpint *p, int n, int accuracy); /* generate n-bit probable prime */
void	gensafeprime(mpint *p, mpint *alpha, int n, int accuracy); /* prime & generator */
void	genstrongprime(mpint *p, int n, int accuracy); /* generate n-bit strong prime */
void	DSAprimes(mpint *q, mpint *p, u8 seed[SHA1dlen]);
int	probably_prime(mpint *n, int nrep);	/* miller-rabin test */
int	smallprimetest(mpint *p);  /* returns -1 if not prime, 0 otherwise */

/*
 * rc4
 */
typedef struct RC4state RC4state;
struct RC4state
{
	 u8	state[256];
	 u8	x;
	 u8	y;
};

void	setupRC4state(RC4state*, u8*, int);
void	rc4(RC4state*, u8*, int);
void	rc4skip(RC4state*, int);
void	rc4back(RC4state*, int);

/*
 * rsa
 */
typedef struct RSApub RSApub;
typedef struct RSApriv RSApriv;
typedef struct PEMChain PEMChain;

/* public/encryption key */
struct RSApub
{
	mpint	*n;	/* modulus */
	mpint	*ek;	/* exp (encryption key) */
};

/* private/decryption key */
struct RSApriv
{
	RSApub	pub;

	mpint	*dk;	/* exp (decryption key) */

	/* precomputed values to help with chinese remainder theorem calc */
	mpint	*p;
	mpint	*q;
	mpint	*kp;	/* dk mod p-1 */
	mpint	*kq;	/* dk mod q-1 */
	mpint	*c2;	/* (inv p) mod q */
};

struct PEMChain{
	PEMChain*next;
	u8	*pem;
	int	pemlen;
};

RSApriv*	rsagen(int nlen, int elen, int rounds);
RSApriv*	rsafill(mpint *n, mpint *e, mpint *d, mpint *p, mpint *q);
mpint*		rsaencrypt(RSApub *k, mpint *in, mpint *out);
mpint*		rsadecrypt(RSApriv *k, mpint *in, mpint *out);
RSApub*		rsapuballoc(void);
void		rsapubfree(RSApub*);
RSApriv*	rsaprivalloc(void);
void		rsaprivfree(RSApriv*);
RSApub*		rsaprivtopub(RSApriv*);
RSApub*		X509toRSApub(u8*, int, char*, int);
u8 *		RSApubtoasn1(RSApub*, int*);
RSApub*		asn1toRSApub(u8*, int);
RSApriv*	asn1toRSApriv(u8*, int);
void		asn1dump(u8 *der, int len);
u8 *		decodePEM(char *s, char *type, int *len,
				  char **new_s);
PEMChain*	decodepemchain(char *s, char *type);
u8 *		X509gen(RSApriv *priv, char *subj,
				u32 valid[2],
				int *certlen);
u8 *		X509req(RSApriv *priv, char *subj, int *certlen);
char*		X509verify(u8 *cert, int ncert, RSApub *pk);
void		X509dump(u8 *cert, int ncert);



/*
 * elgamal
 */
typedef struct EGpub EGpub;
typedef struct EGpriv EGpriv;
typedef struct EGsig EGsig;

/* public/encryption key */
struct EGpub
{
	mpint	*p;	/* modulus */
	mpint	*alpha;	/* generator */
	mpint	*key;	/* (encryption key) alpha**secret mod p */
};

/* private/decryption key */
struct EGpriv
{
	EGpub	pub;
	mpint	*secret;	/* (decryption key) */
};

/* signature */
struct EGsig
{
	mpint	*r, *s;
};

EGpriv*		eggen(int nlen, int rounds);
mpint*		egencrypt(EGpub *k, mpint *in, mpint *out);	/* deprecated */
mpint*		egdecrypt(EGpriv *k, mpint *in, mpint *out);
EGsig*		egsign(EGpriv *k, mpint *m);
int		egverify(EGpub *k, EGsig *sig, mpint *m);
EGpub*		egpuballoc(void);
void		egpubfree(EGpub*);
EGpriv*		egprivalloc(void);
void		egprivfree(EGpriv*);
EGsig*		egsigalloc(void);
void		egsigfree(EGsig*);
EGpub*		egprivtopub(EGpriv*);

/*
 * dsa
 */
typedef struct DSApub DSApub;
typedef struct DSApriv DSApriv;
typedef struct DSAsig DSAsig;

/* public/encryption key */
struct DSApub
{
	mpint	*p;	/* modulus */
	mpint	*q;	/* group order, q divides p-1 */
	mpint	*alpha;	/* group generator */
	mpint	*key;	/* (encryption key) alpha**secret mod p */
};

/* private/decryption key */
struct DSApriv
{
	DSApub	pub;
	mpint	*secret;	/* (decryption key) */
};

/* signature */
struct DSAsig
{
	mpint	*r, *s;
};

DSApriv*	dsagen(DSApub *opub);	/* opub not checked for consistency! */
DSAsig*		dsasign(DSApriv *k, mpint *m);
int		dsaverify(DSApub *k, DSAsig *sig, mpint *m);
DSApub*		dsapuballoc(void);
void		dsapubfree(DSApub*);
DSApriv*	dsaprivalloc(void);
void		dsaprivfree(DSApriv*);
DSAsig*		dsasigalloc(void);
void		dsasigfree(DSAsig*);
DSApub*		dsaprivtopub(DSApriv*);
DSApriv*	asn1toDSApriv(u8*, int);

/*
 * TLS
 */
typedef struct Thumbprint{
	struct Thumbprint *next;
	u8	sha1[SHA1dlen];
} Thumbprint;

typedef struct TLSconn{
	char	dir[40];	/* connection directory */
	u8	*cert;	/* certificate (local on input, remote on output) */
	u8	*sessionID;
	int	certlen;
	int	sessionIDlen;
	int	(*trace)(char*fmt, ...);
	PEMChain*chain;	/* optional extra certificate evidence for servers to present */
	char	*sessionType;
	u8	*sessionKey;
	int	sessionKeylen;
	char	*sessionConst;
} TLSconn;

/* tlshand.c */
int tlsClient(int fd, TLSconn *c);
int tlsServer(int fd, TLSconn *c);

/* thumb.c */
Thumbprint* initThumbprints(char *ok, char *crl);
void	freeThumbprints(Thumbprint *ok);
int	okThumbprint(u8 *sha1, Thumbprint *ok);

/* readcert.c */
u8	*readcert(char *filename, int *pcertlen);
PEMChain*readcertchain(char *filename);

