#include "fpi.h"

/*
 * the following routines depend on memory format, not the machine
 */

enum {
	Sign	= 1u << 31,
};

void
fpis2i(Internal *i, void *v)
{
	Single *s = v;

	i->s = (*s & Sign) ? 1: 0;
	if((*s & ~Sign) == 0){
		SetZero(i);
		return;
	}
	i->e = ((*s>>23) & 0x00FF) - SingleExpBias + ExpBias;
	i->h = (*s & 0x007FFFFF)<<(1+NGuardBits);
	i->l = 0;
	if(i->e)
		i->h |= HiddenBit;
	else
		i->e++;
}

void
fpid2i(Internal *i, void *v)
{
	Double *d = v;

	i->s = (d->h & Sign) ? 1: 0;
	i->e = (d->h>>20) & 0x07FF;
	i->h = ((d->h & 0x000FFFFF)<<(4+NGuardBits))|((d->l>>25) & 0x7F);
	i->l = (d->l & 0x01FFFFFF)<<NGuardBits;
	if(i->e)
		i->h |= HiddenBit;
	else
		i->e++;
}

void
fpiw2i(Internal *i, void *v)
{
	Word w, word = *(Word*)v;
	short e;

	if(word < 0){
		i->s = 1;
		word = -word;
	}
	else
		i->s = 0;
	if(word == 0){
		SetZero(i);
		return;
	}
	if(word > 0){
		for (e = 0, w = word; w; w >>= 1, e++)
			;
	} else
		e = 32;
	if(e > FractBits){
		i->h = word>>(e - FractBits);
		i->l = (word & ((1<<(e - FractBits)) - 1))<<(2*FractBits - e);
	}
	else {
		i->h = word<<(FractBits - e);
		i->l = 0;
	}
	i->e = (e - 1) + ExpBias;
}

void
fpiv2i(Internal *i, void *v)
{
	Vlong w, word = *(Vlong*)v;
	short e;

	if(word < 0){
		i->s = 1;
		word = -word;
	}
	else
		i->s = 0;
	if(word == 0){
		SetZero(i);
		return;
	}
	if(word > 0){
		for (e = 0, w = word; w; w >>= 1, e++)
			;
	} else
		e = 64;
	if(e > FractBits){
		i->h = word>>(e - FractBits);
		i->l = (word & ((1<<(e - FractBits)) - 1))<<(2*FractBits - e);
	}
	else {
		i->h = word<<(FractBits - e);
		i->l = 0;
	}
	i->e = (e - 1) + ExpBias;
}

/*
 * Note that all of these conversions from Internal format
 * potentially alter *i, so it should be a disposable copy
 * of the value to be converted.
 */

void
fpii2s(void *v, Internal *i)
{
	short e;
	Single *s = (Single*)v;

	fpiround(i);
	if(i->h & HiddenBit)
		i->h &= ~HiddenBit;
	else
		i->e--;
	*s = i->s ? Sign: 0;
	e = i->e;
	if(e < ExpBias){
		if(e <= (ExpBias - SingleExpBias))
			return;
		e = SingleExpBias - (ExpBias - e);
	}
	else  if(e >= (ExpBias + (SingleExpMax-SingleExpBias))){
		*s |= SingleExpMax<<23;
		return;
	}
	else
		e = SingleExpBias + (e - ExpBias);
	*s |= (e<<23)|(i->h>>(1+NGuardBits));
}

void
fpii2d(void *v, Internal *i)
{
	Double *d = (Double*)v;

	fpiround(i);
	if(i->h & HiddenBit)
		i->h &= ~HiddenBit;
	else
		i->e--;
	i->l = ((i->h & GuardMask)<<25)|(i->l>>NGuardBits);
	i->h >>= NGuardBits;
	d->h = i->s ? Sign: 0;
	d->h |= (i->e<<20)|((i->h & 0x00FFFFFF)>>4);
	d->l = (i->h<<28)|i->l;
}

void
fpii2w(Word *word, Internal *i)
{
	Word w;
	short e;

	fpiround(i);
	e = (i->e - ExpBias) + 1;
	if(e <= 0)
		w = 0;
	else if(e > 31)
		w = 0x7FFFFFFF;
	else if(e > FractBits)
		w = (i->h<<(e - FractBits))|(i->l>>(2*FractBits - e));
	else
		w = i->h>>(FractBits-e);
	if(i->s)
		w = -w;
	*word = w;
}

void
fpii2v(Vlong *word, Internal *i)
{
	Vlong w;
	short e;

	fpiround(i);
	e = (i->e - ExpBias) + 1;
	if(e <= 0)
		w = 0;
	else if(e > 63)
		w = (1ull<<63) - 1;		/* maxlong */
	else if(e > FractBits)
		w = (Vlong)i->h<<(e - FractBits) | i->l>>(2*FractBits - e);
	else
		w = i->h>>(FractBits-e);
	if(i->s)
		w = -w;
	*word = w;
}
