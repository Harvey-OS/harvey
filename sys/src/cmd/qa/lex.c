#define	EXTERN
#include "a.h"
#include "y.tab.h"
#include <ctype.h>

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = 'q';
	thestring = "power";
	memset(debug, 0, sizeof(debug));
	cinit();
	outfile = 0;
	include[ninclude++] = ".";
	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 || c < sizeof(debug))
			debug[c] = 1;
		break;

	case 'o':
		outfile = ARGF();
		break;

	case 'D':
		p = ARGF();
		if(p)
			Dlist[nDlist++] = p;
		break;

	case 'I':
		p = ARGF();
		setinclude(p);
		break;
	} ARGEND
	if(*argv == 0) {
		print("usage: %ca [-options] file.s\n", thechar);
		errorexit();
	}
	if(argc > 1 && systemtype(Windows)){
		print("can't assemble multiple files on windows\n");
		errorexit();
	}
	if(argc > 1) {
		nproc = 1;
		if(p = getenv("NPROC"))
			nproc = atol(p);
		c = 0;
		nout = 0;
		for(;;) {
			while(nout < nproc && argc > 0) {
				i = myfork();
				if(i < 0) {
					i = mywait(&status);
					if(i < 0)
						errorexit();
					if(status)
						c++;
					nout--;
					continue;
				}
				if(i == 0) {
					print("%s:\n", *argv);
					if(assemble(*argv))
						errorexit();
					exits(0);
				}
				nout++;
				argc--;
				argv++;
			}
			i = mywait(&status);
			if(i < 0) {
				if(c)
					errorexit();
				exits(0);
			}
			if(status)
				c++;
			nout--;
		}
	}
	if(assemble(argv[0]))
		errorexit();
	exits(0);
}

int
assemble(char *file)
{
	char ofile[100], incfile[20], *p;
	int i, of;

	strcpy(ofile, file);
	if(p = strrchr(ofile, pathchar())) {
		include[0] = ofile;
		*p++ = 0;
	} else
		p = ofile;
	if(outfile == 0) {
		outfile = p;
		if(p = strrchr(outfile, '.'))
			if(p[1] == 's' && p[2] == 0)
				p[0] = 0;
		p = strrchr(outfile, 0);
		p[0] = '.';
		p[1] = thechar;
		p[2] = 0;
	}
	p = getenv("INCLUDE");
	if(p) {
		setinclude(p);
	} else {
		if(systemtype(Plan9)) {
			sprint(incfile,"/%s/include", thestring);
			setinclude(strdup(incfile));
		}
	}

	of = mycreat(outfile, 0664);
	if(of < 0) {
		yyerror("%ca: cannot create %s", thechar, outfile);
		errorexit();
	}
	Binit(&obuf, of, OWRITE);

	pass = 1;
	nosched = 0;
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	if(nerrors) {
		cclean();
		return nerrors;
	}

	pass = 2;
	nosched = 0;
	outhist();
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	cclean();
	return nerrors;
}

struct
{
	char	*name;
	ushort	type;
	ushort	value;
} itab[] =
{
	"SP",		LSP,	D_AUTO,
	"SB",		LSB,	D_EXTERN,
	"FP",		LFP,	D_PARAM,
	"PC",		LPC,	D_BRANCH,

	"LR",		LLR,	D_LR,
	"CTR",		LCTR,	D_CTR,

	"XER",		LSPREG,	D_XER,
	"MSR",		LMSR,	D_MSR,
	"FPSCR",	LFPSCR,	D_FPSCR,
	"SPR",		LSPR,	D_SPR,
	"DCR",		LDCR,	D_DCR,

	"SEG",		LSEG,	D_SREG,

	"CR",		LCR,	0,
	"CR0",		LCREG,	0,
	"CR1",		LCREG,	1,
	"CR2",		LCREG,	2,
	"CR3",		LCREG,	3,
	"CR4",		LCREG,	4,
	"CR5",		LCREG,	5,
	"CR6",		LCREG,	6,
	"CR7",		LCREG,	7,

	"R",		LR,	0,
	"R0",		LREG,	0,
	"R1",		LREG,	1,
	"R2",		LREG,	2,
	"R3",		LREG,	3,
	"R4",		LREG,	4,
	"R5",		LREG,	5,
	"R6",		LREG,	6,
	"R7",		LREG,	7,
	"R8",		LREG,	8,
	"R9",		LREG,	9,
	"R10",		LREG,	10,
	"R11",		LREG,	11,
	"R12",		LREG,	12,
	"R13",		LREG,	13,
	"R14",		LREG,	14,
	"R15",		LREG,	15,
	"R16",		LREG,	16,
	"R17",		LREG,	17,
	"R18",		LREG,	18,
	"R19",		LREG,	19,
	"R20",		LREG,	20,
	"R21",		LREG,	21,
	"R22",		LREG,	22,
	"R23",		LREG,	23,
	"R24",		LREG,	24,
	"R25",		LREG,	25,
	"R26",		LREG,	26,
	"R27",		LREG,	27,
	"R28",		LREG,	28,
	"R29",		LREG,	29,
	"R30",		LREG,	30,
	"R31",		LREG,	31,

	"F",		LF,	0,
	"F0",		LFREG,	0,
	"F1",		LFREG,	1,
	"F2",		LFREG,	2,
	"F3",		LFREG,	3,
	"F4",		LFREG,	4,
	"F5",		LFREG,	5,
	"F6",		LFREG,	6,
	"F7",		LFREG,	7,
	"F8",		LFREG,	8,
	"F9",		LFREG,	9,
	"F10",		LFREG,	10,
	"F11",		LFREG,	11,
	"F12",		LFREG,	12,
	"F13",		LFREG,	13,
	"F14",		LFREG,	14,
	"F15",		LFREG,	15,
	"F16",		LFREG,	16,
	"F17",		LFREG,	17,
	"F18",		LFREG,	18,
	"F19",		LFREG,	19,
	"F20",		LFREG,	20,
	"F21",		LFREG,	21,
	"F22",		LFREG,	22,
	"F23",		LFREG,	23,
	"F24",		LFREG,	24,
	"F25",		LFREG,	25,
	"F26",		LFREG,	26,
	"F27",		LFREG,	27,
	"F28",		LFREG,	28,
	"F29",		LFREG,	29,
	"F30",		LFREG,	30,
	"F31",		LFREG,	31,

	"CREQV",	LCROP, ACREQV,
	"CRXOR",	LCROP, ACRXOR,
	"CRAND",	LCROP, ACRAND,
	"CROR",		LCROP, ACROR,
	"CRANDN",	LCROP, ACRANDN,
	"CRORN",	LCROP, ACRORN,
	"CRNAND",	LCROP, ACRNAND,
	"CRNOR",	LCROP, ACRNOR,

	"ADD",		LADDW, AADD,
	"ADDV",		LADDW, AADDV,
	"ADDCC",	LADDW, AADDCC,
	"ADDVCC",	LADDW, AADDVCC,
	"ADDC",		LADDW, AADDC,
	"ADDCV",	LADDW, AADDCV,
	"ADDCCC",	LADDW, AADDCCC,
	"ADDCVCC",	LADDW, AADDCVCC,
	"ADDE",		LLOGW, AADDE,
	"ADDEV",	LLOGW, AADDEV,
	"ADDECC",	LLOGW, AADDECC,
	"ADDEVCC",	LLOGW, AADDEVCC,

	"ADDME",	LABS, AADDME,
	"ADDMEV",	LABS, AADDMEV,
	"ADDMECC",	LABS, AADDMECC,
	"ADDMEVCC",	LABS, AADDMEVCC,
	"ADDZE",	LABS, AADDZE,
	"ADDZEV",	LABS, AADDZEV,
	"ADDZECC",	LABS, AADDZECC,
	"ADDZEVCC",	LABS, AADDZEVCC,

	"SUB",		LADDW, ASUB,
	"SUBV",		LADDW, ASUBV,
	"SUBCC",	LADDW, ASUBCC,
	"SUBVCC",	LADDW, ASUBVCC,
	"SUBE",		LLOGW, ASUBE,
	"SUBECC",	LLOGW, ASUBECC,
	"SUBEV",	LLOGW, ASUBEV,
	"SUBEVCC",	LLOGW, ASUBEVCC,
	"SUBC",		LADDW, ASUBC,
	"SUBCCC",	LADDW, ASUBCCC,
	"SUBCV",	LADDW, ASUBCV,
	"SUBCVCC",	LADDW, ASUBCVCC,

	"SUBME",	LABS, ASUBME,
	"SUBMEV",	LABS, ASUBMEV,
	"SUBMECC",	LABS, ASUBMECC,
	"SUBMEVCC",	LABS, ASUBMEVCC,
	"SUBZE",	LABS, ASUBZE,
	"SUBZEV",	LABS, ASUBZEV,
	"SUBZECC",	LABS, ASUBZECC,
	"SUBZEVCC",	LABS, ASUBZEVCC,

	"AND",		LADDW, AAND,
	"ANDCC",	LADDW, AANDCC,	/* includes andil & andiu */
	"ANDN",		LLOGW, AANDN,
	"ANDNCC",	LLOGW, AANDNCC,
	"EQV",		LLOGW, AEQV,
	"EQVCC",	LLOGW, AEQVCC,
	"NAND",		LLOGW, ANAND,
	"NANDCC",	LLOGW, ANANDCC,
	"NOR",		LLOGW, ANOR,
	"NORCC",	LLOGW, ANORCC,
	"OR",		LADDW, AOR,	/* includes oril & oriu */
	"ORCC",		LADDW, AORCC,
	"ORN",		LLOGW, AORN,
	"ORNCC",	LLOGW, AORNCC,
	"XOR",		LADDW, AXOR,	/* includes xoril & xoriu */
	"XORCC",	LLOGW, AXORCC,

	"EXTSB",	LABS,	AEXTSB,
	"EXTSBCC",	LABS,	AEXTSBCC,
	"EXTSH",	LABS, AEXTSH,
	"EXTSHCC",	LABS, AEXTSHCC,

	"CNTLZW",	LABS, ACNTLZW,
	"CNTLZWCC",	LABS, ACNTLZWCC,

	"RLWMI",	LRLWM, ARLWMI,
	"RLWMICC",	LRLWM, ARLWMICC,
	"RLWNM",	LRLWM, ARLWNM,
	"RLWNMCC", LRLWM, ARLWNMCC,

	"SLW",		LSHW, ASLW,
	"SLWCC",	LSHW, ASLWCC,
	"SRW",		LSHW, ASRW,
	"SRWCC",	LSHW, ASRWCC,
	"SRAW",		LSHW, ASRAW,
	"SRAWCC",	LSHW, ASRAWCC,

	"BR",		LBRA, ABR,
	"BC",		LBRA, ABC,
	"BCL",		LBRA, ABC,
	"BL",		LBRA, ABL,
	"BEQ",		LBRA, ABEQ,
	"BNE",		LBRA, ABNE,
	"BGT",		LBRA, ABGT,
	"BGE",		LBRA, ABGE,
	"BLT",		LBRA, ABLT,
	"BLE",		LBRA, ABLE,
	"BVC",		LBRA, ABVC,
	"BVS",		LBRA, ABVS,

	"CMP",		LCMP, ACMP,
	"CMPU",		LCMP, ACMPU,

	"DIVW",		LLOGW, ADIVW,
	"DIVWV",	LLOGW, ADIVWV,
	"DIVWCC",	LLOGW, ADIVWCC,
	"DIVWVCC",	LLOGW, ADIVWVCC,
	"DIVWU",	LLOGW, ADIVWU,
	"DIVWUV",	LLOGW, ADIVWUV,
	"DIVWUCC",	LLOGW, ADIVWUCC,
	"DIVWUVCC",	LLOGW, ADIVWUVCC,

	"FABS",		LFCONV,	AFABS,
	"FABSCC",	LFCONV,	AFABSCC,
	"FNEG",		LFCONV,	AFNEG,
	"FNEGCC",	LFCONV,	AFNEGCC,
	"FNABS",	LFCONV,	AFNABS,
	"FNABSCC",	LFCONV,	AFNABSCC,

	"FADD",		LFADD,	AFADD,
	"FADDCC",	LFADD,	AFADDCC,
	"FSUB",		LFADD,  AFSUB,
	"FSUBCC",	LFADD,	AFSUBCC,
	"FMUL",		LFADD,	AFMUL,
	"FMULCC",	LFADD,	AFMULCC,
	"FDIV",		LFADD,	AFDIV,
	"FDIVCC",	LFADD,	AFDIVCC,
	"FRSP",		LFCONV,	AFRSP,
	"FRSPCC",	LFCONV,	AFRSPCC,

	"FMADD",	LFMA, AFMADD,
	"FMADDCC",	LFMA, AFMADDCC,
	"FMSUB",	LFMA, AFMSUB,
	"FMSUBCC",	LFMA, AFMSUBCC,
	"FNMADD",	LFMA, AFNMADD,
	"FNMADDCC",	LFMA, AFNMADDCC,
	"FNMSUB",	LFMA, AFNMSUB,
	"FNMSUBCC",	LFMA, AFNMSUBCC,
	"FMADDS",	LFMA, AFMADDS,
	"FMADDSCC",	LFMA, AFMADDSCC,
	"FMSUBS",	LFMA, AFMSUBS,
	"FMSUBSCC",	LFMA, AFMSUBSCC,
	"FNMADDS",	LFMA, AFNMADDS,
	"FNMADDSCC",	LFMA, AFNMADDSCC,
	"FNMSUBS",	LFMA, AFNMSUBS,
	"FNMSUBSCC",	LFMA, AFNMSUBSCC,

	"FCMPU",	LFCMP, AFCMPU,
	"FCMPO",	LFCMP, AFCMPO,
	"MTFSB0",	LMTFSB, AMTFSB0,
	"MTFSB1",	LMTFSB,	AMTFSB1,

	"FMOVD",	LFMOV, AFMOVD,
	"FMOVS",	LFMOV, AFMOVS,
	"FMOVDCC",	LFCONV,	AFMOVDCC,	/* fmr. */

	"GLOBL",	LTEXT, AGLOBL,

	"MOVB",		LMOVB, AMOVB,
	"MOVBZ",	LMOVB, AMOVBZ,
	"MOVBU",	LMOVB, AMOVBU,
	"MOVBZU", LMOVB, AMOVBZU,
	"MOVH",		LMOVB, AMOVH,
	"MOVHZ",	LMOVB, AMOVHZ,
	"MOVHU",	LMOVB, AMOVHU,
	"MOVHZU", LMOVB, AMOVHZU,
	"MOVHBR", 	LXMV, AMOVHBR,
	"MOVWBR",	LXMV, AMOVWBR,
	"MOVW",		LMOVW, AMOVW,
	"MOVWU",	LMOVW, AMOVWU,
	"MOVMW",	LMOVMW, AMOVMW,
	"MOVFL",	LMOVW,	AMOVFL,

	"MULLW",	LADDW, AMULLW,		/* includes multiply immediate 10-139 */
	"MULLWV",	LLOGW, AMULLWV,
	"MULLWCC",	LLOGW, AMULLWCC,
	"MULLWVCC",	LLOGW, AMULLWVCC,

	"MULHW",	LLOGW, AMULHW,
	"MULHWCC",	LLOGW, AMULHWCC,
	"MULHWU",	LLOGW, AMULHWU,
	"MULHWUCC",	LLOGW, AMULHWUCC,

	"NEG",		LABS, ANEG,
	"NEGV",		LABS, ANEGV,
	"NEGCC",	LABS, ANEGCC,
	"NEGVCC",	LABS, ANEGVCC,

	"NOP",		LNOP, ANOP,	/* ori 0,0,0 */
	"SYSCALL",	LNOP, ASYSCALL,

	"RETURN",	LRETRN, ARETURN,
	"RFI",		LRETRN,	ARFI,
	"RFCI",		LRETRN,	ARFCI,

	"DATA",		LDATA, ADATA,
	"END",		LEND, AEND,
	"TEXT",		LTEXT, ATEXT,

	/* IBM powerpc embedded  */
	"MACCHW", LMA, AMACCHW,
	"MACCHWCC", LMA, AMACCHWCC,
	"MACCHWS", LMA, AMACCHWS,
	"MACCHWSCC", LMA, AMACCHWSCC,
	"MACCHWSU", LMA, AMACCHWSU,
	"MACCHWSUCC", LMA, AMACCHWSUCC,
	"MACCHWSUV", LMA, AMACCHWSUV,
	"MACCHWSUVCC", LMA, AMACCHWSUVCC,
	"MACCHWSV", LMA, AMACCHWSV,
	"MACCHWSVCC", LMA, AMACCHWSVCC,
	"MACCHWU", LMA, AMACCHWU,
	"MACCHWUCC", LMA, AMACCHWUCC,
	"MACCHWUV", LMA, AMACCHWUV,
	"MACCHWUVCC", LMA, AMACCHWUVCC,
	"MACCHWV", LMA, AMACCHWV,
	"MACCHWVCC", LMA, AMACCHWVCC,
	"MACHHW", LMA, AMACHHW,
	"MACHHWCC", LMA, AMACHHWCC,
	"MACHHWS", LMA, AMACHHWS,
	"MACHHWSCC", LMA, AMACHHWSCC,
	"MACHHWSU", LMA, AMACHHWSU,
	"MACHHWSUCC", LMA, AMACHHWSUCC,
	"MACHHWSUV", LMA, AMACHHWSUV,
	"MACHHWSUVCC", LMA, AMACHHWSUVCC,
	"MACHHWSV", LMA, AMACHHWSV,
	"MACHHWSVCC", LMA, AMACHHWSVCC,
	"MACHHWU", LMA, AMACHHWU,
	"MACHHWUCC", LMA, AMACHHWUCC,
	"MACHHWUV", LMA, AMACHHWUV,
	"MACHHWUVCC", LMA, AMACHHWUVCC,
	"MACHHWV", LMA, AMACHHWV,
	"MACHHWVCC", LMA, AMACHHWVCC,
	"MACLHW", LMA, AMACLHW,
	"MACLHWCC", LMA, AMACLHWCC,
	"MACLHWS", LMA, AMACLHWS,
	"MACLHWSCC", LMA, AMACLHWSCC,
	"MACLHWSU", LMA, AMACLHWSU,
	"MACLHWSUCC", LMA, AMACLHWSUCC,
	"MACLHWSUV", LMA, AMACLHWSUV,
	"MACLHWSUVCC", LMA, AMACLHWSUVCC,
	"MACLHWSV", LMA, AMACLHWSV,
	"MACLHWSVCC", LMA, AMACLHWSVCC,
	"MACLHWU", LMA, AMACLHWU,
	"MACLHWUCC", LMA, AMACLHWUCC,
	"MACLHWUV", LMA, AMACLHWUV,
	"MACLHWUVCC", LMA, AMACLHWUVCC,
	"MACLHWV", LMA, AMACLHWV,
	"MACLHWVCC", LMA, AMACLHWVCC,
	"MULCHW",	LLOGW, AMULCHW,
	"MULCHWCC",	LLOGW, AMULCHWCC,
	"MULCHWU",	LLOGW, AMULCHWU,
	"MULCHWUCC",	LLOGW, AMULCHWUCC,
	"MULHHW",	LLOGW, AMULHHW,
	"MULHHWCC",	LLOGW, AMULHHWCC,
	"MULHHWU",	LLOGW, AMULHHWU,
	"MULHHWUCC",	LLOGW, AMULHHWUCC,
	"MULLHW",	LLOGW, AMULLHW,
	"MULLHWCC",	LLOGW, AMULLHWCC,
	"MULLHWU",	LLOGW, AMULLHWU,
	"MULLHWUCC",	LLOGW, AMULLHWUCC,
	"NMACCHW", LMA, ANMACCHW,
	"NMACCHWCC", LMA, ANMACCHWCC,
	"NMACCHWS", LMA, ANMACCHWS,
	"NMACCHWSCC", LMA, ANMACCHWSCC,
	"NMACCHWSV", LMA, ANMACCHWSV,
	"NMACCHWSVCC", LMA, ANMACCHWSVCC,
	"NMACCHWV", LMA, ANMACCHWV,
	"NMACCHWVCC", LMA, ANMACCHWVCC,
	"NMACHHW", LMA, ANMACHHW,
	"NMACHHWCC", LMA, ANMACHHWCC,
	"NMACHHWS", LMA, ANMACHHWS,
	"NMACHHWSCC", LMA, ANMACHHWSCC,
	"NMACHHWSV", LMA, ANMACHHWSV,
	"NMACHHWSVCC", LMA, ANMACHHWSVCC,
	"NMACHHWV", LMA, ANMACHHWV,
	"NMACHHWVCC", LMA, ANMACHHWVCC,
	"NMACLHW", LMA, ANMACLHW,
	"NMACLHWCC", LMA, ANMACLHWCC,
	"NMACLHWS", LMA, ANMACLHWS,
	"NMACLHWSCC", LMA, ANMACLHWSCC,
	"NMACLHWSV", LMA, ANMACLHWSV,
	"NMACLHWSVCC", LMA, ANMACLHWSVCC,
	"NMACLHWV", LMA, ANMACLHWV,
	"NMACLHWVCC", LMA, ANMACLHWVCC,

	/* optional on 32-bit */
	"FRES", LFCONV, AFRES,
	"FRESCC", LFCONV, AFRESCC,
	"FRSQRTE", LFCONV, AFRSQRTE,
	"FRSQRTECC", LFCONV, AFRSQRTECC,
	"FSEL", LFMA, AFSEL,
	"FSELCC", LFMA, AFSELCC,
	"FSQRT", LFCONV, AFSQRT,
	"FSQRTCC", LFCONV, AFSQRTCC,
	"FSQRTS", LFCONV, AFSQRTS,
	"FSQRTSCC", LFCONV, AFSQRTSCC,

	/* parallel, cross, and secondary (fp2) */
	"FPSEL", LFMA, AFPSEL,
	"FPMUL", LFADD, AFPMUL,
	"FXMUL", LFADD, AFXMUL,
	"FXPMUL", LFADD, AFXPMUL,
	"FXSMUL", LFADD, AFXSMUL,
	"FPADD", LFADD, AFPADD,
	"FPSUB", LFADD, AFPSUB,
	"FPRE", LFCONV, AFPRE,
	"FPRSQRTE", LFCONV, AFPRSQRTE,
	"FPMADD", LFMA, AFPMADD,
	"FXMADD", LFMA, AFXMADD,
	"FXCPMADD", LFMA, AFXCPMADD,
	"FXCSMADD", LFMA, AFXCSMADD,
	"FPNMADD", LFMA, AFPNMADD,
	"FXNMADD", LFMA, AFXNMADD,
	"FXCPNMADD", LFMA, AFXCPNMADD,
	"FXCSNMADD", LFMA, AFXCSNMADD,
	"FPMSUB", LFMA, AFPMSUB,
	"FXMSUB", LFMA, AFXMSUB,
	"FXCPMSUB", LFMA, AFXCPMSUB,
	"FXCSMSUB", LFMA, AFXCSMSUB,
	"FPNMSUB", LFMA, AFPNMSUB,
	"FXNMSUB", LFMA, AFXNMSUB,
	"FXCPNMSUB", LFMA, AFXCPNMSUB,
	"FXCSNMSUB", LFMA, AFXCSNMSUB,
	"FPABS", LFCONV, AFPABS,
	"FPNEG", LFCONV, AFPNEG,
	"FPRSP", LFCONV, AFPRSP,
	"FPNABS", LFCONV, AFPNABS,
	"FSMOVD", LFMOV, AFSMOVD,
	"FSCMP", LFCMP, AFSCMP,
	"FSABS", LFCONV, AFSABS,
	"FSNEG", LFCONV, AFSNEG,
	"FSNABS", LFCONV, AFSNABS,
	"FPCTIW", LFCONV, AFPCTIW,
	"FPCTIWZ", LFCONV, AFPCTIWZ,
	"FMOVSPD", LFCONV, AFMOVSPD,
	"FMOVPSD", LFCONV, AFMOVPSD,
	"FXCPNPMA", LFMA, AFXCPNPMA,
	"FXCSNPMA", LFMA, AFXCSNPMA,
	"FXCPNSMA", LFMA, AFXCPNSMA,
	"FXCSNSMA", LFMA, AFXCSNSMA,
	"FXCXNPMA", LFMA, AFXCXNPMA,
	"FXCXNSMA", LFMA, AFXCXNSMA,
	"FXCXMA", LFMA, AFXCXMA,
	"FXCXNMS", LFMA, AFXCXNMS,

	/* parallel, cross, and secondary load and store (fp2) */
	"FSMOVS", LFMOVX, AFSMOVS,
	"FSMOVSU", LFMOVX, AFSMOVSU,
	"FSMOVD", LFMOVX, AFSMOVD,
	"FSMOVDU", LFMOVX, AFSMOVDU,
	"FXMOVS", LFMOVX, AFXMOVS,
	"FXMOVSU", LFMOVX, AFXMOVSU,
	"FXMOVD", LFMOVX, AFXMOVD,
	"FXMOVDU", LFMOVX, AFXMOVDU,
	"FPMOVS", LFMOVX, AFPMOVS,
	"FPMOVSU", LFMOVX, AFPMOVSU,
	"FPMOVD", LFMOVX, AFPMOVD,
	"FPMOVDU", LFMOVX, AFPMOVDU,
	"FPMOVIW", LFMOVX, AFPMOVIW,

	"AFMOVSPD",	LFMOV,	AFMOVSPD,
	"AFMOVPSD",	LFMOV,	AFMOVPSD,

	/* special instructions */
	"DCBF",		LXOP,	ADCBF,
	"DCBI",		LXOP,	ADCBI,
	"DCBST",	LXOP,	ADCBST,
	"DCBT",		LXOP,	ADCBT,
	"DCBTST",	LXOP,	ADCBTST,
	"DCBZ",		LXOP,	ADCBZ,
	"ICBI",		LXOP,	AICBI,

	"ECIWX",	LXLD,	AECIWX,
	"ECOWX",	LXST,	AECOWX,
	"LWAR", LXLD, ALWAR,
	"STWCCC", LXST, ASTWCCC,
	"EIEIO",	LRETRN,	AEIEIO,
	"TLBIE",	LNOP,	ATLBIE,
	"LSW",	LXLD, ALSW,
	"STSW",	LXST, ASTSW,
	
	"ISYNC",	LRETRN, AISYNC,
	"SYNC",		LRETRN, ASYNC,
/*	"TW",		LADDW,	ATW,*/

	"WORD",		LWORD, AWORD,
	"SCHED",	LSCHED, 0,
	"NOSCHED",	LSCHED,	0x80,

	0
};

void
cinit(void)
{
	Sym *s;
	int i;

	nullgen.sym = S;
	nullgen.offset = 0;
	nullgen.type = D_NONE;
	nullgen.name = D_NONE;
	nullgen.reg = NREG;
	nullgen.xreg = NREG;
	if(FPCHIP)
		nullgen.dval = 0;
	for(i=0; i<sizeof(nullgen.sval); i++)
		nullgen.sval[i] = 0;

	nerrors = 0;
	iostack = I;
	iofree = I;
	peekc = IGN;
	nhunk = 0;
	for(i=0; i<NHASH; i++)
		hash[i] = S;
	for(i=0; itab[i].name; i++) {
		s = slookup(itab[i].name);
		s->type = itab[i].type;
		s->value = itab[i].value;
	}
	ALLOCN(pathname, 0, 100);
	if(mygetwd(pathname, 99) == 0) {
		ALLOCN(pathname, 100, 900);
		if(mygetwd(pathname, 999) == 0)
			strcpy(pathname, "/???");
	}
}

void
syminit(Sym *s)
{

	s->type = LNAME;
	s->value = 0;
}

void
cclean(void)
{

	outcode(AEND, &nullgen, NREG, &nullgen);
	Bflush(&obuf);
}

void
zname(char *n, int t, int s)
{

	Bputc(&obuf, ANAME);
	Bputc(&obuf, ANAME>>8);
	Bputc(&obuf, t);	/* type */
	Bputc(&obuf, s);	/* sym */
	while(*n) {
		Bputc(&obuf, *n);
		n++;
	}
	Bputc(&obuf, 0);
}

void
zaddr(Gen *a, int s)
{
	long l;
	int i;
	char *n;
	Ieee e;

	Bputc(&obuf, a->type);
	Bputc(&obuf, a->reg);
	Bputc(&obuf, s);
	Bputc(&obuf, a->name);
	switch(a->type) {
	default:
		print("unknown type %d\n", a->type);
		exits("arg");

	case D_NONE:
	case D_REG:
	case D_FREG:
	case D_CREG:
	case D_FPSCR:
	case D_MSR:
	case D_SREG:
	case D_OPT:
		break;

	case D_DCR:
	case D_SPR:
	case D_OREG:
	case D_CONST:
	case D_BRANCH:
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		break;

	case D_SCONST:
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(&obuf, *n);
			n++;
		}
		break;

	case D_FCONST:
		ieeedtod(&e, a->dval);
		Bputc(&obuf, e.l);
		Bputc(&obuf, e.l>>8);
		Bputc(&obuf, e.l>>16);
		Bputc(&obuf, e.l>>24);
		Bputc(&obuf, e.h);
		Bputc(&obuf, e.h>>8);
		Bputc(&obuf, e.h>>16);
		Bputc(&obuf, e.h>>24);
		break;
	}
}

int
outsim(Gen *g)
{
	Sym *s;
	int sno, t;

	s = g->sym;
	if(s == S)
		return 0;
	sno = s->sym;
	if(sno < 0 || sno >= NSYM)
		sno = 0;
	t = g->name;
	if(h[sno].type == t && h[sno].sym == s)
		return sno;
	zname(s->name, t, sym);
	s->sym = sym;
	h[sym].sym = s;
	h[sym].type = t;
	sno = sym;
	sym++;
	if(sym >= NSYM)
		sym = 1;
	return sno;
}

void
outcode(int a, Gen *g1, int reg, Gen *g2)
{
	int sf, st;

	if(a != AGLOBL && a != ADATA)
		pc++;
	if(pass == 1)
		return;
	if(g1->xreg != NREG) {
		if(reg != NREG || g2->xreg != NREG)
			yyerror("bad addressing modes");
		reg = g1->xreg;
	} else
	if(g2->xreg != NREG) {
		if(reg != NREG)
			yyerror("bad addressing modes");
		reg = g2->xreg;
	}
	do {
		sf = outsim(g1);
		st = outsim(g2);
	} while(sf != 0 && st == sf);
	Bputc(&obuf, a);
	Bputc(&obuf, a>>8);
	Bputc(&obuf, reg|nosched);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, sf);
	zaddr(g2, st);
}

void
outgcode(int a, Gen *g1, int reg, Gen *g2, Gen *g3)
{
	int s1, s2, s3, flag; 

	if(a != AGLOBL && a != ADATA)
		pc++;
	if(pass == 1)
		return;
	do {
		s1 = outsim(g1);
		s2 = outsim(g2);
		s3 = outsim(g3);
	} while(s1 && (s2 && s1 == s2 || s3 && s1 == s3) || s2 && (s3 && s2 == s3));
	flag = 0;
	if(g2->type != D_NONE)
		flag = 0x40;	/* flags extra operand */
	Bputc(&obuf, a);
	Bputc(&obuf, a>>8);
	Bputc(&obuf, reg | nosched | flag);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(g1, s1);
	if(flag)
		zaddr(g2, s2);
	zaddr(g3, s3);
}

void
outhist(void)
{
	Gen g;
	Hist *h;
	char *p, *q, *op, c;
	int n;

	g = nullgen;
	c = pathchar();
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		op = 0;
		/* on windows skip drive specifier in pathname */
		if(systemtype(Windows) && p && p[1] == ':'){
			p += 2;
			c = *p;
		}
		if(p && p[0] != c && h->offset == 0 && pathname){
			/* on windows skip drive specifier in pathname */
			if(systemtype(Windows) && pathname[1] == ':') {
				op = p;
				p = pathname+2;
				c = *p;
			} else if(pathname[0] == c){
				op = p;
				p = pathname;
			}
		}
		while(p) {
			q = strchr(p, c);
			if(q) {
				n = q-p;
				if(n == 0){
					n = 1;	/* leading "/" */
					*p = '/';	/* don't emit "\" on windows */
				}
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(&obuf, ANAME);
				Bputc(&obuf, ANAME>>8);
				Bputc(&obuf, D_FILE);	/* type */
				Bputc(&obuf, 1);	/* sym */
				Bputc(&obuf, '<');
				Bwrite(&obuf, p, n);
				Bputc(&obuf, 0);
			}
			p = q;
			if(p == 0 && op) {
				p = op;
				op = 0;
			}
		}
		g.offset = h->offset;

		Bputc(&obuf, AHISTORY);
		Bputc(&obuf, AHISTORY>>8);
		Bputc(&obuf, 0);
		Bputc(&obuf, h->line);
		Bputc(&obuf, h->line>>8);
		Bputc(&obuf, h->line>>16);
		Bputc(&obuf, h->line>>24);
		zaddr(&nullgen, 0);
		zaddr(&g, 0);
	}
}

#include "../cc/lexbody"
#include "../cc/macbody"
#include "../cc/compat"
