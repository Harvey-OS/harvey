#include	"l.h"

Optab	optab[] =
{
	{ ATEXT,	C_LEXT,	C_NONE,	C_LCON, 	 0, 0, 0 },
	{ ATEXT,	C_LEXT,	C_REG,	C_LCON, 	 0, 0, 0 },
	{ ATEXT,	C_ADDR,	C_NONE,	C_LCON, 	 0, 0, 0 },
	{ ATEXT,	C_ADDR,	C_REG,	C_LCON, 	 0, 0, 0 },

	/* arithmetic operations */
	{ AADD,		C_REG,	C_REG,	C_REG,		 1, 4, 0 },
	{ AADD,		C_REG,	C_NONE,	C_REG,		 1, 4, 0 },
	{ AADC,		C_REG,	C_REG,	C_REG,		 1, 4, 0 },
	{ AADC,		C_REG,	C_NONE,	C_REG,		 1, 4, 0 },
	{ ANEG,		C_REG,	C_NONE,	C_REG,		25, 4, 0 },
	{ ANGC,		C_REG,	C_NONE,	C_REG,		17, 4, 0 },
	{ ACMP,		C_REG,	C_RSP,	C_NONE,		 1, 4, 0 },

	{ AADD,		C_ADDCON,	C_RSP,	C_RSP,		 2, 4, 0 },
	{ AADD,		C_ADDCON,	C_NONE,	C_RSP,		 2, 4, 0 },
	{ ACMP,		C_ADDCON,	C_RSP,	C_NONE,		 2, 4, 0 },

	{ AADD,		C_LCON,	C_REG,	C_REG,		13, 8, 0,	LFROM },
	{ AADD,		C_LCON,	C_NONE,	C_REG,		13, 8, 0,	LFROM },
	{ ACMP,		C_LCON,	C_REG,	C_NONE,		13, 8, 0,	LFROM },

	{ AADD,		C_SHIFT,C_REG,	C_REG,		 3, 4, 0 },
	{ AADD,		C_SHIFT,C_NONE,	C_REG,		 3, 4, 0 },
	{ AMVN,		C_SHIFT,C_NONE,	C_REG,		 3, 4, 0 },
	{ ACMP,		C_SHIFT,C_REG,	C_NONE,		 3, 4, 0 },
	{ ANEG,		C_SHIFT,C_NONE,	C_REG,		26, 4, 0 },

	{ AADD,		C_REG,	C_RSP,	C_RSP,		27, 4, 0 },
	{ AADD,		C_REG,	C_NONE,	C_RSP,		27, 4, 0 },
	{ AADD,		C_EXTREG,C_RSP,	C_RSP,		27, 4, 0 },
	{ AADD,		C_EXTREG,C_NONE,	C_RSP,		27, 4, 0 },
	{ AMVN,		C_EXTREG,C_NONE,	C_RSP,		27, 4, 0 },
	{ ACMP,		C_EXTREG,C_RSP,	C_NONE,		27, 4, 0 },

	{ AADD,		C_REG,	C_REG,	C_REG,		 1, 4, 0 },
	{ AADD,		C_REG,	C_NONE,	C_REG,		 1, 4, 0 },

	/* logical operations */
	{ AAND,		C_REG,	C_REG,	C_REG,		 1, 4, 0 },
	{ AAND,		C_REG,	C_NONE,	C_REG,		 1, 4, 0 },
	{ ABIC,		C_REG,	C_REG,	C_REG,		 1, 4, 0 },
	{ ABIC,		C_REG,	C_NONE,	C_REG,		 1, 4, 0 },

	{ AAND,		C_BITCON,	C_REG,	C_REG,		53, 4, 0 },
	{ AAND,		C_BITCON,	C_NONE,	C_REG,		53, 4, 0 },
	{ ABIC,		C_BITCON,	C_REG,	C_REG,		53, 4, 0 },
	{ ABIC,		C_BITCON,	C_NONE,	C_REG,		53, 4, 0 },

	{ AAND,		C_LCON,	C_REG,	C_REG,		28, 8, 0,	LFROM },
	{ AAND,		C_LCON,	C_NONE,	C_REG,		28, 8, 0,	LFROM },
	{ ABIC,		C_LCON,	C_REG,	C_REG,		28, 8, 0,	LFROM },
	{ ABIC,		C_LCON,	C_NONE,	C_REG,		28, 8, 0,	LFROM },

	{ AAND,		C_SHIFT,C_REG,	C_REG,		 3, 4, 0 },
	{ AAND,		C_SHIFT,C_NONE,	C_REG,		 3, 4, 0 },
	{ ABIC,		C_SHIFT,C_REG,	C_REG,		 3, 4, 0 },
	{ ABIC,		C_SHIFT,C_NONE,	C_REG,		 3, 4, 0 },

	{ AMOV,		C_RSP,	C_NONE,	C_RSP,		24, 4, 0 },
	{ AMVN,		C_REG,	C_NONE,	C_REG,		24, 4, 0 },
	{ AMOVB,		C_REG,	C_NONE,	C_REG,		45, 4, 0 },
	{ AMOVBU,	C_REG,	C_NONE,	C_REG,		45, 4, 0 },
	{ AMOVH,		C_REG,	C_NONE,	C_REG,		45, 4, 0 },	/* also MOVHU */
	{ AMOVW,		C_REG,	C_NONE,	C_REG,		45, 4, 0 },	/* also MOVWU */
	/* TO DO: MVN C_SHIFT */

	/* MOVs that become MOVK/MOVN/MOVZ/ADD/SUB/OR */
	{ AMOVW,		C_MOVCON,	C_NONE,	C_REG,		32, 4, 0 },
	{ AMOV,		C_MOVCON,	C_NONE,	C_REG,		32, 4, 0 },
//	{ AMOVW,		C_ADDCON,	C_NONE,	C_REG,		2, 4, 0 },
//	{ AMOV,		C_ADDCON,	C_NONE,	C_REG,		2, 4, 0 },
//	{ AMOVW,		C_BITCON,	C_NONE,	C_REG,		53, 4, 0 },
//	{ AMOV,		C_BITCON,	C_NONE,	C_REG,		53, 4, 0 },

	{ AMOVK,		C_LCON,	C_NONE,	C_REG,			33, 4, 0 },

	{ AMOV,	C_AECON,C_NONE,	C_REG,		 4, 4, REGSB },
	{ AMOV,	C_AACON,C_NONE,	C_REG,		 4, 4, REGSP },

	{ ASDIV,	C_REG,	C_NONE,	C_REG,		1, 4, 0 },
	{ ASDIV,	C_REG,	C_REG,	C_REG,		1, 4, 0 },

	{ AB,		C_NONE,	C_NONE,	C_SBRA,		 5, 4, 0 },
	{ ABL,	C_NONE,	C_NONE,	C_SBRA,		 5, 4, 0 },

	{ AB,		C_NONE,	C_NONE,	C_ZOREG,	 	6, 4, 0 },
	{ ABL,	C_NONE,	C_NONE,	C_ZOREG,	 	6, 4, 0 },
	{ ARET,	C_NONE,	C_NONE,	C_REG,		6, 4, 0 },
	{ ARET,	C_NONE, C_NONE, C_ZOREG,		6, 4, 0 },

	{ AADRP,	C_SBRA,	C_NONE,	C_REG,		60, 4, 0 },
	{ AADR,	C_SBRA,	C_NONE,	C_REG,		61, 4, 0 },

	{ ABFM,	C_LCON, C_REG, C_REG,		42, 4, 0 },
	{ ABFI,	C_LCON, C_REG, C_REG,		43, 4, 0 },

	{ AEXTR,	C_LCON, C_REG, C_REG,		44, 4, 0 },
	{ ASXTB,	C_REG,	C_NONE,	C_REG,	45, 4, 0 },
	{ ACLS,	C_REG,	C_NONE,	C_REG,	46, 4, 0 },

	{ ABEQ,	C_NONE,	C_NONE,	C_SBRA,		 7, 4, 0 },

	{ ALSL,		C_LCON,	C_REG,	C_REG,		 8, 4, 0 },
	{ ALSL,		C_LCON,	C_NONE,	C_REG,		 8, 4, 0 },

	{ ALSL,		C_REG,	C_NONE,	C_REG,		 9, 4, 0 },
	{ ALSL,		C_REG,	C_REG,	C_REG,		 9, 4, 0 },

	{ ASVC,		C_NONE,	C_NONE,	C_LCON,		10, 4, 0 },
	{ ASVC,		C_NONE,	C_NONE,	C_NONE,		10, 4, 0 },

	{ ADWORD,	C_NONE,	C_NONE,	C_VCON,		11, 8, 0 },
	{ ADWORD,	C_NONE,	C_NONE,	C_LEXT,		11, 8, 0 },
	{ ADWORD,	C_NONE,	C_NONE,	C_ADDR,		11, 8, 0 },

	{ AWORD,	C_NONE,	C_NONE,	C_LCON,		14, 4, 0 },
	{ AWORD,	C_NONE,	C_NONE,	C_LEXT,		14, 4, 0 },
	{ AWORD,	C_NONE,	C_NONE,	C_ADDR,		14, 4, 0 },

	{ AMOVW,	C_LCON,	C_NONE,	C_REG,		12, 4, 0,	LFROM },
	{ AMOV,	C_LCON,	C_NONE,	C_REG,		12, 4, 0,	LFROM },

	{ AMOVW,	C_REG,	C_NONE,	C_ADDR,		64, 8, 0,	LTO },
	{ AMOVB,	C_REG,	C_NONE,	C_ADDR,		64, 8, 0,	LTO },
	{ AMOVBU,	C_REG,	C_NONE,	C_ADDR,		64, 8, 0,	LTO },
	{ AMOVW,	C_ADDR,	C_NONE,	C_REG,		65, 8, 0,	LFROM },
	{ AMOVBU,	C_ADDR,	C_NONE,	C_REG,		65, 8, 0,	LFROM },

	{ AMUL,		C_REG,	C_REG,	C_REG,		15, 4, 0 },
	{ AMUL,		C_REG,	C_NONE,	C_REG,		15, 4, 0 },
	{ AMADD,		C_REG,	C_REG,	C_REG,		15, 4, 0 },

	{ AREM,		C_REG,	C_REG,	C_REG,		16, 8, 0 },
	{ AREM,		C_REG,	C_NONE,	C_REG,		16, 8, 0 },

	{ ACSEL,		C_COND,	C_REG,	C_REG,		18, 4, 0 },	/* from3 optional */
	{ ACSET,		C_COND,	C_NONE,	C_REG,		18, 4, 0 },

	{ ACCMN,		C_COND,	C_REG,	C_LCON,		19, 4, 0 },	/* from3 either C_REG or C_LCON */

	/* scaled 12-bit unsigned displacement store */

	{ AMOVB,	C_REG,	C_NONE,	C_SEXT1,		20, 4, REGSB },  // 
	{ AMOVB,	C_REG,	C_NONE,	C_UAUTO4K,	20, 4, REGSP },  // 
	{ AMOVB,	C_REG,	C_NONE,	C_UOREG4K,		20, 4, 0 },  // 
	{ AMOVBU,	C_REG,	C_NONE,	C_SEXT1,		20, 4, REGSB },  // 
	{ AMOVBU,	C_REG,	C_NONE,	C_UAUTO4K,	20, 4, REGSP },  // 
	{ AMOVBU,	C_REG,	C_NONE,	C_UOREG4K,		20, 4, 0 },  // 

	{ AMOVH,	C_REG,	C_NONE,	C_SEXT2,		20, 4, REGSB },  //
	{ AMOVH,	C_REG,	C_NONE,	C_UAUTO8K,	20,	4, REGSP },	//
	{ AMOVH,	C_REG,	C_NONE,	C_ZOREG,		20, 4, 0 },  // 
	{ AMOVH,	C_REG,	C_NONE,	C_UOREG8K,	20,	4, 0 },	//

	{ AMOVW,	C_REG,	C_NONE,	C_SEXT4,		20, 4, REGSB },  //
	{ AMOVW,	C_REG,	C_NONE,	C_UAUTO16K,	20,	4, REGSP },	//
	{ AMOVW,	C_REG,	C_NONE,	C_ZOREG,		20, 4, 0 },  // 
	{ AMOVW,	C_REG,	C_NONE,	C_UOREG16K,	20,	4, 0 },	//

	/* unscaled 9-bit signed displacement store */
	{ AMOVB,	C_REG,	C_NONE,	C_NSAUTO,	20, 4, REGSP },  // 
	{ AMOVB,	C_REG,	C_NONE,	C_NSOREG,	20, 4, 0 },  // 
	{ AMOVBU,	C_REG,	C_NONE,	C_NSAUTO,	20, 4, REGSP },  // 
	{ AMOVBU,	C_REG,	C_NONE,	C_NSOREG,	20, 4, 0 },  // 

	{ AMOVH,	C_REG,	C_NONE,	C_NSAUTO,	20,	4, REGSP },	//
	{ AMOVH,	C_REG,	C_NONE,	C_NSOREG,	20,	4, 0 },	//
	{ AMOVW,	C_REG,	C_NONE,	C_NSAUTO,	20,	4, REGSP },	//
	{ AMOVW,	C_REG,	C_NONE,	C_NSOREG,	20,	4, 0 },	//

	{ AMOV,	C_REG,	C_NONE,	C_SEXT8,		20, 4, REGSB },
	{ AMOV,	C_REG,	C_NONE,	C_UAUTO32K,	20,	4, REGSP },
	{ AMOV,	C_REG,	C_NONE,	C_ZOREG,		20, 4, 0 },
	{ AMOV,	C_REG,	C_NONE,	C_UOREG32K,	20,	4, 0 },

	{ AMOV,	C_REG,	C_NONE,	C_NSOREG,	20,	4, 0 },	//
	{ AMOV,	C_REG,	C_NONE,	C_NSAUTO,	20,	4, REGSP },	//

	/* short displacement load */

	{ AMOVB,	C_SEXT1,	C_NONE,	C_REG,		21, 4, REGSB },  // 
	{ AMOVB,	C_UAUTO4K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVB,	C_NSAUTO,C_NONE,	C_REG,	21, 4, REGSP },  // 
	{ AMOVB,	C_ZOREG,C_NONE,	C_REG,		21, 4, 0 },  // 
	{ AMOVB,	C_UOREG4K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVB,	C_NSOREG,C_NONE,	C_REG,	21, 4, REGSP },  // 

	{ AMOVBU,	C_SEXT1,	C_NONE,	C_REG,		21, 4, REGSB },  // 
	{ AMOVBU,	C_UAUTO4K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVBU,	C_NSAUTO,C_NONE,	C_REG,	21, 4, REGSP },  // 
	{ AMOVBU,	C_ZOREG,C_NONE,	C_REG,		21, 4, 0 },  // 
	{ AMOVBU,	C_UOREG4K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVBU,	C_NSOREG,C_NONE,	C_REG,	21, 4, REGSP },  // 

	{ AMOVH,	C_SEXT2,	C_NONE,	C_REG,		21, 4, REGSB },  // 
	{ AMOVH,	C_UAUTO8K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVH,	C_NSAUTO,C_NONE,	C_REG,	21, 4, REGSP },  // 
	{ AMOVH,	C_ZOREG,C_NONE,	C_REG,		21, 4, 0 },  // 
	{ AMOVH,	C_UOREG8K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVH,	C_NSOREG,C_NONE,	C_REG,	21, 4, REGSP },  // 

	{ AMOVW,	C_SEXT4,	C_NONE,	C_REG,		21, 4, REGSB },  // 
	{ AMOVW,	C_UAUTO16K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVW,	C_NSAUTO,C_NONE,	C_REG,	21, 4, REGSP },  // 
	{ AMOVW,	C_ZOREG,C_NONE,	C_REG,		21, 4, 0 },  // 
	{ AMOVW,	C_UOREG16K,C_NONE,	C_REG,		21, 4, REGSP },  // 
	{ AMOVW,	C_NSOREG,C_NONE,	C_REG,	21, 4, REGSP },  // 

	{ AMOV,	C_SEXT8,	C_NONE,	C_REG,		21, 4, REGSB },
	{ AMOV,	C_UAUTO32K,C_NONE,	C_REG,		21, 4, REGSP },
	{ AMOV,	C_NSAUTO,C_NONE,	C_REG,	21, 4, REGSP },
	{ AMOV,	C_ZOREG,C_NONE,	C_REG,		21, 4, 0 },
	{ AMOV,	C_UOREG32K,C_NONE,	C_REG,		21, 4, REGSP },
	{ AMOV,	C_NSOREG,C_NONE,	C_REG,	21, 4, REGSP },

	/* long displacement store */
	{ AMOVB,	C_REG,	C_NONE,	C_LEXT,		30, 8, REGSB },  // 
	{ AMOVB,	C_REG,	C_NONE,	C_LAUTO,	30, 8, REGSP },  // 
	{ AMOVB,	C_REG,	C_NONE,	C_LOREG,	30, 8, 0 },  // 
	{ AMOVH,	C_REG,	C_NONE,	C_LEXT,		30, 8, REGSB },  // 
	{ AMOVH,	C_REG,	C_NONE,	C_LAUTO,	30, 8, REGSP },  // 
	{ AMOVH,	C_REG,	C_NONE,	C_LOREG,	30, 8, 0 },  // 
	{ AMOVW,	C_REG,	C_NONE,	C_LEXT,		30, 8, REGSB },  // 
	{ AMOVW,	C_REG,	C_NONE,	C_LAUTO,	30, 8, REGSP },  // 
	{ AMOVW,	C_REG,	C_NONE,	C_LOREG,	30, 8, 0 },  // 
	{ AMOV,	C_REG,	C_NONE,	C_LEXT,		30, 8, REGSB },  // 
	{ AMOV,	C_REG,	C_NONE,	C_LAUTO,	30, 8, REGSP },  // 
	{ AMOV,	C_REG,	C_NONE,	C_LOREG,	30, 8, 0 },  // 

	/* long displacement load */
	{ AMOVB,		C_LEXT,	C_NONE,	C_REG,		31, 8, REGSB },  // 
	{ AMOVB,		C_LAUTO,C_NONE,	C_REG,		31, 8, REGSP },  // 
	{ AMOVB,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },  // 
	{ AMOVB,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },	//
	{ AMOVH,		C_LEXT,	C_NONE,	C_REG,		31, 8, REGSB },  // 
	{ AMOVH,		C_LAUTO,C_NONE,	C_REG,		31, 8, REGSP },  // 
	{ AMOVH,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },  // 
	{ AMOVH,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },	//
	{ AMOVW,		C_LEXT,	C_NONE,	C_REG,		31, 8, REGSB },  // 
	{ AMOVW,		C_LAUTO,C_NONE,	C_REG,		31, 8, REGSP },  // 
	{ AMOVW,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },  // 
	{ AMOVW,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },	//
	{ AMOV,		C_LEXT,	C_NONE,	C_REG,		31, 8, REGSB },  // 
	{ AMOV,		C_LAUTO,C_NONE,	C_REG,		31, 8, REGSP },  // 
	{ AMOV,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },  // 
	{ AMOV,		C_LOREG,C_NONE,	C_REG,		31, 8, 0 },	//

	/* load long effective stack address (load long offset and add) */
	{ AMOV,		C_LACON,C_NONE,	C_REG,		34, 8, REGSP,	LFROM },  //

	/* pre/post-indexed load (unscaled, signed 9-bit offset) */
	{ AMOV,		C_XPOST,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVW, 	C_XPOST,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVH,		C_XPOST,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVB, 		C_XPOST,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVBU, 	C_XPOST,	C_NONE, C_REG,		22, 4, 0 },
	{ AFMOVS, 	C_XPOST,	C_NONE,	C_FREG,	22, 4, 0 },
	{ AFMOVD, 	C_XPOST,	C_NONE,	C_FREG,	22, 4, 0 },

	{ AMOV,		C_XPRE,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVW, 	C_XPRE,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVH,		C_XPRE,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVB, 		C_XPRE,	C_NONE,	C_REG,		22, 4, 0 },
	{ AMOVBU, 	C_XPRE,	C_NONE, C_REG,		22, 4, 0 },
	{ AFMOVS, 	C_XPRE,	C_NONE,	C_FREG,	22, 4, 0 },
	{ AFMOVD, 	C_XPRE,	C_NONE,	C_FREG,	22, 4, 0 },

	/* pre/post-indexed store (unscaled, signed 9-bit offset) */
	{ AMOV,		C_REG,	C_NONE,	C_XPOST,		23, 4, 0 },
	{ AMOVW,		C_REG,	C_NONE,	C_XPOST,		23, 4, 0 },
	{ AMOVH,		C_REG,	C_NONE,	C_XPOST,		23, 4, 0 },
	{ AMOVB,		C_REG,	C_NONE, C_XPOST,		23, 4, 0 },
	{ AMOVBU, 	C_REG,	C_NONE, C_XPOST,		23, 4, 0 },
	{ AFMOVS, 	C_FREG,	C_NONE,	C_XPOST,		23, 4, 0 },
	{ AFMOVD, 	C_FREG,	C_NONE,	C_XPOST,		23, 4, 0 },

	{ AMOV,		C_REG,	C_NONE,	C_XPRE,		23, 4, 0 },
	{ AMOVW,		C_REG,	C_NONE,	C_XPRE,		23, 4, 0 },
	{ AMOVH,		C_REG,	C_NONE,	C_XPRE,		23, 4, 0 },
	{ AMOVB,		C_REG,	C_NONE, C_XPRE,		23, 4, 0 },
	{ AMOVBU, 	C_REG,	C_NONE, C_XPRE,		23, 4, 0 },
	{ AFMOVS, 	C_FREG,	C_NONE,	C_XPRE,		23, 4, 0 },
	{ AFMOVD, 	C_FREG,	C_NONE,	C_XPRE,		23, 4, 0 },

	/* special */
	{ AMOV,		C_SPR,	C_NONE,	C_REG,		35, 4, 0 },
	{ AMRS,		C_SPR,	C_NONE,	C_REG,		35, 4, 0 },

	{ AMOV,		C_REG,	C_NONE,	C_SPR,		36, 4, 0 },
	{ AMSR,		C_REG,	C_NONE,	C_SPR,		36, 4, 0 },

	{ AMOV,		C_LCON,	C_NONE,	C_SPR,		37, 4, 0 },
	{ AMSR,		C_LCON,	C_NONE,	C_SPR,		37, 4, 0 },

	{ AERET,		C_NONE,	C_NONE,	C_NONE,		41, 4, 0 },

	{ AFMOVS,	C_FREG,	C_NONE,	C_SEXT4,		20, 4, REGSB },
	{ AFMOVS,	C_FREG,	C_NONE,	C_UAUTO16K,	20, 4, REGSP },
	{ AFMOVS,	C_FREG,	C_NONE,	C_NSAUTO,	20, 4, REGSP },
	{ AFMOVS,	C_FREG,	C_NONE,	C_ZOREG,		20, 4, 0 },
	{ AFMOVS,	C_FREG,	C_NONE,	C_UOREG16K,	20, 4, 0 },
	{ AFMOVS,	C_FREG,	C_NONE,	C_NSOREG,	20, 4, 0 },

	{ AFMOVD,	C_FREG,	C_NONE,	C_SEXT8,		20, 4, REGSB },
	{ AFMOVD,	C_FREG,	C_NONE,	C_UAUTO32K,	20, 4, REGSP },
	{ AFMOVD,	C_FREG,	C_NONE,	C_NSAUTO,	20, 4, REGSP },
	{ AFMOVD,	C_FREG,	C_NONE,	C_ZOREG,		20, 4, 0 },
	{ AFMOVD,	C_FREG,	C_NONE,	C_UOREG32K,	20, 4, 0 }, 
	{ AFMOVD,	C_FREG,	C_NONE,	C_NSOREG,	20, 4, 0 },

	{ AFMOVS,	C_SEXT4,	C_NONE,	C_FREG,		21, 4, REGSB },
	{ AFMOVS,	C_UAUTO16K,C_NONE,	C_FREG,		21, 4, REGSP },
	{ AFMOVS,	C_NSAUTO,C_NONE,	C_FREG,		21, 4, REGSP },
	{ AFMOVS,	C_ZOREG,C_NONE,	C_FREG,		21, 4, 0 },
	{ AFMOVS,	C_UOREG16K,C_NONE,	C_FREG,		21, 4, 0 },
	{ AFMOVS,	C_NSOREG,C_NONE,	C_FREG,		21, 4, 0 },

	{ AFMOVD,	C_SEXT8,	C_NONE,	C_FREG,		21, 4, REGSB },
	{ AFMOVD,	C_UAUTO32K,C_NONE,	C_FREG,		21, 4, REGSP },
	{ AFMOVD,	C_NSAUTO,C_NONE,	C_FREG,		21, 4, REGSP },
	{ AFMOVD,	C_ZOREG,C_NONE,	C_FREG,		21, 4, 0 },
	{ AFMOVD,	C_UOREG32K,C_NONE,	C_FREG,		21, 4, 0 },
	{ AFMOVD,	C_NSOREG,C_NONE,	C_FREG,		21, 4, 0 },

	{ AFMOVS,	C_FREG,	C_NONE,	C_LEXT,		30, 8, REGSB,	LTO },
	{ AFMOVS,	C_FREG,	C_NONE,	C_LAUTO,	30, 8, REGSP,	LTO },
	{ AFMOVS,	C_FREG,	C_NONE,	C_LOREG,	30, 8, 0,	LTO },

	{ AFMOVS,	C_LEXT,	C_NONE,	C_FREG,		31, 8, REGSB,	LFROM },
	{ AFMOVS,	C_LAUTO,C_NONE,	C_FREG,		31, 8, REGSP,	LFROM },
	{ AFMOVS,	C_LOREG,C_NONE,	C_FREG,		31, 8, 0,	LFROM },

	{ AFMOVS,	C_FREG,	C_NONE,	C_ADDR,		64, 8, 0,	LTO },
	{ AFMOVS,	C_ADDR,	C_NONE,	C_FREG,		65, 8, 0,	LFROM },

	{ AFADDS,		C_FREG,	C_NONE,	C_FREG,		54, 4, 0 },
	{ AFADDS,		C_FREG,	C_REG,	C_FREG,		54, 4, 0 },
	{ AFADDS,		C_FCON,	C_NONE,	C_FREG,		54, 4, 0 },
	{ AFADDS,		C_FCON,	C_REG,	C_FREG,		54, 4, 0 },

	{ AFMOVS,	C_FCON,	C_NONE,	C_FREG,		54, 4, 0 },
	{ AFMOVS,	C_FREG, C_NONE, C_FREG,		54, 4, 0 },
	{ AFMOVD,	C_FCON,	C_NONE,	C_FREG,		54, 4, 0 },
	{ AFMOVD,	C_FREG, C_NONE, C_FREG,		54, 4, 0 },

	{ AFCVTZSD,	C_FREG,	C_NONE,	C_REG,		29, 4, 0 },
	{ ASCVTFD,	C_REG,	C_NONE,	C_FREG,		29, 4, 0 },

	{ AFCMPS,		C_FREG,	C_REG,	C_NONE,		56, 4, 0 },
	{ AFCMPS,		C_FCON,	C_REG,	C_NONE,		56, 4, 0 },

	{ AFCCMPS,	C_COND,	C_REG,	C_LCON,		57, 4, 0 },

	{ AFCSELD,	C_COND,	C_REG,	C_FREG,		18, 4, 0 },

	{ AFCVTSD,	C_FREG,	C_NONE,	C_FREG,		29, 4, 0 },

	{ ACASE,	C_REG,	C_NONE,	C_REG,		62, 4*4, 0 },
	{ ABCASE,	C_NONE, C_NONE, C_SBRA,		63, 4, 0 },

	{ ACLREX,		C_NONE,	C_NONE,	C_LCON,		38, 4, 0 },
	{ ACLREX,		C_NONE,	C_NONE,	C_NONE,		38, 4, 0 },

	{ ACBZ,		C_REG,	C_NONE,	C_SBRA,		39, 4, 0 },
	{ ATBZ,		C_LCON,	C_REG,	C_SBRA,		40, 4, 0 },

	{ ASYS,		C_LCON,	C_NONE,	C_NONE,		50, 4, 0 },
	{ ASYS,		C_LCON,	C_REG,	C_NONE,		50, 4, 0 },
	{ ASYSL,		C_LCON,	C_NONE,	C_REG,		50, 4, 0 },

	{ ADMB,		C_LCON,	C_NONE, 	C_NONE,		51, 4, 0 },
	{ AHINT,		C_LCON,	C_NONE,	C_NONE,		52, 4, 0 },

	{ ALDXR,		C_ZOREG,	C_NONE,	C_REG,		58, 4, 0 },
	{ ALDXP,		C_ZOREG,	C_REG,	C_REG,		58, 4, 0 },
	{ ASTXR,		C_REG,	C_REG,	C_ZOREG,		59, 4, 0 },
	{ ASTXP,		C_REG, C_REG,	C_ZOREG,		59, 4, 0 },

	{ AAESD,	C_VREG,	C_NONE,	C_VREG,	29, 4, 0 },
	{ ASHA1C,	C_VREG,	C_REG,	C_VREG,	1, 4, 0 },

	{ AXXX,		C_NONE,	C_NONE,	C_NONE,		 0, 4, 0 },
};
