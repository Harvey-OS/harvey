#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include "ppp.h"
#include "thwack.h"

enum
{
	DMaxFastLen	= 7,
	DBigLenCode	= 0x3c,		/* minimum code for large lenth encoding */
	DBigLenBits	= 6,
	DBigLenBase	= 1		/* starting items to encode for big lens */
};

static uchar lenval[1 << (DBigLenBits - 1)] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4,
	5,
	6,
	255,
	255
};

static uchar lenbits[] =
{
	0, 0, 0,
	2, 3, 5, 5,
};

static uchar offbits[16] =
{
	5, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 12, 13
};

static ushort offbase[16] =
{
	0, 0x20,
	0x40, 0x60,
	0x80, 0xc0,
	0x100, 0x180,
	0x200, 0x300,
	0x400, 0x600,
	0x800, 0xc00,
	0x1000,
	0x2000
};

void
unthwackinit(Unthwack *ut)
{
	int i;

	memset(ut, 0, sizeof *ut);
	for(i = 0; i < DWinBlocks; i++)
		ut->blocks[i].data = ut->data[i];
}

ulong
unthwackstate(Unthwack *ut, uchar *mask)
{
	ulong bseq, seq;
	int slot, m;

	seq = ~0UL;
	m = 0;
	slot = ut->slot;
	for(;;){
		slot--;
		if(slot < 0)
			slot += DWinBlocks;
		if(slot == ut->slot)
			break;
		if(ut->blocks[slot].maxoff == 0)
			continue;
		bseq = ut->blocks[slot].seq;
		if(seq == ~0UL)
			seq = bseq;
		else if(seq - bseq > MaxSeqMask)
			break;
		else
			m |= 1 << (seq - bseq - 1);
	}
	*mask = m;
	return seq;
}

/*
 * insert this block in it's correct sequence number order.
 * replace the oldest block, which is always pointed to by ut->slot.
 * the encoder doesn't use a history at wraparound,
 * so don't worry about that case.
 */
static int
unthwackinsert(Unthwack *ut, int len, ulong seq)
{
	uchar *d;
	int slot, tslot;

	tslot = ut->slot;
	for(;;){
		slot = tslot - 1;
		if(slot < 0)
			slot += DWinBlocks;
		if(ut->blocks[slot].seq <= seq || ut->blocks[slot].maxoff == 0)
			break;
		d = ut->blocks[tslot].data;
		ut->blocks[tslot] = ut->blocks[slot];
		ut->blocks[slot].data = d;
		tslot = slot;
	}
	ut->blocks[tslot].seq = seq;
	ut->blocks[tslot].maxoff = len;

	ut->slot++;
	if(ut->slot >= DWinBlocks)
		ut->slot = 0;

	ut->blocks[ut->slot].seq = ~0UL;
	ut->blocks[ut->slot].maxoff = 0;

	return tslot;
}

int
unthwackadd(Unthwack *ut, uchar *src, int nsrc, ulong seq)
{
	int tslot;

	if(nsrc > ThwMaxBlock)
		return -1;

	tslot = unthwackinsert(ut, nsrc, seq);
	if(tslot < 0)
		return -1;
	
	memmove(ut->blocks[tslot].data, src, nsrc);

	return nsrc;
}

int
unthwack(Unthwack *ut, uchar *dst, int ndst, uchar *src, int nsrc, ulong seq)
{
	UnthwBlock blocks[CompBlocks], *b, *eblocks;
	uchar *s, *d, *dmax, *smax, lit;
	ulong cmask, cseq, bseq, utbits;
	int i, off, len, bits, slot, use, code, utnbits, overbits, lithist;

	if(nsrc < 4 || nsrc > ThwMaxBlock){
		snprint(ut->err, ThwErrLen, "block too small or large");
		return -1;
	}

	slot = ut->slot;
	b = blocks;
	*b = ut->blocks[slot];
	d = b->data;
	dmax = d + ndst;

	/*
	 * set up the history blocks
	 */
	cseq = seq - src[0];
	cmask = src[1];
	b++;
	while(cseq != seq && b < blocks + CompBlocks){
		slot--;
		if(slot < 0)
			slot += DWinBlocks;
		if(slot == ut->slot)
			break;
		bseq = ut->blocks[slot].seq;
		if(bseq == cseq){
			*b = ut->blocks[slot];
			b++;
			if(cmask == 0){
				cseq = seq;
				break;
			}
			do{
				bits = cmask & 1;
				cseq--;
				cmask >>= 1;
			}while(!bits);
		}
	}
	eblocks = b;
	if(cseq != seq){
		snprint(ut->err, ThwErrLen, "blocks dropped: seq=%ld cseq=%ld %d cmask=%#lx %#x\n", seq, cseq, src[0], cmask, src[1]);
		return -2;
	}

	smax = src + nsrc;
	src += 2;
	utnbits = 0;
	utbits = 0;
	overbits = 0;
	lithist = ~0;
	while(src < smax || utnbits - overbits >= MinDecode){
		while(utnbits <= 24){
			utbits <<= 8;
			if(src < smax)
				utbits |= *src++;
			else
				overbits += 8;
			utnbits += 8;
		}

		/*
		 * literal
		 */
		len = lenval[(utbits >> (utnbits - 5)) & 0x1f];
		if(len == 0){
			if(lithist & 0xf){
				utnbits -= 9;
				lit = (utbits >> utnbits) & 0xff;
				lit &= 255;
			}else{
				utnbits -= 8;
				lit = (utbits >> utnbits) & 0x7f;
				if(lit < 32){
					if(lit < 24){
						utnbits -= 2;
						lit = (lit << 2) | ((utbits >> utnbits) & 3);
					}else{
						utnbits -= 3;
						lit = (lit << 3) | ((utbits >> utnbits) & 7);
					}
					lit = (lit - 64) & 0xff;
				}
			}
			if(d >= dmax){
				snprint(ut->err, ThwErrLen, "too much output");
				return -1;
			}
			*d++ = lit;
			lithist = (lithist << 1) | (lit < 32) | (lit > 127);
			blocks->maxoff++;
			continue;
		}

		/*
		 * length
		 */
		if(len < 255)
			utnbits -= lenbits[len];
		else{
			utnbits -= DBigLenBits;
			code = ((utbits >> utnbits) & ((1 << DBigLenBits) - 1)) - DBigLenCode;
			len = DMaxFastLen;
			use = DBigLenBase;
			bits = (DBigLenBits & 1) ^ 1;
			while(code >= use){
				len += use;
				code -= use;
				code <<= 1;
				utnbits--;
				if(utnbits < 0){
					snprint(ut->err, ThwErrLen, "len out of range");
					return -1;
				}
				code |= (utbits >> utnbits) & 1;
				use <<= bits;
				bits ^= 1;
			}
			len += code;

			while(utnbits <= 24){
				utbits <<= 8;
				if(src < smax)
					utbits |= *src++;
				else
					overbits += 8;
				utnbits += 8;
			}
		}

		/*
		 * offset
		 */
		utnbits -= 4;
		bits = (utbits >> utnbits) & 0xf;
		off = offbase[bits];
		bits = offbits[bits];

		utnbits -= bits;
		off |= (utbits >> utnbits) & ((1 << bits) - 1);
		off++;

		b = blocks;
		while(off > b->maxoff){
			off -= b->maxoff;
			b++;
			if(b >= eblocks){
				snprint(ut->err, ThwErrLen, "offset out of range");
				return -1;
			}
		}
		if(d + len > dmax
		|| b != blocks && len > off){
			snprint(ut->err, ThwErrLen, "len out of range");
			return -1;
		}
		s = b->data + b->maxoff - off;
		blocks->maxoff += len;

		for(i = 0; i < len; i++)
			d[i] = s[i];
		d += len;
	}
	if(utnbits < overbits){
		snprint(ut->err, ThwErrLen, "compressed data overrun");
		return -1;
	}

	len = d - blocks->data;
	memmove(dst, blocks->data, len);

	unthwackinsert(ut, len, seq);

	return len;
}
