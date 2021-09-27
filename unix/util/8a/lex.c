#include <ctype.h>
#define	EXTERN
#include "a.h"
#include "y.tab.h"

void
main(int argc, char *argv[])
{
	char *p;
	int nout, nproc, status, i, c;

	thechar = '8';
	thestring = "386";
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

	"AL",		LBREG,	D_AL,
	"CL",		LBREG,	D_CL,
	"DL",		LBREG,	D_DL,
	"BL",		LBREG,	D_BL,
	"AH",		LBREG,	D_AH,
	"CH",		LBREG,	D_CH,
	"DH",		LBREG,	D_DH,
	"BH",		LBREG,	D_BH,

	"AX",		LLREG,	D_AX,
	"CX",		LLREG,	D_CX,
	"DX",		LLREG,	D_DX,
	"BX",		LLREG,	D_BX,
/*	"SP",		LLREG,	D_SP,	*/
	"BP",		LLREG,	D_BP,
	"SI",		LLREG,	D_SI,
	"DI",		LLREG,	D_DI,

	"F0",		LFREG,	D_F0+0,
	"F1",		LFREG,	D_F0+1,
	"F2",		LFREG,	D_F0+2,
	"F3",		LFREG,	D_F0+3,
	"F4",		LFREG,	D_F0+4,
	"F5",		LFREG,	D_F0+5,
	"F6",		LFREG,	D_F0+6,
	"F7",		LFREG,	D_F0+7,

	"CS",		LSREG,	D_CS,
	"SS",		LSREG,	D_SS,
	"DS",		LSREG,	D_DS,
	"ES",		LSREG,	D_ES,
	"FS",		LSREG,	D_FS,
	"GS",		LSREG,	D_GS,

	"GDTR",		LBREG,	D_GDTR,
	"IDTR",		LBREG,	D_IDTR,
	"LDTR",		LBREG,	D_LDTR,
	"MSW",		LBREG,	D_MSW,
	"TASK",		LBREG,	D_TASK,

	"CR0",		LBREG,	D_CR+0,
	"CR1",		LBREG,	D_CR+1,
	"CR2",		LBREG,	D_CR+2,
	"CR3",		LBREG,	D_CR+3,
	"CR4",		LBREG,	D_CR+4,
	"CR5",		LBREG,	D_CR+5,
	"CR6",		LBREG,	D_CR+6,
	"CR7",		LBREG,	D_CR+7,

	"DR0",		LBREG,	D_DR+0,
	"DR1",		LBREG,	D_DR+1,
	"DR2",		LBREG,	D_DR+2,
	"DR3",		LBREG,	D_DR+3,
	"DR4",		LBREG,	D_DR+4,
	"DR5",		LBREG,	D_DR+5,
	"DR6",		LBREG,	D_DR+6,
	"DR7",		LBREG,	D_DR+7,

	"TR0",		LBREG,	D_TR+0,
	"TR1",		LBREG,	D_TR+1,
	"TR2",		LBREG,	D_TR+2,
	"TR3",		LBREG,	D_TR+3,
	"TR4",		LBREG,	D_TR+4,
	"TR5",		LBREG,	D_TR+5,
	"TR6",		LBREG,	D_TR+6,
	"TR7",		LBREG,	D_TR+7,

	"AAA",		LTYPE0,	AAAA,
	"AAD",		LTYPE0,	AAAD,
	"AAM",		LTYPE0,	AAAM,
	"AAS",		LTYPE0,	AAAS,
	"ADCB",		LTYPE3,	AADCB,
	"ADCL",		LTYPE3,	AADCL,
	"ADCW",		LTYPE3,	AADCW,
	"ADDB",		LTYPE3,	AADDB,
	"ADDL",		LTYPE3,	AADDL,
	"ADDW",		LTYPE3,	AADDW,
	"ADJSP",	LTYPE2,	AADJSP,
	"ANDB",		LTYPE3,	AANDB,
	"ANDL",		LTYPE3,	AANDL,
	"ANDW",		LTYPE3,	AANDW,
	"ARPL",		LTYPE3,	AARPL,
	"BOUNDL",	LTYPE3,	ABOUNDL,
	"BOUNDW",	LTYPE3,	ABOUNDW,
	"BSFL",		LTYPE3,	ABSFL,
	"BSFW",		LTYPE3,	ABSFW,
	"BSRL",		LTYPE3,	ABSRL,
	"BSRW",		LTYPE3,	ABSRW,
	"BTCL",		LTYPE3,	ABTCL,
	"BTCW",		LTYPE3,	ABTCW,
	"BTL",		LTYPE3,	ABTL,
	"BTRL",		LTYPE3,	ABTRL,
	"BTRW",		LTYPE3,	ABTRW,
	"BTSL",		LTYPE3,	ABTSL,
	"BTSW",		LTYPE3,	ABTSW,
	"BTW",		LTYPE3,	ABTW,
	"BYTE",		LTYPE2,	ABYTE,
	"CALL",		LTYPEC,	ACALL,
	"CLC",		LTYPE0,	ACLC,
	"CLD",		LTYPE0,	ACLD,
	"CLI",		LTYPE0,	ACLI,
	"CLTS",		LTYPE0,	ACLTS,
	"CMC",		LTYPE0,	ACMC,
	"CMPB",		LTYPE4,	ACMPB,
	"CMPL",		LTYPE4,	ACMPL,
	"CMPW",		LTYPE4,	ACMPW,
	"CMPSB",	LTYPE0,	ACMPSB,
	"CMPSL",	LTYPE0,	ACMPSL,
	"CMPSW",	LTYPE0,	ACMPSW,
	"CMPXCHGB",	LTYPE3,	ACMPXCHGB,
	"CMPXCHGL",	LTYPE3,	ACMPXCHGL,
	"CMPXCHGW",	LTYPE3,	ACMPXCHGW,
	"DAA",		LTYPE0,	ADAA,
	"DAS",		LTYPE0,	ADAS,
	"DATA",		LTYPED,	ADATA,
	"DECB",		LTYPE1,	ADECB,
	"DECL",		LTYPE1,	ADECL,
	"DECW",		LTYPE1,	ADECW,
	"DIVB",		LTYPE2,	ADIVB,
	"DIVL",		LTYPE2,	ADIVL,
	"DIVW",		LTYPE2,	ADIVW,
	"END",		LTYPE0,	AEND,
	"ENTER",	LTYPE2,	AENTER,
	"GLOBL",	LTYPEG,	AGLOBL,
	"HLT",		LTYPE0,	AHLT,
	"IDIVB",	LTYPE2,	AIDIVB,
	"IDIVL",	LTYPE2,	AIDIVL,
	"IDIVW",	LTYPE2,	AIDIVW,
	"IMULB",	LTYPEI,	AIMULB,
	"IMULL",	LTYPEI,	AIMULL,
	"IMULW",	LTYPEI,	AIMULW,
	"INB",		LTYPE0,	AINB,
	"INL",		LTYPE0,	AINL,
	"INW",		LTYPE0,	AINW,
	"INCB",		LTYPE1,	AINCB,
	"INCL",		LTYPE1,	AINCL,
	"INCW",		LTYPE1,	AINCW,
	"INSB",		LTYPE0,	AINSB,
	"INSL",		LTYPE0,	AINSL,
	"INSW",		LTYPE0,	AINSW,
	"INT",		LTYPE2,	AINT,
	"INTO",		LTYPE0,	AINTO,
	"IRETL",	LTYPE0,	AIRETL,
	"IRETW",	LTYPE0,	AIRETW,

	"JOS",		LTYPER,	AJOS,
	"JO",		LTYPER,	AJOS,	/* alternate */
	"JOC",		LTYPER,	AJOC,
	"JNO",		LTYPER,	AJOC,	/* alternate */
	"JCS",		LTYPER,	AJCS,
	"JB",		LTYPER,	AJCS,	/* alternate */
	"JC",		LTYPER,	AJCS,	/* alternate */
	"JNAE",		LTYPER,	AJCS,	/* alternate */
	"JLO",		LTYPER,	AJCS,	/* alternate */
	"JCC",		LTYPER,	AJCC,
	"JAE",		LTYPER,	AJCC,	/* alternate */
	"JNB",		LTYPER,	AJCC,	/* alternate */
	"JNC",		LTYPER,	AJCC,	/* alternate */
	"JHS",		LTYPER,	AJCC,	/* alternate */
	"JEQ",		LTYPER,	AJEQ,
	"JE",		LTYPER,	AJEQ,	/* alternate */
	"JZ",		LTYPER,	AJEQ,	/* alternate */
	"JNE",		LTYPER,	AJNE,
	"JNZ",		LTYPER,	AJNE,	/* alternate */
	"JLS",		LTYPER,	AJLS,
	"JBE",		LTYPER,	AJLS,	/* alternate */
	"JNA",		LTYPER,	AJLS,	/* alternate */
	"JHI",		LTYPER,	AJHI,
	"JA",		LTYPER,	AJHI,	/* alternate */
	"JNBE",		LTYPER,	AJHI,	/* alternate */
	"JMI",		LTYPER,	AJMI,
	"JS",		LTYPER,	AJMI,	/* alternate */
	"JPL",		LTYPER,	AJPL,
	"JNS",		LTYPER,	AJPL,	/* alternate */
	"JPS",		LTYPER,	AJPS,
	"JP",		LTYPER,	AJPS,	/* alternate */
	"JPE",		LTYPER,	AJPS,	/* alternate */
	"JPC",		LTYPER,	AJPC,
	"JNP",		LTYPER,	AJPC,	/* alternate */
	"JPO",		LTYPER,	AJPC,	/* alternate */
	"JLT",		LTYPER,	AJLT,
	"JL",		LTYPER,	AJLT,	/* alternate */
	"JNGE",		LTYPER,	AJLT,	/* alternate */
	"JGE",		LTYPER,	AJGE,
	"JNL",		LTYPER,	AJGE,	/* alternate */
	"JLE",		LTYPER,	AJLE,
	"JNG",		LTYPER,	AJLE,	/* alternate */
	"JGT",		LTYPER,	AJGT,
	"JG",		LTYPER,	AJGT,	/* alternate */
	"JNLE",		LTYPER,	AJGT,	/* alternate */

	"JCXZ",		LTYPER,	AJCXZ,
	"JMP",		LTYPEC,	AJMP,
	"LAHF",		LTYPE0,	ALAHF,
	"LARL",		LTYPE3,	ALARL,
	"LARW",		LTYPE3,	ALARW,
	"LEAL",		LTYPE3,	ALEAL,
	"LEAW",		LTYPE3,	ALEAW,
	"LEAVEL",	LTYPE0,	ALEAVEL,
	"LEAVEW",	LTYPE0,	ALEAVEW,
	"LOCK",		LTYPE0,	ALOCK,
	"LODSB",	LTYPE0,	ALODSB,
	"LODSL",	LTYPE0,	ALODSL,
	"LODSW",	LTYPE0,	ALODSW,
	"LONG",		LTYPE2,	ALONG,
	"LOOP",		LTYPER,	ALOOP,
	"LOOPEQ",	LTYPER,	ALOOPEQ,
	"LOOPNE",	LTYPER,	ALOOPNE,
	"LSLL",		LTYPE3,	ALSLL,
	"LSLW",		LTYPE3,	ALSLW,
	"MOVB",		LTYPE3,	AMOVB,
	"MOVL",		LTYPEM,	AMOVL,
	"MOVW",		LTYPEM,	AMOVW,
	"MOVBLSX",	LTYPE3, AMOVBLSX,
	"MOVBLZX",	LTYPE3, AMOVBLZX,
	"MOVBWSX",	LTYPE3, AMOVBWSX,
	"MOVBWZX",	LTYPE3, AMOVBWZX,
	"MOVWLSX",	LTYPE3, AMOVWLSX,
	"MOVWLZX",	LTYPE3, AMOVWLZX,
	"MOVSB",	LTYPE0,	AMOVSB,
	"MOVSL",	LTYPE0,	AMOVSL,
	"MOVSW",	LTYPE0,	AMOVSW,
	"MULB",		LTYPE2,	AMULB,
	"MULL",		LTYPE2,	AMULL,
	"MULW",		LTYPE2,	AMULW,
	"NEGB",		LTYPE1,	ANEGB,
	"NEGL",		LTYPE1,	ANEGL,
	"NEGW",		LTYPE1,	ANEGW,
	"NOP",		LTYPEN,	ANOP,
	"NOTB",		LTYPE1,	ANOTB,
	"NOTL",		LTYPE1,	ANOTL,
	"NOTW",		LTYPE1,	ANOTW,
	"ORB",		LTYPE3,	AORB,
	"ORL",		LTYPE3,	AORL,
	"ORW",		LTYPE3,	AORW,
	"OUTB",		LTYPE0,	AOUTB,
	"OUTL",		LTYPE0,	AOUTL,
	"OUTW",		LTYPE0,	AOUTW,
	"OUTSB",	LTYPE0,	AOUTSB,
	"OUTSL",	LTYPE0,	AOUTSL,
	"OUTSW",	LTYPE0,	AOUTSW,
	"POPAL",	LTYPE0,	APOPAL,
	"POPAW",	LTYPE0,	APOPAW,
	"POPFL",	LTYPE0,	APOPFL,
	"POPFW",	LTYPE0,	APOPFW,
	"POPL",		LTYPE1,	APOPL,
	"POPW",		LTYPE1,	APOPW,
	"PUSHAL",	LTYPE0,	APUSHAL,
	"PUSHAW",	LTYPE0,	APUSHAW,
	"PUSHFL",	LTYPE0,	APUSHFL,
	"PUSHFW",	LTYPE0,	APUSHFW,
	"PUSHL",	LTYPE2,	APUSHL,
	"PUSHW",	LTYPE2,	APUSHW,
	"RCLB",		LTYPE3,	ARCLB,
	"RCLL",		LTYPE3,	ARCLL,
	"RCLW",		LTYPE3,	ARCLW,
	"RCRB",		LTYPE3,	ARCRB,
	"RCRL",		LTYPE3,	ARCRL,
	"RCRW",		LTYPE3,	ARCRW,
	"REP",		LTYPE0,	AREP,
	"REPN",		LTYPE0,	AREPN,
	"RET",		LTYPE0,	ARET,
	"ROLB",		LTYPE3,	AROLB,
	"ROLL",		LTYPE3,	AROLL,
	"ROLW",		LTYPE3,	AROLW,
	"RORB",		LTYPE3,	ARORB,
	"RORL",		LTYPE3,	ARORL,
	"RORW",		LTYPE3,	ARORW,
	"SAHF",		LTYPE0,	ASAHF,
	"SALB",		LTYPE3,	ASALB,
	"SALL",		LTYPE3,	ASALL,
	"SALW",		LTYPE3,	ASALW,
	"SARB",		LTYPE3,	ASARB,
	"SARL",		LTYPE3,	ASARL,
	"SARW",		LTYPE3,	ASARW,
	"SBBB",		LTYPE3,	ASBBB,
	"SBBL",		LTYPE3,	ASBBL,
	"SBBW",		LTYPE3,	ASBBW,
	"SCASB",	LTYPE0,	ASCASB,
	"SCASL",	LTYPE0,	ASCASL,
	"SCASW",	LTYPE0,	ASCASW,
	"SETCC",	LTYPE1,	ASETCC,
	"SETCS",	LTYPE1,	ASETCS,
	"SETEQ",	LTYPE1,	ASETEQ,
	"SETGE",	LTYPE1,	ASETGE,
	"SETGT",	LTYPE1,	ASETGT,
	"SETHI",	LTYPE1,	ASETHI,
	"SETLE",	LTYPE1,	ASETLE,
	"SETLS",	LTYPE1,	ASETLS,
	"SETLT",	LTYPE1,	ASETLT,
	"SETMI",	LTYPE1,	ASETMI,
	"SETNE",	LTYPE1,	ASETNE,
	"SETOC",	LTYPE1,	ASETOC,
	"SETOS",	LTYPE1,	ASETOS,
	"SETPC",	LTYPE1,	ASETPC,
	"SETPL",	LTYPE1,	ASETPL,
	"SETPS",	LTYPE1,	ASETPS,
	"CDQ",		LTYPE0,	ACDQ,
	"CWD",		LTYPE0,	ACWD,
	"SHLB",		LTYPE3,	ASHLB,
	"SHLL",		LTYPES,	ASHLL,
	"SHLW",		LTYPES,	ASHLW,
	"SHRB",		LTYPE3,	ASHRB,
	"SHRL",		LTYPES,	ASHRL,
	"SHRW",		LTYPES,	ASHRW,
	"STC",		LTYPE0,	ASTC,
	"STD",		LTYPE0,	ASTD,
	"STI",		LTYPE0,	ASTI,
	"STOSB",	LTYPE0,	ASTOSB,
	"STOSL",	LTYPE0,	ASTOSL,
	"STOSW",	LTYPE0,	ASTOSW,
	"SUBB",		LTYPE3,	ASUBB,
	"SUBL",		LTYPE3,	ASUBL,
	"SUBW",		LTYPE3,	ASUBW,
	"SYSCALL",	LTYPE0,	ASYSCALL,
	"TESTB",	LTYPE3,	ATESTB,
	"TESTL",	LTYPE3,	ATESTL,
	"TESTW",	LTYPE3,	ATESTW,
	"TEXT",		LTYPET,	ATEXT,
	"VERR",		LTYPE2,	AVERR,
	"VERW",		LTYPE2,	AVERW,
	"WAIT",		LTYPE0,	AWAIT,
	"WORD",		LTYPE2,	AWORD,
	"XCHGB",	LTYPE3,	AXCHGB,
	"XCHGL",	LTYPE3,	AXCHGL,
	"XCHGW",	LTYPE3,	AXCHGW,
	"XLAT",		LTYPE2,	AXLAT,
	"XORB",		LTYPE3,	AXORB,
	"XORL",		LTYPE3,	AXORL,
	"XORW",		LTYPE3,	AXORW,

	"CMOVLCC",	LTYPE3,	ACMOVLCC,
	"CMOVLCS",	LTYPE3,	ACMOVLCS,
	"CMOVLEQ",	LTYPE3,	ACMOVLEQ,
	"CMOVLGE",	LTYPE3,	ACMOVLGE,
	"CMOVLGT",	LTYPE3,	ACMOVLGT,
	"CMOVLHI",	LTYPE3,	ACMOVLHI,
	"CMOVLLE",	LTYPE3,	ACMOVLLE,
	"CMOVLLS",	LTYPE3,	ACMOVLLS,
	"CMOVLLT",	LTYPE3,	ACMOVLLT,
	"CMOVLMI",	LTYPE3,	ACMOVLMI,
	"CMOVLNE",	LTYPE3,	ACMOVLNE,
	"CMOVLOC",	LTYPE3,	ACMOVLOC,
	"CMOVLOS",	LTYPE3,	ACMOVLOS,
	"CMOVLPC",	LTYPE3,	ACMOVLPC,
	"CMOVLPL",	LTYPE3,	ACMOVLPL,
	"CMOVLPS",	LTYPE3,	ACMOVLPS,
	"CMOVWCC",	LTYPE3,	ACMOVWCC,
	"CMOVWCS",	LTYPE3,	ACMOVWCS,
	"CMOVWEQ",	LTYPE3,	ACMOVWEQ,
	"CMOVWGE",	LTYPE3,	ACMOVWGE,
	"CMOVWGT",	LTYPE3,	ACMOVWGT,
	"CMOVWHI",	LTYPE3,	ACMOVWHI,
	"CMOVWLE",	LTYPE3,	ACMOVWLE,
	"CMOVWLS",	LTYPE3,	ACMOVWLS,
	"CMOVWLT",	LTYPE3,	ACMOVWLT,
	"CMOVWMI",	LTYPE3,	ACMOVWMI,
	"CMOVWNE",	LTYPE3,	ACMOVWNE,
	"CMOVWOC",	LTYPE3,	ACMOVWOC,
	"CMOVWOS",	LTYPE3,	ACMOVWOS,
	"CMOVWPC",	LTYPE3,	ACMOVWPC,
	"CMOVWPL",	LTYPE3,	ACMOVWPL,
	"CMOVWPS",	LTYPE3,	ACMOVWPS,

	"FMOVB",	LTYPE3, AFMOVB,
	"FMOVBP",	LTYPE3, AFMOVBP,
	"FMOVD",	LTYPE3, AFMOVD,
	"FMOVDP",	LTYPE3, AFMOVDP,
	"FMOVF",	LTYPE3, AFMOVF,
	"FMOVFP",	LTYPE3, AFMOVFP,
	"FMOVL",	LTYPE3, AFMOVL,
	"FMOVLP",	LTYPE3, AFMOVLP,
	"FMOVV",	LTYPE3, AFMOVV,
	"FMOVVP",	LTYPE3, AFMOVVP,
	"FMOVW",	LTYPE3, AFMOVW,
	"FMOVWP",	LTYPE3, AFMOVWP,
	"FMOVX",	LTYPE3, AFMOVX,
	"FMOVXP",	LTYPE3, AFMOVXP,
	"FCMOVCC",	LTYPE3, AFCMOVCC,
	"FCMOVCS",	LTYPE3, AFCMOVCS,
	"FCMOVEQ",	LTYPE3, AFCMOVEQ,
	"FCMOVHI",	LTYPE3, AFCMOVHI,
	"FCMOVLS",	LTYPE3, AFCMOVLS,
	"FCMOVNE",	LTYPE3, AFCMOVNE,
	"FCMOVNU",	LTYPE3, AFCMOVNU,
	"FCMOVUN",	LTYPE3, AFCMOVUN,
	"FCOMB",	LTYPE3, AFCOMB,
	"FCOMBP",	LTYPE3, AFCOMBP,
	"FCOMD",	LTYPE3, AFCOMD,
	"FCOMDP",	LTYPE3, AFCOMDP,
	"FCOMDPP",	LTYPE3, AFCOMDPP,
	"FCOMF",	LTYPE3, AFCOMF,
	"FCOMFP",	LTYPE3, AFCOMFP,
	"FCOMI",	LTYPE3, AFCOMI,
	"FCOMIP",	LTYPE3, AFCOMIP,
	"FCOML",	LTYPE3, AFCOML,
	"FCOMLP",	LTYPE3, AFCOMLP,
	"FCOMW",	LTYPE3, AFCOMW,
	"FCOMWP",	LTYPE3, AFCOMWP,
	"FUCOM",	LTYPE3, AFUCOM,
	"FUCOMI",	LTYPE3, AFUCOMI,
	"FUCOMIP",	LTYPE3, AFUCOMIP,
	"FUCOMP",	LTYPE3, AFUCOMP,
	"FUCOMPP",	LTYPE3, AFUCOMPP,
	"FADDW",	LTYPE3, AFADDW,
	"FADDL",	LTYPE3, AFADDL,
	"FADDF",	LTYPE3, AFADDF,
	"FADDD",	LTYPE3, AFADDD,
	"FADDDP",	LTYPE3, AFADDDP,
	"FSUBDP",	LTYPE3, AFSUBDP,
	"FSUBW",	LTYPE3, AFSUBW,
	"FSUBL",	LTYPE3, AFSUBL,
	"FSUBF",	LTYPE3, AFSUBF,
	"FSUBD",	LTYPE3, AFSUBD,
	"FSUBRDP",	LTYPE3, AFSUBRDP,
	"FSUBRW",	LTYPE3, AFSUBRW,
	"FSUBRL",	LTYPE3, AFSUBRL,
	"FSUBRF",	LTYPE3, AFSUBRF,
	"FSUBRD",	LTYPE3, AFSUBRD,
	"FMULDP",	LTYPE3, AFMULDP,
	"FMULW",	LTYPE3, AFMULW,
	"FMULL",	LTYPE3, AFMULL,
	"FMULF",	LTYPE3, AFMULF,
	"FMULD",	LTYPE3, AFMULD,
	"FDIVDP",	LTYPE3, AFDIVDP,
	"FDIVW",	LTYPE3, AFDIVW,
	"FDIVL",	LTYPE3, AFDIVL,
	"FDIVF",	LTYPE3, AFDIVF,
	"FDIVD",	LTYPE3, AFDIVD,
	"FDIVRDP",	LTYPE3, AFDIVRDP,
	"FDIVRW",	LTYPE3, AFDIVRW,
	"FDIVRL",	LTYPE3, AFDIVRL,
	"FDIVRF",	LTYPE3, AFDIVRF,
	"FDIVRD",	LTYPE3, AFDIVRD,
	"FXCHD",	LTYPE3, AFXCHD,
	"FFREE",	LTYPE1, AFFREE,
	"FLDCW",	LTYPE2, AFLDCW,
	"FLDENV",	LTYPE1, AFLDENV,
	"FRSTOR",	LTYPE2, AFRSTOR,
	"FSAVE",	LTYPE1, AFSAVE,
	"FSTCW",	LTYPE1, AFSTCW,
	"FSTENV",	LTYPE1, AFSTENV,
	"FSTSW",	LTYPE1, AFSTSW,
	"F2XM1",	LTYPE0, AF2XM1,
	"FABS",		LTYPE0, AFABS,
	"FCHS",		LTYPE0, AFCHS,
	"FCLEX",	LTYPE0, AFCLEX,
	"FCOS",		LTYPE0, AFCOS,
	"FDECSTP",	LTYPE0, AFDECSTP,
	"FINCSTP",	LTYPE0, AFINCSTP,
	"FINIT",	LTYPE0, AFINIT,
	"FLD1",		LTYPE0, AFLD1,
	"FLDL2E",	LTYPE0, AFLDL2E,
	"FLDL2T",	LTYPE0, AFLDL2T,
	"FLDLG2",	LTYPE0, AFLDLG2,
	"FLDLN2",	LTYPE0, AFLDLN2,
	"FLDPI",	LTYPE0, AFLDPI,
	"FLDZ",		LTYPE0, AFLDZ,
	"FNOP",		LTYPE0, AFNOP,
	"FPATAN",	LTYPE0, AFPATAN,
	"FPREM",	LTYPE0, AFPREM,
	"FPREM1",	LTYPE0, AFPREM1,
	"FPTAN",	LTYPE0, AFPTAN,
	"FRNDINT",	LTYPE0, AFRNDINT,
	"FSCALE",	LTYPE0, AFSCALE,
	"FSIN",		LTYPE0, AFSIN,
	"FSINCOS",	LTYPE0, AFSINCOS,
	"FSQRT",	LTYPE0, AFSQRT,
	"FTST",		LTYPE0, AFTST,
	"FXAM",		LTYPE0, AFXAM,
	"FXTRACT",	LTYPE0, AFXTRACT,
	"FYL2X",	LTYPE0, AFYL2X,
	"FYL2XP1",	LTYPE0, AFYL2XP1,

	0
};

void
cinit(void)
{
	Sym *s;
	int i;

	nullgen.sym = S;
	nullgen.offset = 0;
	if(FPCHIP)
		nullgen.dval = 0;
	for(i=0; i<sizeof(nullgen.sval); i++)
		nullgen.sval[i] = 0;
	nullgen.type = D_NONE;
	nullgen.index = D_NONE;
	nullgen.scale = 0;

	nerrors = 0;
	iostack = I;
	iofree = I;
	peekc = IGN;
	nhunk = 0;
	for(i=0; i<NHASH; i++)
		hash[i] = S;
	for(i=0; itab[i].name; i++) {
		s = slookup(itab[i].name);
		if(s->type != LNAME)
			yyerror("double initialization %s", itab[i].name);
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
checkscale(int scale)
{

	switch(scale) {
	case 1:
	case 2:
	case 4:
	case 8:
		return;
	}
	yyerror("scale must be 1248: %d", scale);
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

	Bputc(&obuf, ANAME);		/* as(2) */
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
	if(a->index != D_NONE || a->scale != 0)
		t |= T_INDEX;
	if(a->offset != 0)
		t |= T_OFFSET;
	if(s != 0)
		t |= T_SYM;

	switch(a->type) {
	default:
		t |= T_TYPE;
		break;
	case D_FCONST:
		t |= T_FCONST;
		break;
	case D_CONST2:
		t |= T_OFFSET|T_OFFSET2;
		break;
	case D_SCONST:
		t |= T_SCONST;
		break;
	case D_NONE:
		break;
	}
	Bputc(&obuf, t);

	if(t & T_INDEX) {	/* implies index, scale */
		Bputc(&obuf, a->index);
		Bputc(&obuf, a->scale);
	}
	if(t & T_OFFSET) {	/* implies offset */
		l = a->offset;
		Bputc(&obuf, l);
		Bputc(&obuf, l>>8);
		Bputc(&obuf, l>>16);
		Bputc(&obuf, l>>24);
	}
	if(t & T_OFFSET2) {
		l = a->offset2;
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
	if(t & T_TYPE)
		Bputc(&obuf, a->type);
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
		t = g2->from.type;
		if(t == D_ADDR)
			t = g2->from.index;
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
		t = g2->to.type;
		if(t == D_ADDR)
			t = g2->to.index;
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
