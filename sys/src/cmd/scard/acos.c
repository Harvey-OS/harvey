#include <u.h>
#include <libc.h>
#include <libsec.h>
#include <ip.h>
#include "icc.h"
#include "tlv.h"
#include "file.h"
#include "sec.h"

enum{
	DesKLen = 8,
	Des3KLen = 16,
};

/* res and text may be the same or different pointers */
static void
enc(uchar res[8], uchar text[8], uchar *key, uchar klen)
{
	DESstate s;
	DES3state s3;
	uchar  k3[3][8];

	memmove(res, text, 8);	/* ciphering is destructive, save it */
	if(klen == DesKLen){
		setupDESstate(&s, key, nil);
		block_cipher(s.expanded, res, 0);
	}
	else{
		memmove(k3[0], key, 8); /* k3 = k1, keying option 2 for DES3 */
		memmove(k3[1], key+8, 8);
		memmove(k3[2], key, 8);
		setupDES3state(&s3, k3, nil);
		triple_block_cipher(s3.expanded, res, 0);
	}
}


/* put data and others, similar */
static void
calcR(uchar r[8], uchar rand[8], uchar *k, uchar klen)
{
	enc(r, rand, k, klen);
}


static void
calcKsl(uchar ksl[Des3KLen/2], uchar randc[8], uchar *kc, uchar *kt)
{
	enc(ksl, randc, kc, Des3KLen);
	enc(ksl, ksl, kt, Des3KLen);
}

static void
calcKsr(uchar ksr[Des3KLen/2], uchar randt[8], uchar *kt)
{
	uchar revkt[Des3KLen];

	memmove(revkt+Des3KLen/2, kt, Des3KLen/2);
	memmove(revkt, kt+Des3KLen/2, Des3KLen/2);

	enc(ksr, randt, revkt, Des3KLen);
}

static void
calcKslr(uchar *kslr, uchar randc[8], uchar randt[8], uchar *kc, uchar *kt)
{
	int i;

	enc(kslr, randc, kc, DesKLen);
	for(i = 0; i < DesKLen; i++)
		kslr[i] ^= randt[i];
	enc(kslr, kslr, kt, DesKLen);
}


static void
calcKs(uchar *ks, uchar randc[8], uchar randt[8], uchar *kc, uchar *kt, uchar klen)
{

	if(klen == DesKLen){
		calcKslr(ks, randc, randt, kc, kt);
	}
	else{
		calcKsl(ks, randc, kc, kt);
		calcKsr(ks+Des3KLen/2, randt, kt);
	}
}

uchar keys[32][16];

static void
auth(int fd, uchar *ks, uchar kcidx, uchar ktidx, uchar klen)
{
	CmdiccR *cr;
	uchar randc[8];
	uchar *r1, *randt;
	uchar r[16];
	uchar *kc, *kt;

	kc = keys[kcidx];
	kt = keys[ktidx];
	
	cr = getchall(fd, 0x00, 0x00, 0x8);
	if(cr == nil)
		fprint(2, "tagrd: getchall  failed %r\n");
	memmove(randc, cr->dataf, cr->ndataf);
	free(cr);

	r1 = r;
	randt = r+8;
	memmove(r+8, randt, 8);
	genrandom(randt, 8);
	
	calcR(r1, randc, kt, klen);
	calcKs(ks, randc, randt, kc, kt, klen);
	cr = extauth(fd, kcidx, ktidx, 0, r, 16);
	free(cr);
}

