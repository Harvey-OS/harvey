/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Whack		Whack;
typedef struct Unwhack		Unwhack;

enum
{
	WhackStats	= 8,
	WhackErrLen	= 64,		/* max length of error message from thwack or unthwack */
	WhackMaxOff	= 16*1024,	/* max allowed offset */

	HashLog		= 14,
	HashSize	= 1<<HashLog,
	HashMask	= HashSize - 1,

	MinMatch	= 3,		/* shortest match possible */

	MinDecode	= 8,		/* minimum bits to decode a match or lit; >= 8 */

	MaxSeqMask	= 8,		/* number of bits in coding block mask */
	MaxSeqStart	= 256		/* max offset of initial coding block */
};

struct Whack
{
	uint16_t		begin;			/* time of first byte in hash */
	uint16_t		hash[HashSize];
	uint16_t		next[WhackMaxOff];
	unsigned char		*data;
};

struct Unwhack
{
	char		err[WhackErrLen];
};

void	whackinit(Whack*, int level);
void	unwhackinit(Unwhack*);
int	whack(Whack*, unsigned char *dst, unsigned char *src, int nsrc,
		 uint32_t stats[WhackStats]);
int	unwhack(Unwhack*, unsigned char *dst, int ndst, unsigned char *src, int nsrc);

int	whackblock(unsigned char *dst, unsigned char *src, int ssize);
