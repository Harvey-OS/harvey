#define	NSNAME	8
#define	NSYM	50
#define	NREG	27	/* r0-r22 + pc + pcsh + +n + -n */
#define	NFREG	4
#define	NCREG	16

enum
{
	REGZERO		= 0,	/* always zero */
	REGSP		= 1,	/* stack pointer */
	REGRET		= 3,	/* return register and first temp, grows++ to REGMAX*/
	REGMAX		= 12,
	REGEXT		= 14,	/* first external register, grows-- to REGMAX+1 */
	REGPTR		= 14,	/* last floating pointer register */
	REGTMP		= 2,	/* used by the loader */
	REGLINK		= 18,	/* subroutine linkage */
	REGARG		= 3,	/* first arg passed in */

	REGPC		= 23,	/* pc */
	REGPCSH		= 24,	/* shadowd pc */
	REGPOS		= 25,	/* +n */
	REGNEG		= 26,	/* -n */
	
	FREGRET		= 0,
	FREGEXT		= 0,	/* no external registers */
	FREGTMP		= 3,	/* used by compiler only */
/*
 * GENERAL:
 *
 * no static base register
 * compiler allocates R2 up as temps
 * compiler allocates external registers R14-R12
 * compiler allocates register variables F0-F3
 * compiler allocates external registers F3 down
 */
};

enum	as
{
	AXXX	= 0,

	AADD,
	AADDCR,
	AADDCRH,
	AADDH,
	AAND,
	AANDH,
	AANDN,
	AANDNH,
	ABIT,
	ABITH,
	ACMP,
	ACMPH,
	ADIV,		/* macro op */
	ADIVL,		/* macro op */
	AMOVB,
	AMOVBU,
	AMOVHB,
	AMOVW,
	AMOVH,
	AMOVHU,
	AMOD,		/* macro op */
	AMODL,		/* macro op */
	AMUL,		/* macro op */
	AOR,
	AORH,
	AROL,
	AROLH,
	AROR,
	ARORH,
	ASLL,
	ASLLH,
	ASRA,
	ASRAH,
	ASRL,
	ASRLH,
	ASUB,
	ASUBH,
	ASUBR,		/* reverse operand order sub */
	ASUBRH,
	AXOR,
	AXORH,

	ABRA,		/* conditional jump */
	ACALL,
	ADATA,
	ADBRA,
	ABMOVW,		/* macro op */
	ADO,
	ADOLOCK,
	ADOEND,		/* end of do/dolock */
	AGLOBL,
	AGOK,
	AHISTORY,
	AIRET,
	AJMP,		/* unconditional jump */
	ANAME,
	ANOP,
	ARETURN,
	ASFTRST,
	ATEXT,
	AWAITI,
	AWORD,

	AFADD,
	AFADDN,
	AFADDT,
	AFADDTN,
	AFDIV,		/* macro op */
	AFDSP,
	AFIFEQ,
	AFIFGT,
	AFIEEE,
	AFIFLT,
	AFMOVF,
	AFMOVFN,
	AFMOVFB,
	AFMOVFW,
	AFMOVFH,
	AFMOVBF,
	AFMOVWF,
	AFMOVHF,
	AFMADD,
	AFMADDN,
	AFMADDT,
	AFMADDTN,
	AFMSUB,
	AFMSUBN,
	AFMSUBT,
	AFMSUBTN,
	AFMUL,
	AFMULN,
	AFMULT,
	AFMULTN,
	AFRND,
	AFSEED,
	AFSUB,
	AFSUBN,
	AFSUBT,
	AFSUBTN,

	AEND
};

/* type/name */
enum
{
	D_GOK	= 0,
	D_NONE,

/* name */
	D_EXTERN,
	D_STATIC,
	D_AUTO,
	D_PARAM,

/* type */
	D_BRANCH,
	D_OREG,			/* for branching only */
	D_NAME,
	D_REG,
	D_INDREG,
	D_INC,
	D_DEC,
	D_INCREG,
	D_CONST,
	D_FCONST,
	D_SCONST,
	D_FREG,
	D_CREG,
	D_FILE,
	D_LINE,
	D_AFCONST,		/* address of floating constant */
	D_FILE1,
};

/* conditions */
enum{
	CCXXX = 0,
	CCTRUE,
	CCFALSE,
	CCEQ,
	CCNE,
	CCGT,
	CCLE,
	CCLT,
	CCGE,
	CCHI,		/* greater unsigned */
	CCLS,		/* less or equal, unsigned */
	CCCC,		/* greater or equal, unsigned */
	CCCS,		/* less then, unsigned */
	CCMI,
	CCPL,
	CCOC,
	CCOS,
	CCFNE,
	CCFEQ,
	CCFGE,
	CCFLT,
	CCFOC,
	CCFOS,
	CCFUC,
	CCFUS,
	CCFGT,
	CCFLE,
	CCIBE,
	CCIBF,
	CCOBE,
	CCOBF,
	CCSYC,
	CCSYS,
	CCFBC,
	CCFBS,
	CCIR0C,
	CCIR0S,
	CCIR1C,
	CCIR1S,
	CCEND
};

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"

/*
 * this is the simulated dsp floating point
 */
enum{
	Dspbits	= 23,			/* bits in a dsp mantissa */
	Dspmask	= (1<<Dspbits) - 1,	/* mask for mantissa */
	Dspbias	= 128,			/* exponent bias */
	Dspexp	= 0xff,			/* mask for eponent */
};
typedef	ulong	Dsp;	/* contains sign	0x80000000
				    man		0x7fffff00
				    exp		0x000000ff */
