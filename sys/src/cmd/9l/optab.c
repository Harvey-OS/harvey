#include	"l.h"

Optab	optab[] =
{
	{ ATEXT,	C_LEXT,	C_NONE,	C_LCON, 	 0, 0, 0 },
	{ ATEXT,	C_LEXT,	C_REG,	C_LCON, 	 0, 0, 0 },

/*
 * mov $con/reg,reg
 */
	{ AMOVL,	C_REG,	C_NONE,	C_REG,		 1, 4, 0 },
	{ AMOVL,	C_PCON,	C_NONE,	C_REG,		 2, 4, 0 },
	{ AMOVL,	C_NCON,	C_NONE,	C_REG,		 2, 4, 0 },
	{ AMOVL,	C_LCON,	C_NONE,	C_REG,		 3, 8, 0 },
	{ AMOVL,	C_SACON,C_NONE,	C_REG,		 4, 4, 0 },
	{ AMOVL,	C_MACON,C_NONE,	C_REG,		 4, 4, 0 },
	{ AMOVL,	C_PACON,C_NONE,	C_REG,		 5, 8, 0 },
	{ AMOVL,	C_NACON,C_NONE,	C_REG,		 5, 8, 0 },
	{ AMOVL,	C_LACON,C_NONE,	C_REG,		 6, 12, 0 },

/*
 * mov reg,mem
 */
	{ AMOVL,	C_REG,	C_NONE,	C_ZAUTO,	10, 4, REGSP },
	{ AMOVL,	C_REG,	C_NONE,	C_ZOREG,	10, 4, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_SAUTO,	11, 8, REGSP },
	{ AMOVL,	C_REG,	C_NONE,	C_SOREG,	11, 8, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_MAUTO,	11, 8, REGSP },
	{ AMOVL,	C_REG,	C_NONE,	C_MOREG,	11, 8, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_PAUTO,	12, 12, REGSP },
	{ AMOVL,	C_REG,	C_NONE,	C_POREG,	12, 12, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_NAUTO,	12, 12, REGSP },
	{ AMOVL,	C_REG,	C_NONE,	C_NOREG,	12, 12, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_LAUTO,	13, 16, REGSP },
	{ AMOVL,	C_REG,	C_NONE,	C_LOREG,	13, 16, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_SEXT,		14, 8, 0 },
	{ AMOVL,	C_REG,	C_NONE,	C_LEXT,		15, 12, 0 },

	{ ASTOREM,	C_REG,	C_NONE,	C_ZOREG,	10, 4, 0 },
/*
 * mov mem,reg
 */
	{ AMOVL,	C_ZAUTO,C_NONE,	C_REG,		20, 4, REGSP },
	{ AMOVL,	C_ZOREG,C_NONE,	C_REG,		20, 4, 0 },
	{ AMOVL,	C_SAUTO,C_NONE,	C_REG,		21, 8, REGSP },
	{ AMOVL,	C_SOREG,C_NONE,	C_REG,		21, 8, 0 },
	{ AMOVL,	C_MAUTO,C_NONE,	C_REG,		21, 8, REGSP },
	{ AMOVL,	C_MOREG,C_NONE,	C_REG,		21, 8, 0 },
	{ AMOVL,	C_PAUTO,C_NONE,	C_REG,		22, 12, REGSP },
	{ AMOVL,	C_POREG,C_NONE,	C_REG,		22, 12, 0 },
	{ AMOVL,	C_NAUTO,C_NONE,	C_REG,		22, 12, REGSP },
	{ AMOVL,	C_NOREG,C_NONE,	C_REG,		22, 12, 0 },
	{ AMOVL,	C_LAUTO,C_NONE,	C_REG,		23, 16, REGSP },
	{ AMOVL,	C_LOREG,C_NONE,	C_REG,		23, 16, 0 },
	{ AMOVL,	C_SEXT,	C_NONE,	C_REG,		24, 8, 0 },
	{ AMOVL,	C_LEXT,	C_NONE,	C_REG,		25, 12, 0 },

	{ ALOADM,	C_ZOREG,C_NONE,	C_REG,		20, 4, 0 },

/*
 * movd reg,mem
 */
	{ AMOVD,	C_REG,	C_NONE,	C_ZAUTO,	60, 8, REGSP },
	{ AMOVD,	C_REG,	C_NONE,	C_ZOREG,	60, 8, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_SAUTO,	61, 12, REGSP },
	{ AMOVD,	C_REG,	C_NONE,	C_SOREG,	61, 12, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_MAUTO,	61, 12, REGSP },
	{ AMOVD,	C_REG,	C_NONE,	C_MOREG,	61, 12, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_PAUTO,	62, 16, REGSP },
	{ AMOVD,	C_REG,	C_NONE,	C_POREG,	62, 16, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_NAUTO,	62, 16, REGSP },
	{ AMOVD,	C_REG,	C_NONE,	C_NOREG,	62, 16, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_LAUTO,	63, 20, REGSP },
	{ AMOVD,	C_REG,	C_NONE,	C_LOREG,	63, 20, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_SEXT,		64, 12, 0 },
	{ AMOVD,	C_REG,	C_NONE,	C_LEXT,		65, 16, 0 },
/*
 * movd mem,reg
 */
	{ AMOVD,	C_ZAUTO,C_NONE,	C_REG,		70, 8, REGSP },
	{ AMOVD,	C_ZOREG,C_NONE,	C_REG,		70, 8, 0 },
	{ AMOVD,	C_SAUTO,C_NONE,	C_REG,		71, 12, REGSP },
	{ AMOVD,	C_SOREG,C_NONE,	C_REG,		71, 12, 0 },
	{ AMOVD,	C_MAUTO,C_NONE,	C_REG,		71, 12, REGSP },
	{ AMOVD,	C_MOREG,C_NONE,	C_REG,		71, 12, 0 },
	{ AMOVD,	C_PAUTO,C_NONE,	C_REG,		72, 16, REGSP },
	{ AMOVD,	C_POREG,C_NONE,	C_REG,		72, 16, 0 },
	{ AMOVD,	C_NAUTO,C_NONE,	C_REG,		72, 16, REGSP },
	{ AMOVD,	C_NOREG,C_NONE,	C_REG,		72, 16, 0 },
	{ AMOVD,	C_LAUTO,C_NONE,	C_REG,		73, 20, REGSP },
	{ AMOVD,	C_LOREG,C_NONE,	C_REG,		73, 20, 0 },
	{ AMOVD,	C_SEXT,	C_NONE,	C_REG,		74, 12, 0 },
	{ AMOVD,	C_LEXT,	C_NONE,	C_REG,		75, 16, 0 },
/*
 * movd mem,reg
 */
	{ AMOVD,	C_REG,C_NONE,	C_REG,		76, 4, 0 },
/*
 * op $con/reg,reg,reg
 */
	{ ASETIP,	C_REG,	C_REG,	C_REG,		30, 4, 0 },
	{ ASETIP,	C_REG,	C_NONE,	C_REG,		30, 4, 0 },
	{ AADDL,	C_REG,	C_REG,	C_REG,		30, 4, 0 },
	{ AADDL,	C_REG,	C_NONE,	C_REG,		30, 4, 0 },
	{ AADDL,	C_SCON,	C_REG,	C_REG,		31, 4, 0 },
	{ AADDL,	C_SCON,	C_NONE,	C_REG,		31, 4, 0 },
	{ AADDL,	C_PCON,	C_REG,	C_REG,		32, 8, 0 },
	{ AADDL,	C_PCON,	C_NONE,	C_REG,		32, 8, 0 },
	{ AADDL,	C_NCON,	C_REG,	C_REG,		32, 8, 0 },
	{ AADDL,	C_NCON,	C_NONE,	C_REG,		32, 8, 0 },
	{ AADDL,	C_LCON,	C_REG,	C_REG,		33,12, 0 },
	{ AADDL,	C_LCON,	C_NONE,	C_REG,		33,12, 0 },
/*
 * fop reg,reg,reg
 */
	{ AADDD,	C_REG,	C_REG,	C_REG,		30, 4, 0 },
	{ AADDD,	C_REG,	C_NONE,	C_REG,		30, 4, 0 },
/*
 * jump/call
 */
	{ AJMP,		C_NONE,	C_NONE,	C_SBRA,		40, 4, 0 },
	{ ACALL,	C_NONE,	C_NONE,	C_SBRA,		40, 4, REGLINK },
	{ ACALL,	C_REG,	C_NONE,	C_SBRA,		40, 4, 0 },

	{ AJMPF,	C_REG,	C_NONE,	C_SBRA,		41, 4, 0 },

	{ AJMP,		C_NONE,	C_NONE,	C_LBRA,		42, 12, 0 },
	{ AJMPF,	C_REG,	C_NONE,	C_LBRA,		42, 12, 0 },
	{ ACALL,	C_NONE,	C_NONE,	C_LBRA,		42, 12, REGLINK },
	{ ACALL,	C_REG,	C_NONE,	C_LBRA,		42, 12, 0 },

	{ AJMP,		C_NONE,	C_NONE,	C_ZOREG,	43, 4, 0 },
	{ AJMPF,	C_REG,	C_NONE,	C_ZOREG,	43, 4, 0 },
	{ ACALL,	C_NONE,	C_NONE,	C_ZOREG,	43, 4, REGLINK },
	{ ACALL,	C_REG,	C_NONE,	C_ZOREG,	43, 4, 0 },

/*
 * special
 */
	{ ADELAY,	C_NONE,	C_NONE,	C_NONE,		50, 4, 0 },
	{ AEMULATE,	C_NONE,	C_NONE,	C_ZOREG,	51, 4, 0 },
	{ AMTSR,	C_REG,	C_NONE,	C_REG,		52, 4, 0 },
	{ AMTSR,	C_PCON,	C_NONE,	C_REG,		53, 4, 0 },
	{ AMTSR,	C_LCON,	C_NONE,	C_REG,		54, 12, 0 },
	{ AMFSR,	C_REG,	C_NONE,	C_REG,		55, 4, 0 },
	{ AIRET,	C_NONE,	C_NONE,	C_NONE,		56, 4, 0 },
	{ AINV,		C_NONE,	C_NONE,	C_SCON,		57, 4, 0 },
	{ AWORD,	C_NONE,	C_NONE,	C_LCON,		66, 4, 0 },

	{ AXXX,		C_NONE,	C_NONE,	C_NONE,		 0, 4, 0 },
};
