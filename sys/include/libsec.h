/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma lib "libsec.a"
#pragma src "/sys/src/libsec"

#ifndef _MPINT
typedef struct mpint mpint;
#endif

/*
 * AES definitions
 */

enum {
	AESbsize = 16,
	AESmaxkey = 32,
	AESmaxrounds = 14
};

typedef struct AESstate AESstate;
struct AESstate {
	uint32_t setup;
	int rounds;
	int keybytes;
	uint ctrsz;
	uint8_t key[AESmaxkey];		       /* unexpanded key */
	uint32_t ekey[4 * (AESmaxrounds + 1)]; /* encryption key */
	uint32_t dkey[4 * (AESmaxrounds + 1)]; /* decryption key */
	uint8_t ivec[AESbsize];		       /* initialization vector */
	uint8_t mackey[3 * AESbsize];	  /* 3 XCBC mac 96 keys */
};

/* block ciphers */
void aes_encrypt(uint32_t rk[], int Nr, uint8_t pt[16],
		 uint8_t ct[16]);
void aes_decrypt(uint32_t rk[], int Nr, uint8_t ct[16],
		 uint8_t pt[16]);

void setupAESstate(AESstate *s, uint8_t key[], int keybytes,
		   uint8_t *ivec);
void aesCBCencrypt(uint8_t *p, int len, AESstate *s);
void aesCBCdecrypt(uint8_t *p, int len, AESstate *s);
void aesCTRdecrypt(uint8_t *p, int len, AESstate *s);
void aesCTRencrypt(uint8_t *p, int len, AESstate *s);

void setupAESXCBCstate(AESstate *s);
uint8_t *aesXCBCmac(uint8_t *p, int len, AESstate *s);

/*
 * Blowfish Definitions
 */

enum {
	BFbsize = 8,
	BFrounds = 16
};

/* 16-round Blowfish */
typedef struct BFstate BFstate;
struct BFstate {
	uint32_t setup;

	uint8_t key[56];
	uint8_t ivec[8];

	uint32_t pbox[BFrounds + 2];
	uint32_t sbox[1024];
};

void setupBFstate(BFstate *s, uint8_t key[], int keybytes,
		  uint8_t *ivec);
void bfCBCencrypt(uint8_t *, int, BFstate *);
void bfCBCdecrypt(uint8_t *, int, BFstate *);
void bfECBencrypt(uint8_t *, int, BFstate *);
void bfECBdecrypt(uint8_t *, int, BFstate *);

/*
 * DES definitions
 */

enum {
	DESbsize = 8
};

/* single des */
typedef struct DESstate DESstate;
struct DESstate {
	uint32_t setup;
	uint8_t key[8];	/* unexpanded key */
	uint32_t expanded[32]; /* expanded key */
	uint8_t ivec[8];       /* initialization vector */
};

void setupDESstate(DESstate *s, uint8_t key[8], uint8_t *ivec);
void des_key_setup(uint8_t[8], uint32_t[32]);
void block_cipher(uint32_t *, uint8_t *, int);
void desCBCencrypt(uint8_t *, int, DESstate *);
void desCBCdecrypt(uint8_t *, int, DESstate *);
void desECBencrypt(uint8_t *, int, DESstate *);
void desECBdecrypt(uint8_t *, int, DESstate *);

/* for backward compatibility with 7-byte DES key format */
void des56to64(uint8_t *k56, uint8_t *k64);
void des64to56(uint8_t *k64, uint8_t *k56);
void key_setup(uint8_t[7], uint32_t[32]);

/* triple des encrypt/decrypt orderings */
enum {
	DES3E = 0,
	DES3D = 1,
	DES3EEE = 0,
	DES3EDE = 2,
	DES3DED = 5,
	DES3DDD = 7
};

typedef struct DES3state DES3state;
struct DES3state {
	uint32_t setup;
	uint8_t key[3][8];	/* unexpanded key */
	uint32_t expanded[3][32]; /* expanded key */
	uint8_t ivec[8];	  /* initialization vector */
};

void setupDES3state(DES3state *s, uint8_t key[3][8], uint8_t *ivec);
void triple_block_cipher(uint32_t keys[3][32], uint8_t *, int);
void des3CBCencrypt(uint8_t *, int, DES3state *);
void des3CBCdecrypt(uint8_t *, int, DES3state *);
void des3ECBencrypt(uint8_t *, int, DES3state *);
void des3ECBdecrypt(uint8_t *, int, DES3state *);

/*
 * digests
 */

enum {
	SHA1dlen = 20,     /* SHA digest length */
	SHA2_224dlen = 28, /* SHA-224 digest length */
	SHA2_256dlen = 32, /* SHA-256 digest length */
	SHA2_384dlen = 48, /* SHA-384 digest length */
	SHA2_512dlen = 64, /* SHA-512 digest length */
	MD4dlen = 16,      /* MD4 digest length */
	MD5dlen = 16,      /* MD5 digest length */
	AESdlen = 16,      /* TODO: see rfc */

	Hmacblksz = 64, /* in bytes; from rfc2104 */
};

typedef struct DigestState DigestState;
struct DigestState {
	uint64_t len;
	union {
		uint32_t state[8];
		uint64_t bstate[8];
	};
	uint8_t buf[256];
	int blen;
	char malloced;
	char seeded;
};
typedef struct DigestState SHAstate; /* obsolete name */
typedef struct DigestState SHA1state;
typedef struct DigestState SHA2_224state;
typedef struct DigestState SHA2_256state;
typedef struct DigestState SHA2_384state;
typedef struct DigestState SHA2_512state;
typedef struct DigestState MD5state;
typedef struct DigestState MD4state;
typedef struct DigestState AEShstate;

DigestState *md4(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *md5(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *sha1(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *sha2_224(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *sha2_256(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *sha2_384(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *sha2_512(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *aes(uint8_t *, uint32_t, uint8_t *, DigestState *);
DigestState *hmac_x(uint8_t *p, uint32_t len, uint8_t *key,
		    uint32_t klen,
		    uint8_t *digest, DigestState *s,
		    DigestState *(*x)(uint8_t *, uint32_t, uint8_t *, DigestState *),
		    int xlen);
DigestState *hmac_md5(uint8_t *, uint32_t, uint8_t *, uint32_t,
		      uint8_t *,
		      DigestState *);
DigestState *hmac_sha1(uint8_t *, uint32_t, uint8_t *, uint32_t,
		       uint8_t *,
		       DigestState *);
DigestState *hmac_sha2_224(uint8_t *, uint32_t, uint8_t *, uint32_t,
			   uint8_t *, DigestState *);
DigestState *hmac_sha2_256(uint8_t *, uint32_t, uint8_t *, uint32_t,
			   uint8_t *, DigestState *);
DigestState *hmac_sha2_384(uint8_t *, uint32_t, uint8_t *, uint32_t,
			   uint8_t *, DigestState *);
DigestState *hmac_sha2_512(uint8_t *, uint32_t, uint8_t *, uint32_t,
			   uint8_t *, DigestState *);
DigestState *hmac_aes(uint8_t *, uint32_t, uint8_t *, uint32_t,
		      uint8_t *,
		      DigestState *);
char *md5pickle(MD5state *);
MD5state *md5unpickle(char *);
char *sha1pickle(SHA1state *);
SHA1state *sha1unpickle(char *);

/*
 * random number generation
 */
void genrandom(uint8_t *buf, int nbytes);
void prng(uint8_t *buf, int nbytes);
uint32_t fastrand(void);
uint32_t nfastrand(uint32_t);

/*
 * primes
 */
void genprime(mpint *p, int n, int accuracy);			/* generate n-bit probable prime */
void gensafeprime(mpint *p, mpint *alpha, int n, int accuracy); /* prime & generator */
void genstrongprime(mpint *p, int n, int accuracy);		/* generate n-bit strong prime */
void DSAprimes(mpint *q, mpint *p, uint8_t seed[SHA1dlen]);
int probably_prime(mpint *n, int nrep); /* miller-rabin test */
int smallprimetest(mpint *p);		/* returns -1 if not prime, 0 otherwise */

/*
 * rc4
 */
typedef struct RC4state RC4state;
struct RC4state {
	uint8_t state[256];
	uint8_t x;
	uint8_t y;
};

void setupRC4state(RC4state *, uint8_t *, int);
void rc4(RC4state *, uint8_t *, int);
void rc4skip(RC4state *, int);
void rc4back(RC4state *, int);

/*
 * rsa
 */
typedef struct RSApub RSApub;
typedef struct RSApriv RSApriv;
typedef struct PEMChain PEMChain;

/* public/encryption key */
struct RSApub {
	mpint *n;  /* modulus */
	mpint *ek; /* exp (encryption key) */
};

/* private/decryption key */
struct RSApriv {
	RSApub pub;

	mpint *dk; /* exp (decryption key) */

	/* precomputed values to help with chinese remainder theorem calc */
	mpint *p;
	mpint *q;
	mpint *kp; /* dk mod p-1 */
	mpint *kq; /* dk mod q-1 */
	mpint *c2; /* (inv p) mod q */
};

struct PEMChain {
	PEMChain *next;
	uint8_t *pem;
	int pemlen;
};

RSApriv *rsagen(int nlen, int elen, int rounds);
RSApriv *rsafill(mpint *n, mpint *e, mpint *d, mpint *p, mpint *q);
mpint *rsaencrypt(RSApub *k, mpint *in, mpint *out);
mpint *rsadecrypt(RSApriv *k, mpint *in, mpint *out);
RSApub *rsapuballoc(void);
void rsapubfree(RSApub *);
RSApriv *rsaprivalloc(void);
void rsaprivfree(RSApriv *);
RSApub *rsaprivtopub(RSApriv *);
RSApub *X509toRSApub(uint8_t *, int, char *, int);
uint8_t *RSApubtoasn1(RSApub *, int *);
RSApub *asn1toRSApub(uint8_t *, int);
RSApriv *asn1toRSApriv(uint8_t *, int);
void asn1dump(uint8_t *der, int len);
uint8_t *decodePEM(char *s, char *type, int *len,
		   char **new_s);
PEMChain *decodepemchain(char *s, char *type);
uint8_t *X509gen(RSApriv *priv, char *subj,
		 uint32_t valid[2],
		 int *certlen);
uint8_t *X509req(RSApriv *priv, char *subj, int *certlen);
char *X509verify(uint8_t *cert, int ncert, RSApub *pk);
void X509dump(uint8_t *cert, int ncert);

/*
 * elgamal
 */
typedef struct EGpub EGpub;
typedef struct EGpriv EGpriv;
typedef struct EGsig EGsig;

/* public/encryption key */
struct EGpub {
	mpint *p;     /* modulus */
	mpint *alpha; /* generator */
	mpint *key;   /* (encryption key) alpha**secret mod p */
};

/* private/decryption key */
struct EGpriv {
	EGpub pub;
	mpint *secret; /* (decryption key) */
};

/* signature */
struct EGsig {
	mpint *r, *s;
};

EGpriv *eggen(int nlen, int rounds);
mpint *egencrypt(EGpub *k, mpint *in, mpint *out); /* deprecated */
mpint *egdecrypt(EGpriv *k, mpint *in, mpint *out);
EGsig *egsign(EGpriv *k, mpint *m);
int egverify(EGpub *k, EGsig *sig, mpint *m);
EGpub *egpuballoc(void);
void egpubfree(EGpub *);
EGpriv *egprivalloc(void);
void egprivfree(EGpriv *);
EGsig *egsigalloc(void);
void egsigfree(EGsig *);
EGpub *egprivtopub(EGpriv *);

/*
 * dsa
 */
typedef struct DSApub DSApub;
typedef struct DSApriv DSApriv;
typedef struct DSAsig DSAsig;

/* public/encryption key */
struct DSApub {
	mpint *p;     /* modulus */
	mpint *q;     /* group order, q divides p-1 */
	mpint *alpha; /* group generator */
	mpint *key;   /* (encryption key) alpha**secret mod p */
};

/* private/decryption key */
struct DSApriv {
	DSApub pub;
	mpint *secret; /* (decryption key) */
};

/* signature */
struct DSAsig {
	mpint *r, *s;
};

DSApriv *dsagen(DSApub *opub); /* opub not checked for consistency! */
DSAsig *dsasign(DSApriv *k, mpint *m);
int dsaverify(DSApub *k, DSAsig *sig, mpint *m);
DSApub *dsapuballoc(void);
void dsapubfree(DSApub *);
DSApriv *dsaprivalloc(void);
void dsaprivfree(DSApriv *);
DSAsig *dsasigalloc(void);
void dsasigfree(DSAsig *);
DSApub *dsaprivtopub(DSApriv *);
DSApriv *asn1toDSApriv(uint8_t *, int);

/*
 * TLS
 */
typedef struct Thumbprint {
	struct Thumbprint *next;
	uint8_t sha1[SHA1dlen];
} Thumbprint;

typedef struct TLSconn {
	char dir[40];  /* connection directory */
	uint8_t *cert; /* certificate (local on input, remote on output) */
	uint8_t *sessionID;
	int certlen;
	int sessionIDlen;
	int (*trace)(char *fmt, ...);
	PEMChain *chain; /* optional extra certificate evidence for servers to present */
	char *sessionType;
	uint8_t *sessionKey;
	int sessionKeylen;
	char *sessionConst;
} TLSconn;

/* tlshand.c */
int tlsClient(int fd, TLSconn *c);
int tlsServer(int fd, TLSconn *c);

/* thumb.c */
Thumbprint *initThumbprints(char *ok, char *crl);
void freeThumbprints(Thumbprint *ok);
int okThumbprint(uint8_t *sha1, Thumbprint *ok);

/* readcert.c */
uint8_t *readcert(char *filename, int *pcertlen);
PEMChain *readcertchain(char *filename);
