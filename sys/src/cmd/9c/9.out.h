#define	NSNAME	8
#define	NSYM	50
#define	NREG	192

#define	NOPROF	(1<<0)
#define	DUPOK	(1<<1)

#define	REGQ		131
#define	REGC		135
#define	FREGRET		68
#define	REGRET		69	/* compiler allocates R70 up as temps */
#define	REGARG		69
#define	REGMAX		96

/* compiler allocates register variables R68-R96 */
#define	REGEXT		100
#define	REGEXTMAX	128
/* compiler allocates external registers R100 up */
#define	REGSB		67
#define	REGTMP		66
#define	REGSP		65
#define	REGLINK		64

#define	FREGZERO	128	/* both float and double */
#define	FREGHALF	130	/* double */
#define	FREGONE		132	/* double */
#define	FREGTWO		134	/* double */

enum	as
{
	AXXX,

	AADDL,
	AADDUL,
	AADDSL,
	AADDCL,
	AADDCUL,
	AADDCSL,
	ASUBL,
	ASUBUL,
	ASUBSL,
	ASUBCL,
	ASUBCUL,
	ASUBCSL,
	AISUBL,
	AISUBUL,
	AISUBSL,
	AISUBCL,
	AISUBCUL,
	AISUBCSL,
	AANDL,
	AANDNL,
	ANANDL,
	ANORL,
	AORL,
	AXNORL,
	AXORL,
	ASRAL,
	ASRLL,
	ASLLL,

	AASEQ,
	AASGE,
	AASGEU,
	AASGT,
	AASGTU,
	AASLE,
	AASLEU,
	AASLT,
	AASLTU,
	AASNEQ,

	ACPEQL,
	ACPGEL,
	ACPGEUL,
	ACPGTL,
	ACPGTUL,
	ACPLEL,
	ACPLEUL,
	ACPLTL,
	ACPLTUL,
	ACPNEQL,

	ALOCKL,
	ALOCKH,
	ALOCKHU,
	ALOCKB,
	ALOCKBU,

	AMOVL,
	AMOVH,
	AMOVHU,
	AMOVB,
	AMOVBU,
	AMOVF,
	AMOVD,
	AMOVFL,
	AMOVDL,
	AMOVLF,
	AMOVLD,
	AMOVFD,
	AMOVDF,
	AMTSR,
	AMFSR,

	ACALL,
	ARET,
	AJMP,
	AJMPF,
	AJMPFDEC,
	AJMPT,

	ADSTEPL,
	ADSTEP0L,
	ADSTEPLL,
	ADSTEPRL,
	ADIVL,
	ADIVUL,

	AMSTEPL,
	AMSTEPUL,
	AMSTEPLL,
	AMULL,
	AMULUL,
	AMULML,
	AMULMUL,

	AADDD,
	ASUBD,
	ADIVD,
	AMULD,
	ASQRTD,
	AEQD,
	AGED,
	AGTD,

	AADDF,
	ASUBF,
	ADIVF,
	AMULF,
	ASQRTF,
	AEQF,
	AGEF,
	AGTF,

	ACLZ,
	ACPBYTE,
	ACLASS,
	AEMULATE,
	AEXBYTE,
	AEXHW,
	AEXHWS,
	AEXTRACT,
	AHALT,
	AINBYTE,
	AINHW,
	AINV,
	AIRET,
	AIRETINV,
	ALOADM,
	ALOADSET,
	ASETIP,
	ASTOREM,
	ACONVERT,

	ADATA,
	AGLOBL,
	AGOK,
	AHISTORY,
	ANAME,
	ATEXT,
	AWORD,
	ANOP,		/* removed */
	ADELAY,		/* delay slot */

	ADYNT,
	AINIT,

	AEND,
	ANOSCHED,
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
	D_FILE,
	D_OCONST,
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
