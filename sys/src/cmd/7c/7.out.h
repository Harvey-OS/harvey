/*
 * arm64
 */

#define	NSNAME		8
#define	NSYM		50
#define	NREG		32

#define NOPROF		(1<<0)
#define DUPOK		(1<<1)

#define MOSTNEGIMMOFF	(-4096)	/* most negative offset that can be immediate */

#define	REGRET		0
#define	REGARG		0
/* R1 to R7 are potential parameter/return registers */
#define	REGIRL		8	/* indirect result location (TO DO) */
/* compiler allocates R9 up as temps */
/* compiler allocates register variables R10 up */
#define	REGMIN		9
#define	REGMAX		15
#define	REGIP0		16
#define	REGIP1		17
#define	REGTMP		REGIP1
/* compiler allocates external registers R27 down */
#define	REGEXT		27
#define	REGSB		28
#define	REGFP		29
#define	REGLINK		30
#define	REGSP		31
#define	REGZERO		31

#define	NFREG		32
#define	FREGRET		0
#define	FREGMIN		7
#define	FREGEXT		15

/* compiler allocates register variables F0 up */
/* compiler allocates external registers F15 down */

enum	as
{
	AXXX,

	AADC,
	AADCS,
	AADCSW,
	AADCW,
	AADD,
	AADDS,
	AADDSW,
	AADDW,
	AADR,
	AADRP,
	AAND,
	AANDS,
	AANDSW,
	AANDW,
	AASR,
	AASRW,
	AAT,
	AB,
	ABFI,
	ABFIW,
	ABFM,
	ABFMW,
	ABFXIL,
	ABFXILW,
	ABIC,
	ABICS,
	ABICSW,
	ABICW,
	ABL,
	ABRK,
	ACBNZ,
	ACBNZW,
	ACBZ,
	ACBZW,
	ACCMN,
	ACCMNW,
	ACCMP,
	ACCMPW,
	ACINC,
	ACINCW,
	ACINV,
	ACINVW,
	ACLREX,
	ACLS,
	ACLSW,
	ACLZ,
	ACLZW,
	ACMN,
	ACMNW,
	ACMP,
	ACMPW,
	ACNEG,
	ACNEGW,
	ACRC32B,
	ACRC32CB,
	ACRC32CH,
	ACRC32CW,
	ACRC32CX,
	ACRC32H,
	ACRC32W,
	ACRC32X,
	ACSEL,
	ACSELW,
	ACSET,
	ACSETM,
	ACSETMW,
	ACSETW,
	ACSINC,
	ACSINCW,
	ACSINV,
	ACSINVW,
	ACSNEG,
	ACSNEGW,
	ADC,
	ADCPS1,
	ADCPS2,
	ADCPS3,
	ADMB,
	ADRPS,
	ADSB,
	AEON,
	AEONW,
	AEOR,
	AEORW,
	AERET,
	AEXTR,
	AEXTRW,
	AHINT,
	AHLT,
	AHVC,
	AIC,
	AISB,
	ALDAR,
	ALDARB,
	ALDARH,
	ALDARW,
	ALDAXP,
	ALDAXPW,
	ALDAXR,
	ALDAXRB,
	ALDAXRH,
	ALDAXRW,
	ALDXR,
	ALDXRB,
	ALDXRH,
	ALDXRW,
	ALDXP,
	ALDXPW,
	ALSL,
	ALSLW,
	ALSR,
	ALSRW,
	AMADD,
	AMADDW,
	AMNEG,
	AMNEGW,
	AMOVK,
	AMOVKW,
	AMOVN,
	AMOVNW,
	AMOVZ,
	AMOVZW,
	AMRS,
	AMSR,
	AMSUB,
	AMSUBW,
	AMUL,
	AMULW,
	AMVN,
	AMVNW,
	ANEG,
	ANEGS,
	ANEGSW,
	ANEGW,
	ANGC,
	ANGCS,
	ANGCSW,
	ANGCW,
	ANOP,
	AORN,
	AORNW,
	AORR,
	AORRW,
	APRFM,
	APRFUM,
	ARBIT,
	ARBITW,
	AREM,
	AREMW,
	ARET,
	AREV,
	AREV16,
	AREV16W,
	AREV32,
	AREVW,
	AROR,
	ARORW,
	ASBC,
	ASBCS,
	ASBCSW,
	ASBCW,
	ASBFIZ,
	ASBFIZW,
	ASBFM,
	ASBFMW,
	ASBFX,
	ASBFXW,
	ASDIV,
	ASDIVW,
	ASEV,
	ASEVL,
	ASMADDL,
	ASMC,
	ASMNEGL,
	ASMSUBL,
	ASMULH,
	ASMULL,
	ASTXR,
	ASTXRB,
	ASTXRH,
	ASTXP,
	ASTXPW,
	ASTXRW,
	ASTLP,
	ASTLPW,
	ASTLR,
	ASTLRB,
	ASTLRH,
	ASTLRW,
	ASTLXP,
	ASTLXPW,
	ASTLXR,
	ASTLXRB,
	ASTLXRH,
	ASTLXRW,
	ASUB,
	ASUBS,
	ASUBSW,
	ASUBW,
	ASVC,
	ASXTB,
	ASXTBW,
	ASXTH,
	ASXTHW,
	ASXTW,
	ASYS,
	ASYSL,
	ATBNZ,
	ATBZ,
	ATLBI,
	ATST,
	ATSTW,
	AUBFIZ,
	AUBFIZW,
	AUBFM,
	AUBFMW,
	AUBFX,
	AUBFXW,
	AUDIV,
	AUDIVW,
	AUMADDL,
	AUMNEGL,
	AUMSUBL,
	AUMULH,
	AUMULL,
	AUREM,
	AUREMW,
	AUXTB,
	AUXTH,
	AUXTW,
	AUXTBW,
	AUXTHW,
	AWFE,
	AWFI,
	AYIELD,

	AMOVB,
	AMOVBU,
	AMOVH,
	AMOVHU,
	AMOVW,
	AMOVWU,
	AMOV,
	AMOVNP,
	AMOVNPW,
	AMOVP,
	AMOVPD,
	AMOVPQ,
	AMOVPS,
	AMOVPSW,
	AMOVPW,

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

	AFABSD,
	AFABSS,
	AFADDD,
	AFADDS,
	AFCCMPD,
	AFCCMPED,
	AFCCMPS,
	AFCCMPES,
	AFCMPD,
	AFCMPED,
	AFCMPES,
	AFCMPS,
	AFCVTSD,
	AFCVTDS,
	AFCVTZSD,
	AFCVTZSDW,
	AFCVTZSS,
	AFCVTZSSW,
	AFCVTZUD,
	AFCVTZUDW,
	AFCVTZUS,
	AFCVTZUSW,
	AFDIVD,
	AFDIVS,
	AFMOVD,
	AFMOVS,
	AFMULD,
	AFMULS,
	AFNEGD,
	AFNEGS,
	AFSQRTD,
	AFSQRTS,
	AFSUBD,
	AFSUBS,
	ASCVTFD,
	ASCVTFS,
	ASCVTFWD,
	ASCVTFWS,
	AUCVTFD,
	AUCVTFS,
	AUCVTFWD,
	AUCVTFWS,

	ATEXT,
	ADATA,
	AGLOBL,
	AHISTORY,
	ANAME,
	AWORD,
	ADYNT,
	AINIT,
	ABCASE,
	ACASE,
	ADWORD,
	ASIGNAME,
	AGOK,
	ARETURN,
	AEND,

	AFCSELS,
	AFCSELD,
	AFMAXS,
	AFMINS,
	AFMAXD,
	AFMIND,
	AFMAXNMS,
	AFMAXNMD,
	AFNMULS,
	AFNMULD,
	AFRINTNS,
	AFRINTND,
	AFRINTPS,
	AFRINTPD,
	AFRINTMS,
	AFRINTMD,
	AFRINTZS,
	AFRINTZD,
	AFRINTAS,
	AFRINTAD,
	AFRINTXS,
	AFRINTXD,
	AFRINTIS,
	AFRINTID,
	AFMADDS,
	AFMADDD,
	AFMSUBS,
	AFMSUBD,
	AFNMADDS,
	AFNMADDD,
	AFNMSUBS,
	AFNMSUBD,
	AFMINNMS,
	AFMINNMD,
	AFCVTDH,
	AFCVTHS,
	AFCVTHD,
	AFCVTSH,

	AAESD,
	AAESE,
	AAESIMC,
	AAESMC,
	ASHA1C,
	ASHA1H,
	ASHA1M,
	ASHA1P,
	ASHA1SU0,
	ASHA1SU1,
	ASHA256H,
	ASHA256H2,
	ASHA256SU0,
	ASHA256SU1,
	
	ALAST,
};

/* form offset parameter to SYS; special register number */
#define	SYSARG5(op0,op1,Cn,Cm,op2)	((op0)<<19|(op1)<<16|(Cn)<<12|(Cm)<<8|(op2)<<5)
#define	SYSARG4(op1,Cn,Cm,op2)	SYSARG5(0,op1,Cn,Cm,op2)

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
	D_OREG,		/* offset(reg) */
	D_XPRE,		/* offset(reg)! - pre-indexed */
	D_XPOST,		/* (reg)offset! - post-indexed */
	D_CONST,	/* 32-bit constant */
	D_DCONST,	/* 64-bit constant */
	D_FCONST,	/* floating-point constant */
	D_SCONST,	/* short string constant */
	D_REG,		/* Rn = Wn or Xn depending on op */
	D_SP,		/* distinguish REGSP from REGZERO */
	D_FREG,		/* Fn = Sn or Dn depending on op */
	D_VREG,		/* Vn = SIMD register */
	D_SPR,		/* special processor register */
	D_FILE,
	D_OCONST,	/* absolute address constant (unused) */
	D_FILE1,

	D_SHIFT,		/* Rm{, ashift #imm} */
	D_PAIR,		/* pair of gprs */
	D_ADDR,		/* address constant (dynamic loading) */
	D_ADRP,		/* pc-relative addressing, page */
	D_ADRLO,	/* low-order 12 bits of external reference */
	D_EXTREG,	/* Rm{, ext #imm} */
	D_ROFF,		/* register offset Rn+ext(Rm)<<s */
	D_COND,		/* condition EQ, NE, etc */
	D_VLANE,		/* Vn lane */
	D_VSET,		/* set of Vn */

	/* offset iff type is D_SPR */
	D_DAIF	= SYSARG5(3,3,4,2,1),
	D_NZCV	= SYSARG5(3,3,4,2,0),
	D_FPSR	= SYSARG5(3,3,4,4,1),
	D_FPCR	= SYSARG5(3,3,4,4,0),
	D_SPSR_EL1 = SYSARG5(3,0,4,0,0),
	D_ELR_EL1 = SYSARG5(3,0,4,0,1),
	D_SPSR_EL2 = SYSARG5(3,4,4,0,0),
	D_ELR_EL2 = SYSARG5(3,4,4,0,1),
//	D_SPSR_EL3 = SYSARG5(3,x,4,x,x),
//	D_ELR_EL3 = SYSARG5(3,x,4,x,x),
//	D_LR_EL0 = SYSARG5(3,x,4,x,x),
	D_CurrentEL = SYSARG5(3,0,4,2,2),
	D_SP_EL0 = SYSARG5(3,0,4,1,0),
//	D_SP_EL1 = SYSARG5(3,x,4,x,x),
//	D_SP_EL2 = SYSARG5(3,x,4,x,x),
	D_SPSel	= SYSARG5(3,0,4,2,0),
//	D_SPSR_abt  = SYSARG5(3,x,4,x,x),
//	D_SPSR_fiq = SYSARG5(3,x,4,x,x),
//	D_SPSR_ieq = SYSARG5(3,x,4,x,x),
//	D_SPSR_und = SYSARG5(3,x,4,x,x),
	D_DAIFSet = (1<<30)|0,
	D_DAIFClr = (1<<30)|1

};

/*
 * this is the ranlib header
 */
#define	SYMDEF	"__.SYMDEF"

/*
 * this is the simulated IEEE floating point
 */
typedef	struct	Ieee	Ieee;
struct	Ieee
{
	long	l;	/* contains ls-man	0xffffffff */
	long	h;	/* contains sign	0x80000000
				    exp		0x7ff00000
				    ms-man	0x000fffff */
};
