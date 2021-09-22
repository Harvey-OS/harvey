/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 * "$Header: dviselect.c,v 1.3 87/12/06 12:27:33 grunwald Exp $";
 */

/*
 * DVI page selection program
 *
 * Reads DVI version 2 files and selects pages, writing a new DVI
 * file.  The new DVI file is technically correct, though we do not
 * adjust the tallest and widest page values, nor the DVI stack size.
 * This is all right since the values will never become too small,
 * but it might be nice to fix them up.  Perhaps someday . . . .
 */

#define _POSIX_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

/*
 * C-TeX types (system dependent).
 */

/* a 16 bit integer (signed) */
typedef short i16;

/* a 32 bit integer (signed) */
typedef long i32;

/* macros to sign extend quantities that are less than 32 bits long */
/* Plan 9 mishandles (int)(char)(constant) */
/* #define Sign8(n)	((i32) (signed char) (n)) */
#define Sign8(n)	(((n) << 24) >> 24)
#define Sign16(n)	((i32) (short) (n))
#define Sign24(n)	(((n) << 8) >> 8)

/* macros to truncate quantites that are signed but shouldn't be */
#define UnSign8(n)	((n) & 0xff)
#define UnSign16(n)	((n) & 0xffff)
#define UnSign24(n)	((n) & 0xffffff)

/* DVI file info */

/*
 * Units of distance are stored in scaled points, but we can convert to
 * units of 10^-7 meters by multiplying by the numbers in the preamble.
 */

/* the structure of the stack used to hold the values (h,v,w,x,y,z) */
struct dvi_stack {
	i32	h;		/* the saved h */
	i32	v;		/* the saved v */
	i32	w;		/* etc */
	i32	x;
	i32	y;
	i32	z;
};

struct	dvi_stack dvi_current;	/* the current values of h, v, etc */

int	dvi_f;			/* the current font */

#define dvi_h dvi_current.h
#define dvi_v dvi_current.v
#define dvi_w dvi_current.w
#define dvi_x dvi_current.x
#define dvi_y dvi_current.y
#define dvi_z dvi_current.z

/*
 * Macros to convert DVI opcodes to (hopefully) simpler values.
 */

/*
 * Large range types.
 */
#define DVI_IsChar(code) ((code) < 128)
#define DVI_IsFont(code) ((code) >= 171 && (code) < 235)

/*
 * Symbolic names for generic types (for things with parameters).
 * These are obtained via the macro DVI_DT(int c), where 0 <= c <= 255.
 */
#define	DT_CHAR		 0
#define DT_SET		 1
#define	DT_SETRULE	 2
#define DT_PUT		 3
#define	DT_PUTRULE	 4
#define	DT_NOP		 5
#define	DT_BOP		 6
#define	DT_EOP		 7
#define	DT_PUSH		 8
#define	DT_POP		 9
#define DT_RIGHT	10
#define DT_W0		11
#define	DT_W		12
#define	DT_X0		13
#define DT_X		14
#define DT_DOWN		15
#define	DT_Y0		16
#define DT_Y		17
#define	DT_Z0		18
#define DT_Z		19
#define	DT_FNTNUM	20
#define DT_FNT		21
#define DT_XXX		22
#define DT_FNTDEF	23
#define	DT_PRE		24
#define	DT_POST		25
#define	DT_POSTPOST	26
#define	DT_UNDEF	27

/*
 * Symbolic names for parameter lengths, obtained via the macro
 * DVL_OpLen(int c).
 *
 * N.B.: older drivers may assume that 0 => none, 1-4 => 1-4 bytes
 * and 5-7 => unsigned version of 1-4---so DO NOT change these values!
 */
#define	DPL_NONE	0
#define	DPL_SGN1	1
#define	DPL_SGN2	2
#define	DPL_SGN3	3
#define	DPL_SGN4	4
#define	DPL_UNS1	5
#define	DPL_UNS2	6
#define	DPL_UNS3	7
/* there are no unsigned four byte parameters */

#define DVI_OpLen(code)  (dvi_oplen[code])
#define DVI_DT(code)	 (dvi_dt[code])
extern char dvi_oplen[];
extern char dvi_dt[];

#define DVI_VERSION	2	/* version number that should appear in
				   pre- and post-ambles */

#define DVI_SET1	128	/* set character, 1 byte param */
#define DVI_SET2	129	/* set character, 2 byte param */
#define DVI_SET3	130	/* set character, 3 byte param */
#define DVI_SET4	131	/* set character, 4 byte param */
#define DVI_SETRULE	132	/* set a rule */
#define DVI_PUT1	133	/* put char, don't move right */
#define DVI_PUT2	134	/* put char, 2 byte */
#define DVI_PUT3	135	/* etc */
#define DVI_PUT4	136
#define DVI_PUTRULE	137	/* put rule, don't move right */
#define DVI_NOP		138	/* no-op */
#define DVI_BOP		139	/* begin page */
#define DVI_EOP		140	/* end page */
#define DVI_PUSH	141	/* push h,v,w,x,y,z */
#define DVI_POP		142	/* pop  h,v,w,x,y,z */
#define DVI_RIGHT1	143	/* move right, 1 byte signed param */
#define DVI_RIGHT2	144	/* move right, 2 byte signed param */
#define DVI_RIGHT3	145	/* etc */
#define DVI_RIGHT4	146
#define DVI_W0		147	/* h += w */
#define DVI_W1		148	/* w = 1 byte signed param, h += w */
#define DVI_W2		149	/* w = 2 byte etc, h += w */
#define DVI_W3		150
#define DVI_W4		151
#define DVI_X0		152	/* like DVI_W0 but for x */
#define DVI_X1		153	/* etc */
#define DVI_X2		154
#define DVI_X3		155
#define DVI_X4		156
#define DVI_DOWN1	157	/* v += 1 byte signed param */
#define DVI_DOWN2	158	/* v += 2 byte signed param */
#define DVI_DOWN3	159	/* etc */
#define DVI_DOWN4	160
#define DVI_Y0		161	/* y = 1 byte signed param, v += y */
#define DVI_Y1		162	/* etc */
#define DVI_Y2		163
#define DVI_Y3		164
#define DVI_Y4		165
#define DVI_Z0		166	/* z = 1 byte signed param, v += z */
#define DVI_Z1		167	/* etc */
#define DVI_Z2		168
#define DVI_Z3		169
#define DVI_Z4		170
#define DVI_FNTNUM0	171

#define DVI_FNT1	235	/* select font, 1 byte param */
#define DVI_FNT2	236	/* etc */
#define DVI_FNT3	237
#define DVI_FNT4	238
#define DVI_XXX1	239	/* for \special: if length < 256 */
#define DVI_XXX2	240	/* etc */
#define DVI_XXX3	241
#define DVI_XXX4	242
#define DVI_FNTDEF1	243	/* Define font, 1 byte param (0 to 63) */
#define DVI_FNTDEF2	244	/* etc */
#define DVI_FNTDEF3	245
#define DVI_FNTDEF4	246
#define DVI_PRE		247	/* preamble */
#define DVI_POST	248	/* postamble */
#define DVI_POSTPOST	249	/* end of postamble */
#define DVI_FILLER	223	/* filler bytes at end of dvi file */

/* shorthand---in lowercase for contrast (read on!) */
#define	four(x)		x, x, x, x
#define	six(x)		four(x), x, x
#define	sixteen(x)	four(x), four(x), four(x), four(x)
#define	sixty_four(x)	sixteen(x), sixteen(x), sixteen(x), sixteen(x)
#define	one_twenty_eight(x)	sixty_four(x), sixty_four(x)

/*
 * This table contains the byte length of the single operand, or DPL_NONE
 * if no operand, or if it cannot be decoded this way.
 *
 * The sequences UNS1, UNS2, UNS3, SGN4 (`SEQ_U') and SGN1, SGN2, SGN3,
 * SGN4 (`SEQ_S') are rather common, and so we define macros for these.
 */
#define	SEQ_U	DPL_UNS1, DPL_UNS2, DPL_UNS3, DPL_SGN4
#define	SEQ_S	DPL_SGN1, DPL_SGN2, DPL_SGN3, DPL_SGN4

char dvi_oplen[256] = {
	one_twenty_eight(DPL_NONE),
				/* characters 0 through 127 */
	SEQ_U,			/* DVI_SET1 through DVI_SET4 */
	DPL_NONE,		/* DVI_SETRULE */
	SEQ_U,			/* DVI_PUT1 through DVI_PUT4 */
	DPL_NONE,		/* DVI_PUTRULE */
	DPL_NONE,		/* DVI_NOP */
	DPL_NONE,		/* DVI_BOP */
	DPL_NONE,		/* DVI_EOP */
	DPL_NONE,		/* DVI_PUSH */
	DPL_NONE,		/* DVI_POP */
	SEQ_S,			/* DVI_RIGHT1 through DVI_RIGHT4 */
	DPL_NONE,		/* DVI_W0 */
	SEQ_S,			/* DVI_W1 through DVI_W4 */
	DPL_NONE,		/* DVI_X0 */
	SEQ_S,			/* DVI_X1 through DVI_X4 */
	SEQ_S,			/* DVI_DOWN1 through DVI_DOWN4 */
	DPL_NONE,		/* DVI_Y0 */
	SEQ_S,			/* DVI_Y1 through DVI_Y4 */
	DPL_NONE,		/* DVI_Z0 */
	SEQ_S,			/* DVI_Z1 through DVI_Z4 */
	sixty_four(DPL_NONE),	/* DVI_FNTNUM0 through DVI_FNTNUM63 */
	SEQ_U,			/* DVI_FNT1 through DVI_FNT4 */
	SEQ_U,			/* DVI_XXX1 through DVI_XXX4 */
	SEQ_U,			/* DVI_FNTDEF1 through DVI_FNTDEF4 */
	DPL_NONE,		/* DVI_PRE */
	DPL_NONE,		/* DVI_POST */
	DPL_NONE,		/* DVI_POSTPOST */
	six(DPL_NONE)		/* 250 through 255 */
};

char dvi_dt[256] = {
	one_twenty_eight(DT_CHAR),
				/* characters 0 through 127 */
	four(DT_SET),		/* DVI_SET1 through DVI_SET4 */
	DT_SETRULE,		/* DVI_SETRULE */
	four(DT_PUT),		/* DVI_PUT1 through DVI_PUT4 */
	DT_PUTRULE,		/* DVI_PUTRULE */
	DT_NOP,			/* DVI_NOP */
	DT_BOP,			/* DVI_BOP */
	DT_EOP,			/* DVI_EOP */
	DT_PUSH,		/* DVI_PUSH */
	DT_POP,			/* DVI_POP */
	four(DT_RIGHT),		/* DVI_RIGHT1 through DVI_RIGHT4 */
	DT_W0,			/* DVI_W0 */
	four(DT_W),		/* DVI_W1 through DVI_W4 */
	DT_X0,			/* DVI_X0 */
	four(DT_X),		/* DVI_X1 through DVI_X4 */
	four(DT_DOWN),		/* DVI_DOWN1 through DVI_DOWN4 */
	DT_Y0,			/* DVI_Y0 */
	four(DT_Y),		/* DVI_Y1 through DVI_Y4 */
	DT_Z0,			/* DVI_Z0 */
	four(DT_Z),		/* DVI_Z1 through DVI_Z4 */
	sixty_four(DT_FNTNUM),	/* DVI_FNTNUM0 through DVI_FNTNUM63 */
	four(DT_FNT),		/* DVI_FNT1 through DVI_FNT4 */
	four(DT_XXX),		/* DVI_XXX1 through DVI_XXX4 */
	four(DT_FNTDEF),	/* DVI_FNTDEF1 through DVI_FNTDEF4 */
	DT_PRE,			/* DVI_PRE */
	DT_POST,		/* DVI_POST */
	DT_POSTPOST,		/* DVI_POSTPOST */
	six(DT_UNDEF)		/* 250 through 255 */
};

/*
 * File I/O: numbers.
 *
 * We deal in fixed format numbers and (FILE *)s here.
 * For pointer I/O, see pio.h.
 *
 * N.B.: These do the `wrong thing' at EOF.  It is imperative
 * that the caller add appropriate `if (feof(fp))' statements.
 */

/*
 * Get one unsigned byte.  Note that this is a proper expression.
 * The reset have more limited contexts, and are therefore OddLy
 * CapItaliseD.
 */
#define	fgetbyte(fp)	(getc(fp))

/*
 * Get a two-byte unsigned integer, a three-byte unsigned integer,
 * or a four-byte signed integer.
 */
#define fGetWord(fp, r)	((r)  = getc(fp) << 8,  (r) |= getc(fp))
#define fGet3Byte(fp,r) ((r)  = getc(fp) << 16, (r) |= getc(fp) << 8, \
			 (r) |= getc(fp))
#define fGetLong(fp, r)	((r)  = getc(fp) << 24, (r) |= getc(fp) << 16, \
			 (r) |= getc(fp) << 8,  (r) |= getc(fp))

/*
 * Fast I/O write (and regular write) macros.
 */
#define	putbyte(fp, r)	(putc((r), fp))

#define PutWord(fp, r)	(putc((r) >> 8,  fp), putc((r), fp))
#define Put3Byte(fp, r)	(putc((r) >> 16, fp), putc((r) >> 8, fp), \
			 putc((r), fp))
#define PutLong(fp, r)	(putc((r) >> 24, fp), putc((r) >> 16, fp), \
			 putc((r) >> 8, fp),  putc((r), fp))

/*
 * Function types
 */
i32	GetByte(FILE *), GetWord(FILE *), GetLong(FILE *);

struct search {
	unsigned s_dsize;	/* data object size (includes key size) */
	unsigned s_space;	/* space left (in terms of objects) */
	unsigned s_n;		/* number of objects in the table */
	char	*s_data;	/* data area */
};

/* returns a pointer to the search table (for future search/installs) */
struct	search *SCreate(unsigned int);	/* create a search table */

/* returns a pointer to the data object found or created */
char	*SSearch(struct search *, i32, int *);	/* search for a data object */

void	SEnumerate(struct search *, int (*)(char *, i32));

/* the third argument to SSearch controls operation as follows: */
#define	S_LOOKUP	0x00	/* pseudo flag */
#define	S_CREATE	0x01	/* create object if not found */
#define	S_EXCL		0x02	/* complain if already exists */

/* in addition, it is modified before return to hold status: */
#define	S_COLL		0x04	/* collision (occurs iff S_EXCL set) */
#define	S_FOUND		0x08	/* found (occurs iff existed already) */
#define	S_NEW		0x10	/* created (occurs iff S_CREATE && !S_EXCL) */
#define	S_ERROR		0x20	/* problem creating (out of memory) */

char  *ProgName;
extern int   errno;

/* Functions */
void	error(int, int, char *, ...);
void	panic(char *, ...);

/* Globals */
char	serrbuf[BUFSIZ];	/* buffer for stderr */

/*
 * We will try to keep output lines shorter than MAXCOL characters.
 */
#define MAXCOL	75

/*
 * We use the following structure to keep track of fonts we have seen.
 * The final DVI file lists only the fonts it uses.
 */
struct fontinfo {
	i32	fi_newindex;	/* font number in output file */
	int	fi_reallyused;	/* true => used on a page we copied */
	i32	fi_checksum;	/* the checksum */
	i32	fi_mag;		/* the magnification */
	i32	fi_designsize;	/* the design size */
	short	fi_n1;		/* the name header length */
	short	fi_n2;		/* the name body length */
	char	*fi_name;	/* the name itself */
};

/*
 * We need to remember which pages the user would like.  We build a linked
 * list that allows us to decide (for any given page) whether it should
 * be included in the output file.  Each page has ten \count variables
 * associated with it.  We put a bound on the values allowed for each, and
 * keep a linked list of alternatives should any be outside the allowed
 * range.  For example, `dviselect *.3,10-15' would generate a two-element
 * page list, with the first allowing any value for \count0 (and \counts 2 to
 * 9) but \count1 restricted to the range 3-3, and the second restricting
 * \count0 to the range 10-15 but leaving \counts 1 to 9 unrestrained.
 *
 * In case no bound is specified, the `nol' or `noh' flag is set (so that
 * we need not fix some `large' number as a maximum value).
 *
 * We also allow `absolute' page references, where the first page is
 * page 1, the second 2, and so forth.  These are specified with an
 * equal sign: `dviselect =4:10' picks up the fourth through tenth
 * sequential pages, irrespective of \count values.
 */
struct pagesel {
	i32	ps_low;		/* lower bound */
	int	ps_nol;		/* true iff no lower bound */
	i32	ps_high;	/* upper bound */
	int	ps_noh;		/* true iff no upper bound */
};
struct pagelist {
	struct	pagelist *pl_alt;	/* next in a series of alternates */
	int	pl_len;			/* number of ranges to check */
	int	pl_abs;			/* true iff absolute page ref */
	struct	pagesel pl_pages[10];	/* one for each \count variable */
};

int	SFlag;			/* true => -s, silent operation */

struct	search *FontFinder;	/* maps from input indicies to fontinfo */
i32	NextOutputFontIndex;	/* generates output indicies */
i32	CurrentFontIndex;	/* current (old) index in input */
i32	OutputFontIndex;	/* current (new) index in ouput */

struct	pagelist *PageList;	/* the list of allowed pages */

FILE	*inf;			/* the input DVI file */
FILE	*outf;			/* the output DVI file */

int	ExpectBOP;		/* true => BOP ok */
int	ExpectEOP;		/* true => EOP ok */

long	StartOfLastPage;	/* The file position just before we started
				   the last page (this is later written to
				   the output file as the previous page
				   pointer). */
long	CurrentPosition;	/* The current position of the file */

int	UseThisPage;		/* true => current page is selected */

i32	InputPageNumber;	/* current absolute page in old DVI file */
int	NumberOfOutputPages;	/* number of pages in new DVI file */

i32	Numerator;		/* numerator from DVI file */
i32	Denominator;		/* denominator from DVI file */
i32	DVIMag;			/* magnification from DVI file */

i32	Count[10];		/* the 10 \count variables */

/* save some string space: we use this a lot */
char	writeerr[] = "error writing DVI file";

/*
 * Return true iff the 10 \counts are one of the desired output pages.
 */
DesiredPageP()
{
	register struct pagelist *pl;

	for (pl = PageList; pl != NULL; pl = pl->pl_alt) {
		register struct pagesel *ps = pl->pl_pages;
		register int i;
		register i32 *pagep;

		pagep = pl->pl_abs ? &InputPageNumber : &Count[0];
		for (i = 0; i < pl->pl_len; i++, ps++, pagep++)
			if (!ps->ps_nol && *pagep < ps->ps_low ||
			    !ps->ps_noh && *pagep > ps->ps_high)
				break;	/* not within bounds */
		if (i >= pl->pl_len)
			return (1);	/* success */
	}
	return (0);
}

/*
 * Print a message to stderr, with an optional leading space, and handling
 * long line wraps.
 */
message(space, str, len)
	int space;
	register char *str;
	register int len;
{
	static int beenhere;
	static int col;

	if (!beenhere)
		space = 0, beenhere++;
	if (len == 0)
		len = strlen(str);
	col += len;
	if (space) {
		if (col >= MAXCOL)
			(void) putc('\n', stderr), col = len;
		else
			(void) putc(' ', stderr), col++;
	}
	while (--len >= 0)
		(void) putc(*str++, stderr);
	(void) fflush(stderr);
}

/*
 * Start a page (process a DVI_BOP).
 */
BeginPage()
{
	register i32 *i;

	if (!ExpectBOP)
		GripeUnexpectedOp("BOP");
	ExpectBOP = 0;
	ExpectEOP++;		/* set the new "expect" state */

	OutputFontIndex = -1;	/* new page requires respecifying font */
	InputPageNumber++;	/* count it */
	for (i = Count; i < &Count[10]; i++)
		fGetLong(inf, *i);
	(void) GetLong(inf);	/* previous page pointer */

	if ((UseThisPage = DesiredPageP()) == 0)
		return;

	(void) putc(DVI_BOP, outf);
	for (i = Count; i < &Count[10]; i++)
		PutLong(outf, *i);
	PutLong(outf, StartOfLastPage);
	if (ferror(outf))
		error(1, errno, writeerr);

	StartOfLastPage = CurrentPosition;
	CurrentPosition += 45;	/* we just wrote this much */

	if (!SFlag) {		/* write nice page usage messages */
		register int z = 0;
		register int mlen = 0;
		char msg[80];

		(void) sprintf(msg, "[%d", Count[0]);
		mlen = strlen(msg);
		for (i = &Count[1]; i < &Count[10]; i++) {
			if (*i == 0) {
				z++;
				continue;
			}
			while (--z >= 0)
				msg[mlen++] = '.', msg[mlen++] = '0';
			z = 0;
			(void) sprintf(msg + mlen, ".%d", *i);
			mlen += strlen(msg + mlen);
		}
		message(1, msg, mlen);
	}
}

/*
 * End a page (process a DVI_EOP).
 */
EndPage()
{
	if (!ExpectEOP)
		GripeUnexpectedOp("EOP");
	ExpectEOP = 0;
	ExpectBOP++;

	if (!UseThisPage)
		return;

	if (!SFlag)
		message(0, "]", 1);

	putc(DVI_EOP, outf);
	if (ferror(outf))
		error(1, errno, writeerr);
	CurrentPosition++;
	NumberOfOutputPages++;
}

/*
 * For each of the fonts used in the new DVI file, write out a definition.
 */
PostAmbleFontEnumerator(addr, key)
	char *addr;
	i32 key;
{
#pragma ref key
	if (((struct fontinfo *) addr)->fi_reallyused)
		WriteFont((struct fontinfo *) addr);
}

HandlePostAmble()
{
	register i32 c;

	(void) GetLong(inf);	/* previous page pointer */
	if (GetLong(inf) != Numerator)
		GripeMismatchedValue("numerator");
	if (GetLong(inf) != Denominator)
		GripeMismatchedValue("denominator");
	if (GetLong(inf) != DVIMag)
		GripeMismatchedValue("\\magfactor");

	putc(DVI_POST, outf);
	PutLong(outf, StartOfLastPage);
	PutLong(outf, Numerator);
	PutLong(outf, Denominator);
	PutLong(outf, DVIMag);
	c = GetLong(inf);
	PutLong(outf, c);	/* tallest page height */
	c = GetLong(inf);
	PutLong(outf, c);	/* widest page width */
	c = GetWord(inf);
	PutWord(outf, c);	/* DVI stack size */
	PutWord(outf, NumberOfOutputPages);
	StartOfLastPage = CurrentPosition;	/* point at post */
	CurrentPosition += 29;	/* count all those `put's */
#ifdef notdef
	(void) GetWord(inf);	/* skip original number of pages */
#endif

	/*
	 * just ignore all the incoming font definitions; we are done with
	 * input file 
	 */

	/*
	 * run through the FontFinder table and dump definitions for the
	 * fonts we have used. 
	 */
	SEnumerate(FontFinder, PostAmbleFontEnumerator);

	putc(DVI_POSTPOST, outf);
	PutLong(outf, StartOfLastPage);	/* actually start of postamble */
	putc(DVI_VERSION, outf);
	putc(DVI_FILLER, outf);
	putc(DVI_FILLER, outf);
	putc(DVI_FILLER, outf);
	putc(DVI_FILLER, outf);
	CurrentPosition += 10;
	while (CurrentPosition & 3)
		putc(DVI_FILLER, outf), CurrentPosition++;
	if (ferror(outf))
		error(1, errno, writeerr);
}

/*
 * Write a font definition to the output file
 */
WriteFont(fi)
	register struct fontinfo *fi;
{
	register int l;
	register char *s;

	if (fi->fi_newindex < 256) {
		putc(DVI_FNTDEF1, outf);
		putc(fi->fi_newindex, outf);
		CurrentPosition += 2;
	} else if (fi->fi_newindex < 65536) {
		putc(DVI_FNTDEF2, outf);
		PutWord(outf, fi->fi_newindex);
		CurrentPosition += 3;
	} else if (fi->fi_newindex < 16777216) {
		putc(DVI_FNTDEF3, outf);
		Put3Byte(outf, fi->fi_newindex);
		CurrentPosition += 4;
	} else {
		putc(DVI_FNTDEF4, outf);
		PutLong(outf, fi->fi_newindex);
		CurrentPosition += 5;
	}
	PutLong(outf, fi->fi_checksum);
	PutLong(outf, fi->fi_mag);
	PutLong(outf, fi->fi_designsize);
	putc(fi->fi_n1, outf);
	putc(fi->fi_n2, outf);
	l = fi->fi_n1 + fi->fi_n2;
	CurrentPosition += 14 + l;
	s = fi->fi_name;
	while (--l >= 0)
		putc(*s, outf), s++;
}

/*
 * Handle the preamble.  Someday we should update the comment field.
 */
HandlePreAmble()
{
	register int n, c;

	if (GetByte(inf) != Sign8(DVI_PRE))
		GripeMissingOp("PRE");
	if (GetByte(inf) != Sign8(DVI_VERSION))
		GripeMismatchedValue("DVI version number");
	Numerator = GetLong(inf);
	Denominator = GetLong(inf);
	DVIMag = GetLong(inf);
	putc(DVI_PRE, outf);
	putc(DVI_VERSION, outf);
	PutLong(outf, Numerator);
	PutLong(outf, Denominator);
	PutLong(outf, DVIMag);

	n = UnSign8(GetByte(inf));
	CurrentPosition = 15 + n;	/* well, almost */
	putc(n, outf);
	while (--n >= 0) {
		c = GetByte(inf);
		putc(c, outf);	/* never trust a macro, I always say */
	}
}

/*
 * argument processing
 */
#define	ARGBEGIN	for((argv0? 0: (argv0=*argv)),argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt, _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				while(*_args) switch(_argc=*_args++)
#define	ARGEND		}
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc

main(int argc, char **argv)
{
	int c;
	char *s;
	char *inname = NULL, *outname = NULL;
	char *argv0;

	ProgName = *argv;
	setbuf(stderr, serrbuf);

	ARGBEGIN{
		case 's':	/* silent */
			SFlag++;
			break;

		case 'i':
			if (inname != NULL)
				goto usage;
			inname = ARGF();
			break;

		case 'o':
			if (outname != NULL)
				goto usage;
			outname = ARGF();
			break;

		case '?':
usage:
			fprintf(stderr, "\
Usage: %s [-s] [-i infile] [-o outfile] pages [...] [infile [outfile]]\n",
				ProgName);
			(void) fflush(stderr);
			exit(1);
	} ARGEND

	while (--argc >= 0) {
		s = *argv++;
		c = *s;
		if (!isalpha(c) && c != '/') {
			if (ParsePages(s))
				goto usage;
		} else if (inname == NULL)
			inname = s;
		else if (outname == NULL)
			outname = s;
		else
			goto usage;
	}
	if (PageList == NULL)
		goto usage;
	if (inname == NULL)
		inf = stdin;
	else if ((inf = fopen(inname, "r")) == 0)
		error(1, errno, "cannot read %s", inname);
	if (outname == NULL)
		outf = stdout;
	else if ((outf = fopen(outname, "w")) == 0)
		error(1, errno, "cannot write %s", outname);

	if ((FontFinder = SCreate(sizeof(struct fontinfo))) == 0)
		error(1, 0, "cannot create font finder (out of memory?)");

	ExpectBOP++;
	StartOfLastPage = -1;
	HandlePreAmble();
	HandleDVIFile();
	HandlePostAmble();
	if (!SFlag)
		fprintf(stderr, "\nWrote %d pages, %d bytes\n",
			NumberOfOutputPages, CurrentPosition);
	exit(0);
}

struct pagelist *
InstallPL(ps, n, absolute)
	register struct pagesel *ps;
	register int n;
	int absolute;
{
	register struct pagelist *pl;

	pl = (struct pagelist *) malloc(sizeof *pl);
	if (pl == NULL)
		GripeOutOfMemory(sizeof *pl, "page list");
	pl->pl_alt = PageList;
	PageList = pl;
	pl->pl_len = n;
	while (--n >= 0)
		pl->pl_pages[n] = ps[n];
	pl->pl_abs = absolute;
}

/*
 * Parse a string representing a list of pages.  Return 0 iff ok.  As a
 * side effect, the page selection(s) is (are) prepended to PageList.
 */
ParsePages(s)
	register char *s;
{
	register struct pagesel *ps;
	register int c;		/* current character */
	register i32 n;		/* current numeric value */
	register int innumber;	/* true => gathering a number */
	int i;			/* next index in page select list */
	int range;		/* true => saw a range indicator */
	int negative;		/* true => number being built is negative */
	int absolute;		/* true => absolute, not \count */
	struct pagesel pagesel[10];

#define white(x) ((x) == ' ' || (x) == '\t' || (x) == ',')

	range = 0;
	innumber = 0;
	absolute = 0;
	i = 0;
	ps = pagesel;
	/*
	 * Talk about ad hoc!  (Not to mention convoluted.)
	 */
	for (;;) {
		c = *s++;
		if (i == 0 && !innumber && !range) {
			/* nothing special going on */
			if (c == 0)
				return 0;
			if (white(c))
				continue;
		}
		if (c == '_') {
			/* kludge: should be '-' for negatives */
			if (innumber || absolute)
				return (-1);
			innumber++;
			negative = 1;
			n = 0;
			continue;
		}
		if (c == '=') {
			/* absolute page */
			if (innumber || range || i > 0)
				return (-1);
			absolute++;
			/*
			 * Setting innumber means that there is always
			 * a lower bound, but this is all right since
			 * `=:4' is treated as if it were `=0:4'.  As
			 * there are no negative absolute page numbers,
			 * this selects pages 1:4, which is the proper
			 * action.
			 */
			innumber++;
			negative = 0;
			n = 0;
			continue;
		}
		if (isdigit(c)) {
			/* accumulate numeric value */
			if (!innumber) {
				innumber++;
				negative = 0;
				n = c - '0';
				continue;
			}
			n *= 10;
			n += negative ? '0' - c : c - '0';
			continue;
		}
		if (c == '-' || c == ':') {
			/* here is a range */
			if (range)
				return (-1);
			if (innumber) {	/* have a lower bound */
				ps->ps_low = n;
				ps->ps_nol = 0;
			} else
				ps->ps_nol = 1;
			range++;
			innumber = 0;
			continue;
		}
		if (c == '*') {
			/* no lower bound, no upper bound */
			c = *s++;
			if (innumber || range || i >= 10 ||
			    (c && c != '.' && !white(c)))
				return (-1);
			ps->ps_nol = 1;
			ps->ps_noh = 1;
			goto finishnum;
		}
		if (c == 0 || c == '.' || white(c)) {
			/* end of this range */
			if (i >= 10)
				return (-1);
			if (!innumber) {	/* no upper bound */
				ps->ps_noh = 1;
				if (!range)	/* no lower bound either */
					ps->ps_nol = 1;
			} else {		/* have an upper bound */
				ps->ps_high = n;
				ps->ps_noh = 0;
				if (!range) {
					/* no range => lower bound == upper */
					ps->ps_low = ps->ps_high;
					ps->ps_nol = 0;
				}
			}
finishnum:
			i++;
			if (c == '.') {
				if (absolute)
					return (-1);
				ps++;
			} else {
				InstallPL(pagesel, i, absolute);
				ps = pagesel;
				i = 0;
				absolute = 0;
			}
			if (c == 0)
				return (0);
			range = 0;
			innumber = 0;
			continue;
		}
		/* illegal character */
		return (-1);
	}
#undef white
}

/*
 * Handle a font definition.
 */
HandleFontDef(index)
	i32 index;
{
	register struct fontinfo *fi;
	register int i;
	register char *s;
	int def = S_CREATE | S_EXCL;

	if ((fi = (struct fontinfo *) SSearch(FontFinder, index, &def)) == 0)
		if (def & S_COLL)
			error(1, 0, "font %d already defined", index);
		else
			error(1, 0, "cannot stash font %d (out of memory?)",
				index);
	fi->fi_reallyused = 0;
	fi->fi_checksum = GetLong(inf);
	fi->fi_mag = GetLong(inf);
	fi->fi_designsize = GetLong(inf);
	fi->fi_n1 = UnSign8(GetByte(inf));
	fi->fi_n2 = UnSign8(GetByte(inf));
	i = fi->fi_n1 + fi->fi_n2;
	if ((s = malloc((unsigned) i)) == 0)
		GripeOutOfMemory(i, "font name");
	fi->fi_name = s;
	while (--i >= 0)
		*s++ = GetByte(inf);
}

/*
 * Handle a \special.
 */
HandleSpecial(c, l, p)
	int c;
	register int l;
	register i32 p;
{
	register int i;

	if (UseThisPage) {
		putc(c, outf);
		switch (l) {

		case DPL_UNS1:
			putc(p, outf);
			CurrentPosition += 2;
			break;

		case DPL_UNS2:
			PutWord(outf, p);
			CurrentPosition += 3;
			break;

		case DPL_UNS3:
			Put3Byte(outf, p);
			CurrentPosition += 4;
			break;

		case DPL_SGN4:
			PutLong(outf, p);
			CurrentPosition += 5;
			break;

		default:
			panic("HandleSpecial l=%d", l);
			/* NOTREACHED */
		}
		CurrentPosition += p;
		while (--p >= 0) {
			i = getc(inf);
			putc(i, outf);
		}
		if (feof(inf))
			error(1, 0, "unexpected EOF");
		if (ferror(outf))
			error(1, errno, writeerr);
	} else
		while (--p >= 0)
			(void) getc(inf);
}

ReallyUseFont()
{
	register struct fontinfo *fi;
	int look = S_LOOKUP;

	fi = (struct fontinfo *) SSearch(FontFinder, CurrentFontIndex, &look);
	if (fi == 0)
		error(1, 0, "index %d not in font table!", CurrentFontIndex);
	if (fi->fi_reallyused == 0) {
		fi->fi_reallyused++;
		fi->fi_newindex = NextOutputFontIndex++;
		WriteFont(fi);
	}
	if (fi->fi_newindex != OutputFontIndex) {
		PutFontSelector(fi->fi_newindex);
		OutputFontIndex = fi->fi_newindex;
	}
}

/*
 * Write a font selection command to the output file
 */
PutFontSelector(index)
	i32 index;
{

	if (index < 64) {
		putc(index + DVI_FNTNUM0, outf);
		CurrentPosition++;
	} else if (index < 256) {
		putc(DVI_FNT1, outf);
		putc(index, outf);
		CurrentPosition += 2;
	} else if (index < 65536) {
		putc(DVI_FNT2, outf);
		PutWord(outf, index);
		CurrentPosition += 3;
	} else if (index < 16777216) {
		putc(DVI_FNT3, outf);
		Put3Byte(outf, index);
		CurrentPosition += 4;
	} else {
		putc(DVI_FNT4, outf);
		PutLong(outf, index);
		CurrentPosition += 5;
	}
}

/*
 * The following table describes the length (in bytes) of each of the DVI
 * commands that we can simply copy, starting with DVI_SET1 (128).
 */
char	oplen[128] = {
	0, 0, 0, 0,		/* DVI_SET1 .. DVI_SET4 */
	9,			/* DVI_SETRULE */
	0, 0, 0, 0,		/* DVI_PUT1 .. DVI_PUT4 */
	9,			/* DVI_PUTRULE */
	1,			/* DVI_NOP */
	0,			/* DVI_BOP */
	0,			/* DVI_EOP */
	1,			/* DVI_PUSH */
	1,			/* DVI_POP */
	2, 3, 4, 5,		/* DVI_RIGHT1 .. DVI_RIGHT4 */
	1,			/* DVI_W0 */
	2, 3, 4, 5,		/* DVI_W1 .. DVI_W4 */
	1,			/* DVI_X0 */
	2, 3, 4, 5,		/* DVI_X1 .. DVI_X4 */
	2, 3, 4, 5,		/* DVI_DOWN1 .. DVI_DOWN4 */
	1,			/* DVI_Y0 */
	2, 3, 4, 5,		/* DVI_Y1 .. DVI_Y4 */
	1,			/* DVI_Z0 */
	2, 3, 4, 5,		/* DVI_Z1 .. DVI_Z4 */
	0,			/* DVI_FNTNUM0 (171) */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 172 .. 179 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 180 .. 187 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 188 .. 195 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 196 .. 203 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 204 .. 211 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 212 .. 219 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 220 .. 227 */
	0, 0, 0, 0, 0, 0, 0,	/* 228 .. 234 */
	0, 0, 0, 0,		/* DVI_FNT1 .. DVI_FNT4 */
	0, 0, 0, 0,		/* DVI_XXX1 .. DVI_XXX4 */
	0, 0, 0, 0,		/* DVI_FNTDEF1 .. DVI_FNTDEF4 */
	0,			/* DVI_PRE */
	0,			/* DVI_POST */
	0,			/* DVI_POSTPOST */
	0, 0, 0, 0, 0, 0,	/* 250 .. 255 */
};

/*
 * Here we read the input DVI file and write relevant pages to the
 * output DVI file. We also keep track of font changes, handle font
 * definitions, and perform some other housekeeping.
 */
HandleDVIFile()
{
	register int c, l;
	register i32 p;
	register int CurrentFontOK = 0;

	/* Only way out is via "return" statement */
	for (;;) {
		c = getc(inf);	/* getc() returns unsigned values */
		if (DVI_IsChar(c)) {
			/*
			 * Copy chars, note font usage, but ignore if
			 * page is not interesting.
			 */
			if (!UseThisPage)
				continue;
			if (!CurrentFontOK) {
				ReallyUseFont();
				CurrentFontOK++;
			}
			putc(c, outf);
			CurrentPosition++;
			continue;
		}
		if (DVI_IsFont(c)) {	/* note font change */
			CurrentFontIndex = c - DVI_FNTNUM0;
			CurrentFontOK = 0;
			continue;
		}
		if ((l = (oplen - 128)[c]) != 0) {	/* simple copy */
			if (!UseThisPage) {
				while (--l > 0)
					(void) getc(inf);
				continue;
			}
			CurrentPosition += l;
			putc(c, outf);
			while (--l > 0) {
				c = getc(inf);
				putc(c, outf);
			}
			if (ferror(outf))
				error(1, errno, writeerr);
			continue;
		}
		if ((l = DVI_OpLen(c)) != 0) {
			/*
			 * Handle other generics.
			 * N.B.: there should only be unsigned parameters
			 * here (save SGN4), for commands with negative
			 * parameters have been taken care of above.
			 */
			switch (l) {

			case DPL_UNS1:
				p = getc(inf);
				break;

			case DPL_UNS2:
				fGetWord(inf, p);
				break;

			case DPL_UNS3:
				fGet3Byte(inf, p);
				break;

			case DPL_SGN4:
				fGetLong(inf, p);
				break;

			default:
				panic("HandleDVIFile l=%d", l);
			}

			/*
			 * Now that we have the parameter, perform the
			 * command.
			 */
			switch (DVI_DT(c)) {

			case DT_SET:
			case DT_PUT:
				if (!UseThisPage)
					continue;
				if (!CurrentFontOK) {
					ReallyUseFont();
					CurrentFontOK++;
				}
				putc(c, outf);
				switch (l) {

				case DPL_UNS1:
					putc(p, outf);
					CurrentPosition += 2;
					continue;

				case DPL_UNS2:
					PutWord(outf, p);
					CurrentPosition += 3;
					continue;

				case DPL_UNS3:
					Put3Byte(outf, p);
					CurrentPosition += 4;
					continue;

				case DPL_SGN4:
					PutLong(outf, p);
					CurrentPosition += 5;
					continue;
				}

			case DT_FNT:
				CurrentFontIndex = p;
				CurrentFontOK = 0;
				continue;

			case DT_XXX:
				HandleSpecial(c, l, p);
				continue;

			case DT_FNTDEF:
				HandleFontDef(p);
				continue;

			default:
				panic("HandleDVIFile DVI_DT(%d)=%d",
				      c, DVI_DT(c));
			}
			continue;
		}

		switch (c) {	/* handle the few remaining cases */

		case DVI_BOP:
			BeginPage();
			CurrentFontOK = 0;
			break;

		case DVI_EOP:
			EndPage();
			break;

		case DVI_PRE:
			GripeUnexpectedOp("PRE");
			/* NOTREACHED */

		case DVI_POST:
			return;

		case DVI_POSTPOST:
			GripeUnexpectedOp("POSTPOST");
			/* NOTREACHED */

		default:
			GripeUndefinedOp(c);
			/* NOTREACHED */
		}
	}
}

/*
 * Key search routines (for a 32 bit key)
 *
 * SCreate initializes the search control area.
 *
 * SSearch returns the address of the data area (if found or created)
 * or a null pointer (if not).  The last argument controls the disposition
 * in various cases, and is a ``value-result'' parameter.
 *
 * SEnumerate calls the given function on each data object within the
 * search table.  Note that no ordering is specified (though currently
 * it runs in increasing-key-value sequence).
 */

#define	HARD_ALIGNMENT	4	/* should suffice for most everyone */

int DOffset;		/* part of alignment code */

struct search *
SCreate(unsigned int dsize)
{
	struct search *s;

	if ((s = (struct search *) malloc(sizeof *s)) == 0)
		return (0);

	if (DOffset == 0) {
		DOffset = (sizeof(i32) + HARD_ALIGNMENT - 1) &
			~(HARD_ALIGNMENT - 1);
	}
	dsize += DOffset;	/* tack on space for keys */

	/*
	 * For machines with strict alignment constraints, it may be
	 * necessary to align the data at a multiple of some positive power
	 * of two.  In general, it would suffice to make dsize a power of
	 * two, but this could be very space-wasteful, so instead we align it
	 * to HARD_ALIGNMENT.  64 bit machines might ``#define HARD_ALIGNMENT
	 * 8'', for example.  N.B.:  we assume that HARD_ALIGNMENT is a power
	 * of two.
	 */

	dsize = (dsize + HARD_ALIGNMENT - 1) & ~(HARD_ALIGNMENT - 1);

	s->s_dsize = dsize;	/* save data object size */
	s->s_space = 10;	/* initially, room for 10 objects */
	s->s_n = 0;		/* and none in the table */
	if ((s->s_data = malloc(s->s_space * dsize)) == 0) {
		free((char *) s);
		return (0);
	}
	return (s);
}

/*
 * We actually use a binary search right now - this may change.
 */
char *
SSearch(struct search *s, i32 key, int *disp)
{
	register char *keyaddr;
	int itemstomove;

	*disp &= S_CREATE | S_EXCL;	/* clear return codes */
	if (s->s_n) {		/* look for the key */
		register int h, l, m;

		h = s->s_n - 1;
		l = 0;
		while (l <= h) {
			m = (l + h) >> 1;
			keyaddr = s->s_data + m * s->s_dsize;
			if (*(i32 *) keyaddr > key)
				h = m - 1;
			else if (*(i32 *) keyaddr < key)
				l = m + 1;
			else {	/* found it, now what? */
				if (*disp & S_EXCL) {
					*disp |= S_COLL;
					return (0);	/* collision */
				}
				*disp |= S_FOUND;
				return (keyaddr + DOffset);
			}
		}
		keyaddr = s->s_data + l * s->s_dsize;
	} else
		keyaddr = s->s_data;

	/* keyaddr is now where the key should have been found, if anywhere */
	if ((*disp & S_CREATE) == 0)
		return (0);	/* not found */

	/* avoid using realloc so as to retain old data if out of memory */
	if (s->s_space <= 0) {	/* must expand; double it */
		register char *new;

		if ((new = malloc((s->s_n << 1) * s->s_dsize)) == 0) {
			*disp |= S_ERROR;	/* no space */
			return (0);
		}
		keyaddr = (keyaddr - s->s_data) + new;	/* relocate */
		memmove(new, s->s_data, s->s_n * s->s_dsize);
		free(s->s_data);
		s->s_data = new;
		s->s_space = s->s_n;
	}
	/* now move any keyed data that is beyond keyaddr down */
	itemstomove = s->s_n - (keyaddr - s->s_data) / s->s_dsize;
	if (itemstomove) {
		register char *from, *to;

		from = s->s_data + s->s_n * s->s_dsize;
		to = from + s->s_dsize;
		while (from > keyaddr)
			*--to = *--from;
	}
	*disp |= S_NEW;
	s->s_n++;
	s->s_space--;
	*(i32 *) keyaddr = key;
	keyaddr += DOffset;	/* now actually dataaddr */
	/* the bzero is just a frill... */
	memset(keyaddr, 0, s->s_dsize - DOffset);
	return (keyaddr);
}

/*
 * Call function `f' for each element in the search table `s'.
 */
void
SEnumerate(struct search *s, int (*f)(char *, i32))
{
	register int n;
	register char *p;

	n = s->s_n;
	p = s->s_data;
	while (--n >= 0) {
		(*f)(p + DOffset, *(i32 *) p);
		p += s->s_dsize;
	}
}
char eofmsg[] = "unexpected EOF";

i32
GetByte(FILE *fp)
{
	i32 n;

	n = getc(fp);
	if (feof(fp))
		error(1, 0, eofmsg);
	return Sign8(n);
}
i32
GetWord(FILE *fp)
{
	i32 n;

	fGetWord(fp, n);
	if (feof(fp))
		error(1, 0, eofmsg); \
	return Sign16(n);
}
i32
GetLong(FILE *fp)
{
	i32 n;

	fGetLong(fp, n);
	if (feof(fp))
		error(1, 0, eofmsg); \
	return n;
}

char areyousure[] = "Are you sure this is a DVI file?";

GripeUnexpectedOp(s)
	char *s;
{

	error(0, 0, "unexpected %s", s);
	error(1, 0, areyousure);
	/* NOTREACHED */
}
GripeUndefinedOp(n)
	int n;
{

	error(0, 0, "undefined DVI opcode %d", n);
	error(1, 0, areyousure);
	/* NOTREACHED */
}
GripeMissingOp(s)
	char *s;
{

	error(0, 0, "missing %s", s);
	error(1, 0, areyousure);
	/* NOTREACHED */
}
GripeMismatchedValue(s)
	char *s;
{

	error(0, 0, "mismatched %s", s);
	error(1, 0, areyousure);
	/* NOTREACHED */
}
GripeOutOfMemory(n, why)
	int n;
	char *why;
{

	error(1, errno, "ran out of memory allocating %d bytes for %s",
		n, why);
	/* NOTREACHED */
}

/*
 * Print an error message with an optional system error number, and
 * optionally quit.
 */

void
error(int quit, int e, char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	(void) fflush(stdout);	/* sync error messages */
	(void) fprintf(stderr, "%s: ", ProgName);
	if (e < 0)
		e = errno;
	(void) vfprintf(stderr, fmt, l);
	va_end(l);
	if (e)
		perror("");
	(void) putc('\n', stderr);
	(void) fflush(stderr);	/* just in case */
	if (quit)
		exit(quit);
}

void
panic(char *fmt, ...)
{
	va_list l;

	(void) fflush(stdout);
	(void) fprintf(stderr, "%s: panic: ", ProgName);
	va_start(l, fmt);
	(void) vfprintf(stderr, fmt, l);
	va_end(l);
	(void) putc('\n', stderr);
	(void) fflush(stderr);
	abort();
}
