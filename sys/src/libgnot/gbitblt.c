#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	"/sys/include/gnot.h"
/*
 * Compiling bitblt - for each call of gbitblt, generate
 * machine code for the specific arguments passed in,
 * then execute that code as a subroutine call.
 *
 * The files bb?.h (where ? is replaced by the
 * architectures single-letter code: v, k, etc., depending
 * on which of Tmips, Tsparc, etc., is defined) define macros
 * which define operations on an abstract machine.
 * If there is no architecture-specific file, bbc.h is used,
 * which interprets a program with the same operations as
 * the assumed abstract machine.  See bbc.h for a description
 * of the machine's registers and operations, and for the
 * macros that must be defined to make a new bb?.h
 *
 * The code will work for bitblts to and from any bitmaps
 * with ldepths 0, 1, 2, or 3.  Some of those conversions
 * may be #ifdef'd out, because no Plan 9 code currently
 * uses them, and the tables take up space.  The converting
 * bitblts will need work if they are to work with ldepths > 3.
 *
 * This file also contains a thorough bitblt tester.
 * When TEST is defined, a main program is created to
 * try forward and backward cases for all bitblt opcodes,
 * first on single pixels, and then on parts of the middle two rows
 * of random 4-row bitmaps.  If anything fails, it prints information
 * about the case that failed (or perhaps it will just die,
 * if bad code has been generated).
 * The testing programs takes as arguments two numbers: the source
 * and destination ldepths (both 0 by default).  A -s flag says to
 * use simpler, repeatable tests.  A -i num flag says how many
 * iterations to do for each opcode.
 */

/* Bitblt cases:
 * - bitmap overlap sometimes dictates that you go forward (f) through
 *   the bitmaps, sometimes backwards (b)
 * - different relative alignments of the source and destination
 *   starting points within a word require different code:
 *   the bit offsets may be the same (e), the source may start
 *   later in the word (g), or the source may start earlier in the word (l)
 * - when a row of the destination is all within one word (o), better
 *   code can be generated
 * - when the object machine has bitfield extraction/insertion instructions,
 *   it is better to do < 32bit wide bitblts using them (bf)
 * - if the source and destination bitmaps have different depths,
 *   it is either and expansion (exp) or a contraction (con) of pixels.
 *   These two cases aren't further differentiated into f vs. b, etc.
 */

/*
 * To calculate the potential size of the bitblt program, use the following
 * formulas:
 *  nonconverting max (bshg): X + 10L + 6S + 2F + 3E + 8LX + 3SX
 *  converting max (contracting by factor of 8, 32-bit memory accesses):
 *    X + XT + 46L + 48S + 96T + 64A + 32AX + 2F + 3E + 30LX + 48SX
 *
 * where X = Initsd+Extrainit+Iloop+Oloop+Rts  ; XT = Inittab
 *       E = Emitop  ;  F = Field
 *       L = load, fetch or store ; LX = extra if pre or post decrement
 *       S = shift (sha or shb)   ; SX = extra if OR too
 *       T = Table  ;  A = Assemble  ; AX = Assemblex
 */
enum
{
	Tfshe	= 0,	/* each of the triples must be in order e, l, g */
	Tfshl,
	Tfshg,

	Tbshe,
	Tbshl,
	Tbshg,

	Toshe,
	Toshl,
	Toshg,

	Tobf,

	Texp,

	Tcon,

	Tlast,		/* total number of cases */
};

#ifdef TEST
/*
 * globals used for testing
 */
int	FORCEFORW;
int	FORCEBAKW;
GBitmap	*curdm, *cursm;
Point	curpt;
Rectangle curr;
Fcode	curf;
void	*mem;
#endif

extern void	*bbmalloc(int);
extern void	bbfree(void *, int);
extern int	bbonstack(void);

/*
 * set up to compile -DT$objtype
 */
#ifdef	Tmips
#include	"bbv.h"
#else
#ifdef	T68020
#include	"bb2.h"
#else
#ifdef	Tsparc
#include	"bbk.h"
#else
#ifdef	T386
#include	"bb8l.h"
#else
#ifdef	Thobbit
#include	"bbcl.h"
#else
#include	"bbc.h"
#endif
#endif
#endif
#endif
#endif

/*
 * bitblt operates a 'word' at a time.
 * WBITS is the number of bits in a word
 * LWBITS=log2(WBITS),
 * W2L is the number of words in a long
 * WMASK has bits set for the low order word of a long
 * WType is a pointer to a word
 * if LENDIAN is defined, then left-to-right in bitmap
 * means low-order-bit to high-order-bit within a word,
 * otherwise it is high-order-bit to low-order-bit.
 */
#ifndef WBITS
#define WBITS	32
#define LWBITS	5
#define	W2L	1
#define WMASK	~0UL
typedef ulong	*WType;
#endif
/*
 * scrshl(v,o) shifts a word v by o bits screen-leftward
 * scrshr(v,o) shifts a word v by o bits screen-rightward
 * scrpix(v,i,l) gets the value of pixel i within word v when ldepth is l
 * scrmask(i,l) has ones for pixel i when ldepth is l
 */
#ifdef LENDIAN
#define scrshl(v,o)	((v)>>(o))
#define scrshr(v,o)	((v)<<(o))
#define scrpix(v,i,l)	(((v)>>((i)<<(l)))&((1<<(1<<(l)))-1))
#define scrmask(i,l)	(((1<<(1<<(l)))-1)<<((i)<<(l)))
#else
#define scrshl(v,o)	((v)<<(o))
#define scrshr(v,o)	((v)>>(o))
#define scrpix(v,i,l)	(((v)>>(32-(((i)+1)<<(l))))&((1<<(1<<(l)))-1))
#define scrmask(i,l)	(((1<<(1<<(l)))-1)<<(32-(((i)+1)<<l)))
#endif

void
gbitblt(GBitmap *dm, Point pt, GBitmap *sm, Rectangle r, Fcode fcode)
{
	int	type;		/* category of bitblt: Tfshe or ... */
	int	width;		/* width in bits of dst */
	int	height;		/* height in pixels minus 1 */
	int	sh;		/* left shift of src to align with dst */
	int	soff;		/* bit offset of src start point */
	int	doff;		/* bit offset of dst start point */
	int	sc;		/* src words used so far */
	int 	dc;		/* dst words used so far */
	int	le;		/* log expansion factor */
	int	sspan;		/* words between scanlines in src */
	int	dspan;		/* words between scanlines in dst */
	int	sdep;		/* src ldepth */
	int 	ddep;		/* dst ldepth */
	int	onstack;	/* compiling to stack arena */
	int	backward;	/* does bitblt have to go backwards? */
	ulong*	saddr;		/* addr of word in src containing start point */
	ulong*	daddr;		/* addr of word in dst containing start point */
	ulong	lmask;		/* affected pixels in leftmost dst word */
	ulong	rmask;		/* affected pixels in rightmost dst word */
	Type*	lo;		/* addr in program for beginning of outer loop */
	Type*	li;		/* addr in program for beginning of inner loop */
	uchar	*tab;		/* conversion table */
	int	osiz;		/* size of table entries, in bytes */
	Type*	memstart;	/* start of program */
	Type*	p;		/* next free address in program */
	Type*	fi;		/* pointer to beginning of instrs for Rs f= Rd */
	int	fin;		/* number of Types to copy after fi */
	long	v;		/* for use in Emitop macro */
	int	c;		/* a count */
	int	fs;		/* if need to fetch source */
	int	fd;		/* if need to fetch dest */
	int	b;		/* for expansion: take b bits at a time from src */
	int	db;		/* number of bits yielded by table lookup */
	int	dl;		/* log2 number of bytes yielded by table lookup */
	int	sf;		/* a bit offset in src */
	int 	df;		/* a bit offset in dst */
	int	firstd;		/* doing first part of dst ? */
	int	firsts;		/* doing first part of src ? */
	int	f;		/* int version of fcode */
	long	tmp;		/* for use by some macros */
	int	sha;		/* |sh|%32 */
	int	shb;		/* WBITS-sha, if sha!=0 */
	int	sfo;		/* bit offset origin (needed if WBITS==8) */
	Fstr	*pf;
	Type	arena[Progmaxnoconv];	/* for non-converting bitblts */

	onstack = bbonstack();
	gbitbltclip(&dm);

#ifdef TEST
	curdm = dm;
	cursm = sm;
	curpt = pt;
	curr = r;
	curf = fcode;
#endif

	width = r.max.x - r.min.x;
	if(width <= 0)
		return;
	height = r.max.y - r.min.y - 1;
	if(height < 0)
		return;
	ddep = dm->ldepth;
	pt.x <<= ddep;
	width <<= ddep;

	sdep = sm->ldepth;
	r.min.x <<= sdep;
	r.max.x <<= sdep;

	dspan = dm->width * W2L;
	sspan = sm->width * W2L;

	daddr = (ulong*)((WType)dm->base
			+ dm->zero*W2L + pt.y*dspan
			+ (pt.x >> LWBITS));
	saddr = (ulong*)((WType)sm->base
			+ sm->zero*W2L + r.min.y*sspan
			+ (r.min.x >> LWBITS));

	c = doff = pt.x & (WBITS-1);
	soff = r.min.x & (WBITS-1);

	pf = &fstr[(f = fcode&0xF)];
	fs = pf->fetchs;
	fd = pf->fetchd;
	fin = pf->n;
	fi = (Type *)pf->instr;

	if(ddep == sdep || !fs) {
#ifdef TEST
		if(!FORCEBAKW &&
		   (FORCEFORW || sm != dm || !fs || saddr > daddr ||
		    (saddr == daddr && soff > doff)))
			backward = 0;
		else
			backward = 1;
#else
		if(sm != dm || !fs || saddr > daddr ||
		  (saddr == daddr && soff > doff))
			backward = 0;
		else
			backward = 1;
#endif
#ifdef HAVEBF
		if(width <= WBITS) {
			sh = 0;
			type = Tobf;
			if(backward) {
				daddr = (ulong *)((WType)daddr + height*dspan);
				saddr = (ulong *)((WType)saddr + height*sspan);
			}
			goto init;
		}
#else
		if(doff+width <= WBITS) {
			type = Toshe;
			if(backward) {
				daddr = (ulong *)((WType)daddr + height*dspan);
				saddr = (ulong *)((WType)saddr + height*sspan);
			}
		}
#endif
		else {
			if(!backward)
				type = Tfshe;
			else {
				type = Tbshe;
				doff = (WBITS-(doff+width)) & (WBITS-1);
				soff = (WBITS-(soff+width)) & (WBITS-1);
				daddr = (ulong*)((WType)dm->base
					+ dm->zero*W2L + (pt.y+height)*dspan
					+ ((pt.x + width+(WBITS-1))>>LWBITS));
				saddr = (ulong*)((WType)sm->base
					+ sm->zero*W2L + (r.max.y-1)*sspan
					+ ((r.max.x + (WBITS-1))>>LWBITS));
			}
		}
		if(fs) {
			if((sh = soff - doff) != 0) {
				if(sh < 0)
					type += Tbshl-Tbshe;
				else
					type += Tbshg-Tbshe;
			}
		} else
			sh = 0;
	} else {
		if(sdep < 0 || sdep > 3 ||
		   ddep < 0 || ddep > 3 ||
		   (tab = tabs[sdep][ddep]) == 0)
			return;	/* sorry, conversion not enabled */
		
		osiz = tabosiz[sdep][ddep];
		le = ddep - sdep;
		if(le > 0) {
			type = Texp;
			sh = soff - (doff >> le);
		} else {
			type = Tcon;
			sh = soff - (doff << -le);
		}
		onstack = 0;
		backward = 0;
	}

	/* c has original doff (relative to beginning) */
	lmask = scrshr(WMASK,c);
	rmask = scrshl(WMASK,(WBITS - ((c+width) & (WBITS-1))))&WMASK;
	if(!rmask)
		rmask = WMASK;
	if(sh != 0) {
		if(sh > 0)
			sha = sh;
		else
			sha = (-sh)&(WBITS-1);
		shb = WBITS - sha;
	}

	/* init: set up constant regs and outer loop */
    init:
	if(onstack)
		memstart = arena;
	else
		memstart = (Type*)bbmalloc(Progmax * sizeof(Type));
	p = memstart;
	Initsd(saddr,daddr);
	if(sh) {
		Initsh(sha,shb);
	}
	Extrainit;

	if(height > 0) {
		Olabel(height+1);
		lo = p;
	}
	sc = 0;
	dc = 0;

	/* emit inner loop */
	switch(type){
#ifdef HAVEBF
	case Tobf:
		if(fd) {
			Bfextu_RdAd(doff,width);
		}
		if(fs) {
			Bfextu_RsAs(soff,width);
		}
		Emitop;
		Bfins_AdRs(doff,width);
		break;
#else
	case Toshe:
		/* one word dest, src and dest offsets same (or src not involved) */
		lmask &= rmask;
		if(fs) {
			Load_Rs(0);
		}
		Fetch_Rd(1);
		Emitop;
		Ofield(lmask);
		Store_Rs;
		break;

	case Toshl:
		/* one word dest, src offset less than dest offset */
		lmask &= rmask;
		Loadzx_Rt(0);
		Fetch_Rd(0);
		Orsha_RsRt;
		Emitop;
		Ofield(lmask);
		Store_Rs;
		break;

	case Toshg:
		/* one word dest, src offset greater than dest offset */
		lmask &= rmask;
		if(sha+doff+width > WBITS) {
			Load_Rt_P;
			Olsha_RsRt;
			Loadzx_Rt(0);
			Fetch_Rd(0);
			Oorrshb_RsRt;
			if(backward)
				sc--;
			else
				sc++;
		} else {
			Load_Rt(0);
			Fetch_Rd(0);
			Olsha_RsRt;
		}
		Emitop;
		Ofield(lmask);
		Store_Rs;
		break;
#endif /* HAVEBF */

	case Tfshe:
		/* forward, src and dest offsets same (or src not involved) */
		Fetch_Rd(0);
		if(fs) {
			Load_Rs_P;
			sc++;
		} else {
			Nop;
		}
		Emitop;
		Ofield(lmask);
		Store_Rs_P;
		dc++;
		width -= WBITS - doff;
	
		c = width >> LWBITS;
		if(c) {
			if(f == Zero || f == F) {
				/* set up Rs outside loop */
				Emitop;
			}
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			if(fd) {
				Fetch_Rd(!fs);
			}
			if(fs) {
				Load_Rs_P;
				sc += c;
			}
			if(!(f == Zero || f == F)) {
				Emitop;
			}
			Store_Rs_P;
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		if(width & (WBITS-1)) {
			if(fs) {
				Load_Rs(0);
			}
			Fetch_Rd(1);
			Emitop;
			Ofield(rmask);
			Store_Rs;
		}
		break;

	case Tfshl:
		/* forward, src offset less than dest offset */
		Loadzx_Rt_P;
		Fetch_Rd(0);
		Orsha_RsRt;
		sc++;
		Emitop;
		Ofield(lmask);
		Store_Rs_P;
		dc++;
		width -= WBITS - doff;
	
		c = width >> LWBITS;
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			Olshb_RsRt;
			Loadzx_Rt_P;
			if(fd) {
				Fetch_Rd(0);
			}
			Oorrsha_RsRt;
			sc += c;
			Emitop;
			Store_Rs_P;
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		width &= (WBITS-1);
		if(width) {
			Olshb_RsRt;
			if(width > sha) {
				Loadzx_Rt(0);
				Fetch_Rd(0);
				Oorrsha_RsRt;
			} else {
				Fetch_Rd(1);
			}
			Emitop;
			Ofield(rmask);
			Store_Rs;
		}
		break;

	case Tfshg:
		/* forward, src offset greater than dest offset */
		Load_Rt_P;
		Olsha_RsRt;
		Loadzx_Rt_P;
		Fetch_Rd(0);
		Oorrshb_RsRt;
		sc += 2;
		Emitop;
		Ofield(lmask);
		Store_Rs_P;
		dc++;
		width -= WBITS - doff;
	
		c = width >> LWBITS;
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			Olsha_RsRt;
			Loadzx_Rt_P;
			if(fd) {
				Fetch_Rd(0);
			}
			Oorrshb_RsRt;
			sc += c;
			Emitop;
			Store_Rs_P;
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		width &= WBITS-1;
		if(width) {
			Olsha_RsRt;
			if(width > shb) {
				Loadzx_Rt(0);
				Fetch_Rd(0);
				Oorrshb_RsRt;
			} else {
				Fetch_Rd(1);
			}
			Emitop;
			Ofield(rmask);
			Store_Rs;
		}
		break;

	case Tbshe:
		/* backward, src and dest offsets same (or src not involved) */
		Load_Rs_D(0);
		sc++;
		Fetch_Rd_D(1);
		Emitop;
		Ofield(rmask);
		Store_Rs;
		dc++;
		width -= WBITS - doff;
	
		c = width >> LWBITS;
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			Load_Rs_D(0);
			sc += c;
			if(fd) {
				Fetch_Rd_D(1);
				Emitop;
				Store_Rs;
			} else {
				Nop;
				Emitop;
				Store_Rs_D;
			}
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		if(width & (WBITS-1)) {
			Load_Rs_D(0);
			sc++;
			Fetch_Rd_D(1);
			dc++;
			Emitop;
			Ofield(lmask);
			Store_Rs;
		}
		break;

	case Tbshl:
		/* backward, src offset less than dest offset */
		Loadzx_Rt_D(0);
		Fetch_Rd_D(0);
		Olsha_RsRt;
		sc++;
		Emitop;
		Ofield(rmask);
		Store_Rs;
		dc++;
		width -= WBITS - doff;
	
		c = width >> LWBITS;
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			Orshb_RsRt;
			Loadzx_Rt_D(0);
			if(fd) {
				Fetch_Rd_D(0);
			} else {
				Nop;
			}
			Oorlsha_RsRt;
			sc += c;
			Emitop;
			if(fd) {
				Store_Rs;
			} else {
				Store_Rs_D;
			}
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		width &= (WBITS-1);
		if(width) {
			Orshb_RsRt;
			if(width > sha) {
				Load_Rt_D(0);
				Fetch_Rd_D(0);
				Oorlsha_RsRt;
				sc++;
			} else {
				Fetch_Rd_D(1);
			}
			dc++;
			Emitop;
			Ofield(lmask);
			Store_Rs;
		}
		break;

	case Tbshg:
		/* backward, src offset greater than dest offset */
		Loadzx_Rt_D(0);
		Fetch_Rd_D(0);
		Orsha_RsRt;
		Loadzx_Rt_D(1);
		Oorlshb_RsRt;
		sc += 2;
		Emitop;
		Ofield(rmask);
		Store_Rs;
		dc++;
		width -= WBITS - doff;
	
		c = width >> LWBITS;
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			Orsha_RsRt;
			Loadzx_Rt_D(0);
			if(fd) {
				Fetch_Rd_D(0);
			} else {
				Nop;
			}
			Oorlshb_RsRt;
			sc += c;
			Emitop;
			if(fd) {
				Store_Rs;
			} else {
				Store_Rs_D;
			}
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		width &= WBITS-1;
		if(width) {
			Orsha_RsRt;
			if(width > shb) {
				Loadzx_Rt_D(0);
				Fetch_Rd_D(0);
				sc++;
			} else {
				Fetch_Rd_D(1);
			}
			dc++;
			Oorlshb_RsRt;
			Emitop;
			Ofield(lmask);
			Store_Rs;
		}
		break;

	case Texp:
		/* expansion: dest ldepth > src ldepth */
		if(WBITS == 8) {
			b = 8 >> le;
			/* db == 8, dl == 0 */
		} else {
			b = (le <= 2) ? 8 : 32 / (1 << le);
			db = b << le;
			dl = (le <= 2) ? le : 2;
		}
		Inittab(tab,osiz);
	
		/*
		 * method:
		 * load the source a word at a time, into Rt;
		 * (if there is a shift, use Ru to hold next or last partial word,
		 *  or, if WBITS == 8, it is  <<8 in Rt)
		 * take b bits at a time from source and convert via table into Rd
		 * (each table lookup yields db bits, in 1<<dl bytes);
		 * assemble into Rs until have WBITS bits;
		 * fetch dest word into Rd, operate into Rs, store in dest
		 *
		 * this code needs reworking for expansion factor > 8
		 */

		if(WBITS == 8) {
			if(sh == 0) {
				Load_Rt_P;
				sfo = 24;
			} else if(sh > 0) {
				Load_Rt_P;
				Olsh_RtRt(8);
				if((doff+width)>>le  > shb) {
					Loador_Rt_P;
					sc++;
				}
				/* relevant source bits: Rt[16+sh..23+sh] */
				sfo = 16 + sh;
			} else {
				Load_Rt_P;
				/* relevant source bits: Rt[16+(8+sh)..23+(8+sh) */
				sfo = 24 + sh;
			}
			sf = 0;
			firstd = 1;
		} else {
			if(sh == 0) {
				Load_Rt_P;
			} else if(sh > 0) {
				Load_Rt_P;
				
				Olsha_RtRt;
				if((doff+width)>>le > shb) {
					Load_Ru_P;
					Oorrshb_RtRu;
					sc++;
				}
			} else {
				Load_Ru_P;
				Orsha_RtRu;
			}
			sf = (soff - sh) & ~(b-1);
			firstd = 1;
			firsts = 1;
		}
		sc++;
		while(sf < WBITS && width > 0) {
			if(WBITS == 8) {
				Table_RsRt(sf+sfo,b,0);
				sf += b;
			} else {
				if(firstd)
					df = (sf << le) & (WBITS-1);
				else
					df = 0;
				while(df < WBITS) {
					Table_RdRt(sf,b,dl);
					if(df==0 || firsts) {
						c = WBITS - (df + db);
						Olsh_RsRd(c);
					} else if(df == WBITS - db) {
						Oor_RsRd;
					} else {
						c = WBITS - (df + db);
						Oorlsh_RsRd(c);
					}
					sf += b;
					df += db;
					firsts = 0;
				}
			}
			Fetch_Rd(1);
			Emitop;
			if(firstd) {
				width -= WBITS - doff;
				if(width > 0 && lmask != WMASK) {
					Ofield(lmask);
				} else if(width <= 0) {
					lmask &= rmask;
					Ofield(lmask);
				}
			} else {
				width -= WBITS;
				if(width < 0) {
					Ofield(rmask);
				}
			}
			Store_Rs_P;
			dc++;
			firstd = 0;
		}
		if(width <= 0)
			break;
	
		c = width >> (LWBITS+le);
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			if(WBITS == 8) {
				Olsh_RtRt(8);
				Loador_Rt_P;
			} else {
				if(sh == 0) {
					Load_Rt_P;
				} else if(sh > 0) {
					Olsha_RtRu;
					Load_Ru_P;
					Oorrshb_RtRu;
				} else {
					Olshb_RtRu;
					Load_Ru_P;
					Oorrsha_RtRu;
				}
			}
			sc += c;
			for(sf = 0; sf < WBITS;) {
				if(WBITS == 8) {
					Table_RsRt(sf+sfo,b,0);
					sf += b;
				} else {
					for(df = 0; df < WBITS;) {
						Table_RdRt(sf,b,dl);
						Assemblex(df,db);
						sf += b;
						df += db;
					}
				}
				if(fd) {
					Fetch_Rd(1);
				}
				Emitop;
				Store_Rs_P;
				dc += c;
			}
			if(c > 1) {
				Iloop(li);
			}
		}
		width -= c << (LWBITS+le);
		if(width <= 0)
			break;

		if(WBITS == 8) {
			if(sh == 0) {
				Load_Rt_P;
				sc++;
			} else if(sh > 0) {
				Olsh_RtRt(8);
				if(width>>le  > shb) {
					Loador_Rt_P;
					sc++;
				}
			} else {
				Olsh_RtRt(8);
				if(width>>le > sha) {
					Loador_Rt_P;
					sc++;
				}
			}
		} else {
			if(sh == 0) {
				Load_Rt_P;
				sc++;
			} else if(sh > 0) {
				Olsha_RtRu;
				if(width>>le  > shb) {
					Load_Ru_P;
					Oorrshb_RtRu;
					sc++;
				}
			} else {
				Olshb_RtRu;
				if(width>>le > sha) {
					Load_Ru_P;
					Oorrsha_RtRu;
					sc++;
				}
			}
		}
		for(sf = 0; sf < WBITS && width > 0; ) {
			if(WBITS == 8) {
				Table_RsRt(sf+sfo,b,0);
				sf += b;
			} else {
				for(df = 0; df < WBITS;) {
					Table_RdRt(sf,b,dl);
					Assemblex(df,db);
					sf += b;
					df += db;
				}
			}
			Fetch_Rd(1);
			Emitop;
			width -= WBITS;
			if(width < 0) {
				Ofield(rmask);
			}
			Store_Rs_P;
			dc++;
		}
		break;

	case Tcon:
		/* contraction: dest ldepth < src ldepth */
		db = 8 >> -le;
		Inittab(tab,osiz);
	
		/*
		 * method:
		 * load the source a word at a time, into Rt;
		 * (if there is a shift, use Ru to hold next or last partial word,
		 * or, if WBITS==8, it is <<8 in Rt)
		 * take 8 bits at a time from source and convert via table into Rd
		 * (each table lookup yields db bits, in 1 byte);
		 * assemble into Rs until have WBITS bits (takes several src words);
		 * fetch dest word into Rd, operate into Rs, store in dest
		 *
		 * Something should be done to improve this code, but
		 * it isn't used much.
		 */

		if(sh < 0) {
			c = (-sh)/WBITS;
			sh += c*WBITS;
			if(WBITS == 8) sfo = 24 + sh;
		} else {
			c = 0;
			if(WBITS == 8) sfo = sh ? 16 + sh : 24;
		}
		firstd = 1;
		firsts = 1;
		for(df = c*db*(4/W2L); df < WBITS && df < doff + width; ) {
			c = (doff + width - df) << -le;
			/* c = number of source bits needed to fill rest */
			if(WBITS == 8) {
				if(sh == 0) {
					Load_Rt_P;
					sc++;
				} else if(sh > 0) {
					if(firsts) {
						Load_Rt_P;
						Olsh_RtRt(8);
						sc++;
					} else {
						Olsh_RtRt(8);
					}
					if(shb < c) {
						Loador_Rt_P;
						sc++;
					}
				} else {
					if(firsts) {
						Load_Rt_P;
						sc++;
					} else {
						Olsh_RtRt(8);
						if(sha < c) {
							Loador_Rt_P;
							sc++;
						}
					}
				}
			} else {
				if(sh == 0) {
					Load_Rt_P;
					sc++;
				} else if(sh > 0) {
					if(firsts) {
						Load_Rt_P;
						Olsha_RtRt;
						sc++;
					} else {
						Olsha_RtRu;
					}
					if(shb < c) {
						Load_Ru_P;
						sc++;
						Oorrshb_RtRu;
					}
				} else {
					if(!firsts) {
						Olshb_RtRu;
					}
					if(sha < c) {
						Load_Ru_P;
						sc++;
						if(firsts) {
							Orsha_RtRu;
						} else {
							Oorrsha_RtRu;
						}
					}
				}
			}
			firsts = 0;
			if(WBITS == 8) {
				Table_RdRt(sfo,8,0);
				if(firstd) {
					Olsh_RsRd(8-(df+db));
				} else {
					Assemble(df,db);
				}
				df += db;
				firstd = 0;
			} else {
				for(sf = 0; sf < WBITS; ) {
					Table_RdRt(sf,8,0);
					if(firstd) {
						c = WBITS-(df+db);
						Olsh_RsRd(c);
					} else {
						Assemble(df,db);
					}
					df += db;
					sf += 8;
					firstd = 0;
				}
			}
		}
		Fetch_Rd(1);
		Emitop;
		width -= WBITS - doff;
		if(width > 0 && lmask != WMASK) {
				Ofield(lmask);
		} else if(width <= 0) {
			lmask &= rmask;
			Ofield(lmask);
		}
		Store_Rs_P;
		dc++;
		if(width <= 0)
			break;
	
		c = width >> LWBITS;
		if(c) {
			li = 0;
			if(c > 1) {
				Ilabel(c);
				li = p;
			}
			for(df = 0; df < WBITS; ) {
				if(WBITS == 8) {
					if(sh == 0) {
						Load_Rt_P;
					} else {
						Olsh_RtRt(8);
						Loador_Rt_P;
					}
					sc += c;
					Table_RdRt(sfo,8,0);
					Assemble(df,db);
					df += db;
				} else {
					if(sh == 0) {
						Load_Rt_P;
					} else if(sh > 0) {
						Olsha_RtRu;
						Load_Ru_P;
						Oorrshb_RtRu;
					} else {
						Olshb_RtRu;
						Load_Ru_P;
						Oorrsha_RtRu;
					}
					sc += c;
					for(sf = 0; sf < WBITS; ) {
						Table_RdRt(sf,8,0);
						Assemblex(df,db);
						df += db;
						sf += 8;
					}
				}
			}
			if(fd) {
				Fetch_Rd(1);
			}
			Emitop;
			Store_Rs_P;
			dc += c;
			if(c > 1) {
				Iloop(li);
			}
		}
	
		width -= c << LWBITS;
		if(width <= 0)
			break;
	
		for(df = 0; df < width; ) {
			c = (width - df) << -le;
			if(WBITS == 8) {
				if(sh == 0) {
					Load_Rt_P;
					sc++;
				} else if(sh > 0) {
					Olsh_RtRt(8);
					if(shb < c) {
						sc++;
						Loador_Rt_P;
					}
				} else {
					Olsh_RtRt(8);
					if(sha < c) {
						Loador_Rt_P;
						sc++;
					}
				}
				Table_RdRt(sfo,8,0);
				Assemble(df,db);
				df += db;
			} else {
				if(sh == 0) {
					Load_Rt_P;
					sc++;
				} else if(sh > 0) {
					Olsha_RtRu;
					if(shb < c) {
						Load_Ru_P;
						Oorrshb_RtRu;
						sc++;
					}
				} else {
					Olshb_RtRu;
					if(sha < c) {
						Load_Ru_P;
						Oorrsha_RtRu;
						sc++;
					}
				}
				for(sf = 0; sf < 32; ) {
					Table_RdRt(sf,8,0);
					Assemble(df,db);
					df += db;
					sf += 8;
				}
			}
		}
		Fetch_Rd(1);
		Emitop;
		Ofield(rmask);
		Store_Rs_P;
		dc++;
		break;

	}

	/* finish outer loop, put in rts, and execute */

	if(height > 0) {
		if(backward)
			c = (dc - dspan) * (WBITS/8);
		else
			c = (dspan - dc) * (WBITS/8);
		if(c) {
			Add_Ad(c);
		}
		if(fs) {
			if(backward)
				c = (sc - sspan) * (WBITS/8);
			else
				c = (sspan - sc) * (WBITS/8);
			if(c) {
				Add_As(c);
			}
		}
		Oloop(lo);
	}
	Orts;
#ifdef TEST
	if(onstack && p - memstart > Progmaxnoconv)
		print("Increase Progmaxnoconv to at least %d!\n", p - memstart);
	else if(p - memstart > Progmax)
		print("Increase Progmax to at least %d!\n", p - memstart);
	mem = memstart;
#endif
	Execandfree(memstart,onstack);
}

#ifdef TEST
void	prprog(void);
GBitmap *bb1, *bb2;
ulong	*src, *dst, *xdst, *xans;
int	swds, dwds;
long	ticks;
int	timeit;

#ifdef BYTEREV
ulong
byterev(ulong v)
{
	return (v>>24)|((v>>8)&0x0000FF00)|((v<<8)&0x00FF0000)|(v<<24);
}
#endif
#ifdef T386
long _clock;
#endif

long
func(int f, long s, int sld, long d, int dld)
{
	long a;
	int sh, i, db, sb;

	db = 1 << dld;
	sb = 1 << sld;
	sh = db - sb;
	if(sh > 0) {
		a = s;
		for(i = sb; i<db; i += sb){
			a <<= sb;
			s |= a;
		}
	} else if(sh < 0)
		s >>= -sh;

	switch(f){
	case Zero:	d = 0;			break;
	case DnorS:	d = ~(d|s);		break;
	case DandnotS:	d = d & ~s;		break;
	case notS:	d = ~s;			break;
	case notDandS:	d = ~d & s;		break;
	case notD:	d = ~d;			break;
	case DxorS:	d = d ^ s;		break;
	case DnandS:	d = ~(d&s);		break;
	case DandS:	d = d & s;		break;
	case DxnorS:	d = ~(d^s);		break;
	case S:		d = s;			break;
	case DornotS:	d = d | ~s;		break;
	case D:		d = d;			break;
	case notDorS:	d = ~d | s;		break;
	case DorS:	d = d | s;		break;
	case F:		d = ~0;			break;
	}

	d &= ((1<<db)-1);
	return d;
}

void
run(int fr, int to, int w, int op)
{
	int i, j, f, t, fy, ty;
	extern long *_clock;

	fr += bb2->r.min.x;
	to += bb1->r.min.x;
	fy = bb2->r.min.y + 1;
	ty = bb1->r.min.y + 1;
	if(timeit) {
		memcpy(dst, xdst, dwds * sizeof(long));
		ticks -= *_clock;
		gbitblt(bb1, Pt(to,ty), bb2, Rect(fr,fy,fr+w,fy+2), op);
		ticks += *_clock;
		return;
	}
	f = fr;
	t = to;
	memcpy(dst, xdst, dwds * sizeof(long));
	for(i=0; i<w; i++) {
		gbitblt(bb1, Pt(t,ty), bb2, Rect(f,fy,f+1,fy+1), op);
		gbitblt(bb1, Pt(t,ty+1), bb2, Rect(f,fy+1,f+1,fy+2), op);
		f++;
		t++;
	}
	memcpy(xans, dst, dwds * sizeof(long));

	memcpy(dst, xdst, dwds * sizeof(long));
	gbitblt(bb1, Pt(to,ty), bb2, Rect(fr,fy,fr+w,fy+2), op);

	if(memcmp(xans, dst, dwds * sizeof(long))) {
		/*
		 * print src and dst row offset, width in bits, and forw/back
		 * then print for each of the four rows: the source (s),
		 * the dest (d), the good value of the answer (g),
		 * and the actual bad value of the answer (b)
		 */
		print("fr=%d to=%d w=%d fb=%d%d\n",
			fr, to, w, FORCEFORW, FORCEBAKW);
		print("dst bitmap b %#lux, z %d, w %d, ld %d, r [%d,%d][%d,%d]\n",
			bb1->base, bb1->zero, bb1->width, bb1->ldepth,
			bb1->r.min.x, bb1->r.min.y, bb1->r.max.x, bb1->r.max.y);
		print("src bitmap b %#lux, z %d, w %d, ld %d, r [%d,%d][%d,%d]\n",
			bb2->base, bb2->zero, bb2->width, bb2->ldepth,
			bb2->r.min.x, bb2->r.min.y, bb2->r.max.x, bb2->r.max.y);
		for(j=0; 7*j < dwds; j++) {
			print("\ns");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", src[7*j + i]);
			print("\nd");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", xdst[7*j + i]);
			print("\ng");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", xans[7*j + i]);
			print("\nb");
			for(i=0; i<7 && 7*j+i < dwds; i++)
				print(" %.8lux", dst[7*j + i]);
			print("\n");
		}
		prprog();
	}
}

void
main(int argc, char *argv[])
{
	int f, t, w, i, sld, dld, op, iters, simple;
	ulong s, d, spix, dpix, apix, fpix, m, *ps, *pd;
	Point sorg, dorg;
	GBitmap *bs, *bd;
	long seed;
	char *ct;

	sld = 0;
	dld = 0;
	timeit = 0;
	iters = 200;
	simple = 0;
	ARGBEGIN {
	case 'i':
		iters = atoi(ARGF());
		break;
	case 's':
		simple = 1;
		break;
	case 't':
		timeit = 1;
		ct = ARGF();
		if(ct)
			iters = atoi(ct);
		break;
	} ARGEND
	if(argc > 0)
		sld = atoi(argv[0]);
	if(argc > 1)
		dld = atoi(argv[1]);
	if(sld < 0 || sld > 3 || dld < 0 || dld > 3 ||
           (sld != dld && !tabs[sld][dld])){
		print("conversion from ldepth %d to %d not enabled\n",
			sld, dld);
		exits(0);
	}
	if(!timeit && !simple) {
		seed = time(0);
		print("seed %lux\n", seed); srand(seed);	/**/
	}

	print("sld %d dld %d\n", sld, dld);
	op = 1/*Zero*/;

	/* bitmaps for 1-bit tests */
	bd = gballoc(Rect(0,0,32,1), dld);
	bs = gballoc(Rect(0,0,32,1), sld);
	for(i=0; i<bs->width; i++)
		bs->base[i] = lrand();

	/* bitmaps for rect tests */
	if(simple) {
		dorg = Pt(0,0);
		sorg = Pt(0,0);
	} else {
		dorg = Pt(nrand(63)-31,nrand(63)-31);
		sorg = Pt(nrand(63)-31,nrand(63)-31);
	}
	bb1 = gballoc(Rpt(dorg,add(dorg,Pt(200,4))), dld);
	bb2 = gballoc(Rpt(sorg,add(sorg,Pt(200,4))), sld);
	dwds = bb1->width * Dy(bb1->r);
	swds = bb2->width * Dy(bb2->r);
	dst = bb1->base;
	src = bb2->base;
	xdst = malloc(dwds * sizeof(long));
	xans =  malloc(dwds * sizeof(long));
	for(i=0; i<swds; i++)
		src[i] = lrand();
	for(i=0; i<dwds; i++)
		xdst[i] = lrand();
loop:
	print("Op %d\n", op);
	if(!timeit) {
		print("one pixel\n");
		ps = bs->base;
		pd = bd->base;
		FORCEFORW = 1;
		FORCEBAKW = 0;
		for(i=0; i<1000; i++, FORCEFORW = !FORCEFORW, FORCEBAKW = !FORCEBAKW) {
			f = nrand(32 >> sld);
			t = nrand(32 >> dld);
			s = lrand();
			d = lrand();
			ps[0] = s;
			pd[0] = d;
#ifdef BYTEREV
			spix = scrpix(byterev(s),f,sld);
			dpix = scrpix(byterev(d),t,dld);
#else
			spix = scrpix(s,f,sld);
			dpix = scrpix(d,t,dld);
#endif
			apix = func(op, spix, sld, dpix, dld);
			gbitblt(bd, Pt(t,0), bs, Rect(f,0,f+1,1), op);
			if(ps[0] != s) {
				print("bb src %.8lux %.8lux %d %d\n", ps[0], s, f, t);
				exits("error");
			}
			m = scrmask(t,dld);
#ifdef BYTEREV
			m = byterev(m);
#endif
			if((pd[0] & ~m) != (d & ~m)) {
					print("bb dst1 %.8lux %.8lux\n",
						s, d);
					print("bb      %.8lux %.8lux %d %d\n",
						ps[0], pd[0], f, t);
					prprog();
					exits("error");
			}
#ifdef BYTEREV
			fpix = scrpix(byterev(pd[0]),t,dld);
#else
			fpix = scrpix(pd[0],t,dld);
#endif
			if(apix != fpix) {
				print("bb dst2 %.8lux %.8lux\n",
					s, d);
				print("bb      %.8lux %.8lux %d %d\n",
					ps[0], pd[0], f, t);
				print("bb      %.8lux %.8lux %.8lux %.8lux\n",
					spix, dpix, apix, fpix);
				prprog();
				exits("error");
			}
		}
	}

	print("for\n");
	FORCEFORW = 1;
	FORCEBAKW = 0;

	for(i=0; i<iters; i++) {
		f = nrand(64);
		t = nrand(64);
		w = nrand(130);
		run(f, t, w, op);
	}

	if(sld == dld) {
		print("bak\n");
		FORCEFORW = 0;
		FORCEBAKW = 1;
	
		for(i=0; i<iters; i++) {
			f = nrand(64);
			t = nrand(64);
			w = nrand(130);
			run(f, t, w, op);
		}
	}

	if(op < F) {
		op++;
		goto loop;
	}
	if(timeit)
		print("time: %d ticks\n", ticks);
	exits(0);
}


#endif
