#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "thwack.h"

typedef struct Huff	Huff;

enum
{
	MaxFastLen	= 9,
	BigLenCode	= 0x1f4,	/* minimum code for large lenth encoding */
	BigLenBits	= 9,
	BigLenBase	= 4		/* starting items to encode for big lens */
};

enum
{
	StatBytes,
	StatOutBytes,
	StatLits,
	StatMatches,
	StatOffBits,
	StatLenBits,

	StatDelay,
	StatHist,

	MaxStat
};

struct Huff
{
	short	bits;				/* length of the code */
	ulong	encode;				/* the code */
};

static	Huff	lentab[MaxFastLen] =
{
	{2,	0x2},		/* 10 */
	{3,	0x6},		/* 110 */
	{5,	0x1c},		/* 11100 */
	{5,	0x1d},		/* 11101 */
	{6,	0x3c},		/* 111100 */
	{7,	0x7a},		/* 1111010 */
	{7,	0x7b},		/* 1111011 */
	{8,	0xf8},		/* 11111000 */
	{8,	0xf9},		/* 11111001 */
};

void
thwackinit(Thwack *tw)
{
	int i;

	memset(tw, 0, sizeof *tw);
	for(i = 0; i < EWinBlocks; i++){
		tw->blocks[i].data = tw->data[i];
		tw->blocks[i].edata = tw->blocks[i].data;
		tw->blocks[i].hash = tw->hash[i];
		tw->blocks[i].acked = 0;
	}
}

/*
 * acknowledgement for block seq & nearby preds
 */
void
thwackack(Thwack *tw, ulong seq, ulong mask)
{
	int slot, b;

	slot = tw->slot;
	for(;;){
		slot--;
		if(slot < 0)
			slot += EWinBlocks;
		if(slot == tw->slot)
			break;
		if(tw->blocks[slot].seq != seq)
			continue;

		tw->blocks[slot].acked = 1;

		if(mask == 0)
			break;
		do{
			b = mask & 1;
			seq--;
			mask >>= 1;
		}while(!b);
	}
}

/*
 * find a string in the dictionary
 */
static int
thwmatch(ThwBlock *b, ThwBlock *eblocks, uchar **ss, uchar *esrc, ulong h)
{
	int then, toff, w, ok;
	uchar *s, *t;

	s = *ss;
	if(esrc < s + MinMatch)
		return 0;

	toff = 0;
	for(; b < eblocks; b++){
		then = b->hash[(h ^ b->seq) & HashMask];
		toff += b->maxoff;
		w = (ushort)(then - b->begin);

		if(w >= b->maxoff)
			continue;


		/*
		 * don't need to check for the end because
		 * 1) s too close check above
		 * 2) entries too close not added to hash tables
		 */
		t = w + b->data;
		if(s[0] != t[0] || s[1] != t[1] || s[2] != t[2])
			continue;
		ok = b->edata - t;
		if(esrc - s > ok)
			esrc = s + ok;

		t += 3;
		for(s += 3; s < esrc; s++){
			if(*s != *t)
				break;
			t++;
		}
		*ss = s;
		return toff - w;
	}
	return 0;
}

/*
 * knuth vol. 3 multiplicative hashing
 * each byte x chosen according to rules
 * 1/4 < x < 3/10, 1/3 x < < 3/7, 4/7 < x < 2/3, 7/10 < x < 3/4
 * with reasonable spread between the bytes & their complements
 *
 * the 3 byte value appears to be as almost good as the 4 byte value,
 * and might be faster on some machines
 */
/*
#define hashit(c)	(((ulong)(c) * 0x6b43a9) >> (24 - HashLog))
*/
#define hashit(c)	((((ulong)(c) & 0xffffff) * 0x6b43a9b5) >> (32 - HashLog))

/*
 * lz77 compression with single lookup in a hash table for each block
 */
int
thwack(Thwack *tw, uchar *dst, uchar *src, int n, ulong seq, ulong stats[ThwStats])
{
	ThwBlock *eblocks, *b, blocks[CompBlocks];
	uchar *s, *ss, *sss, *esrc, *half, *twdst, *twdmax;
	ulong cont, cseq, bseq, cmask, code, twbits;
	int now, toff, lithist, h, len, slot, bits, use, twnbits, lits, matches, offbits, lenbits, nhist;

	if(n > ThwMaxBlock || n < MinMatch)
		return -1;

	twdst = dst;
	twdmax = dst + n;

	/*
	 * add source to the coding window
	 * there is always enough space
	 */
	slot = tw->slot;
	b = &tw->blocks[slot];
	b->seq = seq;
	b->acked = 0;
	now = b->begin + b->maxoff;
	s = b->data;
	memmove(s, src, n);
	b->edata = s + n;
	b->begin = now;
	b->maxoff = n;

	/*
	 * set up the history blocks
	 */
	cseq = seq;
	cmask = 0;
	*blocks = *b;
	b = blocks;
	b->maxoff = 0;
	b++;
	nhist = 0;
	while(b < blocks + CompBlocks){
		slot--;
		if(slot < 0)
			slot += EWinBlocks;
		if(slot == tw->slot)
			break;
		if(!tw->blocks[slot].acked)
			continue;
		bseq = tw->blocks[slot].seq;
		if(cseq == seq){
			if(seq - bseq >= MaxSeqStart)
				break;
			cseq = bseq;
		}else if(cseq - bseq > MaxSeqMask)
			break;
		else
			cmask |= 1 << (cseq - bseq - 1);
		*b = tw->blocks[slot];
		nhist += b->maxoff;
		b++;
	}
	eblocks = b;
	*twdst++ = seq - cseq;
	*twdst++ = cmask;

	cont = (s[0] << 16) | (s[1] << 8) | s[2];

	esrc = s + n;
	half = s + (n >> 1);
	twnbits = 0;
	twbits = 0;
	lits = 0;
	matches = 0;
	offbits = 0;
	lenbits = 0;
	lithist = ~0;
	while(s < esrc){
		h = hashit(cont);

		sss = s;
		toff = thwmatch(blocks, eblocks, &sss, esrc, h);
		ss = sss;

		len = ss - s;
		for(; twnbits >= 8; twnbits -= 8){
			if(twdst >= twdmax)
				return -1;
			*twdst++ = twbits >> (twnbits - 8);
		}
		if(len < MinMatch){
			toff = *s;
			lithist = (lithist << 1) | (toff < 32) | (toff > 127);
			if(lithist & 0x1e){
				twbits = (twbits << 9) | toff;
				twnbits += 9;
			}else if(lithist & 1){
				toff = (toff + 64) & 0xff;
				if(toff < 96){
					twbits = (twbits << 10) | toff;
					twnbits += 10;
				}else{
					twbits = (twbits << 11) | toff;
					twnbits += 11;
				}
			}else{
				twbits = (twbits << 8) | toff;
				twnbits += 8;
			}
			lits++;
			blocks->maxoff++;

			/*
			 * speed hack
			 * check for compression progress, bail if none achieved
			 */
			if(s > half){
				if(4 * blocks->maxoff < 5 * lits)
					return -1;
				half = esrc;
			}

			if(s + MinMatch <= esrc){
				blocks->hash[(h ^ blocks->seq) & HashMask] = now;
				if(s + MinMatch < esrc)
					cont = (cont << 8) | s[MinMatch];
			}
			now++;
			s++;
			continue;
		}

		blocks->maxoff += len;
		matches++;

		/*
		 * length of match
		 */
		len -= MinMatch;
		if(len < MaxFastLen){
			bits = lentab[len].bits;
			twbits = (twbits << bits) | lentab[len].encode;
			twnbits += bits;
			lenbits += bits;
		}else{
			code = BigLenCode;
			bits = BigLenBits;
			use = BigLenBase;
			len -= MaxFastLen;
			while(len >= use){
				len -= use;
				code = (code + use) << 1;
				use <<= (bits & 1) ^ 1;
				bits++;
			}
			twbits = (twbits << bits) | (code + len);
			twnbits += bits;
			lenbits += bits;

			for(; twnbits >= 8; twnbits -= 8){
				if(twdst >= twdmax)
					return -1;
				*twdst++ = twbits >> (twnbits - 8);
			}
		}

		/*
		 * offset in history
		 */
		toff--;
		for(bits = OffBase; toff >= (1 << bits); bits++)
			;
		if(bits < MaxOff+OffBase-1){
			twbits = (twbits << 3) | (bits - OffBase);
			if(bits != OffBase)
				bits--;
			twnbits += bits + 3;
			offbits += bits + 3;
		}else{
			twbits = (twbits << 4) | 0xe | (bits - (MaxOff+OffBase-1));
			bits--;
			twnbits += bits + 4;
			offbits += bits + 4;
		}
		twbits = (twbits << bits) | toff & ((1 << bits) - 1);

		for(; s != ss; s++){
			if(s + MinMatch <= esrc){
				h = hashit(cont);
				blocks->hash[(h ^ blocks->seq) & HashMask] = now;
				if(s + MinMatch < esrc)
					cont = (cont << 8) | s[MinMatch];
			}
			now++;
		}
	}


	if(twnbits & 7){
		twbits <<= 8 - (twnbits & 7);
		twnbits += 8 - (twnbits & 7);
	}
	for(; twnbits >= 8; twnbits -= 8){
		if(twdst >= twdmax)
			return -1;
		*twdst++ = twbits >> (twnbits - 8);
	}

	tw->slot++;
	if(tw->slot >= EWinBlocks)
		tw->slot = 0;

	stats[StatBytes] += blocks->maxoff;
	stats[StatLits] += lits;
	stats[StatMatches] += matches;
	stats[StatOffBits] += offbits;
	stats[StatLenBits] += lenbits;
	stats[StatDelay] = stats[StatDelay]*7/8 + dst[0];
	stats[StatHist] = stats[StatHist]*7/8 + nhist;
	stats[StatOutBytes] += twdst - dst;

	return twdst - dst;
}
