/*
 *	Bounding Box stuff (brucee 04/03/30).
 */

#include <mp.h>
#include <libsec.h>

typedef	struct	BB	BB;
typedef	struct	BBset	BBset;
typedef uchar		Hval[SHA1dlen];

#define	BBEQ(a, b)		(memcmp((a), (b), SHA1dlen) == 0)
#define	BBMKHASH(b, n, h)	sha1((uchar *)(b), (n), (h), nil)
#define	BBCP(d, s)		memmove(d, s, SHA1dlen)

enum
{
	Bpre	= 1 << 0,	/* has a flow in */
	Bjo	= 1 << 1,	/* a jump only */
	Bbig	= 1 << 2,	/* too big */
	Bdel	= 1 << 3,	/* deleted or not of interest */
	Bpin	= 1 << 4,	/* pinned by embedded labels */

	BBHASH	= 64,		/* power of 2 <= 256 */
	BBMASK	= BBHASH - 1,

	BBINIT	= 128,
	BBBIG	= 64,
	BBBSIZE	= 8192,
	BINST	= 128,

	COSTHI	= 0x7F,
	COSTJO	= 0xFF,
};

struct	BB
{
	Reg*	first;
	Reg*	last;
	BBset*	set;
	BB*	link;
	BB*	aux;
	short	flags;
	short	len;
};

struct	BBset
{
	Hval	hash;
	BB*	ents;
	BBset*	next;
	BBset*	link;
	short	index;
	uchar	damage;
	uchar	recalc;
};
