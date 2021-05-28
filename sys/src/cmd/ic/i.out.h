#define	NSNAME	8
#define	NSYM	50
#define	NREG	32
#define NOPROF	(1<<0)
#define DUPOK	(1<<1)

/*
 * Register roles are influenced by the compressed extension:
 *	CIW, CL, CS and CB format use only R8-R15
 *	CL and CS floating load/store use only F8-F15
 *	CI and CSS load/store assume stack pointer is R2
 *	C.JAL assumes link register is R1
 */
enum
{
	REGZERO		= 0,	/* always zero */
	REGLINK		= 1,	/* call return address */
	REGSP		= 2,	/* stack pointer */
	REGSB		= 3,	/* static base */
	REGTMP		= 4,	/* assembler temporary */
	REGEXT		= 7,	/* extern reg from here down */
	REGRET		= 8,	/* fn return value */
	REGARG		= 8,	/* fn arg value */
	REGALLOC	= 15,	/* highest reg to allocate (allow for RV32E) */

	FREGRET		= 0,	/* fn return value */
	FREGEXT		= 27,	/* extern reg from here down */
	FREGZERO	= 28,
	FREGHALF	= 29,
	FREGONE		= 30,
	FREGTWO		= 31,
};

enum	as
{
	AXXX = 0,

	/* processor instructions */
	AADD,
	AADDW,
	AAMO_D,
	AAMO_W,
	AAND,
	ABEQ,
	ABGE,
	ABGEU,
	ABLT,
	ABLTU,
	ABNE,
	ACSRRC,
	ACSRRCI,
	ACSRRS,
	ACSRRSI,
	ACSRRW,
	ACSRRWI,
	ADIV,
	ADIVU,
	ADIVUW,
	ADIVW,
	AFENCE,
	AFENCE_I,
	AJAL,
	AJALR,
	ALR_D,
	ALR_W,
	ALUI,
	AMOVB,
	AMOVBU,
	AMOVH,
	AMOVHU,
	AMOV,
	AMOVW,
	AMOVWU,
	AMUL,
	AMULH,
	AMULHSU,
	AMULHU,
	AMULW,
	AOR,
	AREM,
	AREMU,
	AREMUW,
	AREMW,
	ASC_D,
	ASC_W,
	ASLL,
	ASLLW,
	ASLT,
	ASLTU,
	ASRA,
	ASRAW,
	ASRL,
	ASRLW,
	ASUB,
	ASUBW,
	ASWAP_D,
	ASWAP_W,
	ASYS,
	AXOR,

	/* floating point */
	AMOVF,      /* FLW, FSW, FSGNJ.S */
	AMOVD,      /* FLD, FSD, FSGNJ.D */
	AMOVFD,     /* FCVT.D.S */
	AMOVDF,     /* FCVT.S.D */
	AMOVWF,     /* FCVT.S.W */
	AMOVUF,     /* FCVT.S.WU */
	AMOVFW,     /* FCVT.W.S */
	AMOVWD,     /* FCVT.D.W */
	AMOVUD,     /* FCVT.D.WU */
	AMOVDW,     /* FCVT.W.D */
	AADDF,      /* FADD.S */
	AADDD,      /* FADD.D */
	ASUBF,      /* FSUB.S */
	ASUBD,      /* FSUB.D */
	AMULF,      /* FMUL.S */
	AMULD,      /* FMUL.D */
	ADIVF,      /* FDIV.S */
	ADIVD,      /* FDIV.D */
	ACMPLTF,     /* FLT.S */
	ACMPLTD,     /* FLT.D */
	ACMPEQF,     /* FEQ.S */
	ACMPEQD,     /* FEQ.D */
	ACMPLEF,     /* FLE.S */
	ACMPLED,     /* FLE.S */

	/* floating point instructions not included */
/*
	FMADD.S    FMADD.D
	FMSUB.S    FMSUB.D
	FNMSUB.S   FNMSUB.D
	FNMADD.S   FNMADD.D
	FSQRT.S    FSQRT.D
	FSGNJ.S    FSGNJ.D
	FSGNJN.S   FSGNJN.D
	FSGNNX.S   FSGNNX.D
	FMIN.S     FMIN.D
	FMAX.S     FMAX.D
	FMV.X.W
	FCLASS.S   FCLASS.D
	FCVT.WU.S  FCVT.WU.D
	FMV.W.X
 */


	/* pseudo-ops */
	ABGT,
	ABGTU,
	ABLE,
	ABLEU,
	ASGT,
	ASGTU,
	AJMP,
	ARET,
	ANOP,

	/* C compiler pseudo-ops */
	ADATA,
	AGLOBL,
	AGOK,
	AHISTORY,
	ANAME,
	ATEXT,
	AWORD,
	AEND,
	ADYNT,
	AINIT,
	ASIGNAME,

	/* RV64 extension */
	ADWORD,
	AMOVFV,
	AMOVDV,
	AMOVVF,
	AMOVUVF,
	AMOVVD,
	AMOVUVD,

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
	D_CTLREG,
	D_FREG,
	D_FCREG,
	D_FILE,
	D_FILE1,
	D_VCONST,
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
