#define	NSNAME		8
#define	NSYM		50
#define	NREG		16

#define NOPROF		(1<<0)
#define DUPOK		(1<<1)
#define	ALLTHUMBS	(1<<2)

#define	REGRET		0
#define	REGARG		0
/* compiler allocates R1 up as temps */
/* compiler allocates register variables R3 up */
#define	REGEXT		6
/* compiler allocates external registers R5 down */
#define	REGTMPT		7	/* used by the loader - thumb */
#define	REGTMP		11	/* used by the loader - arm */
#define	REGSB		12
#define	REGSP		13
#define	REGLINK		14
#define	REGPC		15
	
#define	NFREG		8
#define	FREGRET		0
#define	FREGEXT		7
/* compiler allocates register variables F0 up */
/* compiler allocates external registers F7 down */

enum	as
{

	AXXX,

	AAND,
	AEOR,
	ASUB,
	ARSB,	// not used
	AADD,
	AADC,
	ASBC,
	ARSC,	// not used
	ATST,
	ATEQ,	// not used
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

	ASRL,	// right logical
	ASRA,	// right arithmetic
	ASLL,	// left logical = left arithmetic
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
	ASWPBU,	// not used
	ASWPW,	// not used

	ANOP,
	ARFE,
	ASWI,
	AMULA,	// not used

	ADATA,
	AGLOBL,
	AGOK,
	AHISTORY,
	ANAME,
	ARET,	// fn return
	ATEXT,	// fn start
	AWORD,
	ADYNT,	// not used
	AINIT,	// not used
	ABCASE,	// not used
	ACASE,	// not used

	AEND,

	AMULL,
	AMULAL,
	AMULLU,
	AMULALU,

	ABX,
	ABXRET,

	ADWORD,

	ASIGNAME,

	ALAST,

};

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

#define	D_SHIFT		(D_NONE+19)	/* not used */
#define	D_FPCR		(D_NONE+20)
#define 	D_REGREG	(D_NONE+21)

/* name */
#define	D_EXTERN	(D_NONE+3)
#define	D_STATIC		(D_NONE+4)
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
