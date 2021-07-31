#include <ctype.h>
#define	EXTERN
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = '2';
	thestring = "68020";
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
	if(argc > 1 && !systemtype(Windows)) {
		nproc = 1;
		if(p = getenv("NPROC"))
			nproc = atol(p);	/* */
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
	p = utfrrune(ofile, pathchar());
	if(p) {
		include[0] = ofile;
		*p++ = 0;
	} else
		p = ofile;
	if(outfile == 0) {
		outfile = p;
		if(outfile){
			p = utfrrune(outfile, '.');
			if(p)
				if(p[1] == 's' && p[2] == 0)
					p[0] = 0;
			p = utfrune(outfile, 0);
			p[0] = '.';
			p[1] = thechar;
			p[2] = 0;
		} else
			outfile = "/dev/null";
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
	pinit(file);
	for(i=0; i<nDlist; i++)
		dodefine(Dlist[i]);
	yyparse();
	if(nerrors) {
		cclean();
		return nerrors;
	}

	pass = 2;
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
	"TOS",		LTOS,	D_TOS,
	"CCR",		LTOS,	D_CCR,
	"SR",		LTOS,	D_SR,
	"SFC",		LTOS,	D_SFC,
	"DFC",		LTOS,	D_DFC,
	"CACR",		LTOS,	D_CACR,
	"USP",		LTOS,	D_USP,
	"VBR",		LTOS,	D_VBR,
	"CAAR",		LTOS,	D_CAAR,
	"MSP",		LTOS,	D_MSP,
	"ISP",		LTOS,	D_ISP,
	"FPCR",		LTOS,	D_FPCR,
	"FPSR",		LTOS,	D_FPSR,
	"FPIAR",	LTOS,	D_FPIAR,
	"TC",		LTOS,	D_TC,
	"ITT0",		LTOS,	D_ITT0,
	"ITT1",		LTOS,	D_ITT1,
	"DTT0",		LTOS,	D_DTT0,
	"DTT1",		LTOS,	D_DTT1,
	"MMUSR",	LTOS,	D_MMUSR,
	"URP",		LTOS,	D_URP,
	"SRP",		LTOS,	D_SRP,

	"R0",		LDREG,	D_R0+0,
	"R1",		LDREG,	D_R0+1,
	"R2",		LDREG,	D_R0+2,
	"R3",		LDREG,	D_R0+3,
	"R4",		LDREG,	D_R0+4,
	"R5",		LDREG,	D_R0+5,
	"R6",		LDREG,	D_R0+6,
	"R7",		LDREG,	D_R0+7,

	".W",		LWID,	0,
	".L",		LWID,	4,

	"A0",		LAREG,	D_A0+0,
	"A1",		LAREG,	D_A0+1,
	"A2",		LAREG,	D_A0+2,
	"A3",		LAREG,	D_A0+3,
	"A4",		LAREG,	D_A0+4,
	"A5",		LAREG,	D_A0+5,
	"A6",		LAREG,	D_A0+6,
	"A7",		LAREG,	D_A0+7,

	"F0",		LFREG,	D_F0+0,
	"F1",		LFREG,	D_F0+1,
	"F2",		LFREG,	D_F0+2,
	"F3",		LFREG,	D_F0+3,
	"F4",		LFREG,	D_F0+4,
	"F5",		LFREG,	D_F0+5,
	"F6",		LFREG,	D_F0+6,
	"F7",		LFREG,	D_F0+7,

	"ABCD",		LTYPE1, AABCD,
	"ADDB",		LTYPE1, AADDB,
	"ADDL",		LTYPE1, AADDL,
	"ADDW",		LTYPE1, AADDW,
	"ADDXB",	LTYPE1, AADDXB,
	"ADDXL",	LTYPE1, AADDXL,
	"ADDXW",	LTYPE1, AADDXW,
	"ADJSP",	LTYPE5, AADJSP,
	"ANDB",		LTYPE1, AANDB,
	"ANDL",		LTYPE1, AANDL,
	"ANDW",		LTYPE1, AANDW,
	"ASLB",		LTYPE1, AASLB,
	"ASLL",		LTYPE1, AASLL,
	"ASLW",		LTYPE1, AASLW,
	"ASRB",		LTYPE1, AASRB,
	"ASRL",		LTYPE1, AASRL,
	"ASRW",		LTYPE1, AASRW,
	"BCASE",	LTYPE7, ABCASE,
	"BCC",		LTYPE6, ABCC,
	"BCHG",		LTYPE1, ABCHG,
	"BCLR",		LTYPE1, ABCLR,
	"BCS",		LTYPE6, ABCS,
	"BEQ",		LTYPE6, ABEQ,
	"BFCHG",	LTYPEA, ABFCHG,
	"BFCLR",	LTYPEA, ABFCLR,
	"BFEXTS",	LTYPEA, ABFEXTS,
	"BFEXTU",	LTYPEA, ABFEXTU,
	"BFFFO",	LTYPEA, ABFFFO,
	"BFINS",	LTYPEA, ABFINS,
	"BFSET",	LTYPEA, ABFSET,
	"BFTST",	LTYPEA, ABFTST,
	"BGE",		LTYPE6, ABGE,
	"BGT",		LTYPE6, ABGT,
	"BHI",		LTYPE6, ABHI,
	"BKPT",		LTYPE1, ABKPT,
	"BLE",		LTYPE6, ABLE,
	"BLS",		LTYPE6, ABLS,
	"BLT",		LTYPE6, ABLT,
	"BMI",		LTYPE6, ABMI,
	"BNE",		LTYPE6, ABNE,
	"BPL",		LTYPE6, ABPL,
	"BRA",		LTYPE6, ABRA,
	"BSET",		LTYPE1, ABSET,
	"BSR",		LTYPE3, ABSR,
	"BTST",		LTYPE1, ABTST,
	"BVC",		LTYPE6, ABVC,
	"BVS",		LTYPE6, ABVS,
	"CALLM",	LTYPE1, ACALLM,
	"CAS2B",	LTYPE1, ACAS2B,
	"CAS2L",	LTYPE1, ACAS2L,
	"CAS2W",	LTYPE1, ACAS2W,
	"CASB",		LTYPE1, ACASB,
	"CASEW",	LTYPE2, ACASEW,
	"CASL",		LTYPE1, ACASL,
	"CASW",		LTYPE1, ACASW,
	"CHK2B",	LTYPE1, ACHK2B,
	"CHK2L",	LTYPE1, ACHK2L,
	"CHK2W",	LTYPE1, ACHK2W,
	"CHKL",		LTYPE1, ACHKL,
	"CHKW",		LTYPE1, ACHKW,
	"CLRB",		LTYPE3, ACLRB,
	"CLRL",		LTYPE3, ACLRL,
	"CLRW",		LTYPE3, ACLRW,
	"CMP2B",	LTYPE1, ACMP2B,
	"CMP2L",	LTYPE1, ACMP2L,
	"CMP2W",	LTYPE1, ACMP2W,
	"CMPB",		LTYPE1, ACMPB,
	"CMPL",		LTYPE1, ACMPL,
	"CMPW",		LTYPE1, ACMPW,
	"DATA",		LTYPE4, ADATA,
	"DBCC",		LTYPE7, ADBCC,
	"DBCS",		LTYPE7, ADBCS,
	"DBEQ",		LTYPE7, ADBEQ,
	"DBF",		LTYPE7, ADBF,
	"DBGE",		LTYPE7, ADBGE,
	"DBGT",		LTYPE7, ADBGT,
	"DBHI",		LTYPE7, ADBHI,
	"DBLE",		LTYPE7, ADBLE,
	"DBLS",		LTYPE7, ADBLS,
	"DBLT",		LTYPE7, ADBLT,
	"DBMI",		LTYPE7, ADBMI,
	"DBNE",		LTYPE7, ADBNE,
	"DBPL",		LTYPE7, ADBPL,
	"DBT",		LTYPE7, ADBT,
	"DBVC",		LTYPE7, ADBVC,
	"DBVS",		LTYPE7, ADBVS,
	"DIVSL",	LTYPE1, ADIVSL,
	"DIVSW",	LTYPE1, ADIVSW,
	"DIVUL",	LTYPE1, ADIVUL,
	"DIVUW",	LTYPE1, ADIVUW,
	"END",		LTYPE2, AEND,
	"EORB",		LTYPE1, AEORB,
	"EORL",		LTYPE1, AEORL,
	"EORW",		LTYPE1, AEORW,
	"EXG",		LTYPE1, AEXG,
	"EXTBL",	LTYPE3, AEXTBL,
	"EXTBW",	LTYPE3, AEXTBW,
	"EXTWL",	LTYPE3, AEXTWL,
	"FABSB",	LTYPE1, AFABSB,
	"FABSD",	LTYPE1, AFABSD,
	"FABSF",	LTYPE1, AFABSF,
	"FABSL",	LTYPE1, AFABSL,
	"FABSW",	LTYPE1, AFABSW,
	"FACOSB",	LTYPE1, AFACOSB,
	"FACOSD",	LTYPE1, AFACOSD,
	"FACOSF",	LTYPE1, AFACOSF,
	"FACOSL",	LTYPE1, AFACOSL,
	"FACOSW",	LTYPE1, AFACOSW,
	"FADDB",	LTYPE1, AFADDB,
	"FADDD",	LTYPE1, AFADDD,
	"FADDF",	LTYPE1, AFADDF,
	"FADDL",	LTYPE1, AFADDL,
	"FADDW",	LTYPE1, AFADDW,
	"FASINB",	LTYPE1, AFASINB,
	"FASIND",	LTYPE1, AFASIND,
	"FASINF",	LTYPE1, AFASINF,
	"FASINL",	LTYPE1, AFASINL,
	"FASINW",	LTYPE1, AFASINW,
	"FATANB",	LTYPE1, AFATANB,
	"FATAND",	LTYPE1, AFATAND,
	"FATANF",	LTYPE1, AFATANF,
	"FATANHB",	LTYPE1, AFATANHB,
	"FATANHD",	LTYPE1, AFATANHD,
	"FATANHF",	LTYPE1, AFATANHF,
	"FATANHL",	LTYPE1, AFATANHL,
	"FATANHW",	LTYPE1, AFATANHW,
	"FATANL",	LTYPE1, AFATANL,
	"FATANW",	LTYPE1, AFATANW,
	"FBEQ",		LTYPE6, AFBEQ,
	"FBF",		LTYPE6, AFBF,
	"FBGE",		LTYPE6, AFBGE,
	"FBGT",		LTYPE6, AFBGT,
	"FBLE",		LTYPE6, AFBLE,
	"FBLT",		LTYPE6, AFBLT,
	"FBNE",		LTYPE6, AFBNE,
	"FBT",		LTYPE6, AFBT,
	"FCMPB",	LTYPE1, AFCMPB,
	"FCMPD",	LTYPE1, AFCMPD,
	"FCMPF",	LTYPE1, AFCMPF,
	"FCMPL",	LTYPE1, AFCMPL,
	"FCMPW",	LTYPE1, AFCMPW,
	"FCOSB",	LTYPE1, AFCOSB,
	"FCOSD",	LTYPE1, AFCOSD,
	"FCOSF",	LTYPE1, AFCOSF,
	"FCOSHB",	LTYPE1, AFCOSHB,
	"FCOSHD",	LTYPE1, AFCOSHD,
	"FCOSHF",	LTYPE1, AFCOSHF,
	"FCOSHL",	LTYPE1, AFCOSHL,
	"FCOSHW",	LTYPE1, AFCOSHW,
	"FCOSL",	LTYPE1, AFCOSL,
	"FCOSW",	LTYPE1, AFCOSW,
	"FDBEQ",	LTYPE7, AFDBEQ,
	"FDBF",		LTYPE7, AFDBF,
	"FDBGE",	LTYPE7, AFDBGE,
	"FDBGT",	LTYPE7, AFDBGT,
	"FDBLE",	LTYPE7, AFDBLE,
	"FDBLT",	LTYPE7, AFDBLT,
	"FDBNE",	LTYPE7, AFDBNE,
	"FDBT",		LTYPE7, AFDBT,
	"FDIVB",	LTYPE1, AFDIVB,
	"FDIVD",	LTYPE1, AFDIVD,
	"FDIVF",	LTYPE1, AFDIVF,
	"FDIVL",	LTYPE1, AFDIVL,
	"FDIVW",	LTYPE1, AFDIVW,
	"FETOXB",	LTYPE1, AFETOXB,
	"FETOXD",	LTYPE1, AFETOXD,
	"FETOXF",	LTYPE1, AFETOXF,
	"FETOXL",	LTYPE1, AFETOXL,
	"FETOXM1B",	LTYPE1, AFETOXM1B,
	"FETOXM1D",	LTYPE1, AFETOXM1D,
	"FETOXM1F",	LTYPE1, AFETOXM1F,
	"FETOXM1L",	LTYPE1, AFETOXM1L,
	"FETOXM1W",	LTYPE1, AFETOXM1W,
	"FETOXW",	LTYPE1, AFETOXW,
	"FGETEXPB",	LTYPE1, AFGETEXPB,
	"FGETEXPD",	LTYPE1, AFGETEXPD,
	"FGETEXPF",	LTYPE1, AFGETEXPF,
	"FGETEXPL",	LTYPE1, AFGETEXPL,
	"FGETEXPW",	LTYPE1, AFGETEXPW,
	"FGETMANB",	LTYPE1, AFGETMANB,
	"FGETMAND",	LTYPE1, AFGETMAND,
	"FGETMANF",	LTYPE1, AFGETMANF,
	"FGETMANL",	LTYPE1, AFGETMANL,
	"FGETMANW",	LTYPE1, AFGETMANW,
	"FINTB",	LTYPE1, AFINTB,
	"FINTD",	LTYPE1, AFINTD,
	"FINTF",	LTYPE1, AFINTF,
	"FINTL",	LTYPE1, AFINTL,
	"FINTRZB",	LTYPE1, AFINTRZB,
	"FINTRZD",	LTYPE1, AFINTRZD,
	"FINTRZF",	LTYPE1, AFINTRZF,
	"FINTRZL",	LTYPE1, AFINTRZL,
	"FINTRZW",	LTYPE1, AFINTRZW,
	"FINTW",	LTYPE1, AFINTW,
	"FLOG10B",	LTYPE1, AFLOG10B,
	"FLOG10D",	LTYPE1, AFLOG10D,
	"FLOG10F",	LTYPE1, AFLOG10F,
	"FLOG10L",	LTYPE1, AFLOG10L,
	"FLOG10W",	LTYPE1, AFLOG10W,
	"FLOG2B",	LTYPE1, AFLOG2B,
	"FLOG2D",	LTYPE1, AFLOG2D,
	"FLOG2F",	LTYPE1, AFLOG2F,
	"FLOG2L",	LTYPE1, AFLOG2L,
	"FLOG2W",	LTYPE1, AFLOG2W,
	"FLOGNB",	LTYPE1, AFLOGNB,
	"FLOGND",	LTYPE1, AFLOGND,
	"FLOGNF",	LTYPE1, AFLOGNF,
	"FLOGNL",	LTYPE1, AFLOGNL,
	"FLOGNP1B",	LTYPE1, AFLOGNP1B,
	"FLOGNP1D",	LTYPE1, AFLOGNP1D,
	"FLOGNP1F",	LTYPE1, AFLOGNP1F,
	"FLOGNP1L",	LTYPE1, AFLOGNP1L,
	"FLOGNP1W",	LTYPE1, AFLOGNP1W,
	"FLOGNW",	LTYPE1, AFLOGNW,
	"FMODB",	LTYPE1, AFMODB,
	"FMODD",	LTYPE1, AFMODD,
	"FMODF",	LTYPE1, AFMODF,
	"FMODL",	LTYPE1, AFMODL,
	"FMODW",	LTYPE1, AFMODW,
	"FMOVEB",	LTYPE1, AFMOVEB,
	"FMOVED",	LTYPE1, AFMOVED,
	"FMOVEF",	LTYPE1, AFMOVEF,
	"FMOVEL",	LTYPE1, AFMOVEL,
	"FMOVEW",	LTYPE1, AFMOVEW,
	"FMULB",	LTYPE1, AFMULB,
	"FMULD",	LTYPE1, AFMULD,
	"FMULF",	LTYPE1, AFMULF,
	"FMULL",	LTYPE1, AFMULL,
	"FMULW",	LTYPE1, AFMULW,
	"FNEGB",	LTYPE8, AFNEGB,
	"FNEGD",	LTYPE8, AFNEGD,
	"FNEGF",	LTYPE8, AFNEGF,
	"FNEGL",	LTYPE8, AFNEGL,
	"FNEGW",	LTYPE8, AFNEGW,
	"FREMB",	LTYPE1, AFREMB,
	"FREMD",	LTYPE1, AFREMD,
	"FREMF",	LTYPE1, AFREMF,
	"FREML",	LTYPE1, AFREML,
	"FREMW",	LTYPE1, AFREMW,
	"FSCALEB",	LTYPE1, AFSCALEB,
	"FSCALED",	LTYPE1, AFSCALED,
	"FSCALEF",	LTYPE1, AFSCALEF,
	"FSCALEL",	LTYPE1, AFSCALEL,
	"FSCALEW",	LTYPE1, AFSCALEW,
	"FSEQ",		LTYPE1, AFSEQ,
	"FSF",		LTYPE1, AFSF,
	"FSGE",		LTYPE1, AFSGE,
	"FSGT",		LTYPE1, AFSGT,
	"FSINB",	LTYPE1, AFSINB,
	"FSIND",	LTYPE1, AFSIND,
	"FSINF",	LTYPE1, AFSINF,
	"FSINHB",	LTYPE1, AFSINHB,
	"FSINHD",	LTYPE1, AFSINHD,
	"FSINHF",	LTYPE1, AFSINHF,
	"FSINHL",	LTYPE1, AFSINHL,
	"FSINHW",	LTYPE1, AFSINHW,
	"FSINL",	LTYPE1, AFSINL,
	"FSINW",	LTYPE1, AFSINW,
	"FSLE",		LTYPE1, AFSLE,
	"FSLT",		LTYPE1, AFSLT,
	"FSNE",		LTYPE1, AFSNE,
	"FSQRTB",	LTYPE1, AFSQRTB,
	"FSQRTD",	LTYPE1, AFSQRTD,
	"FSQRTF",	LTYPE1, AFSQRTF,
	"FSQRTL",	LTYPE1, AFSQRTL,
	"FSQRTW",	LTYPE1, AFSQRTW,
	"FST",		LTYPE1, AFST,
	"FSUBB",	LTYPE1, AFSUBB,
	"FSUBD",	LTYPE1, AFSUBD,
	"FSUBF",	LTYPE1, AFSUBF,
	"FSUBL",	LTYPE1, AFSUBL,
	"FSUBW",	LTYPE1, AFSUBW,
	"FTANB",	LTYPE1, AFTANB,
	"FTAND",	LTYPE1, AFTAND,
	"FTANF",	LTYPE1, AFTANF,
	"FTANHB",	LTYPE1, AFTANHB,
	"FTANHD",	LTYPE1, AFTANHD,
	"FTANHF",	LTYPE1, AFTANHF,
	"FTANHL",	LTYPE1, AFTANHL,
	"FTANHW",	LTYPE1, AFTANHW,
	"FTANL",	LTYPE1, AFTANL,
	"FTANW",	LTYPE1, AFTANW,
	"FTENTOXB",	LTYPE1, AFTENTOXB,
	"FTENTOXD",	LTYPE1, AFTENTOXD,
	"FTENTOXF",	LTYPE1, AFTENTOXF,
	"FTENTOXL",	LTYPE1, AFTENTOXL,
	"FTENTOXW",	LTYPE1, AFTENTOXW,
	"FTSTB",	LTYPE1, AFTSTB,
	"FTSTD",	LTYPE1, AFTSTD,
	"FTSTF",	LTYPE1, AFTSTF,
	"FTSTL",	LTYPE1, AFTSTL,
	"FTSTW",	LTYPE1, AFTSTW,
	"FTWOTOXB",	LTYPE1, AFTWOTOXB,
	"FTWOTOXD",	LTYPE1, AFTWOTOXD,
	"FTWOTOXF",	LTYPE1, AFTWOTOXF,
	"FTWOTOXL",	LTYPE1, AFTWOTOXL,
	"FTWOTOXW",	LTYPE1, AFTWOTOXW,
	"FMOVEM",	LTYPE1, AFMOVEM,
	"FMOVEMC",	LTYPE1, AFMOVEMC,
	"FRESTORE",	LTYPE3, AFRESTORE,
	"FSAVE",	LTYPE3, AFSAVE,
	"GLOBL",	LTYPE1, AGLOBL,
	"GOK",		LTYPE2, AGOK,
	"HISTORY",	LTYPE2, AHISTORY,
	"ILLEG",	LTYPE2, AILLEG,
	"INSTR",	LTYPE3, AINSTR,
	"JMP",		LTYPE3, AJMP,
	"JSR",		LTYPE3, AJSR,
	"LEA",		LTYPE1, ALEA,
	"LINKL",	LTYPE1, ALINKL,
	"LINKW",	LTYPE1, ALINKW,
	"LOCATE",	LTYPE1, ALOCATE,
	"LONG",		LTYPE3, ALONG,
	"LSLB",		LTYPE1, ALSLB,
	"LSLL",		LTYPE1, ALSLL,
	"LSLW",		LTYPE1, ALSLW,
	"LSRB",		LTYPE1, ALSRB,
	"LSRL",		LTYPE1, ALSRL,
	"LSRW",		LTYPE1, ALSRW,
	"MOVB",		LTYPE1, AMOVB,
	"MOVEM",	LTYPE1, AMOVEM,
	"MOVEPL",	LTYPE1, AMOVEPL,
	"MOVEPW",	LTYPE1, AMOVEPW,
	"MOVESB",	LTYPE1, AMOVESB,
	"MOVESL",	LTYPE1, AMOVESL,
	"MOVESW",	LTYPE1, AMOVESW,
	"MOVL",		LTYPE1, AMOVL,
	"MOVW",		LTYPE1, AMOVW,
	"MULSL",	LTYPE1, AMULSL,
	"MULSW",	LTYPE1, AMULSW,
	"MULUL",	LTYPE1, AMULUL,
	"MULUW",	LTYPE1, AMULUW,
	"NAME",		LTYPE1, ANAME,
	"NBCD",		LTYPE3, ANBCD,
	"NEGB",		LTYPE3, ANEGB,
	"NEGL",		LTYPE3, ANEGL,
	"NEGW",		LTYPE3, ANEGW,
	"NEGXB",	LTYPE3, ANEGXB,
	"NEGXL",	LTYPE3, ANEGXL,
	"NEGXW",	LTYPE3, ANEGXW,
	"NOP",		LTYPE9, ANOP,
	"NOTB",		LTYPE3, ANOTB,
	"NOTL",		LTYPE3, ANOTL,
	"NOTW",		LTYPE3, ANOTW,
	"ORB",		LTYPE1, AORB,
	"ORL",		LTYPE1, AORL,
	"ORW",		LTYPE1, AORW,
	"PACK",		LTYPE1, APACK,
	"PEA",		LTYPE3, APEA,
	"RESET",	LTYPE2, ARESET,
	"ROTLB",	LTYPE1, AROTLB,
	"ROTLL",	LTYPE1, AROTLL,
	"ROTLW",	LTYPE1, AROTLW,
	"ROTRB",	LTYPE1, AROTRB,
	"ROTRL",	LTYPE1, AROTRL,
	"ROTRW",	LTYPE1, AROTRW,
	"ROXLB",	LTYPE1, AROXLB,
	"ROXLL",	LTYPE1, AROXLL,
	"ROXLW",	LTYPE1, AROXLW,
	"ROXRB",	LTYPE1, AROXRB,
	"ROXRL",	LTYPE1, AROXRL,
	"ROXRW",	LTYPE1, AROXRW,
	"RTD",		LTYPE3, ARTD,
	"RTE",		LTYPE2, ARTE,
	"RTM",		LTYPE3, ARTM,
	"RTR",		LTYPE2, ARTR,
	"RTS",		LTYPE2, ARTS,
	"SBCD",		LTYPE1, ASBCD,
	"SCC",		LTYPE3, ASCC,
	"SCS",		LTYPE3, ASCS,
	"SEQ",		LTYPE3, ASEQ,
	"SF",		LTYPE3, ASF,
	"SGE",		LTYPE3, ASGE,
	"SGT",		LTYPE3, ASGT,
	"SHI",		LTYPE3, ASHI,
	"SLE",		LTYPE3, ASLE,
	"SLS",		LTYPE3, ASLS,
	"SLT",		LTYPE3, ASLT,
	"SMI",		LTYPE3, ASMI,
	"SNE",		LTYPE3, ASNE,
	"SPL",		LTYPE3, ASPL,
	"ST",		LTYPE3, AST,
	"STOP",		LTYPE3, ASTOP,
	"SUBB",		LTYPE1, ASUBB,
	"SUBL",		LTYPE1, ASUBL,
	"SUBW",		LTYPE1, ASUBW,
	"SUBXB",	LTYPE1, ASUBXB,
	"SUBXL",	LTYPE1, ASUBXL,
	"SUBXW",	LTYPE1, ASUBXW,
	"SVC",		LTYPE2, ASVC,
	"SVS",		LTYPE2, ASVS,
	"SWAP",		LTYPE3, ASWAP,
	"SYS",		LTYPE2, ASYS,
	"TAS",		LTYPE3, ATAS,
	"TEXT",		LTYPEB, ATEXT,
	"TRAP",		LTYPE3, ATRAP,
	"TRAPCC",	LTYPE2, ATRAPCC,
	"TRAPCS",	LTYPE2, ATRAPCS,
	"TRAPEQ",	LTYPE2, ATRAPEQ,
	"TRAPF",	LTYPE2, ATRAPF,
	"TRAPGE",	LTYPE2, ATRAPGE,
	"TRAPGT",	LTYPE2, ATRAPGT,
	"TRAPHI",	LTYPE2, ATRAPHI,
	"TRAPLE",	LTYPE2, ATRAPLE,
	"TRAPLS",	LTYPE2, ATRAPLS,
	"TRAPLT",	LTYPE2, ATRAPLT,
	"TRAPMI",	LTYPE2, ATRAPMI,
	"TRAPNE",	LTYPE2, ATRAPNE,
	"TRAPPL",	LTYPE2, ATRAPPL,
	"TRAPT",	LTYPE2, ATRAPT,
	"TRAPV",	LTYPE2, ATRAPV,
	"TRAPVC",	LTYPE2, ATRAPVC,
	"TRAPVS",	LTYPE2, ATRAPVS,
	"TSTB",		LTYPE3, ATSTB,
	"TSTL",		LTYPE3, ATSTL,
	"TSTW",		LTYPE3, ATSTW,
	"UNLK",		LTYPE3, AUNLK,
	"UNPK",		LTYPE1, AUNPK,
	"WORD",		LTYPE3, AWORD,

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
	if(FPCHIP)
		nullgen.dval = 0;
	for(i=0; i<sizeof(nullgen.sval); i++)
		nullgen.sval[i] = 0;
	nullgen.displace = 0;
	nullgen.type = D_NONE;
	nullgen.index = D_NONE;
	nullgen.scale = 0;
	nullgen.field = 0;

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

	pathname = allocn(pathname, 0, 100);
	if(mygetwd(pathname, 99) == 0) {
		pathname = allocn(pathname, 100, 900);
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
	Gen2 g2;

	g2.from = nullgen;
	g2.to = nullgen;
	outcode(AEND, &g2);
	Bflush(&obuf);
}

void
zname(char *n, int t, int s)
{

	Bputc(&obuf, ANAME);	/* as */
	Bputc(&obuf, ANAME>>8);
	Bputc(&obuf, t);		/* type */
	Bputc(&obuf, s);		/* sym */
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
	int i, t;
	char *n;
	Ieee e;

	t = 0;
	if(a->field)
		t |= T_FIELD;
	if(a->index != D_NONE || a->displace != 0)
		t |= T_INDEX;
	if(a->offset != 0)
		t |= T_OFFSET;
	if(s != 0)
		t |= T_SYM;

	if(a->type == D_FCONST)
		t |= T_FCONST;
	else
	if(a->type == D_SCONST)
		t |= T_SCONST;
	else
	if(a->type & ~0xff)
		t |= T_TYPE;
	Bputc(&obuf, t);

	if(t & T_FIELD) {	/* implies field */
		i = a->field;
		Bputc(&obuf, i);
		Bputc(&obuf, i>>8);
	}
	if(t & T_INDEX) {	/* implies index, scale, displace */
		i = a->index;
		Bputc(&obuf, i);
		Bputc(&obuf, i>>8);
		Bputc(&obuf, a->scale);
		l = a->displace;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
	}
	if(t & T_OFFSET) {	/* implies offset */
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
	}
	if(t & T_SYM)		/* implies sym */
		Bputc(&obuf, s);
	if(t & T_FCONST) {
		ieeedtod(&e, a->dval);
		l = e.l;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		l = e.h;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
		return;
	}
	if(t & T_SCONST) {
		n = a->sval;
		for(i=0; i<NSNAME; i++) {
			Bputc(&obuf, *n);
			n++;
		}
		return;
	}
	i = a->type;
	Bputc(&obuf, i);
	if(t & T_TYPE)
		Bputc(&obuf, i>>8);
}

void
outcode(int a, Gen2 *g2)
{
	int sf, st, t;
	Sym *s;

	if(pass == 1)
		goto out;

jackpot:
	sf = 0;
	s = g2->from.sym;
	while(s != S) {
		sf = s->sym;
		if(sf < 0 || sf >= NSYM)
			sf = 0;
		t = g2->from.type & D_MASK;
		if(h[sf].type == t)
		if(h[sf].sym == s)
			break;
		zname(s->name, t, sym);
		s->sym = sym;
		h[sym].sym = s;
		h[sym].type = t;
		sf = sym;
		sym++;
		if(sym >= NSYM)
			sym = 1;
		break;
	}
	st = 0;
	s = g2->to.sym;
	while(s != S) {
		st = s->sym;
		if(st < 0 || st >= NSYM)
			st = 0;
		t = g2->to.type & D_MASK;
		if(h[st].type == t)
		if(h[st].sym == s)
			break;
		zname(s->name, t, sym);
		s->sym = sym;
		h[sym].sym = s;
		h[sym].type = t;
		st = sym;
		sym++;
		if(sym >= NSYM)
			sym = 1;
		if(st == sf)
			goto jackpot;
		break;
	}
	Bputc(&obuf, a);
	Bputc(&obuf, a>>8);
	Bputc(&obuf, lineno);
	Bputc(&obuf, lineno>>8);
	Bputc(&obuf, lineno>>16);
	Bputc(&obuf, lineno>>24);
	zaddr(&g2->from, sf);
	zaddr(&g2->to, st);

out:
	if(a != AGLOBL && a != ADATA)
		pc++;
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
