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
#include <mp.h>
#include <libsec.h>

/*=============================================================*/
/*  general ASN1 declarations and parsing
 *
 *  For now, this is used only for extracting the key from an
 *  X509 certificate, so the entire collection is hidden.  But
 *  someday we should probably make the functions visible and
 *  give them their own man page.
 */
typedef struct Elem Elem;
typedef struct Tag Tag;
typedef struct Value Value;
typedef struct Bytes Bytes;
typedef struct Ints Ints;
typedef struct Bits Bits;
typedef struct Elist Elist;

/* tag classes */
#define Universal 0
#define Context 0x80

/* universal tags */
#define BOOLEAN 1
#define INTEGER 2
#define BIT_STRING 3
#define OCTET_STRING 4
#define NULLTAG 5
#define OBJECT_ID 6
#define ObjectDescriptor 7
#define EXTERNAL 8
#define REAL 9
#define ENUMERATED 10
#define EMBEDDED_PDV 11
#define UTF8String 12
#define SEQUENCE 16		/* also SEQUENCE OF */
#define SETOF 17				/* also SETOF OF */
#define NumericString 18
#define PrintableString 19
#define TeletexString 20
#define VideotexString 21
#define IA5String 22
#define UTCTime 23
#define GeneralizedTime 24
#define GraphicString 25
#define VisibleString 26
#define GeneralString 27
#define UniversalString 28
#define BMPString 30

struct Bytes {
	int	len;
	uint8_t	data[1];
};

struct Ints {
	int	len;
	int	data[];
};

struct Bits {
	int	len;		/* number of bytes */
	int	unusedbits;	/* unused bits in last byte */
	uint8_t	data[];		/* most-significant bit first */
};

struct Tag {
	int	class;
	int	num;
};

enum { VBool, VInt, VOctets, VBigInt, VReal, VOther,
	VBitString, VNull, VEOC, VObjId, VString, VSeq, VSet };
struct Value {
	int	tag;		/* VBool, etc. */
	union {
		int	boolval;
		int	intval;
		Bytes*	octetsval;
		Bytes*	bigintval;
		Bytes*	realval;	/* undecoded; hardly ever used */
		Bytes*	otherval;
		Bits*	bitstringval;
		Ints*	objidval;
		char*	stringval;
		Elist*	seqval;
		Elist*	setval;
	} u;  /* (Don't use anonymous unions, for ease of porting) */
};

struct Elem {
	Tag	tag;
	Value	val;
};

struct Elist {
	Elist*	tl;
	Elem	hd;
};

/* decoding errors */
enum { ASN_OK, ASN_ESHORT, ASN_ETOOBIG, ASN_EVALLEN,
		ASN_ECONSTR, ASN_EPRIM, ASN_EINVAL, ASN_EUNIMPL };


/* here are the functions to consider making extern someday */
static Bytes*	newbytes(int len);
static Bytes*	makebytes(uint8_t* buf, int len);
static void	freebytes(Bytes* b);
static Bytes*	catbytes(Bytes* b1, Bytes* b2);
static Ints*	newints(int len);
static Ints*	makeints(int* buf, int len);
static void	freeints(Ints* b);
static Bits*	newbits(int len);
static Bits*	makebits(uint8_t* buf, int len, int unusedbits);
static void	freebits(Bits* b);
static Elist*	mkel(Elem e, Elist* tail);
static void	freeelist(Elist* el);
static int	elistlen(Elist* el);
static int	is_seq(Elem* pe, Elist** pseq);
static int	is_set(Elem* pe, Elist** pset);
static int	is_int(Elem* pe, int* pint);
static int	is_bigint(Elem* pe, Bytes** pbigint);
static int	is_bitstring(Elem* pe, Bits** pbits);
static int	is_octetstring(Elem* pe, Bytes** poctets);
static int	is_oid(Elem* pe, Ints** poid);
static int	is_string(Elem* pe, char** pstring);
static int	is_time(Elem* pe, char** ptime);
static int	decode(uint8_t* a, int alen, Elem* pelem);
static int	encode(Elem e, Bytes** pbytes);
static int	oid_lookup(Ints* o, Ints** tab);
static void	freevalfields(Value* v);
static mpint	*asn1mpint(Elem *e);
static void	edump(Elem);

#define TAG_MASK 0x1F
#define CONSTR_MASK 0x20
#define CLASS_MASK 0xC0
#define MAXOBJIDLEN 20

static int ber_decode(uint8_t** pp, uint8_t* pend, Elem* pelem);
static int tag_decode(uint8_t** pp, uint8_t* pend, Tag* ptag, int* pisconstr);
static int length_decode(uint8_t** pp, uint8_t* pend, int* plength);
static int value_decode(uint8_t** pp, uint8_t* pend, int length, int kind, int isconstr, Value* pval);
static int int_decode(uint8_t** pp, uint8_t* pend, int count, int unsgned, int* pint);
static int uint7_decode(uint8_t** pp, uint8_t* pend, int* pint);
static int octet_decode(uint8_t** pp, uint8_t* pend, int length, int isconstr, Bytes** pbytes);
static int seq_decode(uint8_t** pp, uint8_t* pend, int length, int isconstr, Elist** pelist);
static int enc(uint8_t** pp, Elem e, int lenonly);
static int val_enc(uint8_t** pp, Elem e, int *pconstr, int lenonly);
static void uint7_enc(uint8_t** pp, int num, int lenonly);
static void int_enc(uint8_t** pp, int num, int unsgned, int lenonly);

static void *
emalloc(int n)
{
	void *p;
	if(n==0)
		n=1;
	p = malloc(n);
	if(p == nil)
		sysfatal("out of memory");
	memset(p, 0, n);
	setmalloctag(p, getcallerpc());
	return p;
}

static char*
estrdup(char *s)
{
	char *d;
	int n;

	n = strlen(s)+1;
	d = emalloc(n);
	memmove(d, s, n);
	return d;
}


/*
 * Decode a[0..len] as a BER encoding of an ASN1 type.
 * The return value is one of ASN_OK, etc.
 * Depending on the error, the returned elem may or may not
 * be nil.
 */
static int
decode(uint8_t* a, int alen, Elem* pelem)
{
	uint8_t* p = a;
	int err;

	err = ber_decode(&p, &a[alen], pelem);
	if(err == ASN_OK && p != &a[alen])
		err = ASN_EVALLEN;
	return err;
}

/*
 * All of the following decoding routines take arguments:
 *	uint8_t **pp;
 *	uint8_t *pend;
 * Where parsing is supposed to start at **pp, and when parsing
 * is done, *pp is updated to point at next char to be parsed.
 * The pend pointer is just past end of string; an error should
 * be returned parsing hasn't finished by then.
 *
 * The returned int is ASN_OK if all went fine, else ASN_ESHORT, etc.
 * The remaining argument(s) are pointers to where parsed entity goes.
 */

/* Decode an ASN1 'Elem' (tag, length, value) */
static int
ber_decode(uint8_t** pp, uint8_t* pend, Elem* pelem)
{
	int err;
	int isconstr;
	int length;
	Tag tag;
	Value val;

	memset(pelem, 0, sizeof(*pelem));
	err = tag_decode(pp, pend, &tag, &isconstr);
	if(err == ASN_OK) {
		err = length_decode(pp, pend, &length);
		if(err == ASN_OK) {
			if(tag.class == Universal)
				err = value_decode(pp, pend, length, tag.num, isconstr, &val);
			else
				err = value_decode(pp, pend, length, OCTET_STRING, 0, &val);
			if(err == ASN_OK) {
				pelem->tag = tag;
				pelem->val = val;
			}
		}
	}
	return err;
}

/* Decode a tag field */
static int
tag_decode(uint8_t** pp, uint8_t* pend, Tag* ptag, int* pisconstr)
{
	int err;
	int v;
	uint8_t* p;

	err = ASN_OK;
	p = *pp;
	if(pend-p >= 2) {
		v = *p++;
		ptag->class = v&CLASS_MASK;
		if(v&CONSTR_MASK)
			*pisconstr = 1;
		else
			*pisconstr = 0;
		v &= TAG_MASK;
		if(v == TAG_MASK)
			err = uint7_decode(&p, pend, &v);
		ptag->num = v;
	}
	else
		err = ASN_ESHORT;
	*pp = p;
	return err;
}

/* Decode a length field */
static int
length_decode(uint8_t** pp, uint8_t* pend, int* plength)
{
	int err;
	int num;
	int v;
	uint8_t* p;

	err = ASN_OK;
	num = 0;
	p = *pp;
	if(p < pend) {
		v = *p++;
		if(v&0x80)
			err = int_decode(&p, pend, v&0x7F, 1, &num);
		else
			num = v;
	}
	else
		err = ASN_ESHORT;
	*pp = p;
	*plength = num;
	return err;
}

/* Decode a value field  */
static int
value_decode(uint8_t** pp, uint8_t* pend, int length, int kind, int isconstr, Value* pval)
{
	int err;
	Bytes* va;
	int num;
	int bitsunused;
	int subids[MAXOBJIDLEN];
	int isubid;
	Elist*	vl;
	uint8_t* p;
	uint8_t* pe;

	err = ASN_OK;
	p = *pp;
	if(length == -1) {	/* "indefinite" length spec */
		if(!isconstr)
			err = ASN_EINVAL;
	}
	else if(p + length > pend)
		err = ASN_EVALLEN;
	if(err != ASN_OK)
		return err;

	switch(kind) {
	case 0:
		/* marker for end of indefinite constructions */
		if(length == 0)
			pval->tag = VNull;
		else
			err = ASN_EINVAL;
		break;

	case BOOLEAN:
		if(isconstr)
			err = ASN_ECONSTR;
		else if(length != 1)
			err = ASN_EVALLEN;
		else {
			pval->tag = VBool;
			pval->u.boolval = (*p++ != 0);
		}
		break;

	case INTEGER:
	case ENUMERATED:
		if(isconstr)
			err = ASN_ECONSTR;
		else if(length <= 4) {
			err = int_decode(&p, pend, length, 0, &num);
			if(err == ASN_OK) {
				pval->tag = VInt;
				pval->u.intval = num;
			}
		}
		else {
			pval->tag = VBigInt;
			pval->u.bigintval = makebytes(p, length);
			p += length;
		}
		break;

	case BIT_STRING:
		pval->tag = VBitString;
		if(isconstr) {
			if(length == -1 && p + 2 <= pend && *p == 0 && *(p+1) ==0) {
				pval->u.bitstringval = makebits(0, 0, 0);
				p += 2;
			}
			else	/* TODO: recurse and concat results */
				err = ASN_EUNIMPL;
		}
		else {
			if(length < 2) {
				if(length == 1 && *p == 0) {
					pval->u.bitstringval = makebits(0, 0, 0);
					p++;
				}
				else
					err = ASN_EINVAL;
			}
			else {
				bitsunused = *p;
				if(bitsunused > 7)
					err = ASN_EINVAL;
				else if(length > 0x0FFFFFFF)
					err = ASN_ETOOBIG;
				else {
					pval->u.bitstringval = makebits(p+1, length-1, bitsunused);
					p += length;
				}
			}
		}
		break;

	case OCTET_STRING:
	case ObjectDescriptor:
		err = octet_decode(&p, pend, length, isconstr, &va);
		if(err == ASN_OK) {
			pval->tag = VOctets;
			pval->u.octetsval = va;
		}
		break;

	case NULLTAG:
		if(isconstr)
			err = ASN_ECONSTR;
		else if(length != 0)
			err = ASN_EVALLEN;
		else
			pval->tag = VNull;
		break;

	case OBJECT_ID:
		if(isconstr)
			err = ASN_ECONSTR;
		else if(length == 0)
			err = ASN_EVALLEN;
		else {
			isubid = 0;
			pe = p+length;
			while(p < pe && isubid < MAXOBJIDLEN) {
				err = uint7_decode(&p, pend, &num);
				if(err != ASN_OK)
					break;
				if(isubid == 0) {
					subids[isubid++] = num / 40;
					subids[isubid++] = num % 40;
				}
				else
					subids[isubid++] = num;
			}
			if(err == ASN_OK) {
				if(p != pe)
					err = ASN_EVALLEN;
				else {
					pval->tag = VObjId;
					pval->u.objidval = makeints(subids, isubid);
				}
			}
		}
		break;

	case EXTERNAL:
	case EMBEDDED_PDV:
		/* TODO: parse this internally */
		if(p+length > pend)
			err = ASN_EVALLEN;
		else {
			pval->tag = VOther;
			pval->u.otherval = makebytes(p, length);
			p += length;
		}
		break;

	case REAL:
		/* Let the application decode */
		if(isconstr)
			err = ASN_ECONSTR;
		else if(p+length > pend)
			err = ASN_EVALLEN;
		else {
			pval->tag = VReal;
			pval->u.realval = makebytes(p, length);
			p += length;
		}
		break;

	case SEQUENCE:
		err = seq_decode(&p, pend, length, isconstr, &vl);
		if(err == ASN_OK) {
			pval->tag = VSeq ;
			pval->u.seqval = vl;
		}
		break;

	case SETOF:
		err = seq_decode(&p, pend, length, isconstr, &vl);
		if(err == ASN_OK) {
			pval->tag = VSet;
			pval->u.setval = vl;
		}
		break;

	case UTF8String:
	case NumericString:
	case PrintableString:
	case TeletexString:
	case VideotexString:
	case IA5String:
	case UTCTime:
	case GeneralizedTime:
	case GraphicString:
	case VisibleString:
	case GeneralString:
	case UniversalString:
	case BMPString:
		err = octet_decode(&p, pend, length, isconstr, &va);
		if(err == ASN_OK) {
			uint8_t *s;
			char *d;
			Rune r;
			int n;

			switch(kind){
			case UniversalString:
				n = va->len / 4;
				d = emalloc(n*UTFmax+1);
				pval->u.stringval = d;
				s = va->data;
				while(n > 0){
					r = s[0]<<24 | s[1]<<16 | s[2]<<8 | s[3];
					if(r == 0)
						break;
					n--;
					s += 4;
					d += runetochar(d, &r);
				}
				*d = 0;
				break;
			case BMPString:
				n = va->len / 2;
				d = emalloc(n*UTFmax+1);
				pval->u.stringval = d;
				s = va->data;
				while(n > 0){
					r = s[0]<<8 | s[1];
					if(r == 0)
						break;
					n--;
					s += 2;
					d += runetochar(d, &r);
				}
				*d = 0;
				break;
			default:
				n = va->len;
				d = emalloc(n+1);
				pval->u.stringval = d;
				s = va->data;
				while(n > 0){
					if((*d = *s) == 0)
						break;
					n--;
					s++;
					d++;
				}
				*d = 0;
				break;
			}
			if(n != 0){
				err = ASN_EINVAL;
				free(pval->u.stringval);
			} else
				pval->tag = VString;
			free(va);
		}
		break;

	default:
		if(p+length > pend)
			err = ASN_EVALLEN;
		else {
			pval->tag = VOther;
			pval->u.otherval = makebytes(p, length);
			p += length;
		}
		break;
	}
	*pp = p;
	return err;
}

/*
 * Decode an int in format where count bytes are
 * concatenated to form value.
 * Although ASN1 allows any size integer, we return
 * an error if the result doesn't fit in a 32-bit int.
 * If unsgned is not set, make sure to propagate sign bit.
 */
static int
int_decode(uint8_t** pp, uint8_t* pend, int count, int unsgned, int* pint)
{
	int err;
	int num;
	uint8_t* p;

	p = *pp;
	err = ASN_OK;
	num = 0;
	if(p+count <= pend) {
		if((count > 4) || (unsgned && count == 4 && (*p&0x80)))
			err = ASN_ETOOBIG;
		else {
			if(!unsgned && count > 0 && count < 4 && (*p&0x80))
				num = -1;	/* set all bits, initially */
			while(count--)
				num = (num << 8)|(*p++);
		}
	}
	else
		err = ASN_ESHORT;
	*pint = num;
	*pp = p;
	return err;
}

/*
 * Decode an unsigned int in format where each
 * byte except last has high bit set, and remaining
 * seven bits of each byte are concatenated to form value.
 * Although ASN1 allows any size integer, we return
 * an error if the result doesn't fit in a 32 bit int.
 */
static int
uint7_decode(uint8_t** pp, uint8_t* pend, int* pint)
{
	int err;
	int num;
	int more;
	int v;
	uint8_t* p;

	p = *pp;
	err = ASN_OK;
	num = 0;
	more = 1;
	while(more && p < pend) {
		v = *p++;
		if(num&0x7F000000) {
			err = ASN_ETOOBIG;
			break;
		}
		num <<= 7;
		more = v&0x80;
		num |= (v&0x7F);
	}
	if(p == pend)
		err = ASN_ESHORT;
	*pint = num;
	*pp = p;
	return err;
}

/*
 * Decode an octet string, recursively if isconstr.
 * We've already checked that length==-1 implies isconstr==1,
 * and otherwise that specified length fits within (*pp..pend)
 */
static int
octet_decode(uint8_t** pp, uint8_t* pend, int length, int isconstr, Bytes** pbytes)
{
	int err;
	uint8_t* p;
	Bytes* ans;
	Bytes* newans;
	uint8_t* pstart;
	uint8_t* pold;
	Elem	elem;

	err = ASN_OK;
	p = *pp;
	ans = nil;
	if(length >= 0 && !isconstr) {
		ans = makebytes(p, length);
		p += length;
	}
	else {
		/* constructed, either definite or indefinite length */
		pstart = p;
		for(;;) {
			if(length >= 0 && p >= pstart + length) {
				if(p != pstart + length)
					err = ASN_EVALLEN;
				break;
			}
			pold = p;
			err = ber_decode(&p, pend, &elem);
			if(err != ASN_OK)
				break;
			switch(elem.val.tag) {
			case VOctets:
				newans = catbytes(ans, elem.val.u.octetsval);
				freevalfields(&elem.val);
				freebytes(ans);
				ans = newans;
				break;

			case VEOC:
				if(length == -1)
					goto cloop_done;
				/* no break */
			default:
				freevalfields(&elem.val);
				p = pold;
				err = ASN_EINVAL;
				goto cloop_done;
			}
		}
cloop_done:
		if(err != ASN_OK){
			freebytes(ans);
			ans = nil;
		}
	}
	*pp = p;
	*pbytes = ans;
	return err;
}

/*
 * Decode a sequence or set.
 * We've already checked that length==-1 implies isconstr==1,
 * and otherwise that specified length fits within (*p..pend)
 */
static int
seq_decode(uint8_t** pp, uint8_t* pend, int length, int isconstr, Elist** pelist)
{
	int err;
	uint8_t* p;
	uint8_t* pstart;
	uint8_t* pold;
	Elist* ans;
	Elem elem;
	Elist* lve;
	Elist* lveold;

	err = ASN_OK;
	ans = nil;
	p = *pp;
	if(!isconstr)
		err = ASN_EPRIM;
	else {
		/* constructed, either definite or indefinite length */
		lve = nil;
		pstart = p;
		for(;;) {
			if(length >= 0 && p >= pstart + length) {
				if(p != pstart + length)
					err = ASN_EVALLEN;
				break;
			}
			pold = p;
			err = ber_decode(&p, pend, &elem);
			if(err != ASN_OK)
				break;
			if(elem.val.tag == VEOC) {
				if(length != -1) {
					p = pold;
					err = ASN_EINVAL;
				}
				break;
			}
			else
				lve = mkel(elem, lve);
		}
		if(err != ASN_OK)
			freeelist(lve);
		else {
			/* reverse back to original order */
			while(lve != nil) {
				lveold = lve;
				lve = lve->tl;
				lveold->tl = ans;
				ans = lveold;
			}
		}
	}
	*pp = p;
	*pelist = ans;
	return err;
}

/*
 * Encode e by BER rules, putting answer in *pbytes.
 * This is done by first calling enc with lenonly==1
 * to get the length of the needed buffer,
 * then allocating the buffer and using enc again to fill it up.
 */
static int
encode(Elem e, Bytes** pbytes)
{
	uint8_t* p;
	Bytes* ans;
	int err;
	uint8_t uc;

	p = &uc;
	err = enc(&p, e, 1);
	if(err == ASN_OK) {
		ans = newbytes(p-&uc);
		p = ans->data;
		err = enc(&p, e, 0);
		*pbytes = ans;
	}
	return err;
}

/*
 * The various enc functions take a pointer to a pointer
 * into a buffer, and encode their entity starting there,
 * updating the pointer afterwards.
 * If lenonly is 1, only the pointer update is done,
 * allowing enc to be called first to calculate the needed
 * buffer length.
 * If lenonly is 0, it is assumed that the answer will fit.
 */

static int
enc(uint8_t** pp, Elem e, int lenonly)
{
	int err;
	int vlen;
	int constr;
	Tag tag;
	int v;
	int ilen;
	uint8_t* p;
	uint8_t* psave;

	p = *pp;
	err = val_enc(&p, e, &constr, 1);
	if(err != ASN_OK)
		return err;
	vlen = p - *pp;
	p = *pp;
	tag = e.tag;
	v = tag.class|constr;
	if(tag.num < 31) {
		if(!lenonly)
			*p = (v|tag.num);
		p++;
	}
	else {
		if(!lenonly)
			*p = (v|31);
		p++;
		if(tag.num < 0)
			return ASN_EINVAL;
		uint7_enc(&p, tag.num, lenonly);
	}
	if(vlen < 0x80) {
		if(!lenonly)
			*p = vlen;
		p++;
	}
	else {
		psave = p;
		int_enc(&p, vlen, 1, 1);
		ilen = p-psave;
		p = psave;
		if(!lenonly) {
			*p++ = (0x80 | ilen);
			int_enc(&p, vlen, 1, 0);
		}
		else
			p += 1 + ilen;
	}
	if(!lenonly)
		val_enc(&p, e, &constr, 0);
	else
		p += vlen;
	*pp = p;
	return err;
}

static int
val_enc(uint8_t** pp, Elem e, int *pconstr, int lenonly)
{
	int err;
	uint8_t* p;
	int kind;
	int cl;
	int v;
	Bytes* bb = nil;
	Bits* bits;
	Ints* oid;
	int k;
	Elist* el;
	char* s;

	p = *pp;
	err = ASN_OK;
	kind = e.tag.num;
	cl = e.tag.class;
	*pconstr = 0;
	if(cl != Universal) {
		switch(e.val.tag) {
		case VBool:
			kind = BOOLEAN;
			break;
		case VInt:
			kind = INTEGER;
			break;
		case VBigInt:
			kind = INTEGER;
			break;
		case VOctets:
			kind = OCTET_STRING;
			break;
		case VReal:
			kind = REAL;
			break;
		case VOther:
			kind = OCTET_STRING;
			break;
		case VBitString:
			kind = BIT_STRING;
			break;
		case VNull:
			kind = NULLTAG;
			break;
		case VObjId:
			kind = OBJECT_ID;
			break;
		case VString:
			kind = UniversalString;
			break;
		case VSeq:
			kind = SEQUENCE;
			break;
		case VSet:
			kind = SETOF;
			break;
		}
	}
	switch(kind) {
	case BOOLEAN:
		if(is_int(&e, &v)) {
			if(v != 0)
				v = 255;
			 int_enc(&p, v, 1, lenonly);
		}
		else
			err = ASN_EINVAL;
		break;

	case INTEGER:
	case ENUMERATED:
		if(is_int(&e, &v))
			int_enc(&p, v, 0, lenonly);
		else {
			if(is_bigint(&e, &bb)) {
				if(!lenonly)
					memmove(p, bb->data, bb->len);
				p += bb->len;
			}
			else
				err = ASN_EINVAL;
		}
		break;

	case BIT_STRING:
		if(is_bitstring(&e, &bits)) {
			if(bits->len == 0) {
				if(!lenonly)
					*p = 0;
				p++;
			}
			else {
				v = bits->unusedbits;
				if(v < 0 || v > 7)
					err = ASN_EINVAL;
				else {
					if(!lenonly) {
						*p = v;
						memmove(p+1, bits->data, bits->len);
					}
					p += 1 + bits->len;
				}
			}
		}
		else
			err = ASN_EINVAL;
		break;

	case OCTET_STRING:
	case ObjectDescriptor:
	case EXTERNAL:
	case REAL:
	case EMBEDDED_PDV:
		bb = nil;
		switch(e.val.tag) {
		case VOctets:
			bb = e.val.u.octetsval;
			break;
		case VReal:
			bb = e.val.u.realval;
			break;
		case VOther:
			bb = e.val.u.otherval;
			break;
		}
		if(bb != nil) {
			if(!lenonly)
				memmove(p, bb->data, bb->len);
			p += bb->len;
		}
		else
			err = ASN_EINVAL;
		break;

	case NULLTAG:
		break;

	case OBJECT_ID:
		if(is_oid(&e, &oid)) {
			for(k = 0; k < oid->len; k++) {
				v = oid->data[k];
				if(k == 0) {
					v *= 40;
					if(oid->len > 1)
						v += oid->data[++k];
				}
				uint7_enc(&p, v, lenonly);
			}
		}
		else
			err = ASN_EINVAL;
		break;

	case SEQUENCE:
	case SETOF:
		el = nil;
		if(e.val.tag == VSeq)
			el = e.val.u.seqval;
		else if(e.val.tag == VSet)
			el = e.val.u.setval;
		else
			err = ASN_EINVAL;
		if(el != nil) {
			*pconstr = CONSTR_MASK;
			for(; el != nil; el = el->tl) {
				err = enc(&p, el->hd, lenonly);
				if(err != ASN_OK)
					break;
			}
		}
		break;

	case UTF8String:
	case NumericString:
	case PrintableString:
	case TeletexString:
	case VideotexString:
	case IA5String:
	case UTCTime:
	case GeneralizedTime:
	case GraphicString:
	case VisibleString:
	case GeneralString:
	case UniversalString:
	case BMPString:
		if(e.val.tag == VString) {
			s = e.val.u.stringval;
			if(s != nil) {
				v = strlen(s);
				if(!lenonly)
					memmove(p, s, v);
				p += v;
			}
		}
		else
			err = ASN_EINVAL;
		break;

	default:
		err = ASN_EINVAL;
	}
	*pp = p;
	return err;
}

/*
 * Encode num as unsigned 7 bit values with top bit 1 on all bytes
 * except last, only putting in bytes if !lenonly.
 */
static void
uint7_enc(uint8_t** pp, int num, int lenonly)
{
	int n;
	int v;
	int k;
	uint8_t* p;

	p = *pp;
	n = 1;
	v = num >> 7;
	while(v > 0) {
		v >>= 7;
		n++;
	}
	if(lenonly)
		p += n;
	else {
		for(k = (n - 1)*7; k > 0; k -= 7)
			*p++= ((num >> k)|0x80);
		*p++ = (num&0x7F);
	}
	*pp = p;
}

/*
 * Encode num as unsigned or signed integer,
 * only putting in bytes if !lenonly.
 * Encoding is length followed by bytes to concatenate.
 */
static void
int_enc(uint8_t** pp, int num, int unsgned, int lenonly)
{
	int v;
	int n;
	int prevv;
	int k;
	uint8_t* p;

	p = *pp;
	v = num;
	if(v < 0)
		v = -(v + 1);
	n = 1;
	prevv = v;
	v >>= 8;
	while(v > 0) {
		prevv = v;
		v >>= 8;
		n++;
	}
	if(!unsgned && (prevv&0x80))
		n++;
	if(lenonly)
		p += n;
	else {
		for(k = (n - 1)*8; k >= 0; k -= 8)
			*p++ = (num >> k);
	}
	*pp = p;
}

static int
ints_eq(Ints* a, Ints* b)
{
	int	alen;
	int	i;

	alen = a->len;
	if(alen != b->len)
		return 0;
	for(i = 0; i < alen; i++)
		if(a->data[i] != b->data[i])
			return 0;
	return 1;
}

/*
 * Look up o in tab (which must have nil entry to terminate).
 * Return index of matching entry, or -1 if none.
 */
static int
oid_lookup(Ints* o, Ints** tab)
{
	int i;

	for(i = 0; tab[i] != nil; i++)
		if(ints_eq(o, tab[i]))
			return  i;
	return -1;
}

/*
 * Return true if *pe is a SEQUENCE, and set *pseq to
 * the value of the sequence if so.
 */
static int
is_seq(Elem* pe, Elist** pseq)
{
	if(pe->tag.class == Universal && pe->tag.num == SEQUENCE && pe->val.tag == VSeq) {
		*pseq = pe->val.u.seqval;
		return 1;
	}
	return 0;
}

static int
is_set(Elem* pe, Elist** pset)
{
	if(pe->tag.class == Universal && pe->tag.num == SETOF && pe->val.tag == VSet) {
		*pset = pe->val.u.setval;
		return 1;
	}
	return 0;
}

static int
is_int(Elem* pe, int* pint)
{
	if(pe->tag.class == Universal) {
		if(pe->tag.num == INTEGER && pe->val.tag == VInt) {
			*pint = pe->val.u.intval;
			return 1;
		}
		else if(pe->tag.num == BOOLEAN && pe->val.tag == VBool) {
			*pint = pe->val.u.boolval;
			return 1;
		}
	}
	return 0;
}

/*
 * for convience, all VInt's are readable via this routine,
 * as well as all VBigInt's
 */
static int
is_bigint(Elem* pe, Bytes** pbigint)
{
	if(pe->tag.class == Universal && pe->tag.num == INTEGER && pe->val.tag == VBigInt) {
		*pbigint = pe->val.u.bigintval;
		return 1;
	}
	return 0;
}

static int
is_bitstring(Elem* pe, Bits** pbits)
{
	if(pe->tag.class == Universal && pe->tag.num == BIT_STRING && pe->val.tag == VBitString) {
		*pbits = pe->val.u.bitstringval;
		return 1;
	}
	return 0;
}

static int
is_octetstring(Elem* pe, Bytes** poctets)
{
	if(pe->tag.class == Universal && pe->tag.num == OCTET_STRING && pe->val.tag == VOctets) {
		*poctets = pe->val.u.octetsval;
		return 1;
	}
	return 0;
}

static int
is_oid(Elem* pe, Ints** poid)
{
	if(pe->tag.class == Universal && pe->tag.num == OBJECT_ID && pe->val.tag == VObjId) {
		*poid = pe->val.u.objidval;
		return 1;
	}
	return 0;
}

static int
is_string(Elem* pe, char** pstring)
{
	if(pe->tag.class == Universal) {
		switch(pe->tag.num) {
		case UTF8String:
		case NumericString:
		case PrintableString:
		case TeletexString:
		case VideotexString:
		case IA5String:
		case GraphicString:
		case VisibleString:
		case GeneralString:
		case UniversalString:
		case BMPString:
			if(pe->val.tag == VString) {
				*pstring = pe->val.u.stringval;
				return 1;
			}
		}
	}
	return 0;
}

static int
is_time(Elem* pe, char** ptime)
{
	if(pe->tag.class == Universal
	   && (pe->tag.num == UTCTime || pe->tag.num == GeneralizedTime)
	   && pe->val.tag == VString) {
		*ptime = pe->val.u.stringval;
		return 1;
	}
	return 0;
}


/*
 * malloc and return a new Bytes structure capable of
 * holding len bytes. (len >= 0)
 */
static Bytes*
newbytes(int len)
{
	Bytes* ans;

	if(len < 0)
		abort();
	ans = emalloc(sizeof(Bytes) + len);
	ans->len = len;
	return ans;
}

/*
 * newbytes(len), with data initialized from buf
 */
static Bytes*
makebytes(uint8_t* buf, int len)
{
	Bytes* ans;

	ans = newbytes(len);
	memmove(ans->data, buf, len);
	return ans;
}

static void
freebytes(Bytes* b)
{
	free(b);
}

/*
 * Make a new Bytes, containing bytes of b1 followed by those of b2.
 * Either b1 or b2 or both can be nil.
 */
static Bytes*
catbytes(Bytes* b1, Bytes* b2)
{
	Bytes* ans;
	int n;

	if(b1 == nil) {
		if(b2 == nil)
			ans = newbytes(0);
		else
			ans = makebytes(b2->data, b2->len);
	}
	else if(b2 == nil) {
		ans = makebytes(b1->data, b1->len);
	}
	else {
		n = b1->len + b2->len;
		ans = newbytes(n);
		ans->len = n;
		memmove(ans->data, b1->data, b1->len);
		memmove(ans->data+b1->len, b2->data, b2->len);
	}
	return ans;
}

/* len is number of ints */
static Ints*
newints(int len)
{
	Ints* ans;

	if(len < 0 || len > ((uint)-1>>1)/sizeof(int))
		abort();
	ans = emalloc(sizeof(Ints) + len*sizeof(int));
	ans->len = len;
	return ans;
}

static Ints*
makeints(int* buf, int len)
{
	Ints* ans;

	ans = newints(len);
	memmove(ans->data, buf, len*sizeof(int));
	return ans;
}

static void
freeints(Ints* b)
{
	free(b);
}

/* len is number of bytes */
static Bits*
newbits(int len)
{
	Bits* ans;

	if(len < 0)
		abort();
	ans = emalloc(sizeof(Bits) + len);
	ans->len = len;
	ans->unusedbits = 0;
	return ans;
}

static Bits*
makebits(uint8_t* buf, int len, int unusedbits)
{
	Bits* ans;

	ans = newbits(len);
	memmove(ans->data, buf, len);
	ans->unusedbits = unusedbits;
	return ans;
}

static void
freebits(Bits* b)
{
	free(b);
}

static Elist*
mkel(Elem e, Elist* tail)
{
	Elist* el;

	el = (Elist*)emalloc(sizeof(Elist));
	setmalloctag(el, getcallerpc());
	el->hd = e;
	el->tl = tail;
	return el;
}

static int
elistlen(Elist* el)
{
	int ans = 0;
	while(el != nil) {
		ans++;
		el = el->tl;
	}
	return ans;
}

/* Frees elist, but not fields inside values of constituent elems */
static void
freeelist(Elist* el)
{
	Elist* next;

	while(el != nil) {
		next = el->tl;
		free(el);
		el = next;
	}
}

/* free any allocated structures inside v (recursively freeing Elists) */
static void
freevalfields(Value* v)
{
	Elist* el;
	Elist* l;
	if(v == nil)
		return;
	switch(v->tag) {
 	case VOctets:
		freebytes(v->u.octetsval);
		break;
	case VBigInt:
		freebytes(v->u.bigintval);
		break;
	case VReal:
		freebytes(v->u.realval);
		break;
	case VOther:
		freebytes(v->u.otherval);
		break;
	case VBitString:
		freebits(v->u.bitstringval);
		break;
	case VObjId:
		freeints(v->u.objidval);
		break;
	case VString:
		if(v->u.stringval)
			free(v->u.stringval);
		break;
	case VSeq:
		el = v->u.seqval;
		for(l = el; l != nil; l = l->tl)
			freevalfields(&l->hd.val);
		freeelist(el);
		break;
	case VSet:
		el = v->u.setval;
		for(l = el; l != nil; l = l->tl)
			freevalfields(&l->hd.val);
		freeelist(el);
		break;
	}
}

static mpint*
asn1mpint(Elem *e)
{
	Bytes *b;
	int v;

	if(is_int(e, &v))
		return itomp(v, nil);
	if(is_bigint(e, &b))
		return betomp(b->data, b->len, nil);
	return nil;
}

/* end of general ASN1 functions */





/*=============================================================*/
/*
 * Decode and parse an X.509 Certificate, defined by this ASN1:
 *	Certificate ::= SEQUENCE {
 *		certificateInfo CertificateInfo,
 *		signatureAlgorithm AlgorithmIdentifier,
 *		signature BIT STRING }
 *
 *	CertificateInfo ::= SEQUENCE {
 *		version [0] INTEGER DEFAULT v1 (0),
 *		serialNumber INTEGER,
 *		signature AlgorithmIdentifier,
 *		issuer Name,
 *		validity Validity,
 *		subject Name,
 *		subjectPublicKeyInfo SubjectPublicKeyInfo }
 *	(version v2 has two more fields, optional unique identifiers for
 *  issuer and subject; since we ignore these anyway, we won't parse them)
 *
 *	Validity ::= SEQUENCE {
 *		notBefore UTCTime,
 *		notAfter UTCTime }
 *
 *	SubjectPublicKeyInfo ::= SEQUENCE {
 *		algorithm AlgorithmIdentifier,
 *		subjectPublicKey BIT STRING }
 *
 *	AlgorithmIdentifier ::= SEQUENCE {
 *		algorithm OBJECT IDENTIFER,
 *		parameters ANY DEFINED BY ALGORITHM OPTIONAL }
 *
 *	Name ::= SEQUENCE OF RelativeDistinguishedName
 *
 *	RelativeDistinguishedName ::= SETOF SIZE(1..MAX) OF AttributeTypeAndValue
 *
 *	AttributeTypeAndValue ::= SEQUENCE {
 *		type OBJECT IDENTIFER,
 *		value DirectoryString }
 *	(selected attributes have these Object Ids:
 *		commonName {2 5 4 3}
 *		countryName {2 5 4 6}
 *		localityName {2 5 4 7}
 *		stateOrProvinceName {2 5 4 8}
 *		organizationName {2 5 4 10}
 *		organizationalUnitName {2 5 4 11}
 *	)
 *
 *	DirectoryString ::= CHOICE {
 *		teletexString TeletexString,
 *		printableString PrintableString,
 *		universalString UniversalString }
 *
 *  See rfc1423, rfc2437 for AlgorithmIdentifier, subjectPublicKeyInfo, signature.
 *
 *  Not yet implemented:
 *   CertificateRevocationList ::= SIGNED SEQUENCE{
 *           signature       AlgorithmIdentifier,
 *           issuer          Name,
 *           lastUpdate      UTCTime,
 *           nextUpdate      UTCTime,
 *           revokedCertificates
 *                           SEQUENCE OF CRLEntry OPTIONAL}
 *   CRLEntry ::= SEQUENCE{
 *           userCertificate SerialNumber,
 *           revocationDate UTCTime}
 */

typedef struct CertX509 {
	int	serial;
	char*	issuer;
	char*	validity_start;
	char*	validity_end;
	char*	subject;
	int	publickey_alg;
	Bytes*	publickey;
	int	signature_alg;
	Bytes*	signature;
	int	curve;
} CertX509;

/* Algorithm object-ids */
enum {
	ALG_rsaEncryption,
	ALG_md2WithRSAEncryption,
	ALG_md4WithRSAEncryption,
	ALG_md5WithRSAEncryption,

	ALG_sha1WithRSAEncryption,
	ALG_sha1WithRSAEncryptionOiw,

	ALG_sha256WithRSAEncryption,
	ALG_sha384WithRSAEncryption,
	ALG_sha512WithRSAEncryption,
	ALG_sha224WithRSAEncryption,

	ALG_ecPublicKey,
	ALG_sha1WithECDSA,
	ALG_sha256WithECDSA,
	ALG_sha384WithECDSA,
	ALG_sha512WithECDSA,

	ALG_md5,
	ALG_sha1,
	ALG_sha256,
	ALG_sha384,
	ALG_sha512,
	ALG_sha224,

	NUMALGS
};

typedef struct Ints15 {
	int		len;
	int		data[15];
} Ints15;

typedef struct DigestAlg {
	int		alg;
	DigestState*	(*fun)(uint8_t*,uint32_t,uint8_t*,DigestState*);
	int		len;
} DigestAlg;

static DigestAlg alg_md5 = { ALG_md5, md5, MD5dlen};
static DigestAlg alg_sha1 = { ALG_sha1, sha1, SHA1dlen };
static DigestAlg alg_sha256 = { ALG_sha256, sha2_256, SHA2_256dlen };
static DigestAlg alg_sha384 = { ALG_sha384, sha2_384, SHA2_384dlen };
static DigestAlg alg_sha512 = { ALG_sha512, sha2_512, SHA2_512dlen };
static DigestAlg alg_sha224 = { ALG_sha224, sha2_224, SHA2_224dlen };

/* maximum length of digest output of the digest algs above */
enum {
	MAXdlen = SHA2_512dlen,
};

static Ints15 oid_rsaEncryption = {7, 1, 2, 840, 113549, 1, 1, 1 };

static Ints15 oid_md2WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 2 };
static Ints15 oid_md4WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 3 };
static Ints15 oid_md5WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 4 };
static Ints15 oid_sha1WithRSAEncryption ={7, 1, 2, 840, 113549, 1, 1, 5 };
static Ints15 oid_sha1WithRSAEncryptionOiw ={6, 1, 3, 14, 3, 2, 29 };
static Ints15 oid_sha256WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 11 };
static Ints15 oid_sha384WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 12 };
static Ints15 oid_sha512WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 13 };
static Ints15 oid_sha224WithRSAEncryption = {7, 1, 2, 840, 113549, 1, 1, 14 };

static Ints15 oid_ecPublicKey = {6, 1, 2, 840, 10045, 2, 1 };
static Ints15 oid_sha1WithECDSA = {6, 1, 2, 840, 10045, 4, 1 };
static Ints15 oid_sha256WithECDSA = {7, 1, 2, 840, 10045, 4, 3, 2 };
static Ints15 oid_sha384WithECDSA = {7, 1, 2, 840, 10045, 4, 3, 3 };
static Ints15 oid_sha512WithECDSA = {7, 1, 2, 840, 10045, 4, 3, 4 };

static Ints15 oid_md5 = {6, 1, 2, 840, 113549, 2, 5 };
static Ints15 oid_sha1 = {6, 1, 3, 14, 3, 2, 26 };
static Ints15 oid_sha256= {9, 2, 16, 840, 1, 101, 3, 4, 2, 1 };
static Ints15 oid_sha384= {9, 2, 16, 840, 1, 101, 3, 4, 2, 2 };
static Ints15 oid_sha512= {9, 2, 16, 840, 1, 101, 3, 4, 2, 3 };
static Ints15 oid_sha224= {9, 2, 16, 840, 1, 101, 3, 4, 2, 4 };

static Ints *alg_oid_tab[NUMALGS+1] = {
	(Ints*)&oid_rsaEncryption,
	(Ints*)&oid_md2WithRSAEncryption,
	(Ints*)&oid_md4WithRSAEncryption,
	(Ints*)&oid_md5WithRSAEncryption,

	(Ints*)&oid_sha1WithRSAEncryption,
	(Ints*)&oid_sha1WithRSAEncryptionOiw,

	(Ints*)&oid_sha256WithRSAEncryption,
	(Ints*)&oid_sha384WithRSAEncryption,
	(Ints*)&oid_sha512WithRSAEncryption,
	(Ints*)&oid_sha224WithRSAEncryption,

	(Ints*)&oid_ecPublicKey,
	(Ints*)&oid_sha1WithECDSA,
	(Ints*)&oid_sha256WithECDSA,
	(Ints*)&oid_sha384WithECDSA,
	(Ints*)&oid_sha512WithECDSA,

	(Ints*)&oid_md5,
	(Ints*)&oid_sha1,
	(Ints*)&oid_sha256,
	(Ints*)&oid_sha384,
	(Ints*)&oid_sha512,
	(Ints*)&oid_sha224,
	nil
};

static DigestAlg *digestalg[NUMALGS+1] = {
	&alg_md5, &alg_md5, &alg_md5, &alg_md5,
	&alg_sha1, &alg_sha1,
	&alg_sha256, &alg_sha384, &alg_sha512, &alg_sha224,
	&alg_sha256, &alg_sha1, &alg_sha256, &alg_sha384, &alg_sha512,
	&alg_md5, &alg_sha1, &alg_sha256, &alg_sha384, &alg_sha512, &alg_sha224,
	nil
};

static Bytes* encode_digest(DigestAlg *da, uint8_t *digest);

static Ints15 oid_secp256r1 = {7, 1, 2, 840, 10045, 3, 1, 7};
static Ints15 oid_secp384r1 = {5, 1, 3, 132, 0, 34};

static Ints *namedcurves_oid_tab[] = {
	(Ints*)&oid_secp256r1,
	(Ints*)&oid_secp384r1,
	nil,
};
static void (*namedcurves[])(mpint *p, mpint *a, mpint *b, mpint *x, mpint *y, mpint *n, mpint *h) = {
	secp256r1,
	secp384r1,
	nil,
};

static void
freecert(CertX509* c)
{
	if(c == nil)
		return;
	free(c->issuer);
	free(c->validity_start);
	free(c->validity_end);
	free(c->subject);
	freebytes(c->publickey);
	freebytes(c->signature);
	free(c);
}

/*
 * Parse the Name ASN1 type.
 * The sequence of RelativeDistinguishedName's gives a sort of pathname,
 * from most general to most specific.  Each element of the path can be
 * one or more (but usually just one) attribute-value pair, such as
 * countryName="US".
 * We'll just form a "postal-style" address string by concatenating the elements
 * from most specific to least specific, separated by commas.
 * Return name-as-string (which must be freed by caller).
 */
static char*
parse_name(Elem* e)
{
	Elist* el;
	Elem* es;
	Elist* esetl;
	Elem* eat;
	Elist* eatl;
	char* s;
	enum { MAXPARTS = 100 };
	char* parts[MAXPARTS];
	int i;
	int plen;
	char* ans = nil;

	if(!is_seq(e, &el))
		goto errret;
	i = 0;
	plen = 0;
	while(el != nil) {
		es = &el->hd;
		if(!is_set(es, &esetl))
			goto errret;
		while(esetl != nil) {
			eat = &esetl->hd;
			if(!is_seq(eat, &eatl) || elistlen(eatl) != 2)
				goto errret;
			if(!is_string(&eatl->tl->hd, &s) || i>=MAXPARTS)
				goto errret;
			parts[i++] = s;
			plen += strlen(s) + 2;		/* room for ", " after */
			esetl = esetl->tl;
		}
		el = el->tl;
	}
	if(i > 0) {
		ans = (char*)emalloc(plen);
		*ans = '\0';
		while(--i >= 0) {
			s = parts[i];
			strcat(ans, s);
			if(i > 0)
				strcat(ans, ", ");
		}
	}

errret:
	return ans;
}

/*
 * Parse an AlgorithmIdentifer ASN1 type.
 * Look up the oid in oid_tab and return one of OID_rsaEncryption, etc..,
 * or -1 if not found.
 * For now, ignore parameters, since none of our algorithms need them.
 */
static int
parse_alg(Elem* e)
{
	Elist* el;
	Ints* oid;

	if(!is_seq(e, &el) || el == nil || !is_oid(&el->hd, &oid))
		return -1;
	return oid_lookup(oid, alg_oid_tab);
}

static int
parse_curve(Elem* e)
{
	Elist* el;
	Ints* oid;

	if(!is_seq(e, &el) || elistlen(el)<2 || !is_oid(&el->tl->hd, &oid))
		return -1;
	return oid_lookup(oid, namedcurves_oid_tab);
}

static CertX509*
decode_cert(Bytes* a)
{
	int ok = 0;
	int n;
	CertX509* c = nil;
	Elem  ecert;
	Elem* ecertinfo;
	Elem* esigalg;
	Elem* esig;
	Elem* eserial;
	Elem* eissuer;
	Elem* evalidity;
	Elem* esubj;
	Elem* epubkey;
	Elist* el;
	Elist* elcert = nil;
	Elist* elcertinfo = nil;
	Elist* elvalidity = nil;
	Elist* elpubkey = nil;
	Bits* bits = nil;
	Bytes* b;
	Elem* e;

	if(decode(a->data, a->len, &ecert) != ASN_OK)
		goto errret;

	c = (CertX509*)emalloc(sizeof(CertX509));
	c->serial = -1;
	c->issuer = nil;
	c->validity_start = nil;
	c->validity_end = nil;
	c->subject = nil;
	c->publickey_alg = -1;
	c->publickey = nil;
	c->signature_alg = -1;
	c->signature = nil;

	/* Certificate */
 	if(!is_seq(&ecert, &elcert) || elistlen(elcert) !=3)
		goto errret;
 	ecertinfo = &elcert->hd;
 	el = elcert->tl;
 	esigalg = &el->hd;
	c->signature_alg = parse_alg(esigalg);
 	el = el->tl;
 	esig = &el->hd;

	/* Certificate Info */
	if(!is_seq(ecertinfo, &elcertinfo))
		goto errret;
	n = elistlen(elcertinfo);
  	if(n < 6)
		goto errret;
	eserial =&elcertinfo->hd;
 	el = elcertinfo->tl;
 	/* check for optional version, marked by explicit context tag 0 */
	if(eserial->tag.class == Context && eserial->tag.num == 0) {
 		eserial = &el->hd;
 		if(n < 7)
 			goto errret;
 		el = el->tl;
 	}

	if(parse_alg(&el->hd) != c->signature_alg)
		goto errret;
 	el = el->tl;
 	eissuer = &el->hd;
 	el = el->tl;
 	evalidity = &el->hd;
 	el = el->tl;
 	esubj = &el->hd;
 	el = el->tl;
 	epubkey = &el->hd;
	if(!is_int(eserial, &c->serial)) {
		if(!is_bigint(eserial, &b))
			goto errret;
		c->serial = -1;	/* else we have to change cert struct */
  	}
	c->issuer = parse_name(eissuer);
	if(c->issuer == nil)
		goto errret;
	/* Validity */
  	if(!is_seq(evalidity, &elvalidity))
		goto errret;
	if(elistlen(elvalidity) != 2)
		goto errret;
	e = &elvalidity->hd;
	if(!is_time(e, &c->validity_start))
		goto errret;
	e->val.u.stringval = nil;	/* string ownership transfer */
	e = &elvalidity->tl->hd;
 	if(!is_time(e, &c->validity_end))
		goto errret;
	e->val.u.stringval = nil;	/* string ownership transfer */

	/* resume CertificateInfo */
 	c->subject = parse_name(esubj);
	if(c->subject == nil)
		goto errret;

	/* SubjectPublicKeyInfo */
	if(!is_seq(epubkey, &elpubkey))
		goto errret;
	if(elistlen(elpubkey) != 2)
		goto errret;

	c->publickey_alg = parse_alg(&elpubkey->hd);
	if(c->publickey_alg < 0)
		goto errret;
	c->curve = -1;
	if(c->publickey_alg == ALG_ecPublicKey){
		c->curve = parse_curve(&elpubkey->hd);
		if(c->curve < 0)
			goto errret;
	}
  	if(!is_bitstring(&elpubkey->tl->hd, &bits))
		goto errret;
	if(bits->unusedbits != 0)
		goto errret;
 	c->publickey = makebytes(bits->data, bits->len);

	/*resume Certificate */
	if(c->signature_alg < 0)
		goto errret;
 	if(!is_bitstring(esig, &bits))
		goto errret;
 	c->signature = makebytes(bits->data, bits->len);
	ok = 1;

errret:
	freevalfields(&ecert.val);	/* recurses through lists, too */
	if(!ok){
		freecert(c);
		c = nil;
	}
	return c;
}

/*
 *	RSAPublickKey ::= SEQUENCE {
 *		modulus INTEGER,
 *		publicExponent INTEGER
 *	}
 */
static RSApub*
decode_rsapubkey(Bytes* a)
{
	Elem e;
	Elist *el;
	RSApub* key;

	key = nil;
	if(decode(a->data, a->len, &e) != ASN_OK)
		goto errret;
	if(!is_seq(&e, &el) || elistlen(el) != 2)
		goto errret;

	key = rsapuballoc();
	if((key->n = asn1mpint(&el->hd)) == nil)
		goto errret;
	el = el->tl;
	if((key->ek = asn1mpint(&el->hd)) == nil)
		goto errret;

	freevalfields(&e.val);
	return key;
errret:
	freevalfields(&e.val);
	rsapubfree(key);
	return nil;
}

/*
 *	RSAPrivateKey ::= SEQUENCE {
 *		version Version,
 *		modulus INTEGER, -- n
 *		publicExponent INTEGER, -- e
 *		privateExponent INTEGER, -- d
 *		prime1 INTEGER, -- p
 *		prime2 INTEGER, -- q
 *		exponent1 INTEGER, -- d mod (p-1)
 *		exponent2 INTEGER, -- d mod (q-1)
 *		coefficient INTEGER -- (inverse of q) mod p }
 */
static RSApriv*
decode_rsaprivkey(Bytes* a)
{
	int version;
	Elem e;
	Elist *el;
	RSApriv* key;

	key = nil;
	if(decode(a->data, a->len, &e) != ASN_OK)
		goto errret;
	if(!is_seq(&e, &el))
		goto errret;

	if(!is_int(&el->hd, &version) || version != 0)
		goto errret;

	if(elistlen(el) != 9){
		if(elistlen(el) == 3
		&& parse_alg(&el->tl->hd) == ALG_rsaEncryption
		&& is_octetstring(&el->tl->tl->hd, &a)){
			key = decode_rsaprivkey(a);
			if(key != nil)
				goto done;
		}
		goto errret;
	}

	key = rsaprivalloc();
	el = el->tl;
	if((key->pub.n = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->pub.ek = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->dk = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->q = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->p = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->kq = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->kp = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->c2 = asn1mpint(&el->hd)) == nil)
		goto errret;

done:
	freevalfields(&e.val);
	return key;
errret:
	freevalfields(&e.val);
	rsaprivfree(key);
	return nil;
}

/*
 * 	DSAPrivateKey ::= SEQUENCE{
 *		version Version,
 *		p INTEGER,
 *		q INTEGER,
 *		g INTEGER, -- alpha
 *		pub_key INTEGER, -- key
 *		priv_key INTEGER, -- secret
 *	}
 */
static DSApriv*
decode_dsaprivkey(Bytes* a)
{
	int version;
	Elem e;
	Elist *el;
	DSApriv* key;

	key = dsaprivalloc();
	if(decode(a->data, a->len, &e) != ASN_OK)
		goto errret;
	if(!is_seq(&e, &el) || elistlen(el) != 6)
		goto errret;
	version = -1;
	if(!is_int(&el->hd, &version) || version != 0)
		goto errret;

	el = el->tl;
	if((key->pub.p = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->pub.q = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->pub.alpha = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->pub.key = asn1mpint(&el->hd)) == nil)
		goto errret;

	el = el->tl;
	if((key->secret = asn1mpint(&el->hd)) == nil)
		goto errret;

	freevalfields(&e.val);
	return key;
errret:
	freevalfields(&e.val);
	dsaprivfree(key);
	return nil;
}

RSApriv*
asn1toRSApriv(uint8_t *kd, int kn)
{
	Bytes *b;
	RSApriv *key;

	b = makebytes(kd, kn);
	key = decode_rsaprivkey(b);
	freebytes(b);
	return key;
}

DSApriv*
asn1toDSApriv(uint8_t *kd, int kn)
{
	Bytes *b;
	DSApriv *key;

	b = makebytes(kd, kn);
	key = decode_dsaprivkey(b);
	freebytes(b);
	return key;
}

/*
 * digest(CertificateInfo)
 * Our ASN.1 library doesn't return pointers into the original
 * data array, so we need to do a little hand decoding.
 */
static int
digest_certinfo(Bytes *cert, DigestAlg *da, uint8_t *digest)
{
	uint8_t *info, *p, *pend;
	uint32_t infolen;
	int isconstr, length;
	Tag tag;
	Elem elem;

	p = cert->data;
	pend = cert->data + cert->len;
	if(tag_decode(&p, pend, &tag, &isconstr) != ASN_OK ||
	   tag.class != Universal || tag.num != SEQUENCE ||
	   length_decode(&p, pend, &length) != ASN_OK ||
	   p+length > pend ||
	   p+length < p)
		return -1;
	info = p;
	if(ber_decode(&p, pend, &elem) != ASN_OK)
		return -1;
	freevalfields(&elem.val);
	if(elem.tag.num != SEQUENCE)
		return -1;
	infolen = p - info;
	(*da->fun)(info, infolen, digest, nil);
	return da->len;
}

mpint*
pkcs1padbuf(uint8_t *buf, int len, mpint *modulus, int blocktype)
{
	int i, n = (mpsignif(modulus)-1)/8;
	int pad = n - 2 - len;
	uint8_t *p;
	mpint *mp;

	if(pad < 8){
		werrstr("rsa modulus too small");
		return nil;
	}
	if((p = malloc(n)) == nil)
		return nil;
	p[0] = blocktype;
	switch(blocktype){
	default:
	case 1:
		memset(p+1, 0xFF, pad);
		break;
	case 2:
		for(i=1; i <= pad; i++)
			p[i] = 1 + nfastrand(255);
		break;
	}
	p[1+pad] = 0;
	memmove(p+2+pad, buf, len);
	mp = betomp(p, n, nil);
	free(p);
	return mp;
}

int
pkcs1unpadbuf(uint8_t *buf, int len, mpint *modulus, int blocktype)
{
	uint8_t *p = buf + 1, *e = buf + len;

	if(len < 1 || len != (mpsignif(modulus)-1)/8 || buf[0] != blocktype)
		return -1;
	switch(blocktype){
	default:
	case 1:
		while(p < e && *p == 0xFF)
			p++;
		break;
	case 2:
		while(p < e && *p != 0x00)
			p++;
		break;
	}
	if(p - buf <= 8 || p >= e || *p++ != 0x00)
		return -1;
	memmove(buf, p, len = e - p);
	return len;
}

static char Ebadsig[] = "bad signature";

char*
X509rsaverifydigest(uint8_t *sig, int siglen, uint8_t *edigest, int edigestlen, RSApub *pk)
{
	mpint *x, *y;
	DigestAlg **dp;
	Bytes *digest;
	uint8_t *buf;
	int len;
	char *err;

	x = betomp(sig, siglen, nil);
	y = rsaencrypt(pk, x, nil);
	mpfree(x);
	len = mptobe(y, nil, 0, &buf);
	mpfree(y);

	err = Ebadsig;
	len = pkcs1unpadbuf(buf, len, pk->n, 1);
	if(len == edigestlen && tsmemcmp(buf, edigest, edigestlen) == 0)
		err = nil;
	for(dp = digestalg; err != nil && *dp != nil; dp++){
		if((*dp)->len != edigestlen)
			continue;
		digest = encode_digest(*dp, edigest);
		if(digest->len == len && tsmemcmp(digest->data, buf, len) == 0)
			err = nil;
		freebytes(digest);
	}
	free(buf);
	return err;
}

char*
X509ecdsaverifydigest(uint8_t *sig, int siglen, uint8_t *edigest, int edigestlen, ECdomain *dom, ECpub *pub)
{
	Elem e;
	Elist *el;
	mpint *r, *s;
	char *err;

	r = s = nil;
	err = Ebadsig;
	if(decode(sig, siglen, &e) != ASN_OK)
		goto end;
	if(!is_seq(&e, &el) || elistlen(el) != 2)
		goto end;
	r = asn1mpint(&el->hd);
	if(r == nil)
		goto end;
	el = el->tl;
	s = asn1mpint(&el->hd);
	if(s == nil)
		goto end;
	if(ecdsaverify(dom, pub, edigest, edigestlen, r, s))
		err = nil;
end:
	freevalfields(&e.val);
	mpfree(s);
	mpfree(r);
	return err;
}

ECpub*
X509toECpub(uint8_t *cert, int ncert, char *name, int nname, ECdomain *dom)
{
	CertX509 *c;
	ECpub *pub;
	Bytes *b;

	if(name != nil)
		memset(name, 0, nname);

	b = makebytes(cert, ncert);
	c = decode_cert(b);
	freebytes(b);
	if(c == nil)
		return nil;
	if(name != nil && c->subject != nil){
		char *e = strchr(c->subject, ',');
		if(e != nil)
			*e = 0;	/* take just CN part of Distinguished Name */
		strncpy(name, c->subject, nname);
	}
	pub = nil;
	if(c->publickey_alg == ALG_ecPublicKey){
		ecdominit(dom, namedcurves[c->curve]);
		pub = ecdecodepub(dom, c->publickey->data, c->publickey->len);
		if(pub == nil)
			ecdomfree(dom);
	}
	freecert(c);
	return pub;
}

char*
X509ecdsaverify(uint8_t *cert, int ncert, ECdomain *dom, ECpub *pk)
{
	char *e;
	Bytes *b;
	CertX509 *c;
	int digestlen;
	uint8_t digest[MAXdlen];

	b = makebytes(cert, ncert);
	c = decode_cert(b);
	if(c == nil){
		freebytes(b);
		return "cannot decode cert";
	}
	digestlen = digest_certinfo(b, digestalg[c->signature_alg], digest);
	freebytes(b);
	if(digestlen <= 0){
		freecert(c);
		return "cannot decode certinfo";
	}
	e = X509ecdsaverifydigest(c->signature->data, c->signature->len, digest, digestlen, dom, pk);
	freecert(c);
	return e;
}

RSApub*
X509toRSApub(uint8_t *cert, int ncert, char *name, int nname)
{
	Bytes *b;
	CertX509 *c;
	RSApub *pub;

	if(name != nil)
		memset(name, 0, nname);

	b = makebytes(cert, ncert);
	c = decode_cert(b);
	freebytes(b);
	if(c == nil)
		return nil;
	if(name != nil && c->subject != nil){
		char *e = strchr(c->subject, ',');
		if(e != nil)
			*e = 0;	/* take just CN part of Distinguished Name */
		strncpy(name, c->subject, nname);
	}
	pub = nil;
	if(c->publickey_alg == ALG_rsaEncryption)
		pub = decode_rsapubkey(c->publickey);
	freecert(c);
	return pub;
}

char*
X509rsaverify(uint8_t *cert, int ncert, RSApub *pk)
{
	char *e;
	Bytes *b;
	CertX509 *c;
	int digestlen;
	uint8_t digest[MAXdlen];

	b = makebytes(cert, ncert);
	c = decode_cert(b);
	if(c == nil){
		freebytes(b);
		return "cannot decode cert";
	}
	digestlen = digest_certinfo(b, digestalg[c->signature_alg], digest);
	freebytes(b);
	if(digestlen <= 0){
		freecert(c);
		return "cannot decode certinfo";
	}
	e = X509rsaverifydigest(c->signature->data, c->signature->len, digest, digestlen, pk);
	freecert(c);
	return e;
}

/* ------- Elem constructors ---------- */
static Elem
Null(void)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = NULLTAG;
	e.val.tag = VNull;
	return e;
}

static Elem
mkint(int j)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = INTEGER;
	e.val.tag = VInt;
	e.val.u.intval = j;
	return e;
}

static Elem
mkbigint(mpint *p)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = INTEGER;
	e.val.tag = VBigInt;
	e.val.u.bigintval = newbytes((mpsignif(p)+8)/8);
	if(p->sign < 0){
		mpint *s = mpnew(e.val.u.bigintval->len*8+1);
		mpleft(mpone, e.val.u.bigintval->len*8, s);
		mpadd(p, s, s);
		mptober(s, e.val.u.bigintval->data, e.val.u.bigintval->len);
		mpfree(s);
	} else {
		mptober(p, e.val.u.bigintval->data, e.val.u.bigintval->len);
	}
	return e;
}

static int
printable(char *s)
{
	int c;

	while((c = (uint8_t)*s++) != 0){
		if((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9')
		|| strchr("'=()+,-./:? ", c) != nil)
			continue;
		return 0;
	}
	return 1;
}

#define DirectoryString 0

static Elem
mkstring(char *s, int t)
{
	Elem e;

	if(t == DirectoryString)
		t = printable(s) ? PrintableString : UTF8String;
	e.tag.class = Universal;
	e.tag.num = t;
	e.val.tag = VString;
	e.val.u.stringval = estrdup(s);
	return e;
}

static Elem
mkoctet(uint8_t *buf, int buflen)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = OCTET_STRING;
	e.val.tag = VOctets;
	e.val.u.octetsval = makebytes(buf, buflen);
	return e;
}

static Elem
mkbits(uint8_t *buf, int buflen)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = BIT_STRING;
	e.val.tag = VBitString;
	e.val.u.bitstringval = makebits(buf, buflen, 0);
	return e;
}

static Elem
mkutc(long t)
{
	Elem e;
	char utc[50];
	Tm *tm = gmtime(t);

	e.tag.class = Universal;
	e.tag.num = UTCTime;
	e.val.tag = VString;
	snprint(utc, sizeof(utc), "%.2d%.2d%.2d%.2d%.2d%.2dZ",
		tm->year % 100, tm->mon+1, tm->mday, tm->hour, tm->min, tm->sec);
	e.val.u.stringval = estrdup(utc);
	return e;
}

static Elem
mkoid(Ints *oid)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = OBJECT_ID;
	e.val.tag = VObjId;
	e.val.u.objidval = makeints(oid->data, oid->len);
	return e;
}

static Elem
mkseq(Elist *el)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = SEQUENCE;
	e.val.tag = VSeq;
	e.val.u.seqval = el;
	return e;
}

static Elem
mkset(Elist *el)
{
	Elem e;

	e.tag.class = Universal;
	e.tag.num = SETOF;
	e.val.tag = VSet;
	e.val.u.setval = el;
	return e;
}

static Elem
mkalg(int alg)
{
	return mkseq(mkel(mkoid(alg_oid_tab[alg]), mkel(Null(), nil)));
}

typedef struct Ints7pref {
	int	len;
	int	data[7];
	char	prefix[4];
	int	stype;
} Ints7pref;
Ints7pref DN_oid[] = {
	{4, 2, 5, 4, 6, 0, 0, 0,        "C=", PrintableString},
	{4, 2, 5, 4, 8, 0, 0, 0,        "ST=",DirectoryString},
	{4, 2, 5, 4, 7, 0, 0, 0,        "L=", DirectoryString},
	{4, 2, 5, 4, 10, 0, 0, 0,       "O=", DirectoryString},
	{4, 2, 5, 4, 11, 0, 0, 0,       "OU=",DirectoryString},
	{4, 2, 5, 4, 3, 0, 0, 0,        "CN=",DirectoryString},
	{7, 1,2,840,113549,1,9,1,       "E=", IA5String},
	{7, 0,9,2342,19200300,100,1,25,	"DC=",IA5String},
};

static Elem
mkname(Ints7pref *oid, char *subj)
{
	return mkset(mkel(mkseq(mkel(mkoid((Ints*)oid), mkel(mkstring(subj, oid->stype), nil))), nil));
}

static Elem
mkDN(char *dn)
{
	int i, j, nf;
	char *f[20], *prefix, *d2 = estrdup(dn);
	Elist* el = nil;

	nf = tokenize(d2, f, nelem(f));
	for(i=nf-1; i>=0; i--){
		for(j=0; j<nelem(DN_oid); j++){
			prefix = DN_oid[j].prefix;
			if(strncmp(f[i],prefix,strlen(prefix))==0){
				el = mkel(mkname(&DN_oid[j],f[i]+strlen(prefix)), el);
				break;
			}
		}
	}
	free(d2);
	return mkseq(el);
}

/*
 * DigestInfo ::= SEQUENCE {
 *	digestAlgorithm AlgorithmIdentifier,
 *	digest OCTET STRING }
 */
static Bytes*
encode_digest(DigestAlg *da, uint8_t *digest)
{
	Bytes *ans;
	int err;
	Elem e;

	e = mkseq(
		mkel(mkalg(da->alg),
		mkel(mkoctet(digest, da->len),
		nil)));
	err = encode(e, &ans);
	freevalfields(&e.val);
	if(err != ASN_OK)
		return nil;

	return ans;
}

int
asn1encodedigest(DigestState* (*fun)(uint8_t*, uint32_t, uint8_t*, DigestState*), uint8_t *digest, uint8_t *buf, int len)
{
	Bytes *bytes;
	DigestAlg **dp;

	for(dp = digestalg; *dp != nil; dp++){
		if((*dp)->fun != fun)
			continue;
		bytes = encode_digest(*dp, digest);
		if(bytes == nil)
			break;
		if(bytes->len > len){
			freebytes(bytes);
			break;
		}
		len = bytes->len;
		memmove(buf, bytes->data, len);
		freebytes(bytes);
		return len;
	}
	return -1;
}

static Elem
mkcont(Elem e, int num)
{
	e = mkseq(mkel(e, nil));
	e.tag.class = Context;
	e.tag.num = num;
	return e;
}

static Elem
mkaltname(char *s)
{
	Elem e;
	int i;

	for(i=0; i<nelem(DN_oid); i++){
		if(strstr(s, DN_oid[i].prefix) != nil)
			return mkcont(mkDN(s), 4); /* DN */
	}
	e = mkstring(s, IA5String);
	e.tag.class = Context;
	e.tag.num = strchr(s, '@') != nil ? 1 : 2; /* email : DNS */
	return e;
}

static Elist*
mkaltnames(char *alts)
{
	Elist *el;
	char *s, *p;

	if(alts == nil)
		return nil;

	el = nil;
	alts = estrdup(alts);
	for(s = alts; s != nil; s = p){
		while(*s == ' ')
			s++;
		if(*s == '\0')
			break;
		if((p = strchr(s, ',')) != nil)
			*p++ = 0;
		el = mkel(mkaltname(s), el);
	}
	free(alts);
	return el;
}

static Elist*
mkextel(Elem e, Ints *oid, Elist *el)
{
	Bytes *b = nil;

	if(encode(e, &b) == ASN_OK){
		el = mkel(mkseq(
			mkel(mkoid(oid),
			mkel(mkoctet(b->data, b->len),
			nil))), el);
		freebytes(b);
	}
	freevalfields(&e.val);
	return el;
}

static Ints15 oid_subjectAltName = {4, 2, 5, 29, 17 };
static Ints15 oid_extensionRequest = { 7, 1, 2, 840, 113549, 1, 9, 14};

static Elist*
mkextensions(char *alts, int req)
{
	Elist *sl, *xl;

	xl = nil;
	if((sl = mkaltnames(alts)) != nil)
		xl = mkextel(mkseq(sl), (Ints*)&oid_subjectAltName, xl);
	if(xl != nil){
		if(req) return mkel(mkcont(mkseq(
			mkel(mkoid((Ints*)&oid_extensionRequest),
			mkel(mkset(mkel(mkseq(xl), nil)), nil))), 0), nil);
		return mkel(mkcont(mkseq(xl), 3), nil);
	}
	return nil;
}

static char*
splitalts(char *s)
{
	int q;

	for(q = 0; *s != '\0'; s++){
		if(*s == '\'')
			q ^= 1;
		else if(q == 0 && *s == ','){
			*s++ = 0;
			return s;
		}
	}
	return nil;
}

uint8_t*
X509gen(RSApriv *priv, char *subj, uint32_t valid[2], int *certlen)
{
	int serial = 0, sigalg = ALG_sha256WithRSAEncryption;
	uint8_t *cert = nil;
	RSApub *pk = rsaprivtopub(priv);
	Bytes *certbytes, *pkbytes, *certinfobytes, *sigbytes;
	Elem e, certinfo;
	DigestAlg *da;
	uint8_t digest[MAXdlen], *buf;
	int buflen;
	mpint *pkcs1;
	char *alts;

	subj = estrdup(subj);
	alts = splitalts(subj);

	e = mkseq(mkel(mkbigint(pk->n),mkel(mkint(mptoi(pk->ek)),nil)));
	if(encode(e, &pkbytes) != ASN_OK)
		goto errret;
	freevalfields(&e.val);

	e = mkseq(
		mkel(mkcont(mkint(2), 0),
		mkel(mkint(serial),
		mkel(mkalg(sigalg),
		mkel(mkDN(subj),
		mkel(mkseq(
			mkel(mkutc(valid[0]),
			mkel(mkutc(valid[1]),
			nil))),
		mkel(mkDN(subj),
		mkel(mkseq(
			mkel(mkalg(ALG_rsaEncryption),
			mkel(mkbits(pkbytes->data, pkbytes->len),
			nil))),
		mkextensions(alts, 0)))))))));
	freebytes(pkbytes);
	if(encode(e, &certinfobytes) != ASN_OK)
		goto errret;

	da = digestalg[sigalg];
	(*da->fun)(certinfobytes->data, certinfobytes->len, digest, 0);
	freebytes(certinfobytes);
	certinfo = e;

	sigbytes = encode_digest(da, digest);
	if(sigbytes == nil)
		goto errret;
	pkcs1 = pkcs1padbuf(sigbytes->data, sigbytes->len, pk->n, 1);
	freebytes(sigbytes);
	if(pkcs1 == nil)
		goto errret;

	rsadecrypt(priv, pkcs1, pkcs1);
	buflen = mptobe(pkcs1, nil, 0, &buf);
	mpfree(pkcs1);
	e = mkseq(
		mkel(certinfo,
		mkel(mkalg(sigalg),
		mkel(mkbits(buf, buflen),
		nil))));
	free(buf);
	if(encode(e, &certbytes) != ASN_OK)
		goto errret;
	if(certlen)
		*certlen = certbytes->len;
	cert = malloc(certbytes->len);
	if(cert != nil)
		memmove(cert, certbytes->data, certbytes->len);
	freebytes(certbytes);
errret:
	freevalfields(&e.val);
	free(subj);
	return cert;
}

uint8_t*
X509req(RSApriv *priv, char *subj, int *certlen)
{
	/* RFC 2314, PKCS #10 Certification Request Syntax */
	int version = 0, sigalg = ALG_sha256WithRSAEncryption;
	uint8_t *cert = nil;
	RSApub *pk = rsaprivtopub(priv);
	Bytes *certbytes, *pkbytes, *certinfobytes, *sigbytes;
	Elem e, certinfo;
	DigestAlg *da;
	uint8_t digest[MAXdlen], *buf;
	int buflen;
	mpint *pkcs1;
	char *alts;

	subj = estrdup(subj);
	alts = splitalts(subj);

	e = mkseq(mkel(mkbigint(pk->n),mkel(mkint(mptoi(pk->ek)),nil)));
	if(encode(e, &pkbytes) != ASN_OK)
		goto errret;
	freevalfields(&e.val);
	e = mkseq(
		mkel(mkint(version),
		mkel(mkDN(subj),
		mkel(mkseq(
			mkel(mkalg(ALG_rsaEncryption),
			mkel(mkbits(pkbytes->data, pkbytes->len),
			nil))),
		mkextensions(alts, 1)))));
	freebytes(pkbytes);
	if(encode(e, &certinfobytes) != ASN_OK)
		goto errret;
	da = digestalg[sigalg];
	(*da->fun)(certinfobytes->data, certinfobytes->len, digest, 0);
	freebytes(certinfobytes);
	certinfo = e;

	sigbytes = encode_digest(da, digest);
	if(sigbytes == nil)
		goto errret;
	pkcs1 = pkcs1padbuf(sigbytes->data, sigbytes->len, pk->n, 1);
	freebytes(sigbytes);
	if(pkcs1 == nil)
		goto errret;

	rsadecrypt(priv, pkcs1, pkcs1);
	buflen = mptobe(pkcs1, nil, 0, &buf);
	mpfree(pkcs1);
	e = mkseq(
		mkel(certinfo,
		mkel(mkalg(sigalg),
		mkel(mkbits(buf, buflen),
		nil))));
	free(buf);
	if(encode(e, &certbytes) != ASN_OK)
		goto errret;
	if(certlen)
		*certlen = certbytes->len;
	cert = malloc(certbytes->len);
	if(cert != nil)
		memmove(cert, certbytes->data, certbytes->len);
	freebytes(certbytes);
errret:
	freevalfields(&e.val);
	free(subj);
	return cert;
}

static char*
tagdump(Tag tag)
{
	static char buf[32];

	if(tag.class != Universal){
		snprint(buf, sizeof(buf), "class%d,num%d", tag.class, tag.num);
		return buf;
	}
	switch(tag.num){
	case BOOLEAN: return "BOOLEAN";
	case INTEGER: return "INTEGER";
	case BIT_STRING: return "BIT STRING";
	case OCTET_STRING: return "OCTET STRING";
	case NULLTAG: return "NULLTAG";
	case OBJECT_ID: return "OID";
	case ObjectDescriptor: return "OBJECT_DES";
	case EXTERNAL: return "EXTERNAL";
	case REAL: return "REAL";
	case ENUMERATED: return "ENUMERATED";
	case EMBEDDED_PDV: return "EMBEDDED PDV";
	case SEQUENCE: return "SEQUENCE";
	case SETOF: return "SETOF";
	case UTF8String: return "UTF8String";
	case NumericString: return "NumericString";
	case PrintableString: return "PrintableString";
	case TeletexString: return "TeletexString";
	case VideotexString: return "VideotexString";
	case IA5String: return "IA5String";
	case UTCTime: return "UTCTime";
	case GeneralizedTime: return "GeneralizedTime";
	case GraphicString: return "GraphicString";
	case VisibleString: return "VisibleString";
	case GeneralString: return "GeneralString";
	case UniversalString: return "UniversalString";
	case BMPString: return "BMPString";
	default:
		snprint(buf, sizeof(buf), "Universal,num%d", tag.num);
		return buf;
	}
}

static void
edump(Elem e)
{
	Value v;
	Elist *el;
	int i;

	print("%s{", tagdump(e.tag));
	v = e.val;
	switch(v.tag){
	case VBool: print("Bool %d",v.u.boolval); break;
	case VInt: print("Int %d",v.u.intval); break;
	case VOctets: print("Octets[%d] %.2x%.2x...",v.u.octetsval->len,v.u.octetsval->data[0],v.u.octetsval->data[1]); break;
	case VBigInt: print("BigInt[%d] %.2x%.2x...",v.u.bigintval->len,v.u.bigintval->data[0],v.u.bigintval->data[1]); break;
	case VReal: print("Real..."); break;
	case VOther: print("Other..."); break;
	case VBitString: print("BitString[%d]...", v.u.bitstringval->len*8 - v.u.bitstringval->unusedbits); break;
	case VNull: print("Null"); break;
	case VEOC: print("EOC..."); break;
	case VObjId: print("ObjId");
		for(i = 0; i<v.u.objidval->len; i++)
			print(" %d", v.u.objidval->data[i]);
		break;
	case VString: print("String \"%s\"",v.u.stringval); break;
	case VSeq: print("Seq\n");
		for(el = v.u.seqval; el!=nil; el = el->tl)
			edump(el->hd);
		break;
	case VSet: print("Set\n");
		for(el = v.u.setval; el!=nil; el = el->tl)
			edump(el->hd);
		break;
	}
	print("}\n");
}

void
asn1dump(uint8_t *der, int len)
{
	Elem e;

	if(decode(der, len, &e) != ASN_OK){
		print("didn't parse\n");
		exits("didn't parse");
	}
	edump(e);
}

void
X509dump(uint8_t *cert, int ncert)
{
	char *e;
	Bytes *b;
	CertX509 *c;
	RSApub *rsapub;
	ECpub *ecpub;
	ECdomain ecdom;
	int digestlen;
	uint8_t digest[MAXdlen];

	print("begin X509dump\n");
	b = makebytes(cert, ncert);
	c = decode_cert(b);
	if(c == nil){
		freebytes(b);
		print("cannot decode cert\n");
		return;
	}
	digestlen = digest_certinfo(b, digestalg[c->signature_alg], digest);
	freebytes(b);
	if(digestlen <= 0){
		freecert(c);
		print("cannot decode certinfo\n");
		return;
	}

	print("serial %d\n", c->serial);
	print("issuer %s\n", c->issuer);
	print("validity %s %s\n", c->validity_start, c->validity_end);
	print("subject %s\n", c->subject);
	print("sigalg=%d digest=%.*H\n", c->signature_alg, digestlen, digest);
	print("publickey_alg=%d pubkey[%d] %.*H\n", c->publickey_alg, c->publickey->len,
		c->publickey->len, c->publickey->data);

	switch(c->publickey_alg){
	case ALG_rsaEncryption:
		rsapub = decode_rsapubkey(c->publickey);
		if(rsapub != nil){
			print("rsa pubkey e=%B n(%d)=%B\n", rsapub->ek, mpsignif(rsapub->n), rsapub->n);
			e = X509rsaverifydigest(c->signature->data, c->signature->len, digest, digestlen, rsapub);
			if(e==nil)
				e = "nil (meaning ok)";
			print("self-signed X509rsaverifydigest returns: %s\n", e);
			rsapubfree(rsapub);
		}
		break;
	case ALG_ecPublicKey:
		ecdominit(&ecdom, namedcurves[c->curve]);
		ecpub = ecdecodepub(&ecdom, c->publickey->data, c->publickey->len);
		if(ecpub != nil){
			e = X509ecdsaverifydigest(c->signature->data, c->signature->len, digest, digestlen, &ecdom, ecpub);
			if(e==nil)
				e = "nil (meaning ok)";
			print("self-signed X509ecdsaverifydigest returns: %s\n", e);
			ecpubfree(ecpub);
		}
		ecdomfree(&ecdom);
		break;
	}
	freecert(c);
	print("end X509dump\n");
}
