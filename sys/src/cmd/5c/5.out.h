#define	NSNAME		8
#define	NSYM		50
#define	NREG		16

#define NOPROF		(1<<0)
#define DUPOK		(1<<1)
#define	ALLTHUMBS	(1<<2)

#define	REGRET		0
#define	REGARG		0
/* compiler allocates R1 up as temps */
/* compiler allocates register variables R2 up */
#define	REGMIN		2
#define	REGMAX		8
#define	REGEXT		10
/* compiler allocates external registers R10 down */
#define	REGTMP		11
#define	REGSB		12
#define	REGSP		13
#define	REGLINK		14
#define	REGPC		15

#define	REGTMPT		7	/* used by the loader for thumb code */

#define	NFREG		8
#define	FREGRET		0
#define	FREGEXT		7
#define	FREGTMP		15
/* compiler allocates register variables F0 up */
/* compiler allocates external registers F7 down */

enum	as
{
	AXXX,

	AAND,
	AEOR,
	ASUB,
	ARSB,
	AADD,
	AADC,
	ASBC,
	ARSC,
	ATST,
	ATEQ,
	ACMP,
	ACMN,
	AORR,
	ABIC,

	AMVN,

	AB,
	ABL,

/* 
 * Do not reorder or fragment the conditional branch 
 * opcodes, or the predication code will break 
 */ 
	ABEQ,
	ABNE,
	ABCS,
	ABHS,
	ABCC,
	ABLO,
	ABMI,
	ABPL,
	ABVS,
	ABVC,
	ABHI,
	ABLS,
	ABGE,
	ABLT,
	ABGT,
	ABLE,

	AMOVWD,
	AMOVWF,
	AMOVDW,
	AMOVFW,
	AMOVFD,
	AMOVDF,
	AMOVF,
	AMOVD,

	ACMPF,
	ACMPD,
	AADDF,
	AADDD,
	ASUBF,
	ASUBD,
	AMULF,
	AMULD,
	ADIVF,
	ADIVD,
//	ASQRTF,
//	ASQRTD,

	ASRL,
	ASRA,
	ASLL,
	AMULU,
	ADIVU,
	AMUL,
	ADIV,
	AMOD,
	AMODU,

	AMOVB,
	AMOVBU,
	AMOVH,
	AMOVHU,
	AMOVW,
	AMOVM,
	ASWPBU,
	ASWPW,

	ANOP,
	ARFE,
	ASWI,
	AMULA,

	ADATA,
	AGLOBL,
	AGOK,
	AHISTORY,
	ANAME,
	ARET,
	ATEXT,
	AWORD,
	ADYNT,
	AINIT,
	ABCASE,
	ACASE,

	AEND,

	AMULL,
	AMULAL,
	AMULLU,
	AMULALU,

	ABX,
	ABXRET,
	ADWORD,

	ASIGNAME,

	/* moved here to preserve values of older identifiers */
	ASQRTF,
	ASQRTD,

	ALDREX,
	ASTREX,
	
	ALDREXD,
	ASTREXD,

	ALAST,
};

/* scond byte */
#define	C_SCOND	((1<<4)-1)
#define	C_SBIT	(1<<4)
#define	C_PBIT	(1<<5)
#define	C_WBIT	(1<<6)
#define	C_FBIT	(1<<7)	/* psr flags-only */
#define	C_UBIT	(1<<7)	/* up bit */

/* type/name */
#define	D_GOK	0
#define	D_NONE	1

/* type */
#define	D_BRANCH	(D_NONE+1)
#define	D_OREG		(D_NONE+2)
#define	D_CONST		(D_NONE+7)
#define	D_FCONST	(D_NONE+8)
#define	D_SCONST	(D_NONE+9)
#define	D_PSR		(D_NONE+10)
#define	D_REG		(D_NONE+12)
#define	D_FREG		(D_NONE+13)
#define	D_FILE		(D_NONE+16)
#define	D_OCONST	(D_NONE+17)
#define	D_FILE1		(D_NONE+18)

#define	D_SHIFT		(D_NONE+19)
#define	D_FPCR		(D_NONE+20)
#define	D_REGREG	(D_NONE+21)
#define	D_ADDR		(D_NONE+22)

/* name */
#define	D_EXTERN	(D_NONE+3)
#define	D_STATIC	(D_NONE+4)
#define	D_AUTO		(D_NONE+5)
#define	D_PARAM		(D_NONE+6)

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"

/*
 * this is the simulated IEEE floating point
 */
typedef	struct	ieee	Ieee;
struct	ieee
{
	long	l;	/* contains ls-man	0xffffffff */
	long	h;	/* contains sign	0x80000000
				    exp		0x7ff00000
				    ms-man	0x000fffff */
};
