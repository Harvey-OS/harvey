/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Thwack Thwack;
typedef struct Unthwack Unthwack;
typedef struct ThwBlock ThwBlock;
typedef struct UnthwBlock UnthwBlock;

enum { ThwStats = 8,
       ThwErrLen = 64, /* max length of error message from thwack or unthwack */
       ThwMaxBlock = 1600, /* max size of compressible block */

       HashLog = 12,
       HashSize = 1 << HashLog,
       HashMask = HashSize - 1,

       MinMatch = 3, /* shortest match possible */

       MaxOff = 8,
       OffBase = 6,

       MinDecode = 8, /* minimum bits to decode a match or lit; >= 8 */

       CompBlocks = 10,         /* max blocks used to encode data */
       EWinBlocks = 64,         /* blocks held in encoder window */
       DWinBlocks = EWinBlocks, /* blocks held in decoder window */

       MaxSeqMask = 8,   /* number of bits in coding block mask */
       MaxSeqStart = 256 /* max offset of initial coding block */
};

struct ThwBlock {
	uint32_t seq;    /* sequence number for this data */
	uint8_t acked;   /* ok to use this block; the decoder has it */
	uint16_t begin;  /* time of first byte in hash */
	uint8_t* edata;  /* last byte of valid data */
	uint16_t maxoff; /* time of last valid hash entry */
	uint16_t* hash;
	uint8_t* data;
};

struct Thwack {
	QLock acklock; /* locks slot, blocks[].(acked|seq) */
	int slot;      /* next block to use */
	ThwBlock blocks[EWinBlocks];
	uint16_t hash[EWinBlocks][HashSize];
	Block* data[EWinBlocks];
};

struct UnthwBlock {
	uint32_t seq;    /* sequence number for this data */
	uint16_t maxoff; /* valid data in each block */
	uint8_t* data;
};

struct Unthwack {
	int slot; /* next block to use */
	char err[ThwErrLen];
	UnthwBlock blocks[DWinBlocks];
	uint8_t data[DWinBlocks][ThwMaxBlock];
};

void thwackinit(Thwack*);
void thwackcleanup(Thwack* tw);
void unthwackinit(Unthwack*);
int thwack(Thwack*, int mustadd, uint8_t* dst, int ndst, Block* bsrc,
           uint32_t seq, uint32_t stats[ThwStats]);
void thwackack(Thwack*, uint32_t seq, uint32_t mask);
int unthwack(Unthwack*, uint8_t* dst, int ndst, uint8_t* src, int nsrc,
             uint32_t seq);
uint32_t unthwackstate(Unthwack* ut, uint8_t* mask);
int unthwackadd(Unthwack* ut, uint8_t* src, int nsrc, uint32_t seq);
