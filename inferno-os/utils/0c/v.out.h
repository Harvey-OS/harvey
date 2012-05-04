#define	NSNAME	8
#define	NSYM	50
#define	NREG	32

#define NOPROF	(1<<0)
#define DUPOK	(1<<1)

#define	REGZERO		0
#define	REGRET		1
#define	REGARG		1
/* compiler allocates R1 up as temps */
/* compiler allocates register variables R3-R23 */
#define	REGEXT		25
/* compiler allocates external registers R25 down */
/* dont use R26 R27 */
#define	REGTMP		28
#define	REGSP		29
#define	REGSB		30
#define	REGLINK		31

#define	FREGRET		0
/* compiler allocates register variables F4-F22 */
/* compiler allocates external registers F22 down */
#define	FREGEXT		22
#define	FREGZERO	24	/* both float and double */
#define	FREGHALF	26	/* double */
#define	FREGONE		28	/* double */
#define	FREGTWO		30	/* double */

enum	as
{
	AXXX,

	AABSD,
	AABSF,
	AABSW,
	AADD,
	AADDD,
	AADDF,
	AADDU,
	AADDW,
	AAND,
	ABEQ,
	ABFPF,
	ABFPT,
	ABGEZ,
	ABGEZAL,
	ABGTZ,
	ABLEZ,
	ABLTZ,
	ABLTZAL,
	ABNE,
	ABREAK,
	ACMPEQD,
	ACMPEQF,
	ACMPGED,
	ACMPGEF,
	ACMPGTD,
	ACMPGTF,
	ADATA,
	ADIV,
	ADIVD,
	ADIVF,
	ADIVU,
	ADIVW,
	AGLOBL,
	AGOK,
	AHISTORY,
	AJAL,
	AJMP,
	AMOVB,
	AMOVBU,
	AMOVD,
	AMOVDF,
	AMOVDW,
	AMOVF,
	AMOVFD,
	AMOVFW,
	AMOVH,
	AMOVHU,
	AMOVW,
	AMOVWD,
	AMOVWF,
	AMOVWL,
	AMOVWR,
	AMUL,
	AMULD,
	AMULF,
	AMULU,
	AMULW,
	ANAME,
	ANEGD,
	ANEGF,
	ANEGW,
	ANOP,
	ANOR,
	AOR,
	AREM,
	AREMU,
	ARET,
	ARFE,
	ASGT,
	ASGTU,
	ASLL,
	ASRA,
	ASRL,
	ASUB,
	ASUBD,
	ASUBF,
	ASUBU,
	ASUBW,
	ASYSCALL,
	ATEXT,
	ATLBP,
	ATLBR,
	ATLBWI,
	ATLBWR,
	AWORD,
	AXOR,

	AEND,

	AMOVV,
	AMOVVL,
	AMOVVR,
	ASLLV,
	ASRAV,
	ASRLV,
	ADIVV,
	ADIVVU,
	AREMV,
	AREMVU,
	AMULV,
	AMULVU,
	AADDV,
	AADDVU,
	ASUBV,
	ASUBVU,

	ADYNT,
	AINIT,

	ABCASE,
	ACASE,

	ATRUNCFV,
	ATRUNCDV,
	ATRUNCFW,
	ATRUNCDW,
	AMOVWU,
	AMOVFV,
	AMOVDV,
	AMOVVF,
	AMOVVD,

	ASIGNAME,

	ALAST,
};

/* type/name */
#define	D_GOK	0
#define	D_NONE	1

/* type */
#define	D_BRANCH (D_NONE+1)
#define	D_OREG	(D_NONE+2)
#define	D_EXTERN (D_NONE+3)	/* name */
#define	D_STATIC (D_NONE+4)	/* name */
#define	D_AUTO	(D_NONE+5)	/* name */
#define	D_PARAM	(D_NONE+6)	/* name */
#define	D_CONST	(D_NONE+7)
#define	D_FCONST (D_NONE+8)
#define	D_SCONST (D_NONE+9)
#define	D_HI	(D_NONE+10)
#define	D_LO	(D_NONE+11)
#define	D_REG	(D_NONE+12)
#define	D_FREG	(D_NONE+13)
#define	D_FCREG	(D_NONE+14)
#define	D_MREG	(D_NONE+15)
#define	D_FILE	(D_NONE+16)
#define	D_OCONST (D_NONE+17)
#define	D_FILE1	(D_NONE+18)
#define	D_VCONST (D_NONE+19)

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
