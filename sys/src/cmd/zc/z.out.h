#define	NNAME	20
#define	NSNAME	8
#define	NSYM	50

enum	as
{
	AXXX,
	AADD,
	AADD3,
	AADDI,
	AAND,
	AAND3,
	AANDI,
	ACALL,
	ACALL1,
	ACATCH,
	ACMPEQ,
	ACMPGT,
	ACMPHI,
	ACPU,
	ACRET,
	ADATA,
	ADIV,
	ADIV3,
	ADQM,
	AENTER,
	AFADD,
	AFADD3,
	AFCLASS,
	AFCMPEQ,
	AFCMPEQN,
	AFCMPGE,
	AFCMPGT,
	AFCMPN,
	AFDIV,
	AFDIV3,
	AFLOGB,
	AFLUSHD,
	AFLUSHDCE,
	AFLUSHI,
	AFLUSHP,
	AFLUSHPBE,
	AFLUSHPTE,
	AFMOV,
	AFMUL,
	AFMUL3,
	AFNEXT,
	AFREM,
	AFSCALB,
	AFSQRT,
	AFSUB,
	AFSUB3,
	AGLOBL,
	AGOK,
	AHISTORY,
	AJMP,
	AJMP1,
	AJMPF,
	AJMPF1,
	AJMPFN,
	AJMPFY,
	AJMPT,
	AJMPT1,
	AJMPTN,
	AJMPTY,
	AKCALL,
	AKRET,
	ALDRAA,
	ALONG,
	AMOV,
	AMOVA,
	AMUL,
	AMUL3,
	ANAME,
	ANOP,
	AOR,
	AOR3,
	AORI,
	APOPN,
	AREM,
	AREM3,
	ARETURN,
	ASHL,
	ASHL3,
	ASHR,
	ASHR3,
	ASUB,
	ASUB3,
	ATADD,
	ATESTC,
	ATESTV,
	ATEXT,
	ATSUB,
	AUDIV,
	AUREM,
	AUSHR,
	AUSHR3,
	AWORD,
	AXOR,
	AXOR3,

	AEND
};
enum
{
	D_GOK	= 0,
	D_NONE,
	D_BRANCH,
	D_REG,
	D_EXTERN,
	D_STATIC,
	D_AUTO,
	D_PARAM,
	D_CONST,
	D_FCONST,
	D_SCONST,
	D_ADDR,
	D_CPU,
	D_FILE,
	D_INDIR = 32,
};

enum
{
	W_GOK	= 0,
	W_B,
	W_UB,
	W_H,
	W_UH,
	W_W,
	W_F,
	W_D,
	W_E,
	W_NONE,
};

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"

typedef
struct
{
	union
	{
		long	offset;		/* for calculation */
		char	coffset[4];	/* in file little endian */
	};
	char	name[NNAME];
	char	type;
	char	pad[3];
} Rlent;

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
