#pragma	lib	"libcrypt.a"
#pragma	src	"/sys/src/libcrypt"

/*
 *  To use Brickell bigpow, uncomment the following and recompile everything:
 *  #define BRICKELL
 */

/************************************************************************/
/* system interface (can be overriden)	*/

enum
{
	CRITICAL=	1,
	WARNING=	0
};

void	handle_exception(int, char*);
void	*crypt_malloc(int);
void	crypt_free(void*);
char	*crypt_strdup(char*);

/************************************************************************/
/* infinite precision integer arithmetic */

/* The following all define the size of a basic unit math is performed on */
typedef ulong NumType;
#define NumTypeBits	(sizeof(NumType)*8)
#define NumTypeMask	(ulong)0xFFFFFFFF
typedef long		SignedNumType;
typedef int		Boolean;
typedef NumType		*BigData;

enum
{
	TRUE=	1,
	FALSE=	0
};

typedef int Sign;
enum
{
	POS=	1,
	NEG=	-1
};


/* Fundamental bigmath unit */
typedef struct Bignum {
	Sign sign;
	ulong length; /* length of number (in NumType units)	*/
	ulong space; /* storage space (in NumType units)	*/
	NumType *num;
} Bignum;
typedef Bignum * BigInt;

#pragma	varargck	type	"B"	Bignum*

/* mocros to get return properties of a big integer */
#define NUM(x) ((x)->num)
#define LENGTH(x) ((x)->length)
#define SIGN(x) ((x)->sign)
#define SPACE(x) ((x)->space)
#define LENGTH_IN_BYTES(N) (((N + (NumTypeBits-1) / NumTypeBits) * sizeof(NumType))
#define EVEN(a) (((NUM(a)[0] & 1) == 0) ? TRUE : FALSE)
#define ODD(a) (((NUM(a)[0] & 1) != 0) ? TRUE : FALSE)
#define CHARBITS 8
/* Some predefined constants in bignum form. */
#define ZERO(x) ((LENGTH(x) == 1) && (NUM(x)[0] == 0))
#define ONE(x) ((LENGTH(x) == 1) && (NUM(x)[0] == 1))
#define TWO(x) ((LENGTH(x) == 1) && (NUM(x)[0] == 2))

/* GUARANTEE is the principal macro for bigint memory allocation.
 * Space is updated by this macro.  If a bigint shrinks (say as the result
 * of a modular reduction, space need not be freed.  The length will be reduced.
 * If later the reduced number is square (say) then we don't need to realloc the
 * memory.  Look at bigmath.c to get a feel for how this is used.
 */
#define GUARANTEE(B,S) {\
	if((B)->space < (S)) {\
	(B)->space = (S);\
	(B)->num = (BigData)realloc((uchar *)NUM(B), (unsigned)(sizeof(NumType)*SPACE(B)));\
	}\
}

BigInt	atobig(char*);
void	bigAdd(BigInt, BigInt, BigInt);
void	bigAnd(BigInt, BigInt, BigInt);
int	bigBits(BigInt);
int	bigBytes(BigInt);
int	bigCompare(BigInt, BigInt);
int	bigconv(va_list*, Fconv*);	/* %B */
void	bigCopy(BigInt, BigInt);
void	bigCube(BigInt, BigInt, BigInt);
void	bigDivide(BigInt, BigInt, BigInt, BigInt);
#define bigInit(x) (itobig((NumType)(x)))
void	bigLeftShift(BigInt, int, BigInt);
void	bigMod(BigInt, BigInt, BigInt);
void	bigMultiply(BigInt, BigInt, BigInt);
void	bigOr(BigInt, BigInt, BigInt);
void	bigPow(BigInt, BigInt, BigInt, BigInt);
void	bigRightShift(BigInt, int, BigInt);
void	bigsquare(BigInt a, BigInt res);
void	bigsub(BigInt, BigInt, BigInt);
void	bigSubtract(BigInt, BigInt, BigInt);
Sign	bigTest(BigInt);
int	bigToBuf(BigInt big, int bufsize, uchar *buf);
void	bigXor(BigInt, BigInt, BigInt);
void	bufToBig(uchar *buf, int buflen, BigInt big);
void	crtCombine(BigInt, BigInt, BigInt, BigInt, BigInt, BigInt);
Boolean	even(BigInt);
void	freeBignum(BigInt);
BigInt	itobig(NumType);
void	lbigmult(BigInt a, BigInt b, BigInt res);
NumType	msb(NumType);
void	negate(BigInt, BigInt, BigInt);
Boolean	odd(BigInt);
void	reset_big(BigInt a, NumType val);
void	trim(BigInt);

/* always around */
extern BigInt zero, one, two;

/*********************************************************************/
/* primes */

enum
{
	NIST= 0,
	GORDON= 1
};
typedef int PrimeType;

void	setPrimeTestAttempts(int);
Boolean	primeTest(BigInt);
void	getPrime(int, BigInt);
void	genStrongPrimeSet(int, BigInt, int, BigInt, PrimeType);
void	genStrongPrime(int, BigInt);
void	getPrimitiveElement(BigInt, BigInt, BigInt);

/************************************************************************/
/* random numbers */

/* By default, REALLY ==> truerand() and PSEUDO ==> fsrRandom() */
enum
{
	REALLY=	1,
	PSEUDO= 0
};
typedef int RandType;

void	bigRand(int, BigInt, RandType);
void	bigReallyRand(int, BigInt);
void	bigPseudoRand(int, BigInt);
void	seed_fsr(uchar*, int);
ulong	fsrRandom(void);
void	getRandBetween(BigInt, BigInt, BigInt, RandType);
int	randomBytes(uchar*, int, RandType);
void	pseudoRandomBytes(uchar*, int);
void	reallyRandomBytes(uchar*, int);

#ifdef PRAND
#define seed_rng seed_prand
#else
#define seed_rng seed_fsr
#endif

/************************************************************************/
/* rc4 */

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

/************************************************************************/
/* des */

enum
{
	DESbsize=	8,

	DES3E=		0,	/* various encrypt/decrypt combinations for triple des */
	DES3D=		1,
	DES3EEE=	0,
	DES3EDE=	2,
	DES3DED=	5,
	DES3DDD=	7,

	ECB=		10,
	CBC=		20,
	OFM=		30

};
typedef int ModeType;

/* des key conversion */
uchar	*atodes(char*);
int	desconv(va_list*, void*); /* %D */

/* single des */
typedef struct DESstate DESstate;
struct DESstate
{
	ulong	setup;
	uchar	key[8];		/* unexpanded key */
	ulong	expanded[32];	/* expanded key */
	uchar	ivec[8];	/* initialization vector */
};

void	des56to64(uchar *k56, uchar *k64);
void	des64to56(uchar *k64, uchar *k56);
void	setupDESstate(DESstate*, uchar[7], uchar*);
void	key_setup(uchar[7], ulong[32]);
void	des_key_setup(uchar[8], ulong[32]);
void	key_crunch(uchar*, int, uchar*);
void	block_cipher(ulong*, uchar*, int);
void	bCBCEncrypt(uchar*, uchar*, ulong*, int);
void	blockECBEncrypt(uchar*, uchar*, DESstate*);
void	bCBCDecrypt(uchar*, uchar*, ulong*, int);
void	blockECBDecrypt(uchar*, uchar*, DESstate*);
void	blockCBCEncrypt(uchar*, uchar*, uchar[7], int, DESstate*);
void	blockCBCDecrypt(uchar*, uchar*, uchar[7], int, DESstate*);
void	blockOFMEncrypt(uchar*, uchar*, uchar*, int, DESstate*);
void	blockOFMDecrypt(uchar*, uchar*, uchar*, int, DESstate*);
void	bufferECBEncrypt(uchar*, uchar*, int);
void	bufferECBDecrypt(uchar*, uchar*, int);
void	bufferCBCEncrypt(uchar*, uchar*, uchar*, int);
void	bufferCBCDecrypt(uchar*, uchar*, uchar*, int);
void	bufferOFMEncrypt(uchar*, uchar*, uchar*, int);
void	bufferOFMDecrypt(uchar*, uchar*, uchar*, int);

/* triple des */
typedef struct DES3state DES3state;
struct DES3state
{
	ulong	setup;
	uchar	key[3][8];		/* unexpanded key */
	ulong	expanded[3][32];	/* expanded key */
	uchar	ivec[8];		/* initialization vector */
};

void	setupDES3state(DES3state*, uchar key[3][8], uchar*);
void	triple_block_cipher(ulong keys[3][32], uchar*, int);
void	b3CBCEncrypt(uchar*, uchar*, ulong int_key[3][32], int);
void	block3ECBEncrypt(uchar*, uchar key[3][8], DES3state*);
void	b3CBCDecrypt(uchar*, uchar*, ulong int_key[3][32], int);
void	block3ECBDecrypt(uchar*, uchar key[3][8], DES3state*);
void	block3CBCEncrypt(uchar*, uchar*, uchar key[3][8], int, DES3state*);
void	block3CBCDecrypt(uchar*, uchar*, uchar key[3][8], int, DES3state*);
void	block3OFMEncrypt(uchar*, uchar*, uchar key[3][8], int, DES3state*);
void	block3OFMDecrypt(uchar*, uchar*, uchar key[3][8], int, DES3state*);
void	buffer3ECBEncrypt(uchar *buf, uchar key[3][8], int);
void	buffer3ECBDecrypt(uchar *buf, uchar key[3][8], int);
void	buffer3CBCEncrypt(uchar *buf, uchar*, uchar key[3][8], int);
void	buffer3CBCDecrypt(uchar *buf, uchar*, uchar key[3][8], int);
void	buffer3OFMEncrypt(uchar *buf, uchar*, uchar key[3][8], int);
void	buffer3OFMDecrypt(uchar *buf, uchar*, uchar key[3][8], int);
void	mbitCFMEncrypt(uchar*, uchar*, uchar*, int);
void	mbitCFMDecrypt(uchar*, uchar*, uchar*, int m);

/************************************************************************/
/* digests */

enum
{
	SHAdlen=	20,	/* SHA digest length */
	MD4dlen=	16,	/* MD4 digest length */
	MD5dlen=	16	/* MD5 digest length */
};

typedef struct DigestState DigestState;
struct DigestState
{
	ulong len;
	ulong state[5];
	uchar buf[128];
	int blen;
	char malloced;
	char seeded;
};
typedef struct DigestState SHAstate;
typedef struct DigestState MD5state;
typedef struct DigestState MD4state;

DigestState* md4(uchar*, ulong, uchar*, DigestState*);
DigestState* md5(uchar*, ulong, uchar*, DigestState*);
DigestState* sha(uchar*, ulong, uchar*, DigestState*);

/************************************************************************/

/*	#include "bigpow.h"	*/

typedef struct {
	unsigned length;
	BigInt t[2];
} Table;

typedef struct {
	BigInt N;
	NumType n0p;
} Mont_set;

/* Struct for Montgomery reduction REDC */

/* Generally no reason to play with this stuff, used exclusively within bigPow */
NumType		modInverse(NumType);
Mont_set	*mont_set(BigInt);
BigInt		res_form(BigInt big, Mont_set *ms);

/* When doing a^b mod c for a constant a and c, a table can be created using the following
 * function.  This is useful in El Gamal variants where the base (a) and the modulus are
 * constant in groupd of keys.  This is true of DSA (an El Gamal variant) as well.
 */
Table		*g16_bigpow(BigInt a, BigInt modulus, NumType explength);

/* bigpow using table: Ernie Brickell's method. */
void		brickell_bigpow(Table *table,
	   			BigInt exponent,
	   			BigInt modulus,
	   			BigInt result);

void		freeTable(Table *);
void		freeMs(Mont_set *);

/************************************************************************/


/*	#include "longmult.h"	*/
/* Not part of the cryptoLib interface - low level multiplication routines. */
NumType	smult(NumType *, NumType, NumType *, int);
NumType	LMULT(NumType *, NumType, NumType *, int);
void	BUILDDIAG(NumType *, NumType *, int);
void	SQUAREINNERLOOP(NumType *, NumType, NumType *, int, int);

/************************************************************************/


/*	#include "coremult.h"	*/
/* Not part of the cryptoLib interface - low level multiplication routines. */
void	numtype_bigmult2(NumType *, NumType *, NumType *);
void	numtype_bigmult8(NumType *, NumType *, NumType *);
void	numtype_bigmultN(NumType *, NumType *, NumType *, int);
void	numtype_bigsquareN(NumType *, NumType *, int);
void	bigmult4(BigData, BigData, BigData);
void	bigmult8(BigData, BigData, BigData);
void	bigmultN(BigData, BigData, BigData, int);
void	bigsquare8(BigData, BigData);
void	bigsquareN(BigData, BigData, int);
void	numtype_bigmult(BigInt, NumType, BigInt, int);
void	REDC(BigInt, Mont_set *);

/************************************************************************/


/*	#include "euclid.h"	*/
/* Euclid's extended gcd algorithm for solving
 * ax + by = gcd(a, b)
 */
void extendedGcd(BigInt a,
	       BigInt b,
	       BigInt x,
	       BigInt y,
	       BigInt gcd);

/* If gcd(a, b) = 1, x = inverse of a (mod b) and y = inverse of b (mod a) */
void getInverse(BigInt a,
	      BigInt b,
	      BigInt inverse_of_a_mod_b );

BigInt gcd(BigInt a, BigInt b);

/************************************************************************/

/*	#include "jacobi.h"	*/
/* calculate the jacobi symbol (a/b) */
int bigJacobi(BigInt a,
	    BigInt b);

int jacobi(int a,
	 int b);

/* Return true if a is a quadratic residue mod p*q where p and q are prime. */
int bigIsQR(BigInt a,
	  BigInt p,
	  BigInt q);
int isQR(int a,
	       int p,
	       int q);

/************************************************************************/
/* quadratic residues */

/* Return true if a is a quadratic residue mod p */
Boolean quadResidue(BigInt a,
	  BigInt p);

/* Return true if a is a quadratic residue mod p*q where p and q are prime. */
Boolean compositeQuadResidue(BigInt a,
	   BigInt p,
	   BigInt q);

/* return the square root of a (mod p) where p is prime. */
void squareRoot(BigInt a,
	      BigInt p,
	      BigInt result);

/* return the 2 linearly independent square roots of a (mod p*q) for p and q prime. */
/* The other 2 roots are n-root1 and n-root2 */
void compositeSquareRoot(BigInt a,
	       BigInt p,
	       BigInt q,
	       BigInt root1,
	       BigInt root2);

/************************************************************************/
/* El Gamal Crypto system stuff */

/* p is prime, q is a large prime factor of p-1.  alpha is the base
 * g_table is included as part of the key to enable Brickell exponentiation.
 * If one set {p, q, alpha} is used by "everyone" then this table can be constant.
 * That is, {p, q, alpha, g_table} is public.  The private key is then
 * the exponent (or secret) and the public key is alpha^exponent mod p.
 */
typedef struct EGParams {
	BigInt p, q, alpha;
} EGParams;

typedef struct EGPrivateKey {
	BigInt p, q;
	BigInt alpha;
	BigInt publicKey;
	BigInt secret;
	Table *g_table;
} EGPrivateKey;

typedef struct EGPublicKey {
	BigInt p, q;
	BigInt alpha;
	BigInt publicKey;
	Table *g_table, *y_table;
} EGPublicKey;

typedef struct EGKeySet {
	EGPublicKey *publicKey;
	EGPrivateKey *privateKey;
} EGKeySet;

typedef struct EGSignature {
	BigInt r, s;
} EGSignature;

/* generate global p, q, and alpha */
EGParams * genEGParams(int, int);

/* generate alpha and p numbits long */
void genDiffieHellmanSet(BigInt p,
	       BigInt alpha,
	       int pbits,
	       int qbits);

/* generate public and private keys corresponding to params */
EGKeySet * genEGKeySet(EGParams *params);

/* generate private key */
EGPrivateKey * genEGPrivateKey(EGParams *params);

/* El Gamal signature */
EGSignature * EGSign(BigInt big,
	   EGPrivateKey *key);

/* Verify signature of big */
Boolean EGVerify(BigInt big,
	       EGSignature *sig,
	       EGPublicKey *key);

/* El Gamal encrypt and decrypt functions -- these are not reversible as in RSA 
 * That is, Decrypt(Encrypt(m, pubkey), privkey) is NOT the same as
 * Encrypt(Decrypt(m, privkey), pubkey).  This symmetry doesn't exist with
 * El Gamal.
 */
BigInt EGEncrypt(BigInt,
	       EGPublicKey *);

BigInt EGDecrypt(BigInt,
	       EGPrivateKey *);

/* El Gamal Cleanup functions -- be sure to use these! */
void freeEGPublicKey(EGPublicKey *);
void freeEGPrivateKey(EGPrivateKey *);
void freeEGKeys(EGKeySet *);
void freeEGSig(EGSignature *);
void freeEGParams(EGParams *);

/************************************************************************/
/* DSS == modified Elgamal */

/* The digital signature standard doesn't introduce new structures...we reuse the
 * El Gamal structs and parameter generation stuff.
 */
typedef EGSignature DSSSignature;
typedef EGPublicKey DSSPublicKey;
typedef EGPrivateKey DSSPrivateKey;
typedef EGParams DSSParams;
typedef EGKeySet DSSKeySet;

DSSSignature *DSSSign(BigInt big, EGPrivateKey *key);

Boolean DSSVerify(BigInt big, DSSSignature *sig, EGPublicKey *key);

#define genDSSParams genEGParams
#define genDSSKeySet genEGKeySet
void freeDSSSig(DSSSignature *);
#define freeDSSPublicKey freeEGPublicKey
#define freeDSSPrivateKey freeEGPrivateKey
#define freeDSSKeys freeEGKeys
#define freeDSSParams freeEGParams

/************************************************************************/
/* RSA Cryptosystem stuff */

/* The private key contains material to enable the Chinese Remainder speedup
 * of decryption (or signing).  For RSA,
 * Decrypt(Encrypt(m, pubkey), privkey) = Encrypt(Decrypt(m, privkey), pubkey) = m
 */
typedef struct Key_exps {
	BigInt e;
	BigInt d;
} Key_exps;

typedef struct ChineseRemStruct {
	BigInt p, q;    /* SECRET primes */
	BigInt dp, dq;  /* d mod p, d mod q */
	BigInt c12;     /* inverse of p (mod q) */
} ChineseRemStruct;

typedef struct RSAPublicKey {
	BigInt publicExponent;
	BigInt modulus;
} RSAPublicKey;

typedef struct RSAPrivateKey {
	BigInt publicExponent;
	BigInt modulus;
	BigInt privateExponent;  /* SECRET */
	ChineseRemStruct *crt;   /* SECRET */
} RSAPrivateKey;

typedef struct RSAKeySet {
	RSAPublicKey *publicKey;
	RSAPrivateKey *privateKey;
} RSAKeySet;

typedef Bignum RSASignature;

/* generate an RSA key set with modulus length = modbits and
 * public exponent length = explen.  If explen = 2, the public exponent = 3.
 */
RSAKeySet * genRSAKeySet(int, int);
void freeRSAPublicKey(RSAPublicKey *);
void freeRSAPrivateKey(RSAPrivateKey *);
void freeRSAKeys(RSAKeySet *);
void freeRSASig(RSASignature *);

/* RSA Encryption(m) = m^publicExponent mod modulus */
BigInt RSAEncrypt(BigInt msg,
	RSAPublicKey *key);

/* RSA Decryption = m^privateExponent mod modulus */
BigInt RSADecrypt(BigInt msg,
	RSAPrivateKey *key);

/* Signing is just RSADecrypt */
RSASignature * RSASign(BigInt msg, RSAPrivateKey *key);

/* Verifying is just RSAEncrypt */
Boolean RSAVerify(BigInt msg,
	RSASignature *sig,
	RSAPublicKey *);

/* These are useful functions when creating Dynamic Link Libraries for use with visual basic */
int pubkeyToBuf(RSAPublicKey *key, uchar *buf, int bufsize);
int privkeyToBuf(RSAPrivateKey *key, uchar *buf, int bufsize);
int newRSAEncrypt(uchar *srcbuf,
		ushort srclen,
		uchar *dstbuf,
		ushort dstsize,
		uchar *key,
		ushort explen,
		ushort modlen);

int newRSADecrypt(uchar *srcbuf,
	  int srclen,
	  uchar *dstbuf,
	  int dstsize,
	  uchar *key,
	  int pubexplen,
	  int privexplen,
	  int modlen);
int vbRSAEncrypt(uchar *, int, uchar *, int,
	  char *);
int vbRSADecrypt(uchar *, int, uchar *, int,
	  char *);

int vbgenRSAKeySet(int modlen, int pubexplen,
	   char *pubfile, char *privfile);

void NN_ModExp(ulong *a,
	     ulong *b,
	     ulong *c,
	     ulong cDigits,
	     ulong *d,
	     ulong dDigits);


/************************************************************************/
/* base 64 conversions */

int dec64(uchar *out, int lim, char *in, int n);
int enc64(char *out, int lim, uchar *in, int n);
