#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "arm64.h"

uvlong	decodebitmask(ulong, ulong, ulong);
void	call(uvlong);
void	ret(uvlong);
char	runcond(ulong);
ulong	add32(ulong, ulong, ulong, Pstate *);
uvlong	add64(uvlong, uvlong, uvlong, Pstate *);
vlong	sext(ulong, char);
vlong	doshift(ulong, vlong, ulong, ulong);
vlong	shift64(vlong, ulong, ulong);
long	shift32(long, ulong, ulong);
char	pstatecmp(Registers *, Registers *);

void	cmpb(ulong);
void	condb(ulong);
void	sys(ulong);
void	tb(ulong);
void	uncondbimm(ulong);
void	uncondbreg(ulong);
void	addsubimm(ulong);
void	bitfield(ulong);
void	extract(ulong);
void	logimm(ulong);
void	movwimm(ulong);
void	pcrel(ulong);
void	addsubreg(ulong);
void	addsubsreg(ulong);
void	addsubc(ulong);
void	condcmpimm(ulong);
void	condcmpreg(ulong);
void	condsel(ulong);
void	dp1(ulong);
void	dp2(ulong);
void	dp3(ulong);
void	logsreg(ulong);
void	ldstreg(ulong);
void	ldstex(ulong);
void	ldstnoallocp(ulong);
void	ldstregimmpost(ulong);
void	ldstregimmpre(ulong);
void	ldstregoff(ulong);
void	ldstregupriv(ulong);
void	ldstreguscaleimm(ulong);
void	ldstregusignimm(ulong);
void	ldstregpoff(ulong);
void	ldstregppost(ulong);
void	ldstregppre(ulong);

Inst itab[] =
{
	/* compare and branch */
	[Ccmpb+ 0]	{cmpb, "CBZ",   	Ibranch}, /* 0 */
	[Ccmpb+ 2]	{cmpb, "CBZ",   	Ibranch}, /* 2 */
	[Ccmpb+ 1]	{cmpb, "CBNZ",  	Ibranch}, /* 1 */
	[Ccmpb+ 3]	{cmpb, "CBNZ",  	Ibranch}, /* 3 */

	/* conditional branch */
	[Ccb+ 0]	{condb, "B.cond",	Ibranch}, /* 64 */

	/* exception generation */
	[Cex+ 1]	{syscall, "SVC",   	Isyscall}, /* 129 */

	/* system */
	[Csys+26]	{sys, "CLREX", 	  Isys}, /* 218 */

	/* test and branch */
	[Ctb+ 0]	{tb, "TBZ",   	Ibranch}, /* 256 */
	[Ctb+ 1]	{tb, "TBNZ",  	Ibranch}, /* 257 */

	/* unconditional branch imm */
	[Cubi+ 0]	{uncondbimm, "B",     	Ibranch}, /* 320 */
	[Cubi+ 1]	{uncondbimm, "BL",    	Ibranch}, /* 321 */

	/* unconditional branch reg */
	[Cubr+31]	{uncondbreg, "BR",    	Ibranch}, /* 415 */
	[Cubr+63]	{uncondbreg, "BLR",   	Ibranch}, /* 447 */
	[Cubr+95]	{uncondbreg, "RET",   	Ibranch}, /* 479 */

	/* add/sub imm */
	[Cai+ 0]	{addsubimm, "ADD",   	Iarith}, /* 448 */
	[Cai+ 1]	{addsubimm, "ADDS",  	Iarith}, /* 449 */
	[Cai+ 2]	{addsubimm, "SUB",   	Iarith}, /* 450 */
	[Cai+ 3]	{addsubimm, "SUBS",  	Iarith}, /* 451 */
	[Cai+ 4]	{addsubimm, "ADD",   	Iarith}, /* 452 */
	[Cai+ 5]	{addsubimm, "ADDS",  	Iarith}, /* 453 */
	[Cai+ 6]	{addsubimm, "SUB",   	Iarith}, /* 454 */
	[Cai+ 7]	{addsubimm, "SUBS",  	Iarith}, /* 455 */

	/* bitfield */
	[Cab+ 0]	{bitfield, "SBFM",  	Iarith}, /* 512 */
	[Cab+ 2]	{bitfield, "BFM",   	Iarith}, /* 514 */
	[Cab+ 4]	{bitfield, "UBFM",  	Iarith}, /* 516 */
	[Cab+ 9]	{bitfield, "SBFM",  	Iarith}, /* 521 */
	[Cab+11]	{bitfield, "BFM",   	Iarith}, /* 523 */
	[Cab+13]	{bitfield, "UBFM",  	Iarith}, /* 525 */

	/* extract */
	[Cax+ 0]	{extract, "EXTR",  	Iarith}, /* 576 */
	[Cax+18]	{extract, "EXTR",  	Iarith}, /* 594 */

	/* logic imm */
	[Cali+ 0]	{logimm, "AND",   	  Ilog}, /* 640 */
	[Cali+ 1]	{logimm, "ORR",   	  Ilog}, /* 641 */
	[Cali+ 2]	{logimm, "EOR",   	  Ilog}, /* 642 */
	[Cali+ 3]	{logimm, "ANDS",  	  Ilog}, /* 643 */
	[Cali+ 4]	{logimm, "AND",   	  Ilog}, /* 644 */
	[Cali+ 5]	{logimm, "ORR",   	  Ilog}, /* 645 */
	[Cali+ 6]	{logimm, "EOR",   	  Ilog}, /* 646 */
	[Cali+ 7]	{logimm, "ANDS",  	  Ilog}, /* 647 */

	/* move wide imm */
	[Camwi+ 0]	{movwimm, "MOVN",  	Iarith}, /* 704 */
	[Camwi+ 2]	{movwimm, "MOVZ",  	Iarith}, /* 706 */
	[Camwi+ 3]	{movwimm, "MOVK",  	Iarith}, /* 707 */
	[Camwi+ 4]	{movwimm, "MOVN",  	Iarith}, /* 708 */
	[Camwi+ 6]	{movwimm, "MOVZ",  	Iarith}, /* 710 */
	[Camwi+ 7]	{movwimm, "MOVK",  	Iarith}, /* 711 */

	/* PC-rel addr */
	[Capcr+ 0]	{pcrel, "ADR",   	Iarith}, /* 768 */
	[Capcr+ 1]	{pcrel, "ADRP",  	Iarith}, /* 769 */

	/* add/sub extended reg */
	[Car+ 0]	{addsubreg, "ADD",   	Iarith}, /* 832 */
	[Car+ 4]	{addsubreg, "ADDS",  	Iarith}, /* 836 */
	[Car+ 8]	{addsubreg, "SUB",   	Iarith}, /* 840 */
	[Car+12]	{addsubreg, "SUBS",  	Iarith}, /* 844 */
	[Car+16]	{addsubreg, "ADD",   	Iarith}, /* 848 */
	[Car+20]	{addsubreg, "ADDS",  	Iarith}, /* 852 */
	[Car+24]	{addsubreg, "SUB",   	Iarith}, /* 856 */
	[Car+28]	{addsubreg, "SUBS",  	Iarith}, /* 860 */

	/* add/sub shift-reg */
	[Casr+ 0]	{addsubsreg, "ADD",   	Iarith}, /* 896 */
	[Casr+ 1]	{addsubsreg, "ADDS",  	Iarith}, /* 897 */
	[Casr+ 2]	{addsubsreg, "SUB",   	Iarith}, /* 898 */
	[Casr+ 3]	{addsubsreg, "SUBS",  	Iarith}, /* 899 */
	[Casr+ 4]	{addsubsreg, "ADD",   	Iarith}, /* 900 */
	[Casr+ 5]	{addsubsreg, "ADDS",  	Iarith}, /* 901 */
	[Casr+ 6]	{addsubsreg, "SUB",   	Iarith}, /* 902 */
	[Casr+ 7]	{addsubsreg, "SUBS",  	Iarith}, /* 903 */

	/* add/sub carry */
	[Cac+ 0]	{addsubc, "ADC",   	Iarith}, /* 960 */
	[Cac+ 1]	{addsubc, "ADCS",  	Iarith}, /* 961 */
	[Cac+ 2]	{addsubc, "SBC",   	Iarith}, /* 962 */
	[Cac+ 3]	{addsubc, "SBCS",  	Iarith}, /* 963 */
	[Cac+ 4]	{addsubc, "ADC",   	Iarith}, /* 964 */
	[Cac+ 5]	{addsubc, "ADCS",  	Iarith}, /* 965 */
	[Cac+ 6]	{addsubc, "SBC",   	Iarith}, /* 966 */
	[Cac+ 7]	{addsubc, "SBCS",  	Iarith}, /* 967 */

	/* cond compare imm */
	[Caci+ 1]	{condcmpimm, "CCMN",  	Iarith}, /* 1025 */
	[Caci+ 3]	{condcmpimm, "CCMP",  	Iarith}, /* 1027 */
	[Caci+ 5]	{condcmpimm, "CCMN",  	Iarith}, /* 1029 */
	[Caci+ 7]	{condcmpimm, "CCMP",  	Iarith}, /* 1031 */

	/* cond compare reg */
	[Cacr+ 1]	{condcmpreg, "CCMN",  	Iarith}, /* 1089 */
	[Cacr+ 3]	{condcmpreg, "CCMP",  	Iarith}, /* 1091 */
	[Cacr+ 5]	{condcmpreg, "CCMN",  	Iarith}, /* 1093 */
	[Cacr+ 7]	{condcmpreg, "CCMP",  	Iarith}, /* 1095 */

	/* cond select */
	[Cacs+ 0]	{condsel, "CSEL",  	Iarith}, /* 1152 */
	[Cacs+ 1]	{condsel, "CSINC", 	Iarith}, /* 1153 */
	[Cacs+ 8]	{condsel, "CSINV", 	Iarith}, /* 1160 */
	[Cacs+ 9]	{condsel, "CSNEG", 	Iarith}, /* 1161 */
	[Cacs+16]	{condsel, "CSEL",  	Iarith}, /* 1168 */
	[Cacs+17]	{condsel, "CSINC", 	Iarith}, /* 1169 */
	[Cacs+24]	{condsel, "CSINV", 	Iarith}, /* 1176 */
	[Cacs+25]	{condsel, "CSNEG", 	Iarith}, /* 1177 */

	/* data proc 1 src */
	[Ca1+ 0]	{dp1, "RBIT",  	Iarith}, /* 1216 */
	[Ca1+ 1]	{dp1, "REV16", 	Iarith}, /* 1217 */
	[Ca1+ 2]	{dp1, "REV",   	Iarith}, /* 1218 */
	[Ca1+ 4]	{dp1, "CLZ",   	Iarith}, /* 1220 */
	[Ca1+ 5]	{dp1, "CLZ",   	Iarith}, /* 1221 */
	[Ca1+16]	{dp1, "RBIT",  	Iarith}, /* 1232 */
	[Ca1+17]	{dp1, "REV16", 	Iarith}, /* 1233 */
	[Ca1+18]	{dp1, "REV32", 	Iarith}, /* 1234 */
	[Ca1+19]	{dp1, "REV",   	Iarith}, /* 1235 */
	[Ca1+20]	{dp1, "CLZ",   	Iarith}, /* 1236 */
	[Ca1+21]	{dp1, "CLZ",   	Iarith}, /* 1237 */

	/* data proc 2 src */
	[Ca2+ 2]	{dp2, "UDIV",  	Iarith}, /* 1282 */
	[Ca2+ 3]	{dp2, "SDIV",  	Iarith}, /* 1283 */
	[Ca2+ 8]	{dp2, "LSLV",  	Iarith}, /* 1288 */
	[Ca2+ 9]	{dp2, "LSRV",  	Iarith}, /* 1289 */
	[Ca2+10]	{dp2, "ASRV",  	Iarith}, /* 1290 */
	[Ca2+11]	{dp2, "RORV",  	Iarith}, /* 1291 */
	[Ca2+34]	{dp2, "UDIV",  	Iarith}, /* 1314 */
	[Ca2+35]	{dp2, "SDIV",  	Iarith}, /* 1315 */
	[Ca2+40]	{dp2, "LSLV",  	Iarith}, /* 1320 */
	[Ca2+41]	{dp2, "LSRV",  	Iarith}, /* 1321 */
	[Ca2+42]	{dp2, "ASRV",  	Iarith}, /* 1322 */
	[Ca2+43]	{dp2, "RORV",  	Iarith}, /* 1323 */

	/* data proc 3 src */
	[Ca3+ 0]	{dp3, "MADD",  	Iarith}, /* 1344 */
	[Ca3+ 1]	{dp3, "MSUB",  	Iarith}, /* 1345 */
	[Ca3+16]	{dp3, "MADD",  	Iarith}, /* 1360 */
	[Ca3+17]	{dp3, "MSUB",  	Iarith}, /* 1361 */

	/* logic shift-reg */
	[Calsr+ 0]	{logsreg, "AND",   	  Ilog}, /* 1408 */
	[Calsr+ 1]	{logsreg, "BIC",   	  Ilog}, /* 1409 */
	[Calsr+ 2]	{logsreg, "ORR",   	  Ilog}, /* 1410 */
	[Calsr+ 3]	{logsreg, "ORN",   	  Ilog}, /* 1411 */
	[Calsr+ 4]	{logsreg, "EOR",   	  Ilog}, /* 1412 */
	[Calsr+ 5]	{logsreg, "EON",   	  Ilog}, /* 1413 */
	[Calsr+ 6]	{logsreg, "ANDS",  	  Ilog}, /* 1414 */
	[Calsr+ 7]	{logsreg, "BICS",  	  Ilog}, /* 1415 */
	[Calsr+ 8]	{logsreg, "AND",   	  Ilog}, /* 1416 */
	[Calsr+ 9]	{logsreg, "BIC",   	  Ilog}, /* 1417 */
	[Calsr+10]	{logsreg, "ORR",   	  Ilog}, /* 1418 */
	[Calsr+11]	{logsreg, "ORN",   	  Ilog}, /* 1419 */
	[Calsr+12]	{logsreg, "EOR",   	  Ilog}, /* 1420 */
	[Calsr+13]	{logsreg, "EON",   	  Ilog}, /* 1421 */
	[Calsr+14]	{logsreg, "ANDS",  	  Ilog}, /* 1422 */
	[Calsr+15]	{logsreg, "BICS",  	  Ilog}, /* 1423 */

	/* load/store reg */
	[Clsr+ 0]	{ldstreg, "LDR",   	 Iload}, /* 1472 */
	[Clsr+ 2]	{ldstreg, "LDR",   	 Iload}, /* 1474 */
	[Clsr+ 4]	{ldstreg, "LDRSW", 	 Iload}, /* 1476 */
	[Clsr+ 6]	{ldstreg, "PRFM",  	 Iload}, /* 1478 */

	/* load/store ex */
	[Clsx+ 0]	{ldstex, "STXRB", 	Istore}, /* 1536 */
	[Clsx+ 1]	{ldstex, "STLXRB",	Istore}, /* 1537 */
	[Clsx+ 4]	{ldstex, "LDXRB", 	 Iload}, /* 1540 */
	[Clsx+ 5]	{ldstex, "LDAXRB",	 Iload}, /* 1541 */
	[Clsx+ 9]	{ldstex, "STLRB", 	Istore}, /* 1545 */
	[Clsx+13]	{ldstex, "LDARB", 	 Iload}, /* 1549 */
	[Clsx+16]	{ldstex, "STXRH", 	Istore}, /* 1552 */
	[Clsx+17]	{ldstex, "STLXRH",	Istore}, /* 1553 */
	[Clsx+20]	{ldstex, "LDXRH", 	 Iload}, /* 1556 */
	[Clsx+21]	{ldstex, "LDAXRH",	 Iload}, /* 1557 */
	[Clsx+25]	{ldstex, "STLRH", 	Istore}, /* 1561 */
	[Clsx+29]	{ldstex, "LDARH", 	 Iload}, /* 1565 */
	[Clsx+32]	{ldstex, "STXR",  	Istore}, /* 1568 */
	[Clsx+33]	{ldstex, "STLXR", 	Istore}, /* 1569 */
	[Clsx+34]	{ldstex, "STXP",  	Istore}, /* 1570 */
	[Clsx+35]	{ldstex, "STLXP", 	Istore}, /* 1571 */
	[Clsx+36]	{ldstex, "LDXR",  	 Iload}, /* 1572 */
	[Clsx+37]	{ldstex, "LDAXR", 	 Iload}, /* 1573 */
	[Clsx+38]	{ldstex, "LDXP",  	 Iload}, /* 1574 */
	[Clsx+39]	{ldstex, "LDAXP", 	 Iload}, /* 1575 */
	[Clsx+41]	{ldstex, "STLR",  	Istore}, /* 1577 */
	[Clsx+45]	{ldstex, "STLR",  	Istore}, /* 1581 */
	[Clsx+48]	{ldstex, "STXR",  	Istore}, /* 1584 */
	[Clsx+49]	{ldstex, "STLXR", 	Istore}, /* 1585 */
	[Clsx+50]	{ldstex, "STXP",  	Istore}, /* 1586 */
	[Clsx+51]	{ldstex, "STLXP", 	Istore}, /* 1587 */
	[Clsx+52]	{ldstex, "LDXR",  	 Iload}, /* 1588 */
	[Clsx+53]	{ldstex, "LDAXR", 	 Iload}, /* 1589 */
	[Clsx+54]	{ldstex, "LDXP",  	 Iload}, /* 1590 */
	[Clsx+55]	{ldstex, "LDAXP", 	 Iload}, /* 1591 */
	[Clsx+57]	{ldstex, "STLR",  	Istore}, /* 1593 */
	[Clsx+61]	{ldstex, "LDAR",  	 Iload}, /* 1597 */

	/* load/store no-alloc pair (off) */
	[Clsnp+ 0]	{ldstnoallocp, "STNP",  	Istore}, /* 1600 */
	[Clsnp+ 1]	{ldstnoallocp, "LDNP",  	 Iload}, /* 1601 */
	[Clsnp+ 8]	{ldstnoallocp, "STNP",  	Istore}, /* 1608 */
	[Clsnp+ 9]	{ldstnoallocp, "LDNP",  	 Iload}, /* 1609 */

	/* load/store reg (imm post-index) */
	[Clspos+ 0]	{ldstregimmpost, "STRB",  	Istore}, /* 1664 */
	[Clspos+ 1]	{ldstregimmpost, "LDRB",  	 Iload}, /* 1665 */
	[Clspos+ 2]	{ldstregimmpost, "LDRSB", 	 Iload}, /* 1666 */
	[Clspos+ 3]	{ldstregimmpost, "LDRSB", 	 Iload}, /* 1667 */
	[Clspos+ 8]	{ldstregimmpost, "STRH",  	Istore}, /* 1672 */
	[Clspos+ 9]	{ldstregimmpost, "LDRH",  	 Iload}, /* 1673 */
	[Clspos+10]	{ldstregimmpost, "LDRSH", 	 Iload}, /* 1674 */
	[Clspos+11]	{ldstregimmpost, "LDRSH", 	 Iload}, /* 1675 */
	[Clspos+16]	{ldstregimmpost, "STR",   	Istore}, /* 1680 */
	[Clspos+17]	{ldstregimmpost, "LDR",   	 Iload}, /* 1681 */
	[Clspos+18]	{ldstregimmpost, "LDRSW", 	 Iload}, /* 1682 */
	[Clspos+24]	{ldstregimmpost, "STR",   	Istore}, /* 1688 */
	[Clspos+25]	{ldstregimmpost, "LDR",   	 Iload}, /* 1689 */

	/* load/store reg (imm pre-index) */
	[Clspre+ 0]	{ldstregimmpre, "STRB",  	Istore}, /* 1728 */
	[Clspre+ 1]	{ldstregimmpre, "LDRB",  	 Iload}, /* 1729 */
	[Clspre+ 2]	{ldstregimmpre, "LDRSB", 	 Iload}, /* 1730 */
	[Clspre+ 3]	{ldstregimmpre, "LDRSB", 	 Iload}, /* 1731 */
	[Clspre+ 8]	{ldstregimmpre, "STRH",  	Istore}, /* 1736 */
	[Clspre+ 9]	{ldstregimmpre, "LDRH",  	 Iload}, /* 1737 */
	[Clspre+10]	{ldstregimmpre, "LDRSH", 	 Iload}, /* 1738 */
	[Clspre+11]	{ldstregimmpre, "LDRSH", 	 Iload}, /* 1739 */
	[Clspre+16]	{ldstregimmpre, "STR",   	Istore}, /* 1744 */
	[Clspre+17]	{ldstregimmpre, "LDR",   	 Iload}, /* 1745 */
	[Clspre+18]	{ldstregimmpre, "LDRSW", 	 Iload}, /* 1746 */
	[Clspre+24]	{ldstregimmpre, "STR",   	Istore}, /* 1752 */
	[Clspre+25]	{ldstregimmpre, "LDR",   	 Iload}, /* 1753 */

	/* load/store reg (off) */
	[Clso+ 0]	{ldstregoff, "STRB",  	Istore}, /* 1792 */
	[Clso+ 1]	{ldstregoff, "LDRB",  	 Iload}, /* 1793 */
	[Clso+ 2]	{ldstregoff, "LDRSB", 	 Iload}, /* 1794 */
	[Clso+ 3]	{ldstregoff, "LDRSB", 	 Iload}, /* 1795 */
	[Clso+ 8]	{ldstregoff, "STRH",  	Istore}, /* 1800 */
	[Clso+ 9]	{ldstregoff, "LDRH",  	 Iload}, /* 1801 */
	[Clso+10]	{ldstregoff, "LDRSH", 	 Iload}, /* 1802 */
	[Clso+11]	{ldstregoff, "LDRSH", 	 Iload}, /* 1803 */
	[Clso+16]	{ldstregoff, "STR",   	Istore}, /* 1808 */
	[Clso+17]	{ldstregoff, "LDR",   	 Iload}, /* 1809 */
	[Clso+18]	{ldstregoff, "LDRSW", 	 Iload}, /* 1810 */
	[Clso+24]	{ldstregoff, "STR",   	Istore}, /* 1816 */
	[Clso+25]	{ldstregoff, "LDR",   	 Iload}, /* 1817 */
	[Clso+26]	{ldstregoff, "PRFM",  	 Iload}, /* 1818 */

	/* load/store reg (unpriv) */
	[Clsu+ 0]	{ldstregupriv, "STTRB", 	Istore}, /* 1856 */
	[Clsu+ 1]	{ldstregupriv, "LDTRB", 	 Iload}, /* 1857 */
	[Clsu+ 2]	{ldstregupriv, "LDTRSB",	 Iload}, /* 1858 */
	[Clsu+ 3]	{ldstregupriv, "LDTRSB",	 Iload}, /* 1859 */
	[Clsu+ 8]	{ldstregupriv, "STTRH", 	Istore}, /* 1864 */
	[Clsu+ 9]	{ldstregupriv, "LDTRH", 	 Iload}, /* 1865 */
	[Clsu+10]	{ldstregupriv, "LDTRSH",	 Iload}, /* 1866 */
	[Clsu+11]	{ldstregupriv, "LDTRSH",	 Iload}, /* 1867 */
	[Clsu+16]	{ldstregupriv, "STTR",  	Istore}, /* 1872 */
	[Clsu+17]	{ldstregupriv, "LDTR",  	 Iload}, /* 1873 */
	[Clsu+18]	{ldstregupriv, "LDTRSW",	 Iload}, /* 1874 */
	[Clsu+24]	{ldstregupriv, "STTR",  	Istore}, /* 1880 */
	[Clsu+25]	{ldstregupriv, "LDTR",  	 Iload}, /* 1881 */

	/* load/store reg (unscaled imm) */
	[Clsuci+ 0]	{ldstreguscaleimm, "STURB", 	Istore}, /* 1920 */
	[Clsuci+ 1]	{ldstreguscaleimm, "LDURB", 	 Iload}, /* 1921 */
	[Clsuci+ 2]	{ldstreguscaleimm, "LDURSB",	 Iload}, /* 1922 */
	[Clsuci+ 3]	{ldstreguscaleimm, "LDURSB",	 Iload}, /* 1923 */
	[Clsuci+ 8]	{ldstreguscaleimm, "STURH", 	Istore}, /* 1928 */
	[Clsuci+ 9]	{ldstreguscaleimm, "LDURH", 	 Iload}, /* 1929 */
	[Clsuci+10]	{ldstreguscaleimm, "LDURSH",	 Iload}, /* 1930 */
	[Clsuci+11]	{ldstreguscaleimm, "LDURSH",	 Iload}, /* 1931 */
	[Clsuci+16]	{ldstreguscaleimm, "STUR",  	Istore}, /* 1936 */
	[Clsuci+17]	{ldstreguscaleimm, "LDUR",  	 Iload}, /* 1937 */
	[Clsuci+18]	{ldstreguscaleimm, "LDURSW",	 Iload}, /* 1938 */
	[Clsuci+24]	{ldstreguscaleimm, "STUR",  	Istore}, /* 1944 */
	[Clsuci+25]	{ldstreguscaleimm, "LDUR",  	 Iload}, /* 1945 */
	[Clsuci+26]	{ldstreguscaleimm, "PRFUM", 	 Iload}, /* 1946 */

	/* load/store reg (unsigned imm) */
	[Clsusi+ 0]	{ldstregusignimm, "STRB",  	Istore}, /* 1984 */
	[Clsusi+ 1]	{ldstregusignimm, "LDRB",  	 Iload}, /* 1985 */
	[Clsusi+ 2]	{ldstregusignimm, "LDRSB", 	 Iload}, /* 1986 */
	[Clsusi+ 3]	{ldstregusignimm, "LDRSB", 	 Iload}, /* 1987 */
	[Clsusi+ 8]	{ldstregusignimm, "STRH",  	Istore}, /* 1992 */
	[Clsusi+ 9]	{ldstregusignimm, "LDRH",  	 Iload}, /* 1993 */
	[Clsusi+10]	{ldstregusignimm, "LDRSH", 	 Iload}, /* 1994 */
	[Clsusi+11]	{ldstregusignimm, "LDRSH", 	 Iload}, /* 1995 */
	[Clsusi+16]	{ldstregusignimm, "STR",   	Istore}, /* 2000 */
	[Clsusi+17]	{ldstregusignimm, "LDR",   	 Iload}, /* 2001 */
	[Clsusi+18]	{ldstregusignimm, "LDRSW", 	 Iload}, /* 2002 */
	[Clsusi+24]	{ldstregusignimm, "STR",   	Istore}, /* 2008 */
	[Clsusi+25]	{ldstregusignimm, "LDR",   	 Iload}, /* 2009 */
	[Clsusi+26]	{ldstregusignimm, "PRFM",  	Istore}, /* 2010 */

	/* load/store reg-pair (off) */
	[Clsrpo+ 0]	{ldstregpoff, "STP",   	Istore}, /* 2048 */
	[Clsrpo+ 1]	{ldstregpoff, "LDP",   	 Iload}, /* 2049 */
	[Clsrpo+ 5]	{ldstregpoff, "LDPSW", 	 Iload}, /* 2053 */
	[Clsrpo+ 8]	{ldstregpoff, "STP",   	 Iload}, /* 2056 */
	[Clsrpo+ 9]	{ldstregpoff, "LDP",   	 Iload}, /* 2057 */

	/* load/store reg-pair (post-index) */
	[Clsppo+ 0]	{ldstregppost, "STP",   	Istore}, /* 2112 */
	[Clsppo+ 1]	{ldstregppost, "LDP",   	 Iload}, /* 2113 */
	[Clsppo+ 5]	{ldstregppost, "LDPSW", 	 Iload}, /* 2117 */
	[Clsppo+ 8]	{ldstregppost, "STP",   	Istore}, /* 2120 */
	[Clsppo+ 9]	{ldstregppost, "LDP",   	 Iload}, /* 2121 */

	/* load/store reg-pair (pre-index) */
	[Clsppr+ 0]	{ldstregppre, "STP",   	Istore}, /* 2176 */
	[Clsppr+ 1]	{ldstregppre, "LDP",   	 Iload}, /* 2177 */
	[Clsppr+ 5]	{ldstregppre, "LDPSW", 	 Iload}, /* 2181 */
	[Clsppr+ 8]	{ldstregppre, "STP",   	Istore}, /* 2184 */
	[Clsppr+ 9]	{ldstregppre, "LDP",   	 Iload}, /* 2185 */

	[Cundef]	{undef, "???",   	 Inop}, /* 2208 */

	{ 0 }
};

uvlong
decodebitmask(ulong N, ulong imms, ulong immr)
{
	uvlong S, R, levels, welem, rwelem, wmask;
	ulong nimms, len, size;
	int i;
	
	nimms = N<<6 | (~imms)&0x3F;
	len = 0;
	while(nimms >>= 1)
		len++;
	size = 1<<len;

	levels = ((1<<len)-1)&0x3F;
	S = imms & levels;
	R = immr & levels;
	welem = (1ULL<<(S+1))-1;
	rwelem = (welem >> R) | ((welem&((1ULL<<(R+1))-1)) << (size-R));
	wmask = 0LL;
	for(i = 0; i < 64; i += size)
		wmask |= rwelem << i;
	return wmask;
}

void
call(uvlong npc)
{
	Symbol s;

	if(calltree) {
		findsym(npc, CTEXT, &s);
		Bprint(bioout, "%8lux %s(", reg.pc, s.name);
		printparams(&s, reg.r[31]);
		Bprint(bioout, "from ");
		printsource(reg.pc);
		Bputc(bioout, '\n');
	}
}

void
ret(uvlong npc)
{
	Symbol s;

	if(calltree) {
		findsym(npc, CTEXT, &s);
		Bprint(bioout, "%8lux return to #%llux %s r0=#%llux\n",
					reg.pc, npc, s.name, reg.r[0]);
	}
}

char
runcond(ulong cond)
{
	char r;

	SET(r);	/* silence the compiler */
	switch(cond>>1) {
	case 0:	/* EQ or NE */
		r = reg.Z;
		break;
	case 1:	/* CS or CC */
		r = reg.C;
		break;
	case 2:	/* MI or PL */
		r = reg.N;
		break;
	case 3:	/* VS or VC */
		r = reg.V;
		break;
	case 4:	/* HI or LS */
		r = reg.C && reg.Z == 0;
		break;
	case 5:	/* GE or LT */
		r = reg.N == reg.V;
		break;
	case 6:	/* GT or LE */
		r = reg.N == reg.V && reg.Z == 0;
		break;
	case 7:	/* AL */
		r = 1;
		break;
	}
	if(cond&1 && (cond != 0xF))
		r = r ? 0 : 1;
	return r;
}

ulong
add32(ulong x, ulong y, ulong c, Pstate *p)
{
	uvlong uvsum;

	uvsum = (uvlong)x + (uvlong)y + (uvlong)c;
	if(!p)
		return uvsum;
	p->N = (ulong)uvsum >> 31;
	p->Z = ((ulong)uvsum == 0);
	p->C = uvsum>>32 & 1;
	p->V = (x>>31 == y>>31) && ((ulong)uvsum>>31 != x>>31);
	return uvsum;
}

uvlong
add64(uvlong x, uvlong y, uvlong c, Pstate *p)
{
	uvlong uvsum, wocarry;
	char carry;

	if(!p)
		return x + y + c;
	wocarry = x + y;
	carry = (wocarry < x) || (wocarry < y);
	uvsum = wocarry + c;
	carry |= (uvsum < wocarry) || (uvsum < c);
	p->C = carry;
	p->V = (x>>63 == y>>63) && (uvsum>>63 != x>>63);
	p->N = uvsum >> 63;
	p->Z = (uvsum == 0);
	return uvsum;
}

/* sext sign extends a bit-sized number encoded in v to a vlong. */
vlong
sext(ulong v, char bit)
{
	if((v>>(bit-1))&1)
		return ~((1LL<<bit)-1) | v;
	return v;
}

/* doshift, not shift, because we want to preserve names in the ARM64
manual, and shift is used in many instruction encodings as a field
name.*/
vlong
doshift(ulong sf, vlong v, ulong typ, ulong bits)
{
	if(bits == 0)
		return v;
	if(sf == 1) /* 64-bit */
		return shift64(v, typ, bits);
	/* shift32 returns long, so it will be sign extended. */
	return shift32((uvlong)v, typ, bits);
}

vlong
shift64(vlong v, ulong typ, ulong bits)
{
	switch(typ) {
	case 0:	/* logical left */
		return v << bits;
	case 1: /* logical right */
		return (uvlong)v >> bits;
	case 2:	/* arith right */
		return v >> bits;
	case 3:	/* rotate right */
		return (v << (64-bits)) | ((uvlong)v >> bits);
	}
	/* not reached */
	return 0xbad0ULL << 60;
}

long
shift32(long v, ulong typ, ulong bits)
{
	switch(typ) {
	case 0:	/* logical left */
		return v << bits;
	case 1: /* logical right */
		return (ulong)v >> bits;
	case 2:	/* arith right */
		return v >> bits;
	case 3:	/* rotate right */
		return (v << (32-bits)) | ((ulong)v >> bits);
	}
	/* not reached */
	return 0xbad1U << 28;
}

char
pstatecmp(Registers *a, Registers *b)
{
	if(a->N != b->N)
		return 1;
	if(a->Z != b->Z)
		return 1;
	if(a->C != b->C)
		return 1;
	if(a->V != b->V)
		return 1;
	return 0;
}

void
run(void)
{
	Registers saved;
	int i;

	do {
		reg.ir = ifetch(reg.pc);
		ci = &itab[getxo(reg.ir)];
		if(ci && ci->func){
			saved = reg;
			ci->count++;
			(*ci->func)(reg.ir);
			if(regdb) {
				for(i = 0; i <= 31; i++) {
					if(reg.r[i] != saved.r[i])
						print("R%d	0x%llux -> 0x%llux	(%+lld 0x%llux)\n", i, saved.r[i], reg.r[i], reg.r[i] - saved.r[i], reg.r[i] - saved.r[i]);
				}
				if(pstatecmp(&saved, &reg))
					print("NZCV	%d%d%d%d -> %d%d%d%d\n", saved.N, saved.Z, saved.C, saved.V, reg.N, reg.Z, reg.C, reg.V);
			}
		} else {
			if(ci && ci->name && trace)
				itrace("%s\t[not yet done]", ci->name);
			else
				undef(reg.ir);
		}
		reg.prevpc = saved.pc;
		reg.pc += 4;
		if(bplist)
			brkchk(reg.pc, Instruction);
	}while(--count);
}

/* Font
getxo returns the "super-opcode" (xo) of an instruction. The xo is
the bitwise OR between the class opcode (see arm64.h:/classes) and
the number constructed from the class-select bits (the bullets in
the table below). The xo can index the itab table.

Instruction encoding table, per class. Bullets represent bits which
select a particular instruction from a particular instruction class.

•011|010•|....|....|....|....|....|.... cmpb  compare and branch
0101|010.|....|....|....|....|...•|.... cb    conditional branch
1101|0100|•••.|....|....|....|....|..•• ex    exception generation
1101|0101|00..|....|.•••|....|•••.|.... sys   system
.011|011•|....|....|....|....|....|.... tb    test and branch
•001|01..|....|....|....|....|....|.... ubi   unconditional branch imm
1101|011•|••••|••••|....|....|....|.... ubr   unconditional branch reg
•••1|0001|....|....|....|....|....|.... ai    add/sub imm
•••1|0011|0•..|....|....|....|....|.... ab    bitfield
•••1|0011|1••.|....|....|....|....|.... ax    extract
•••1|0010|0...|....|....|....|....|.... ali   logic imm
•••1|0010|1...|....|....|....|....|.... amwi  move wide imm
•..1|0000|....|....|....|....|....|.... apcr  PC-rel addr
•••0|1011|••1.|....|....|....|....|.... ar    add/sub reg
•••0|1011|..0.|....|....|....|....|.... asr   add/sub shift-reg
•••1|1010|000.|....|....|....|....|.... ac    add/sub carry
•••1|1010|010.|....|....|1...|....|.... aci   cond compare imm
•••1|1010|010.|....|....|0...|....|.... acr   cond compare reg
•••1|1010|100.|....|....|••..|....|.... acs   cond select
•1•1|1010|110.|....|...•|••..|....|.... a1    data proc 1 src
•0.1|1010|110.|....|.•••|••..|....|.... a2    data proc 2 src
•..1|1011|•••.|....|•...|....|....|.... a3    data proc 3 src
•••0|1010|..•.|....|....|....|....|.... alsr  logic shift-reg
••01|1•00|....|....|....|....|....|.... lsr   load/store reg
••00|1000|•••.|....|•...|....|....|.... lsx   load/store ex
••10|1•00|0•..|....|....|....|....|.... lsnp  load/store no-alloc pair (off)
••11|1•00|••0.|....|....|01..|....|.... lspos load/store reg (imm post-index)
••11|1•00|••0.|....|....|11..|....|.... lspre load/store reg (imm pre-index)
••11|1•00|••1.|....|....|10..|....|.... lso   load/store reg (off)
••11|1•00|••0.|....|....|10..|....|.... lsu   load/store reg (unpriv)
••11|1•00|••1.|....|....|00..|....|.... lsuci load/store reg (unscaled imm)
••11|1•01|••..|....|....|....|....|.... lsusi load/store reg (unsigned imm)
••10|1•01|0•..|....|....|....|....|.... lsrpo load/store reg-pair (off)
••10|1•00|1•..|....|....|....|....|.... lsppo load/store reg-pair (post-index)
••10|1•01|1•..|....|....|....|....|.... lsppr load/store reg-pair (pre-index)

*/
ulong
getxo(ulong ir)
{
	// high-level dispatch.
	ulong b2826, b27, b25, b2725;
	// data processing (imm).
	ulong b2824, b2823;
	// branches, exceptions, syscalls.
	ulong b3026, b3025, b3125, b3124, b3122;
	// loads and stores.
	ulong b2927, b2924, b2524, b2523, b21, b1110;
	// data processing (reg).
	ulong b30, b2321, b11;	// +b2824, +b21

	b2826 = (ir>>26)&7;
	b27 = (ir>>27)&1;
	b25 = (ir>>25)&1;
	b2725 = (ir>>25)&7;
	b2824 = (ir>>24)&0x1F;
	b2823 = (ir>>23)&0x3F;
	b3026 = (ir>>26)&0x1F;
	b3025 = (ir>>25)&0x3F;
	b3125 = (ir>>25)&0x7F;
	b3124 = (ir>>24)&0xFF;
	b3122 = (ir>>22)&0x3FF;
	b2927 = (ir>>27)&7;
	b2924 = (ir>>24)&0x3F;
	b2524 = (ir>>24)&3;
	b2523 = (ir>>23)&7;
	b21 = (ir>>21)&1;
	b1110 = (ir>>10)&3;
	b30 = (ir>>30)&1;
	b2321 = (ir>>21)&7;
	b11 = (ir>>11)&1;

	switch(b2826) {
	case 4:	// data processing (imm)
		switch(b2824) {
		case 0x10:	// PC-rel addr
			return Capcr | opapcr(ir);
		case 0x11:	// add/sub imm
			return Cai | opai(ir);
		}
		switch(b2823) {
		case 0x24:	// logic imm
			return Cali | opali(ir);
		case 0x25:	// move wide imm
			return Camwi | opamwi(ir);
		case 0x26:	// bitfield
			return Cab | opab(ir);
		case 0x27:	// extract
			return Cax | opax(ir);
		}
		return Cundef;
	case 5:	// branches, exceptions, syscalls
		if(b3026 == 5)	// unconditional branch imm
			return Cubi | opubi(ir);
		switch(b3025) {
		case 0x1A:	// compare and branch
			return Ccmpb | opcmpb(ir);
		case 0x1B:	// test and branch
			return Ctb | optb(ir);
		}
		switch(b3125) {
		case 0x2A:	// conditional branch
			return Ccb | opcb(ir);
		case 0x6B:	// unconditional branch reg
			return Cubr | opubr(ir);
		}
		if(b3124 == 0xD4)	// exception generation
			return Cex | opex(ir);
		if(b3122 == 0x354)	// system
			return Csys | opsys(ir);
		return Cundef;
	}
	if(b27 == 1 && b25 == 0) {	// loads and stores
		if(b2924 == 8)	// load/store ex
			return Clsx | oplsx(ir);
		if (b2927 == 3 && b2524 == 0)	// load/store reg
			return Clsr | oplsr(ir);
		switch(b2927) {
		case 5:	// lsnp, lsrpo, lsppo, lsppr
			switch(b2523) {
			case 0:	// load/store no-alloc pair (off)
				return Clsnp | oplsnp(ir);
			case 1:	// load/store reg-pair (post-index)
				return Clsppo | oplsppo(ir);
			case 2:	// load/store reg-pair (off)
				return Clsrpo | oplsrpo(ir);
			case 3:	// load/store reg-pair (pre-index)
				return Clsppr | oplsppr(ir);
			}
			return Cundef;
		case 7:	// lspos, lspre, lso, lsu, lsuci, lsusi
			if(b2524 == 1)	// load/store reg (unsigned imm)
				return Clsusi | oplsusi(ir);
			else if(b2524 == 0) {
				if(b21 == 1) {
					if(b1110 == 2)	// load/store reg (off)
						return Clso | oplso(ir);
					return Cundef;
				}
				switch(b1110) {
				case 0:	// load/store reg (unscaled imm)
					return Clsuci | oplsuci(ir);
				case 1:	// load/store reg (imm post-index)
					return Clspos | oplspos(ir);
				case 2:	// load/store reg (unpriv)
					return Clsu | oplsu(ir);
				case 3:	// load/store reg (imm pre-index)
					return Clspre | oplspre(ir);
				}
			}
			return Cundef;
		}
		return Cundef;
	}
	if(b2725 == 5) {	// data processing (reg)
		switch(b2824) {
		case 0xA:	// logic shift-reg
			return Calsr | opalsr(ir);
		case 0xB:	// ar, asr
			if(b21 == 1)	// add/sub extended reg
				return Car | opar(ir);
			return Casr | opasr(ir);	// add/sub shift-reg
		case 0x1A:	// ac, aci, acr, acs, a1, a2
			switch(b2321) {
			case 0:	// add/sub carry
				return Cac | opac(ir);
			case 2:	// aci, acr
				if(b11 == 1)	// cond compare imm
					return Caci | opaci(ir);
				return Cacr | opacr(ir);	// cond compare reg
			case 4:	// cond select
				return Cacs | opacs(ir);
			case 6:	// a1, a2
				if(b30 == 1) // data proc 1 src
					return Ca1 | opa1(ir);
				return Ca2 | opa2(ir);	// data proc 2 src
			}
			return Cundef;
		case 0x1B:	// a3    data proc 3 src
			return Ca3 | opa3(ir);
		}
		return Cundef;
	}
	return Cundef;
}

void
ilock(int)
{
}

void
undef(ulong ir)
{
	ulong xo;
	xo = getxo(ir);
	if(xo != Cundef)
		Bprint(bioout, "undefined instruction #%.8lux (xo=%lud, name=%s, pc=#%.8lux)\n", ir, xo, itab[xo].name, reg.pc);
	else
		Bprint(bioout, "undefined instruction #%.8lux (xo=%lud (Cundef), pc=#%.8lux)\n", ir, xo, reg.pc);
	longjmp(errjmp, 0);
}

/* compare and branch
params: imm19<23,5> Rt<4,0> 
ops: sf<31> op<24> 
	CBZ   	sf=0	op=0	
	CBZ   	sf=1	op=0	
	CBNZ  	sf=0	op=1	
	CBNZ  	sf=1	op=1	
*/
void
cmpb(ulong ir)
{
	ulong sf, op, imm19, Rt;
	uvlong addr, Xt;
	ulong Wt;

	getcmpb(ir);
	Xt = (Rt != 31)? reg.r[Rt] : 0;
	Wt = (ulong)Xt;
	addr = reg.pc + sext(imm19, 19)<<2;
	switch(sf<<1|op) {
	case 0:	/* 32-bit CBZ */
		if(Wt == 0)
			reg.pc = addr - 4;
		break;
	case 2:	/* 64-bit CBZ */
		if(Xt == 0)
			reg.pc = addr - 4;
		break;
	case 1:	/* 32-bit CBNZ */
		if(Wt != 0)
			reg.pc = addr - 4;
		break;
	case 3:	/* 64-bit CBNZ */
		if(Xt != 0)
			reg.pc = addr - 4;
		break;
	}
	if(trace)
		itrace("%s\timm19=%d, Rt=%d", ci->name, imm19, Rt);
}

/* conditional branch
params: o1<24> imm19<23,5> cond<3,0> 
ops: o0<4> 
	B.cond	o0=0	
*/
void
condb(ulong ir)
{
	ulong o1, imm19, o0, cond;
	vlong offset;

	getcb(ir);
	offset = sext(imm19, 19) << 2;
	if(o0 != 0 || o1 != 0)
		undef(ir);
	if(trace)
		itrace("%s\to1=%d, imm19=%d, cond=%d", ci->name, o1, imm19, cond);
	if(runcond(cond))
		reg.pc += offset - 4;
}

/* system
params: CRm<11,8> Rt<4,0> 
ops: CRn<14,12> op2<7,5> 
	CLREX 	CRn=3	op2=2	
*/
void
sys(ulong ir)
{
	ulong CRn, CRm, op2, Rt;

	getsys(ir);
	if(CRn != 3 || op2 != 2)
		undef(ir);
	if(trace)
		itrace("%s\tCRm=%d, Rt=%d", ci->name, CRm, Rt);
}

/* test and branch
params: b5<31> b40<23,19> imm14<18,5> Rt<4,0> 
ops: op<24> 
	TBZ   	op=0	
	TBNZ  	op=1	
*/
void
tb(ulong ir)
{
	ulong b5, op, b40, imm14, Rt;

	gettb(ir);
	USED(op);
	undef(ir);
	if(trace)
		itrace("%s\tb5=%d, b40=%d, imm14=%d, Rt=%d", ci->name, b5, b40, imm14, Rt);
}

/* unconditional branch imm
params: imm26<25,0> 
ops: op<31> 
	B     	op=0	
	BL    	op=1	
*/
void
uncondbimm(ulong ir)
{
	ulong op, imm26;
	uvlong npc;

	getubi(ir);
	npc = reg.pc + (sext(imm26, 26) << 2);
	if(op) {	/* BL */
		call(npc);
		reg.r[30] = reg.pc + 4;
	}
	if(trace)
		itrace("%s\timm26=%d", ci->name, imm26);
	reg.pc = npc - 4;
}

/* unconditional branch reg
params: Rn<9,5> 
ops: opc<24,21> op2<20,16> 
	BR    	opc=0	op2=31	
	BLR   	opc=1	op2=31	
	RET   	opc=2	op2=31	
*/
void
uncondbreg(ulong ir)
{
	ulong opc, op2, Rn;

	getubr(ir);
	USED(op2);
	if(trace)
		itrace("%s\tRn=%d", ci->name, Rn);
	switch(opc) {
	case 0:	/* BR */
		reg.pc = reg.r[Rn] - 4;
		break;
	case 1:	/* BLR */
		call(reg.r[Rn]);
		reg.r[30] = reg.pc + 4;
		reg.pc = reg.r[Rn] - 4;
		break;
	case 2:	/* RET */
		ret(reg.r[Rn]);
		reg.pc = reg.r[Rn] - 4;
		break;
	}
}

/* add/sub imm
params: shift<23,22> imm12<21,10> Rn<9,5> Rd<4,0> 
ops: sf<31> op<30> S<29> 
	ADD   	sf=0	op=0	S=0	
	ADDS  	sf=0	op=0	S=1	
	SUB   	sf=0	op=1	S=0	
	SUBS  	sf=0	op=1	S=1	
	ADD   	sf=1	op=0	S=0	
	ADDS  	sf=1	op=0	S=1	
	SUB   	sf=1	op=1	S=0	
	SUBS  	sf=1	op=1	S=1	
*/
void
addsubimm(ulong ir)
{
	ulong sf, op, S, shift, imm12, Rn, Rd;
	uvlong Xn, m, r;
	ulong Wn, m32;
	Pstate p;

	getai(ir);
	if(shift)
		m = imm12<<12;
	else
		m = imm12;
	m32 = (ulong)m;
	Xn = reg.r[Rn];
	Wn = (ulong)Xn;
	SET(r);	/* silence the compiler */
	switch(sf) {
	case 0:	/* 32-bit */
		switch(op) {
		case 1: /* SUB, SUBS */
			r = (uvlong)add32(Wn, ~m32, 1ull, &p);
			break;
		case 0:	/* ADD, ADDS */
			r = (uvlong)add32(Wn, m32, 0ull, &p);
			break;
		}
		break;
	case 1:	/* 64-bit */
		switch(op) {
		case 1: /* SUB, SUBS */
			r = add64(Xn, ~m, 1ull, &p);
			break;
		case 0:	/* ADD, ADDS */
			r = add64(Xn, m, 0ull, &p);
			break;
		}
		break;
	}
	if(S) {	/* flags */
		if(Rd != 31)
			reg.r[Rd] = r;
		reg.Pstate = p;
	} else {
		reg.r[Rd] = r;
	}

	if(trace)
		itrace("%s\tshift=%d, imm12=%d, Rn=%d, Rd=%d", ci->name, shift, imm12, Rn, Rd);
}

/* bitfield
params: immr<21,16> imms<15,10> Rn<9,5> Rd<4,0> 
ops: sf<31> opc<30,29> N<22> 
	SBFM  	sf=0	opc=0	N=0	
	BFM   	sf=0	opc=1	N=0	
	UBFM  	sf=0	opc=2	N=0	
	SBFM  	sf=1	opc=0	N=1	
	BFM   	sf=1	opc=1	N=1	
	UBFM  	sf=1	opc=2	N=1	
*/
void
bitfield(ulong ir)
{
	ulong sf, opc, N, immr, imms, Rn, Rd;
	uvlong src;
	ulong bits;

	getab(ir);
	USED(N);
	if(imms >= immr) {	/* shift right */
		src = ((uvlong)reg.r[Rn]>>immr)&((1ull<<(imms-immr+1))-1ull);
		switch(opc) {
		case 0: /* SBFM */
			reg.r[Rd] = sext(src, imms - immr + 1);
			break;
		case 1:	/* BFM */
			reg.r[Rd] = (reg.r[Rd]&~((1ull<<(imms-immr+1))-1ull)) | src;
			break;
		case 2:	/* UBFM */
			reg.r[Rd] = src;
			break;
		}
	} else {	/* shift left */
		src = reg.r[Rn]&((1ull<<(imms+1))-1ull);
		bits = sf ? 64 : 32;
		switch(opc) {
		case 0: /* SBFM */
			reg.r[Rd] = sext(src, imms + 1) << (bits-immr);
			break;
		case 1:	/* BFM */
			reg.r[Rd] = (reg.r[Rd]&~((1ull<<(bits-immr+imms+1))-1ull)) | (src << (bits-immr));
			break;
		case 2:	/* UBFM */
			reg.r[Rd] = src << (bits-immr);
			break;
		}
	}
	if(sf == 0)	/* 32-bit */
		reg.r[Rn] = (ulong)reg.r[Rn];
	if(trace)
		itrace("%s\timmr=%d, imms=%d, Rn=%d, Rd=%d", ci->name, immr, imms, Rn, Rd);
}

/* extract
params: Rm<20,16> imms<15,10> Rn<9,5> Rd<4,0> 
ops: sf<31> op21<30,29> N<22> o0<21> 
	EXTR  	sf=0	op21=0	N=0	o0=0	
	EXTR  	sf=1	op21=0	N=1	o0=0	
*/
void
extract(ulong ir)
{
	ulong sf, op21, N, o0, Rm, imms, Rn, Rd;

	getax(ir);
	USED(sf, op21, N, o0);
	undef(ir);
	if(trace)
		itrace("%s\tRm=%d, imms=%d, Rn=%d, Rd=%d", ci->name, Rm, imms, Rn, Rd);
}

/* logic imm
params: N<22> immr<21,16> imms<15,10> Rn<9,5> Rd<4,0> 
ops: sf<31> opc<30,29> 
	AND   	sf=0	opc=0	
	ORR   	sf=0	opc=1	
	EOR   	sf=0	opc=2	
	ANDS  	sf=0	opc=3	
	AND   	sf=1	opc=0	
	ORR   	sf=1	opc=1	
	EOR   	sf=1	opc=2	
	ANDS  	sf=1	opc=3	
*/
void
logimm(ulong ir)
{
	ulong sf, opc, N, immr, imms, Rn, Rd;
	uvlong Xn, r, imm;
	ulong Wn;

	getali(ir);
	if(Rn == 31)
		Xn = 0;
	else
		Xn = reg.r[Rn];
	Wn = (ulong)Xn;
	imm = decodebitmask(N, imms, immr);
	SET(r);
	switch(sf) {
	case 0:	/* 32-bit */
		switch(opc) {
		case 0:	/* AND */
			r = Wn & imm;
			if(trace)
				print("logimm: AND: imm=%llux %lld %llud\n", imm, imm, imm);
			break;
		case 1:	/* ORR */
			r = Wn | imm;
			break;
		case 2:	/* EOR */
			r = Wn ^ imm;
			break;
		case 3:	/* ANDS */
			r = Wn & imm;
			break;
		}
		break;
	case 1:	/* 64-bit */
		switch(opc) {
		case 0:	/* AND */
			r = Xn & imm;
			break;
		case 1:	/* ORR */
			r = Xn | imm;
			break;
		case 2:	/* EOR */
			r = Xn ^ imm;
			break;
		case 3:	/* ANDS */
			r = Xn & imm;
			break;
		}
		break;
	}
	if(opc == 3) {	/* flags */
		if(Rd != 31)
			reg.r[Rd] = r;
		if(sf) {
			reg.N = r>>63 & 1;
			reg.Z = (r == 0);
		} else {
			reg.N = r>>31 & 1;
			reg.Z = ((ulong)r == 0);
		}
	} else {
		reg.r[Rd] = r;
	}
	if(trace)
		itrace("%s\tN=%d, immr=%d, imms=%d, Rn=%d, Rd=%d", ci->name, N, immr, imms, Rn, Rd);
}

/* move wide imm
params: hw<22,21> imm16<20,5> Rd<4,0> 
ops: sf<31> opc<30,29> 
	MOVN  	sf=0	opc=0	
	MOVZ  	sf=0	opc=2	
	MOVK  	sf=0	opc=3	
	MOVN  	sf=1	opc=0	
	MOVZ  	sf=1	opc=2	
	MOVK  	sf=1	opc=3	
*/
void
movwimm(ulong ir)
{
	ulong sf, opc, hw, imm16, Rd;
	uvlong r;

	getamwi(ir);
	SET(r);	/* silence the compiler */
	switch(opc) {
	case 0:	/* MOVN */
		if(sf)	/* 64-bit */
			r = ~((uvlong)imm16 << ((uvlong)hw<<16));
		else
			r = (ulong)~((uvlong)imm16 << ((uvlong)hw<<16));
		break;
	case 2:	/* MOVZ */
		if(sf)	/* 64-bit */
			r = (uvlong)imm16 << ((uvlong)hw<<16);
		else
			r = (ulong)((uvlong)imm16 << ((uvlong)hw<<16));
		break;
	default:
		undef(ir);
	}
	if(Rd != 31)
		reg.r[Rd] = r;
	if(trace)
		itrace("%s\thw=%d, imm16=%d, Rd=%d", ci->name, hw, imm16, Rd);
}

/* PC-rel addr
params: immlo<30,29> immhi<23,5> Rd<4,0> 
ops: op<31> 
	ADR   	op=0	
	ADRP  	op=1	
*/
void
pcrel(ulong ir)
{
	ulong op, immlo, immhi, Rd;
	vlong imm;

	getapcr(ir);
	if(op)
		imm = sext(immhi<<14 | immlo<<12, 33);
	else
		imm = sext(immhi<<2 | immlo, 20);
	if(Rd != 31)
		reg.r[Rd] = reg.pc + imm;
	if(trace)
		itrace("%s\timmlo=%d, immhi=%d, Rd=%d", ci->name, immlo, immhi, Rd);
}

/* add/sub extended reg
params: Rm<20,16> option<15,13> imm3<12,10> Rn<9,5> Rd<4,0> 
ops: sf<31> op<30> S<29> opt<23,22> 
	ADD   	sf=0	op=0	S=0	opt=0	
	ADDS  	sf=0	op=0	S=1	opt=0	
	SUB   	sf=0	op=1	S=0	opt=0	
	SUBS  	sf=0	op=1	S=1	opt=0	
	ADD   	sf=1	op=0	S=0	opt=0	
	ADDS  	sf=1	op=0	S=1	opt=0	
	SUB   	sf=1	op=1	S=0	opt=0	
	SUBS  	sf=1	op=1	S=1	opt=0	
*/
void
addsubreg(ulong ir)
{
	ulong sf, op, S, opt, Rm, option, imm3, Rn, Rd;

	getar(ir);
	USED(sf, op, S, opt);
	undef(ir);
	if(trace)
		itrace("%s\tRm=%d, option=%d, imm3=%d, Rn=%d, Rd=%d", ci->name, Rm, option, imm3, Rn, Rd);
}

/* add/sub shift-reg
params: shift<23,22> Rm<20,16> imm6<15,10> Rn<9,5> Rd<4,0> 
ops: sf<31> op<30> S<29> 
	ADD   	sf=0	op=0	S=0	
	ADDS  	sf=0	op=0	S=1	
	SUB   	sf=0	op=1	S=0	
	SUBS  	sf=0	op=1	S=1	
	ADD   	sf=1	op=0	S=0	
	ADDS  	sf=1	op=0	S=1	
	SUB   	sf=1	op=1	S=0	
	SUBS  	sf=1	op=1	S=1	
*/
void
addsubsreg(ulong ir)
{
	ulong sf, op, S, shift, Rm, imm6, Rn, Rd;
	uvlong Xn, Xm, m, r;
	ulong Wn, m32;
	Pstate p;

	getasr(ir);
	if(Rm == 31)
		Xm = 0;
	else
		Xm = reg.r[Rm];
	m = doshift(sf, Xm, shift, imm6);
	m32 = (ulong)m;
	if(Rn == 31)
		Xn = 0;
	else
		Xn = reg.r[Rn];
	Wn = (ulong)Xn;
	SET(r);	/* silence the compiler */
	switch(sf) {
	case 0:	/* 32-bit */
		switch(op) {
		case 1: /* SUB, SUBS */
			r = (uvlong)add32(Wn, ~m32, 1ull, &p);
			break;
		case 0:	/* ADD, ADDS */
			r = (uvlong)add32(Wn, m32, 0ull, &p);
			break;
		}
		break;
	case 1:	/* 64-bit */
		switch(op) {
		case 1: /* SUB, SUBS */
			r = add64(Xn, ~m, 1ull, &p);
			break;
		case 0:	/* ADD, ADDS */
			r = add64(Xn, m, 0ull, &p);
			break;
		}
		break;
	}
	if(Rd != 31)
		reg.r[Rd] = r;
	if(S) {	/* flags */
		reg.Pstate = p;
	}
	if(trace)
		itrace("%s\tshift=%d, Rm=%d, imm6=%d, Rn=%d, Rd=%d", ci->name, shift, Rm, imm6, Rn, Rd);
}

/* add/sub carry
params: Rm<20,16> Rn<9,5> Rd<4,0> 
ops: sf<31> op<30> S<29> 
	ADC   	sf=0	op=0	S=0	
	ADCS  	sf=0	op=0	S=1	
	SBC   	sf=0	op=1	S=0	
	SBCS  	sf=0	op=1	S=1	
	ADC   	sf=1	op=0	S=0	
	ADCS  	sf=1	op=0	S=1	
	SBC   	sf=1	op=1	S=0	
	SBCS  	sf=1	op=1	S=1	
*/
void
addsubc(ulong ir)
{
	ulong sf, op, S, Rm, Rn, Rd;

	getac(ir);
	USED(sf, op, S);
	undef(ir);
	if(trace)
		itrace("%s\tRm=%d, Rn=%d, Rd=%d", ci->name, Rm, Rn, Rd);
}

/* cond compare imm
params: imm5<20,16> cond<15,12> Rn<9,5> nzcv<3,0> 
ops: sf<31> op<30> S<29> 
	CCMN  	sf=0	op=0	S=1	
	CCMP  	sf=0	op=1	S=1	
	CCMN  	sf=1	op=0	S=1	
	CCMP  	sf=1	op=1	S=1	
*/
void
condcmpimm(ulong ir)
{
	ulong sf, op, S, imm5, cond, Rn, nzcv;

	getaci(ir);
	USED(sf, op, S);
	undef(ir);
	if(trace)
		itrace("%s\timm5=%d, cond=%d, Rn=%d, nzcv=%d", ci->name, imm5, cond, Rn, nzcv);
}

/* cond compare reg
params: Rm<20,16> cond<15,12> Rn<9,5> nzcv<3,0> 
ops: sf<31> op<30> S<29> 
	CCMN  	sf=0	op=0	S=1	
	CCMP  	sf=0	op=1	S=1	
	CCMN  	sf=1	op=0	S=1	
	CCMP  	sf=1	op=1	S=1	
*/
void
condcmpreg(ulong ir)
{
	ulong sf, op, S, Rm, cond, Rn, nzcv;

	getacr(ir);
	USED(sf, op, S);
	undef(ir);
	if(trace)
		itrace("%s\tRm=%d, cond=%d, Rn=%d, nzcv=%d", ci->name, Rm, cond, Rn, nzcv);
}

/* cond select
params: Rm<20,16> cond<15,12> Rn<9,5> Rd<4,0> 
ops: sf<31> op<30> S<29> op2<11,10> 
	CSEL  	sf=0	op=0	S=0	op2=0	
	CSINC 	sf=0	op=0	S=0	op2=1	
	CSINV 	sf=0	op=1	S=0	op2=0	
	CSNEG 	sf=0	op=1	S=0	op2=1	
	CSEL  	sf=1	op=0	S=0	op2=0	
	CSINC 	sf=1	op=0	S=0	op2=1	
	CSINV 	sf=1	op=1	S=0	op2=0	
	CSNEG 	sf=1	op=1	S=0	op2=1	
*/
void
condsel(ulong ir)
{
	ulong sf, op, S, Rm, cond, op2, Rn, Rd;

	getacs(ir);
	USED(sf, op, S, op2);
	undef(ir);
	if(trace)
		itrace("%s\tRm=%d, cond=%d, Rn=%d, Rd=%d", ci->name, Rm, cond, Rn, Rd);
}

/* data proc 1 src
params: Rn<9,5> Rd<4,0> 
ops: sf<31> S<29> opcode<12,10> 
	RBIT  	sf=0	S=0	opcode=0	
	REV16 	sf=0	S=0	opcode=1	
	REV   	sf=0	S=0	opcode=2	
	CLZ   	sf=0	S=0	opcode=4	
	CLZ   	sf=0	S=0	opcode=5	
	RBIT  	sf=1	S=0	opcode=0	
	REV16 	sf=1	S=0	opcode=1	
	REV32 	sf=1	S=0	opcode=2	
	REV   	sf=1	S=0	opcode=3	
	CLZ   	sf=1	S=0	opcode=4	
	CLZ   	sf=1	S=0	opcode=5	
*/
void
dp1(ulong ir)
{
	ulong sf, S, opcode, Rn, Rd;

	geta1(ir);
	USED(sf, S, opcode);
	undef(ir);
	if(trace)
		itrace("%s\tRn=%d, Rd=%d", ci->name, Rn, Rd);
}

/* data proc 2 src
params: Rm<20,16> Rn<9,5> Rd<4,0> 
ops: sf<31> opcode<14,10> 
	UDIV  	sf=0	opcode=2	
	SDIV  	sf=0	opcode=3	
	LSLV  	sf=0	opcode=8	
	LSRV  	sf=0	opcode=9	
	ASRV  	sf=0	opcode=10	
	RORV  	sf=0	opcode=11	
	UDIV  	sf=1	opcode=2	
	SDIV  	sf=1	opcode=3	
	LSLV  	sf=1	opcode=8	
	LSRV  	sf=1	opcode=9	
	ASRV  	sf=1	opcode=10	
	RORV  	sf=1	opcode=11	
*/
void
dp2(ulong ir)
{
	ulong sf, Rm, opcode, Rn, Rd;
	uvlong Xm, Xn, r; 
	ulong Wm, Wn;

	geta2(ir);
	Xm = (Rm != 31)? reg.r[Rm] : 0;
	Wm = (ulong)Xm;
	Xn = (Rn != 31)? reg.r[Rn] : 0;
	Wn = (ulong)Xn;
	SET(r);
	switch(sf) {
	case 0:	/* 32-bit */
		switch(opcode) {
		case 2:	/* UDIV */
			if(Wm != 0)
				r = Wn / Wm;
			else
				r = 0;
			break;
		case 3:	/* SDIV */
			if(Wm != 0)
				r = (ulong)((long)Wn / (long)Wm);
			else
				r = 0;
			break;
		default:
			undef(ir);
			break;
		}
		break;
	case 1:	/* 64-bit */
		switch(opcode) {
		case 2:	/* UDIV */
			if(Xm != 0)
				r = Xn / Xm;
			else
				r = 0;
			break;
		case 3:	/* SDIV */
			if(Xm != 0)
				r = (uvlong)((vlong)Wn / (vlong)Wm);
			else
				r = 0;
			break;
		default:
			undef(ir);
			break;
		}
		break;
	}
	if(Rd != 31)
		reg.r[Rd] = r;
	if(trace)
		itrace("%s\tRm=%d, Rn=%d, Rd=%d", ci->name, Rm, Rn, Rd);
}

/* data proc 3 src
params: Rm<20,16> Ra<14,10> Rn<9,5> Rd<4,0> 
ops: sf<31> op31<23,21> o0<15> 
	MADD  	sf=0	op31=0	o0=0	
	MSUB  	sf=0	op31=0	o0=1	
	MADD  	sf=1	op31=0	o0=0	
	MSUB  	sf=1	op31=0	o0=1	
*/
void
dp3(ulong ir)
{
	ulong sf, op31, Rm, o0, Ra, Rn, Rd;
	uvlong Xm, Xn, Xa, r; 
	ulong Wm, Wn, Wa;

	geta3(ir);
	Xm = (Rm != 31)? reg.r[Rm] : 0;
	Wm = (ulong)Xm;
	Xn = (Rn != 31)? reg.r[Rn] : 0;
	Wn = (ulong)Xn;
	Xa = (Ra != 31)? reg.r[Ra] : 0;
	Wa = (ulong)Xa;
	USED(op31);
	SET(r);
	switch(sf) {
	case 0:	/* 32-bit */
		switch(o0) {
		case 0:	/* MADD */
			r = (ulong)(Wa + Wn * Wm);
			break;
		case 1:	/* MSUB */
			r = (ulong)(Wa - Wn * Wm);
			break;
		}
		break;
	case 1:	/* 64-bit */
		switch(o0) {
		case 0:	/* MADD */
			r = Xa + Xn * Xm;
			break;
		case 1:	/* MSUB */
			r = Xa - Xn * Xm;
			break;
		}
		break;
	}
	if(Rd != 31)
		reg.r[Rd] = r;
	if(trace)
		itrace("%s\tRm=%d, Ra=%d, Rn=%d, Rd=%d", ci->name, Rm, Ra, Rn, Rd);
}

/* logic shift-reg
params: shift<23,22> Rm<20,16> imm6<15,10> Rn<9,5> Rd<4,0> 
ops: sf<31> opc<30,29> N<21> 
	AND   	sf=0	opc=0	N=0	
	BIC   	sf=0	opc=0	N=1	
	ORR   	sf=0	opc=1	N=0	
	ORN   	sf=0	opc=1	N=1	
	EOR   	sf=0	opc=2	N=0	
	EON   	sf=0	opc=2	N=1	
	ANDS  	sf=0	opc=3	N=0	
	BICS  	sf=0	opc=3	N=1	
	AND   	sf=1	opc=0	N=0	
	BIC   	sf=1	opc=0	N=1	
	ORR   	sf=1	opc=1	N=0	
	ORN   	sf=1	opc=1	N=1	
	EOR   	sf=1	opc=2	N=0	
	EON   	sf=1	opc=2	N=1	
	ANDS  	sf=1	opc=3	N=0	
	BICS  	sf=1	opc=3	N=1	
*/
void
logsreg(ulong ir)
{
	ulong sf, opc, shift, N, Rm, imm6, Rn, Rd;
	uvlong Xn, m, r;
	ulong Wn, m32;

	getalsr(ir);
	if(Rm == 31)
		m = 0;
	else
		m = doshift(sf, reg.r[Rm], shift, imm6);
	m32 = (ulong)m;
	if(Rn == 31)
		Xn = 0;
	else
		Xn = reg.r[Rn];
	Wn = (ulong)Xn;
	SET(r);	/* silence the compiler */
	switch(opc<<1|N) {
	case 0:	/* AND */
		if(sf == 1)
			r = Xn & m;
		else
			r = Wn & m32;
		break;
	case 2:	/* ORR */
		if(sf == 1)
			r = Xn | m;
		else
			r = Wn | m32;
		break;
	case 3:	/* ORN */
		if(sf == 1)
			r = Xn | ~m;
		else
			r = Wn | ~m32;
		break;
	case 4:	/* EOR */
		if(sf == 1)
			r = Xn ^ m;
		else
			r = Wn ^ (uvlong)m32;
		break;
	default:
		undef(ir);
	}
	if(Rd != 31)
		reg.r[Rd] = r;
	if(trace)
		itrace("%s\tshift=%d, Rm=%d, imm6=%d, Rn=%d, Rd=%d", ci->name, shift, Rm, imm6, Rn, Rd);
}

/* load/store reg
params: imm19<23,5> Rt<4,0> 
ops: opc<31,30> V<26> 
	LDR   	opc=0	V=0	
	LDR   	opc=1	V=0	
	LDRSW 	opc=2	V=0	
	PRFM  	opc=3	V=0	
*/
void
ldstreg(ulong ir)
{
	ulong opc, V, imm19, Rt;
	long addr;

	getlsr(ir);
	USED(V);
	addr = reg.pc + (sext(imm19, 19) << 2);
	switch(opc) {
	case 0:	/* 32-bit LDR */
		reg.r[Rt] = getmem_w(addr);
		break;
	case 1:	/* 64-bit LDR */
		reg.r[Rt] = getmem_v(addr);
		break;
	case 2: /* LDRSW */
		reg.r[Rt] = sext(getmem_w(addr), 32);
		break;
	default:
		undef(ir);
	}
	if(trace)
		itrace("%s\timm19=%d, Rt=%d", ci->name, imm19, Rt);
}

/* load/store ex
params: Rs<20,16> Rt2<14,10> Rn<9,5> Rt<4,0> 
ops: size<31,30> o2<23> L<22> o1<21> o0<15> 
	STXRB 	size=0	o2=0	L=0	o1=0	o0=0	
	STLXRB	size=0	o2=0	L=0	o1=0	o0=1	
	LDXRB 	size=0	o2=0	L=1	o1=0	o0=0	
	LDAXRB	size=0	o2=0	L=1	o1=0	o0=1	
	STLRB 	size=0	o2=1	L=0	o1=0	o0=1	
	LDARB 	size=0	o2=1	L=1	o1=0	o0=1	
	STXRH 	size=1	o2=0	L=0	o1=0	o0=0	
	STLXRH	size=1	o2=0	L=0	o1=0	o0=1	
	LDXRH 	size=1	o2=0	L=1	o1=0	o0=0	
	LDAXRH	size=1	o2=0	L=1	o1=0	o0=1	
	STLRH 	size=1	o2=1	L=0	o1=0	o0=1	
	LDARH 	size=1	o2=1	L=1	o1=0	o0=1	
	STXR  	size=2	o2=0	L=0	o1=0	o0=0	
	STLXR 	size=2	o2=0	L=0	o1=0	o0=1	
	STXP  	size=2	o2=0	L=0	o1=1	o0=0	
	STLXP 	size=2	o2=0	L=0	o1=1	o0=1	
	LDXR  	size=2	o2=0	L=1	o1=0	o0=0	
	LDAXR 	size=2	o2=0	L=1	o1=0	o0=1	
	LDXP  	size=2	o2=0	L=1	o1=1	o0=0	
	LDAXP 	size=2	o2=0	L=1	o1=1	o0=1	
	STLR  	size=2	o2=1	L=0	o1=0	o0=1	
	STLR  	size=2	o2=1	L=1	o1=0	o0=1	
	STXR  	size=3	o2=0	L=0	o1=0	o0=0	
	STLXR 	size=3	o2=0	L=0	o1=0	o0=1	
	STXP  	size=3	o2=0	L=0	o1=1	o0=0	
	STLXP 	size=3	o2=0	L=0	o1=1	o0=1	
	LDXR  	size=3	o2=0	L=1	o1=0	o0=0	
	LDAXR 	size=3	o2=0	L=1	o1=0	o0=1	
	LDXP  	size=3	o2=0	L=1	o1=1	o0=0	
	LDAXP 	size=3	o2=0	L=1	o1=1	o0=1	
	STLR  	size=3	o2=1	L=0	o1=0	o0=1	
	LDAR  	size=3	o2=1	L=1	o1=0	o0=1	
*/
void
ldstex(ulong ir)
{
	ulong size, o2, L, o1, Rs, o0, Rt2, Rn, Rt;
	uvlong addr, r, Xt;

	getlsx(ir);
	addr = reg.r[Rn];
	SET(r);
	USED(Rt2, o2);
	Xt = (Rt != 31)? reg.r[Rt] : 0;
	switch(L) {
	case 0:	/* stores */
		switch(size) {
		case 3:	/* 64-bit */
			switch(o1) {
			case 0:	/* register */
				switch(o0) {
				case 0: /* STXR */
					putmem_v(addr, Xt);
					reg.r[Rs] = 0;
					break;
				case 1:	/* store-release */
					undef(ir);
					break;
				}
				break;
			case 1:	/* register-pair */
				undef(ir);
				break;
			}
			break;
		default:
			undef(ir);
			break;
		}
		break;
	case 1:	/* loads */
		switch(size) {
		case 3:	/* 64-bit */
			switch(o1) {
			case 0:	/* register */
				switch(o0) {
				case 0: /* LDXR */
					r = getmem_v(addr);
					break;
				case 1:	/* load-acquire */
					undef(ir);
					break;
				}
				break;
			case 1:	/* register-pair */
				undef(ir);
				break;
			}
			break;
		default:
			undef(ir);
			break;
		}
		if(Rt != 31)
			reg.r[Rt] = r;
		break;
	}
	if(trace)
		itrace("%s\tRs=%d, Rt2=%d, Rn=%d, Rt=%d", ci->name, Rs, Rt2, Rn, Rt);
}

/* load/store no-alloc pair (off)
params: imm7<21,15> Rt2<14,10> Rn<9,5> Rt<4,0> 
ops: opc<31,30> V<26> L<22> 
	STNP  	opc=0	V=0	L=0	
	LDNP  	opc=0	V=0	L=1	
	STNP  	opc=2	V=0	L=0	
	LDNP  	opc=2	V=0	L=1	
*/
void
ldstnoallocp(ulong ir)
{
	ulong opc, V, L, imm7, Rt2, Rn, Rt;

	getlsnp(ir);
	USED(opc, V, L);
	undef(ir);
	if(trace)
		itrace("%s\timm7=%d, Rt2=%d, Rn=%d, Rt=%d", ci->name, imm7, Rt2, Rn, Rt);
}

/* load/store reg (imm post-index)
params: imm9<20,12> Rn<9,5> Rt<4,0> 
ops: size<31,30> V<26> opc<23,22> 
	STRB  	size=0	V=0	opc=0	
	LDRB  	size=0	V=0	opc=1	
	LDRSB 	size=0	V=0	opc=2	
	LDRSB 	size=0	V=0	opc=3	
	STRH  	size=1	V=0	opc=0	
	LDRH  	size=1	V=0	opc=1	
	LDRSH 	size=1	V=0	opc=2	
	LDRSH 	size=1	V=0	opc=3	
	STR   	size=2	V=0	opc=0	
	LDR   	size=2	V=0	opc=1	
	LDRSW 	size=2	V=0	opc=2	
	STR   	size=3	V=0	opc=0	
	LDR   	size=3	V=0	opc=1	
*/
void
ldstregimmpost(ulong ir)
{
	ulong size, V, opc, imm9, Rn, Rt;
	uvlong addr, Xt;

	getlspos(ir);
	USED(V);
	/* post-index calculation */
	addr = reg.r[Rn];
	Xt = (Rt != 31)? reg.r[Rt] : 0;
	switch(opc) {
	case 0:	/* stores */
		switch(size) {
		case 3:	/* 64-bit STR */
			putmem_v(addr, Xt);
			break;
		default:
			undef(ir);	
		}
		break;
	case 1: /* immediate loads */
		switch(size) {
		case 3:	/* 64-bit LDR */
			reg.r[Rt] = getmem_v(addr);
			break;
		default:
			undef(ir);	
		}
		break;
	default:
		undef(ir);
	}
	/* write-back */
	reg.r[Rn] += sext(imm9, 9);
	if(trace)
		itrace("%s\timm9=%d, Rn=%d, Rt=%d", ci->name, imm9, Rn, Rt);
}

/* load/store reg (imm pre-index)
params: imm9<20,12> Rn<9,5> Rt<4,0> 
ops: size<31,30> V<26> opc<23,22> 
	STRB  	size=0	V=0	opc=0	
	LDRB  	size=0	V=0	opc=1	
	LDRSB 	size=0	V=0	opc=2	
	LDRSB 	size=0	V=0	opc=3	
	STRH  	size=1	V=0	opc=0	
	LDRH  	size=1	V=0	opc=1	
	LDRSH 	size=1	V=0	opc=2	
	LDRSH 	size=1	V=0	opc=3	
	STR   	size=2	V=0	opc=0	
	LDR   	size=2	V=0	opc=1	
	LDRSW 	size=2	V=0	opc=2	
	STR   	size=3	V=0	opc=0	
	LDR   	size=3	V=0	opc=1	
*/
void
ldstregimmpre(ulong ir)
{
	ulong size, V, opc, imm9, Rn, Rt;
	uvlong addr, Xt;

	getlspre(ir);
	USED(V);
	/* pre-index calculation */
	addr = reg.r[Rn] + sext(imm9, 9);
	Xt = (Rt != 31)? reg.r[Rt] : 0;
	switch(opc) {
	case 0:	/* stores */
		switch(size) {
		case 3:	/* 64-bit STR */
			putmem_v(addr, Xt);
			break;
		default:
			undef(ir);	
		}
		break;
	default:
		undef(ir);
	}
	/* write-back */
	reg.r[Rn] = addr;
	if(trace)
		itrace("%s\timm9=%d, Rn=%d, Rt=%d", ci->name, imm9, Rn, Rt);
}

/* load/store reg (off)
params: Rm<20,16> option<15,13> S<12> Rn<9,5> Rt<4,0> 
ops: size<31,30> V<26> opc<23,22> 
	STRB  	size=0	V=0	opc=0	
	LDRB  	size=0	V=0	opc=1	
	LDRSB 	size=0	V=0	opc=2	
	LDRSB 	size=0	V=0	opc=3	
	STRH  	size=1	V=0	opc=0	
	LDRH  	size=1	V=0	opc=1	
	LDRSH 	size=1	V=0	opc=2	
	LDRSH 	size=1	V=0	opc=3	
	STR   	size=2	V=0	opc=0	
	LDR   	size=2	V=0	opc=1	
	LDRSW 	size=2	V=0	opc=2	
	STR   	size=3	V=0	opc=0	
	LDR   	size=3	V=0	opc=1	
	PRFM  	size=3	V=0	opc=2	
*/
void
ldstregoff(ulong ir)
{
	ulong size, V, opc, Rm, option, S, Rn, Rt;
	ulong m;
	uvlong addr;

	getlso(ir);
	USED(V);
	switch(option) {
	default:
		undef(ir);
	case 3:	/* LSL */
		m = (Rm != 31)? reg.r[Rm] : 0;
		addr = reg.r[Rn];
		if(S)
			addr += m << 2;
		else
			addr += m;
		break;
	}
	switch(opc) {
	case 0:	/* stores */
		undef(ir);
		break;
	case 1: /* immediate loads */
		undef(ir);
		break;
	case 2:	/* signed loads */
		switch(size) {
		default:
			undef(ir);
		case 2:	/* LDRSW */
			reg.r[Rt] = sext(getmem_w(addr), 32);
			break;
		}
		break;
	}
	if(trace)
		itrace("%s\tRm=%d, option=%d, S=%d, Rn=%d, Rt=%d", ci->name, Rm, option, S, Rn, Rt);
}

/* load/store reg (unpriv)
params: imm9<20,12> Rn<9,5> Rt<4,0> 
ops: size<31,30> V<26> opc<23,22> 
	STTRB 	size=0	V=0	opc=0	
	LDTRB 	size=0	V=0	opc=1	
	LDTRSB	size=0	V=0	opc=2	
	LDTRSB	size=0	V=0	opc=3	
	STTRH 	size=1	V=0	opc=0	
	LDTRH 	size=1	V=0	opc=1	
	LDTRSH	size=1	V=0	opc=2	
	LDTRSH	size=1	V=0	opc=3	
	STTR  	size=2	V=0	opc=0	
	LDTR  	size=2	V=0	opc=1	
	LDTRSW	size=2	V=0	opc=2	
	STTR  	size=3	V=0	opc=0	
	LDTR  	size=3	V=0	opc=1	
*/
void
ldstregupriv(ulong ir)
{
	ulong size, V, opc, imm9, Rn, Rt;

	getlsu(ir);
	USED(size, V, opc);
	undef(ir);
	if(trace)
		itrace("%s\timm9=%d, Rn=%d, Rt=%d", ci->name, imm9, Rn, Rt);
}

/* load/store reg (unscaled imm)
params: imm9<20,12> Rn<9,5> Rt<4,0> 
ops: size<31,30> V<26> opc<23,22> 
	STURB 	size=0	V=0	opc=0	
	LDURB 	size=0	V=0	opc=1	
	LDURSB	size=0	V=0	opc=2	
	LDURSB	size=0	V=0	opc=3	
	STURH 	size=1	V=0	opc=0	
	LDURH 	size=1	V=0	opc=1	
	LDURSH	size=1	V=0	opc=2	
	LDURSH	size=1	V=0	opc=3	
	STUR  	size=2	V=0	opc=0	
	LDUR  	size=2	V=0	opc=1	
	LDURSW	size=2	V=0	opc=2	
	STUR  	size=3	V=0	opc=0	
	LDUR  	size=3	V=0	opc=1	
	PRFUM 	size=3	V=0	opc=2	
*/
void
ldstreguscaleimm(ulong ir)
{
	ulong size, V, opc, imm9, Rn, Rt;
	uvlong r, addr, Xt;
	ulong Wt;

	getlsuci(ir);
	Xt = (Rt != 31)? reg.r[Rt] : 0;
	Wt = (ulong)Xt;
	USED(V);
	SET(r);
	addr = reg.r[Rn] + sext(imm9, 9);
	switch(size) {
	case 2:	/* 32-bit */
		switch(opc) {
		case 0:	/* STUR */
			putmem_w(addr, Wt);
			break;
		case 1:	/* LDUR */
			r = getmem_w(addr);
			break;
		case 2:	/* LDURSW */
			r = sext(getmem_w(addr), 32);
			break;
		default:
			undef(ir);
			break;
		}
		break;
	case 3:	/* 64-bit */
		switch(opc) {
		case 1:	/* LDUR */
			r = getmem_v(addr);
			break;
		default:
			undef(ir);
			break;
		}
		break;
	default:
		undef(ir);
		break;
	}
	if(Rt != 31)
		reg.r[Rt] = r;
	if(trace)
		itrace("%s\timm9=%d, Rn=%d, Rt=%d", ci->name, imm9, Rn, Rt);
}

/* load/store reg (unsigned imm)
params: imm12<21,10> Rn<9,5> Rt<4,0> 
ops: size<31,30> V<26> opc<23,22> 
	STRB  	size=0	V=0	opc=0	
	LDRB  	size=0	V=0	opc=1	
	LDRSB 	size=0	V=0	opc=2	
	LDRSB 	size=0	V=0	opc=3	
	STRH  	size=1	V=0	opc=0	
	LDRH  	size=1	V=0	opc=1	
	LDRSH 	size=1	V=0	opc=2	
	LDRSH 	size=1	V=0	opc=3	
	STR   	size=2	V=0	opc=0	
	LDR   	size=2	V=0	opc=1	
	LDRSW 	size=2	V=0	opc=2	
	STR   	size=3	V=0	opc=0	
	LDR   	size=3	V=0	opc=1	
	PRFM  	size=3	V=0	opc=2	
*/
void
ldstregusignimm(ulong ir)
{
	ulong size, V, opc, imm12, Rn, Rt;
	uvlong addr, Xt;

	getlsusi(ir);
	USED(V);
	Xt = (Rt != 31)? reg.r[Rt] : 0;
	addr = reg.r[Rn] + (imm12 << size);
	switch(opc) {
	case 0:	/* stores */
		switch(size) {
		case 0:	/* STRB */
			putmem_b(addr, Xt);
			break;
		case 1:	/* STRH */
			putmem_h(addr, Xt);
			break;
		case 2:	/* 32-bit STR */
			putmem_w(addr, Xt);
			break;
		case 3:	/* 64-bit STR */
			putmem_v(addr, Xt);
			break;
		}
		break;
	case 1: /* immediate loads */
		switch(size) {
		case 0:	/* LDRB */
			reg.r[Rt] = getmem_b(addr);
			break;
		case 1:	/* LDRH */
			reg.r[Rt] = getmem_h(addr);
			break;
		case 2:	/* 32-bit LDR */
			reg.r[Rt] = getmem_w(addr);
			break;
		case 3:	/* 64-bit LDR */
			reg.r[Rt] = getmem_v(addr);
			break;
		}
		break;
	case 2:	/* signed loads */
		switch(size) {
		case 0:	/* LDRSB */
			reg.r[Rt] = sext(getmem_b(addr), 8);
			break;
		case 2:	/* LDRSW */
			reg.r[Rt] = sext(getmem_w(addr), 32);
			break;
		default:
			undef(ir);
		}
		break;
	default:
		undef(ir);
	}
	if(trace)
		itrace("%s\timm12=%d, Rn=%d, Rt=%d", ci->name, imm12, Rn, Rt);
}

/* load/store reg-pair (off)
params: imm7<21,15> Rt2<14,10> Rn<9,5> Rt<4,0> 
ops: opc<31,30> V<26> L<22> 
	STP   	opc=0	V=0	L=0	
	LDP   	opc=0	V=0	L=1	
	LDPSW 	opc=1	V=0	L=1	
	STP   	opc=2	V=0	L=0	
	LDP   	opc=2	V=0	L=1	
*/
void
ldstregpoff(ulong ir)
{
	ulong opc, V, L, imm7, Rt2, Rn, Rt;

	getlsrpo(ir);
	USED(opc, V, L);
	undef(ir);
	if(trace)
		itrace("%s\timm7=%d, Rt2=%d, Rn=%d, Rt=%d", ci->name, imm7, Rt2, Rn, Rt);
}

/* load/store reg-pair (post-index)
params: imm7<21,15> Rt2<14,10> Rn<9,5> Rt<4,0> 
ops: opc<31,30> V<26> L<22> 
	STP   	opc=0	V=0	L=0	
	LDP   	opc=0	V=0	L=1	
	LDPSW 	opc=1	V=0	L=1	
	STP   	opc=2	V=0	L=0	
	LDP   	opc=2	V=0	L=1	
*/
void
ldstregppost(ulong ir)
{
	ulong opc, V, L, imm7, Rt2, Rn, Rt;

	getlsppo(ir);
	USED(opc, V, L);
	undef(ir);
	if(trace)
		itrace("%s\timm7=%d, Rt2=%d, Rn=%d, Rt=%d", ci->name, imm7, Rt2, Rn, Rt);
}

/* load/store reg-pair (pre-index)
params: imm7<21,15> Rt2<14,10> Rn<9,5> Rt<4,0> 
ops: opc<31,30> V<26> L<22> 
	STP   	opc=0	V=0	L=0	
	LDP   	opc=0	V=0	L=1	
	LDPSW 	opc=1	V=0	L=1	
	STP   	opc=2	V=0	L=0	
	LDP   	opc=2	V=0	L=1	
*/
void
ldstregppre(ulong ir)
{
	ulong opc, V, L, imm7, Rt2, Rn, Rt;

	getlsppr(ir);
	USED(opc, V, L);
	undef(ir);
	if(trace)
		itrace("%s\timm7=%d, Rt2=%d, Rn=%d, Rt=%d", ci->name, imm7, Rt2, Rn, Rt);
}
