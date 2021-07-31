#define	NSNAME	8
#define	NSYM	50
#define	NREG	32
#define NOPROF	(1<<0)
#define DUPOK	(1<<1)

enum
{
	REGRET		= 0,	/* return register and first temp, grows++ */
	REGARG		= 0,	/* first arg passed in */
	REGEXT		= 15,	/* first external register, grows-- */

	REGLINK		= 26,	/* subroutine linkage */
	REGTMP		= 27,	/* used by the loader */
	REGTMP2		= 28,	/* used by the loader */
	REGSB		= 29,	/* static pointer */
	REGSP		= 30,	/* stack pointer */
	REGZERO		= 31,	/* always zero */

	FREGRET		= 0,
	FREGEXT		= 27,	/* first external register */
	FREGHALF	= 28,	/* double */
	FREGONE		= 29,	/* double */
	FREGTWO		= 30,	/* double */
	FREGZERO	= 31,	/* both float and double */
};

enum	as
{
	AXXX,
	AGOK,
	ATEXT,
	ADATA,
	AGLOBL,
	AHISTORY,
	ANAME,
	AWORD,

	ANOP,

	AMOVL,
	AMOVLU,
	AMOVQ,
	AMOVQU,
	AMOVS,
	AMOVT,

	AMOVB,
	AMOVBU,
	AMOVW,
	AMOVWU,

	AMOVA,
	AMOVAH,

	AMOVLL,
	AMOVQL,
	AMOVLC,
	AMOVQC,

	AMOVQP,
	AMOVLP,

	AADDL,
	AADDLV,
	AADDQ,
	AADDQV,
	AS4ADDL,
	AS4ADDQ,
	AS8ADDL,
	AS8ADDQ,
	AS4SUBL,
	AS4SUBQ,
	AS8SUBL,
	AS8SUBQ,
	ASUBL,
	ASUBLV,
	ASUBQ,
	ASUBQV,

	ACMPEQ,
	ACMPGT,
	ACMPGE,
	ACMPUGT,
	ACMPUGE,
	ACMPBLE,

	AAND,
	AANDNOT,
	AOR,
	AORNOT,
	AXOR,
	AXORNOT,

	ACMOVEQ,
	ACMOVNE,
	ACMOVLT,
	ACMOVGE,
	ACMOVLE,
	ACMOVGT,
	ACMOVLBC,
	ACMOVLBS,

	AMULL,
	AMULQ,
	AMULLV,
	AMULQV,
	AUMULH,
	ADIVQ,
	AMODQ,
	ADIVQU,
	AMODQU,
	ADIVL,
	AMODL,
	ADIVLU,
	AMODLU,

	ASLLQ,
	ASRLQ,
	ASRAQ,
	ASLLL,
	ASRLL,
	ASRAL,

	AEXTBL,
	AEXTWL,
	AEXTLL,
	AEXTQL,
	AEXTWH,
	AEXTLH,
	AEXTQH,

	AINSBL,
	AINSWL,
	AINSLL,
	AINSQL,
	AINSWH,
	AINSLH,
	AINSQH,

	AMSKBL,
	AMSKWL,
	AMSKLL,
	AMSKQL,
	AMSKWH,
	AMSKLH,
	AMSKQH,

	AZAP,
	AZAPNOT,

	AJMP,
	AJSR,
	ARET,

	ABR,
	ABSR,

	ABEQ,
	ABNE,
	ABLT,
	ABGE,
	ABLE,
	ABGT,
	ABLBC,
	ABLBS,

	AFBEQ,
	AFBNE,
	AFBLT,
	AFBGE,
	AFBLE,
	AFBGT,

	ATRAPB,
	AMB,
	AFETCH,
	AFETCHM,
	ARPCC,

	ACPYS,
	ACPYSN,
	ACPYSE,
	ACVTLQ,
	ACVTQL,
	AFCMOVEQ,
	AFCMOVNE,
	AFCMOVLT,
	AFCMOVGE,
	AFCMOVLE,
	AFCMOVGT,

	AADDS,
	AADDT,
	ACMPTEQ,
	ACMPTGT,
	ACMPTGE,
	ACMPTUN,
	ACVTQS,
	ACVTQT,
	ACVTTS,
	ACVTTQ,
	ADIVS,
	ADIVT,
	AMULS,
	AMULT,
	ASUBS,
	ASUBT,

	ACALL_PAL,
	AREI,

	AEND,

	ADYNT,
	AINIT,

	ASIGNAME,

	ALAST,
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
	D_OREG,
	D_CONST,
	D_FCONST,
	D_SCONST,
	D_REG,
	D_FREG,
	D_FCREG,
	D_PREG,
	D_PCC,
	D_FILE,
	D_FILE1,
};

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
