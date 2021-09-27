#pragma	src	"/usr/inferno/libsec"

#ifndef _MPINT
typedef struct mpint mpint;
#endif

/*/////////////////////////////////////////////////////// */
/* AES definitions */
/*/////////////////////////////////////////////////////// */

enum
{
	AESbsize=	16,
	AESmaxkey=	32,
	AESmaxrounds=	14
};

typedef struct AESstate AESstate;
struct AESstate
{
	ulong	setup;
	int	rounds;
	int	keybytes;
	uchar	key[AESmaxkey];		/* unexpanded key */
	u32int	ekey[4*(AESmaxrounds + 1)];	/* encryption key */
	u32int	dkey[4*(AESmaxrounds + 1)];	/* decryption key */
	uchar	ivec[AESbsize];	/* initialization vector */
};

void	setupAESstate(AESstate *s, uchar key[], int keybytes, uchar *ivec);
void	aesCBCencrypt(uchar *p, int len, AESstate *s);
void	aesCBCdecrypt(uchar *p, int len, AESstate *s);

/*/////////////////////////////////////////////////////// */
/* Blowfish Definitions */
/*/////////////////////////////////////////////////////// */

enum
{
	BFbsize	= 8,
	BFrounds	= 16
};

/* 16-round Blowfish */
typedef struct BFstate BFstate;
struct BFstate
{
	ulong	setup;

	uchar	key[56];
	uchar	ivec[8];

	u32int 	pbox[BFrounds+2];
	u32int	sbox[1024];
};

void	setupBFstate(BFstate *s, uchar key[], int keybytes, uchar *ivec);
void	bfCBCencrypt(uchar*, int, BFstate*);
void	bfCBCdecrypt(uchar*, int, BFstate*);
void	bfECBencrypt(uchar*, int, BFstate*);
void	bfECBdecrypt(uchar*, int, BFstate*);

/*/////////////////////////////////////////////////////// */
/* DES definitions */
/*/////////////////////////////////////////////////////// */

enum
{
	DESbsize=	8
};

/* single des */
typedef struct DESstate DESstate;
struct DESstate
{
	ulong	setup;
	uchar	key[8];		/* unexpanded key */
	ulong	expanded[32];	/* expanded key */
	uchar	ivec[8];	/* initialization vector */
};

void	setupDESstate(DESstate *s, uchar key[8], uchar *ivec);
void	des_key_setup(uchar[8], ulong[32]);
void	block_cipher(ulong*, uchar*, int);
void	desCBCencrypt(uchar*, int, DESstate*);
void	desCBCdecrypt(uchar*, int, DESstate*);
void	desECBencrypt(uchar*, int, DESstate*);
void	desECBdecrypt(uchar*, int, DESstate*);

/* for backward compatibility with 7 byte DES key format */
void	des56to64(uchar *k56, uchar *k64);
void	des64to56(uchar *k64, uchar *k56);
void	key_setup(uchar[7], ulong[32]);

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
	ulong	setup;
	uchar	key[3][8];		/* unexpanded key */
	ulong	expanded[3][32];	/* expanded key */
	uchar	ivec[8];		/* initialization vector */
};

void	setupDES3state(DES3state *s, uchar key[3][8], uchar *ivec);
void	triple_block_cipher(ulong keys[3][32], uchar*, int);
void	des3CBCencrypt(uchar*, int, DES3state*);
void	des3CBCdecrypt(uchar*, int, DES3state*);
void	des3ECBencrypt(uchar*, int, DES3state*);
void	des3ECBdecrypt(uchar*, int, DES3state*);

/* IDEA */
typedef struct IDEAstate IDEAstate;
struct IDEAstate
{
	uchar	key[16];
	ushort	edkey[104];
	uchar	ivec[8];
};

void	setupIDEAstate(IDEAstate*, uchar*, uchar*);
void	idea_key_setup(uchar*, ushort*);
void	idea_cipher(ushort*, uchar*, int);


/*/////////////////////////////////////////////////////// */
/* digests */
/*/////////////////////////////////////////////////////// */

enum
{
	/* digest lengths */
	SHA1dlen=	20,
	MD4dlen=	16,
	MD5dlen=	16,

	SHA224dlen=	28,
	SHA256dlen=	32,

	SHA384dlen=	48,
	SHA512dlen=	64,

	/* block sizes */
	SHA256bsize=	64,
	SHA512bsize=	128,
	Digestbsize=	128,		/* maximum */
};

typedef struct DigestState DigestState;
struct DigestState
{
	u64int len;
	u32int state[5];
	uchar buf[Digestbsize];
	int blen;
	u64int nb128[2];
	u64int h64[8];
	u32int h32[8];
	char malloced;
	char seeded;
};
typedef struct DigestState SHAstate;	/* obsolete name */
typedef struct DigestState SHA1state;
typedef struct DigestState MD5state;
typedef struct DigestState MD4state;
typedef struct DigestState SHA256state;
typedef struct DigestState SHA512state;

DigestState* md4(uchar*, ulong, uchar*, DigestState*);
DigestState* md5(uchar*, ulong, uchar*, DigestState*);
DigestState* sha1(uchar*, ulong, uchar*, DigestState*);
DigestState* sha224(uchar*, ulong, uchar*, DigestState*);
DigestState* sha256(uchar*, ulong, uchar*, DigestState*);
DigestState* sha384(uchar*, ulong, uchar*, DigestState*);
DigestState* sha512(uchar*, ulong, uchar*, DigestState*);
DigestState* hmac_md5(uchar*, ulong, uchar*, ulong, uchar*, DigestState*);
DigestState* hmac_sha1(uchar*, ulong, uchar*, ulong, uchar*, DigestState*);
char* md5pickle(MD5state*);
MD5state* md5unpickle(char*);
char* sha1pickle(SHA1state*);
SHA1state* sha1unpickle(char*);

/*/////////////////////////////////////////////////////// */
/* random number generation */
/*/////////////////////////////////////////////////////// */
void	genrandom(uchar *buf, int nbytes);
void	_genrandomqlock(void);
void	_genrandomqunlock(void);
void	prng(uchar *buf, int nbytes);
ulong	fastrand(void);
ulong	nfastrand(ulong);

/*/////////////////////////////////////////////////////// */
/* primes */
/*/////////////////////////////////////////////////////// */
void	genprime(mpint *p, int n, int accuracy); /* generate an n bit probable prime */
void	gensafeprime(mpint *p, mpint *alpha, int n, int accuracy);	/* prime and generator */
void	genstrongprime(mpint *p, int n, int accuracy);	/* generate an n bit strong prime */
void	DSAprimes(mpint *q, mpint *p, uchar seed[SHA1dlen]);
int	probably_prime(mpint *n, int nrep);	/* miller-rabin test */
int	smallprimetest(mpint *p);		/* returns -1 if not prime, 0 otherwise */

/*/////////////////////////////////////////////////////// */
/* rc4 */
/*/////////////////////////////////////////////////////// */
typedef struct RC4state RC4state;
struct RC4state
{
	 uchar state[256];
	 uchar x;
	 uchar y;
};

void	setupRC4state(RC4state*, uchar*, int);
void	rc4(RC4state*, uchar*, int);
void	rc4skip(RC4state*, int);
void	rc4back(RC4state*, int);

/*/////////////////////////////////////////////////////// */
/* rsa */
/*/////////////////////////////////////////////////////// */
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
	PEMChain *next;
	uchar *pem;
	int pemlen;
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
RSApub*		X509toRSApub(uchar*, int, char*, int);
RSApriv*	asn1toRSApriv(uchar*, int);
void		asn1dump(uchar *der, int len);
uchar*		decodePEM(char *s, char *type, int *len, char **new_s);
PEMChain*	decodepemchain(char *s, char *type);
uchar*		X509gen(RSApriv *priv, char *subj, ulong valid[2], int *certlen);
uchar*		X509req(RSApriv *priv, char *subj, int *certlen);
char*		X509verify(uchar *cert, int ncert, RSApub *pk);
void		X509dump(uchar *cert, int ncert);

/*/////////////////////////////////////////////////////// */
/* elgamal */
/*/////////////////////////////////////////////////////// */
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
	mpint	*secret; /* (decryption key) */
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

/*/////////////////////////////////////////////////////// */
/* dsa */
/*/////////////////////////////////////////////////////// */
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
	mpint	*secret; /* (decryption key) */
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

/*/////////////////////////////////////////////////////// */
/* TLS */
/*/////////////////////////////////////////////////////// */
typedef struct Thumbprint{
	struct Thumbprint *next;
	uchar sha1[SHA1dlen];
} Thumbprint;

typedef struct TLSconn{
	char dir[40];  /* connection directory */
	uchar *cert;   /* certificate (local on input, remote on output) */
	uchar *sessionID;
	int certlen, sessionIDlen;
	int (*trace)(char*fmt, ...);
	PEMChain *chain; /* optional extra certificate evidence for servers to present */
} TLSconn;

/* tlshand.c */
int tlsClient(int fd, TLSconn *c);
int tlsServer(int fd, TLSconn *c);

/* thumb.c */
Thumbprint* initThumbprints(char *ok, char *crl);
void	freeThumbprints(Thumbprint *ok);
int		okThumbprint(uchar *sha1, Thumbprint *ok);

/* readcert.c */
uchar	*readcert(char *filename, int *pcertlen);
PEMChain *readcertchain(char *filename);
