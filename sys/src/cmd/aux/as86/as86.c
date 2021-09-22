/*
 * as86 - Intel 16-bit assembler for Plan 9
 *
 * Reference: Intel `The 8086 Family User's Manual, October 1979'
 * While there are more instructions that the modern Intel chips
 * understand, these are the safe ones.
 *
 * The assembly language is of the Ritchie PDP-11 style.
 *
 * Copyright (c) 2007, Brantley Coile
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Brantley Coile ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

enum {
	NTMP = 500,
};

typedef struct Exec Exec;
typedef struct Itab Itab;
typedef struct Name Name;
typedef struct Operand Operand;
typedef struct R R;
typedef struct Relo Relo;
typedef struct Sym Sym;
typedef struct Tmp Tmp;

struct Name 		/* format in .o */
{
	int	name;
	int	type;
	int	value;
};

struct Sym 		/* internal format */
{
	Sym	*next;
	char	*name;
	int	 type;
	uintptr	 addr;
};

#define N_UNDEF 	0x0
#define N_ABS 		0x2
#define N_TEXT 		0x4
#define N_DATA 		0x6
#define N_BSS 		0x8
#define N_REG		0xa
#define N_KW		0xe
#define N_INST		0x10
#define N_FN 		0x1f
#define N_EXT		01
#define N_TYPE		0x1e

/* ugh.  describes a file format; byte-order problems 'r' us. */
struct Relo {
	int	r_addr;
	unsigned r_symbolnum:24;
	unsigned r_pcrel:1;
	unsigned r_length:2;
	unsigned r_extern:1;
	unsigned :4;
};

struct R {
	R	*next;
	Sym	*what;
	Relo	r;
};

struct Exec {
	int	magic;
	int	text;
	int	data;
	int	bss;
	int	symsz;
	int	entry;
	int	trelo;
	int	drelo;
};

struct Tmp {
	int	loc;
	int	label;
	int	type;
};

struct Operand {
	int 	 mode;
	int 	 type;
	Sym	*sym;
	int	 offset;
	char	 reg;
	char 	 base;
	char	 index;
	char	 scale;
};

struct Itab {
	char	*name;
	char	*fmt;
	int	noperands;
	int	mod;		/* index to mod/rm operand */
	int	omask1, omask2;
};

enum {
	Fxreg	= 1<<0,
	Fareg	= 1<<1,	/* AL, AX, or EAX */
	Fdisp	= 1<<2,	/* displayment */
	Fimm	= 1<<3,
	Fmodrm	= 1<<4,	/* requires mod/rm byte */
	Fsreg	= 1<<9,	/* Seg Register */
	Fabs	= 1<<10,	/* *prefix to operand */
	Freg	= Fxreg|Fareg,
	Fmod 	= Fareg|Freg|Fdisp|Fmodrm,

	BX	= 013,
	BP	= 015,
	SI	= 016,
	DI	= 017,

	Oindex	= 1<<BX|1<<BP|1<<SI|1<<DI,
};

/*
 * note that the hex for formats must be upper case
 * and other format stuff is lower case.
 */

Itab	itab[] = {
/*	  mne      fmt            nops  mod   omask1      omask2 */
	{ "aam",   "D4 04" },
	{ "adcb",  "14 ib",	   2,   -1,   Fimm,      Fareg },
	{ "adcb",  "80 /2 ib",     2,    1,   Fimm,      Fmod },
	{ "adcb",  "10 /r",        2,    1,   Freg,      Fmod },
	{ "adcb",  "12 /r",        2,    0,   Fmod,      Freg },
	{ "adc",   "15 iw",    	   2,   -1,   Fimm,      Fareg },
	{ "adc",   "81 /2 iw",     2,    1,   Fimm,      Fmod },
	{ "adc",   "11 /r",        2,    1,   Freg,      Fmod },
	{ "adc",   "13 /r",        2,    0,   Fmod,      Freg },

	{ "addb",  "04 ib",        2,   -1,   Fimm,      Fareg },
	{ "addb",  "80 /0 ib",     2,    1,   Fimm,	 Fmod },
	{ "addb",  "00 /r",        2,    1,   Freg,	 Fmod },
	{ "addb",  "02 /r",        2,    0,   Fmod,	 Freg },
	{ "add",   "05 iw",	   2,   -1,   Fimm,	 Fareg },
	{ "add",   "81 /0 iw",     2,    1,   Fimm,	 Fmod },
	{ "add",   "01 /r",	   2,    1,   Freg,	 Fmod },
	{ "add",   "03 /r",	   2,    0,   Fmod,	 Freg },
	{ "addq",  "83 /0 ib",     2,    1,   Fimm,      Fmod },

	{ "andb",  "24 ib",        2,   -1,   Fimm,      Fareg },
	{ "andb",  "80 /4 ib",     2,    1,   Fimm,	 Fmod },
	{ "andb",  "20 /r",        2,    1,   Freg,	 Fmod },
	{ "andb",  "22 /r",        2,    0,   Fmod,	 Freg },
	{ "and",   "25 iw",	   2,   -1,   Fimm,	 Fareg },
	{ "and",   "81 /4 iw",     2,    1,   Fimm,	 Fmod },
	{ "and",   "21 /r",	   2,    1,   Freg,	 Fmod },
	{ "and",   "23 /r",	   2,    0,   Fmod,	 Freg },

	{ "bound", "62 /r",        2,    1,   Freg,      Fmod },

	{ "bswap", "0F C8+r",      1,   -1,   Freg, },

	{ "cbw",   "98",           0,   -1   },

	{ "clc",   "F8",           0,   -1 },
	{ "cld",   "FC",           0,   -1 },
	{ "cli",   "FA",           0,   -1 },

	{ "cmc",   "F5",           0,   -1 },

	{ "cmpb",  "3C ib",        2,   -1,   Fimm,      Fareg },
	{ "cmpb",  "80 /7 ib",     2,    1,   Fimm,      Fmod },
	{ "cmpb",  "38 /r",        2,    1,   Freg,      Fmod },
	{ "cmpb",  "3A /r",        2,    0,   Fmod,      Freg },

	{ "cmp",   "3D iw",        2,   -1,   Fimm,      Fareg },
	{ "cmp",   "81 /7 iw",     2,    1,   Fimm,      Fmod },
	{ "cmp",   "39 /r",        2,    1,   Freg,      Fmod },
	{ "cmp",   "3B /r",        2,    0,   Fmod,      Freg },

	{ "cmpsb", "A6" },
	{ "cmpsw", "A7" },


	{ "call",   "E8 cw",       1,   -1,   Fdisp },
	{ "call",   "FF /2",       1,    0,   Fmod|Fabs },

	{ "cpuid", "0F A2",        0,   -1 },

	{ "cwd",   "99",           0,   -1 },

	{ "decb",  "FE /1",        1,    0,   Fmod },
	{ "dec",   "48+r",         1,    1,   Freg },
	{ "dec",   "FF /1",        1,    0,   Fmod },

	{ "divb",   "F6 /6",       1,    0,   Fmod },
	{ "div",    "F7 /6",       1,    0,   Fmod },

	{ "halt",   "F4",          0,   -1 },

	{ "idivb",  "F6 /7",       1,    0,   Fmod },
	{ "idiv",   "F7 /7",       1,    0,   Fmod },

	{ "inb",     "E4 ib",      1,   -1,   Fimm },
	{ "inb",     "EC",         0 },
	{ "in",      "E5 ib",      1,   -1,   Fimm },
	{ "in",      "ED",         0 },

	{ "incb",  "FE /0",        1,    0,   Fmod },
	{ "inc",   "40+r",         1,    1,   Freg },
	{ "inc",   "FF /0",        1,    0,   Fmod },

	{ "insb",  "6C", },
	{ "ins",   "6D" },

	{ "int3",  "CC" },
	{ "int",   "CD ib",        1,    -1,  Fimm },
	{ "into",  "CE" },

	{ "invd",  "0F 08" },

	{ "invlpg", "0F 01 /7",     1,     0,  Fmod },

	{ "iret",  "CF" },

	{ "jcxz",  "E3 cb",     1,    -1,  Fdisp },

	{ "jas",    "77 cb",     1,    -1,  Fdisp },	/* short ones for boot sector code */
	{ "jaes",   "73 cb",     1,    -1,  Fdisp },
	{ "jbs",    "72 cb",     1,    -1,  Fdisp },
	{ "jbes",   "76 cb",     1,    -1,  Fdisp },
	{ "jcs",    "72 cb",     1,    -1,  Fdisp },
	{ "jes",    "74 cb",     1,    -1,  Fdisp },
	{ "jgs",    "7F cb",     1,    -1,  Fdisp },
	{ "jges",   "7D cb",     1,    -1,  Fdisp },
	{ "jls",    "7C cb",     1,    -1,  Fdisp },
	{ "jles",   "7E cb",     1,    -1,  Fdisp },
	{ "jnas",   "76 cb",     1,    -1,  Fdisp },
	{ "jnaes",  "72 cb",     1,    -1,  Fdisp },
	{ "jnbs",   "73 cb",     1,    -1,  Fdisp },
	{ "jnbes",  "77 cb",     1,    -1,  Fdisp },
	{ "jncs",   "73 cb",     1,    -1,  Fdisp },
	{ "jnes",   "75 cb",     1,    -1,  Fdisp },
	{ "jngs",   "7E cb",     1,    -1,  Fdisp },
	{ "jnges",  "7C cb",     1,    -1,  Fdisp },
	{ "jnls",   "7D cb",     1,    -1,  Fdisp },
	{ "jnles",  "7F cb",     1,    -1,  Fdisp },
	{ "jnos",   "71 cb",     1,    -1,  Fdisp },
	{ "jnps",   "7B cb",     1,    -1,  Fdisp },
	{ "jnss",   "79 cb",     1,    -1,  Fdisp },
	{ "jnzs",   "75 cb",     1,    -1,  Fdisp },
	{ "jos",    "70 cb",     1,    -1,  Fdisp },
	{ "jps",    "7A cb",     1,    -1,  Fdisp },
	{ "jpes",   "7A cb",     1,    -1,  Fdisp },
	{ "jpos",   "7B cb",     1,    -1,  Fdisp },
	{ "jss",    "78 cb",     1,    -1,  Fdisp },
	{ "jzs",    "74 cb",     1,    -1,  Fdisp },

	{ "ja",    "0F 87 cw",     1,    -1,  Fdisp },
	{ "jae",   "0F 83 cw",     1,    -1,  Fdisp },
	{ "jb",    "0F 82 cw",     1,    -1,  Fdisp },
	{ "jbe",   "0F 86 cw",     1,    -1,  Fdisp },
	{ "jc",    "0F 82 cw",     1,    -1,  Fdisp },
	{ "je",    "0F 84 cw",     1,    -1,  Fdisp },
	{ "jg",    "0F 8F cw",     1,    -1,  Fdisp },
	{ "jge",   "0F 8D cw",     1,    -1,  Fdisp },
	{ "jl",    "0F 8C cw",     1,    -1,  Fdisp },
	{ "jle",   "0F 8E cw",     1,    -1,  Fdisp },
	{ "jna",   "0F 86 cw",     1,    -1,  Fdisp },
	{ "jnae",  "0F 82 cw",     1,    -1,  Fdisp },
	{ "jnb",   "0F 83 cw",     1,    -1,  Fdisp },
	{ "jnbe",  "0F 87 cw",     1,    -1,  Fdisp },
	{ "jnc",   "0F 83 cw",     1,    -1,  Fdisp },
	{ "jne",   "0F 85 cw",     1,    -1,  Fdisp },
	{ "jng",   "0F 8E cw",     1,    -1,  Fdisp },
	{ "jnge",  "0F 8C cw",     1,    -1,  Fdisp },
	{ "jnl",   "0F 8D cw",     1,    -1,  Fdisp },
	{ "jnle",  "0F 8F cw",     1,    -1,  Fdisp },
	{ "jno",   "0F 81 cw",     1,    -1,  Fdisp },
	{ "jnp",   "0F 8B cw",     1,    -1,  Fdisp },
	{ "jns",   "0F 89 cw",     1,    -1,  Fdisp },
	{ "jnz",   "0F 85 cw",     1,    -1,  Fdisp },
	{ "jo",    "0F 80 cw",     1,    -1,  Fdisp },
	{ "jp",    "0F 8A cw",     1,    -1,  Fdisp },
	{ "jpe",   "0F 8A cw",     1,    -1,  Fdisp },
	{ "jpo",   "0F 8B cw",     1,    -1,  Fdisp },
	{ "js",    "0F 88 cw",     1,    -1,  Fdisp },
	{ "jz",    "0F 84 cw",     1,    -1,  Fdisp },

	{ "jmps",  "EB cb",       1,    -1,  Fdisp },
	{ "jmp",   "E9 cw",       1,    -1,  Fdisp },
	{ "jmp",   "FF /4",       1,     0,  Fmod|Fabs },

	{ "lahf",  "9F" },
	{ "lds",   "C5 /r",	   2,     0,  Fmod,     Freg },
	{ "lea",   "8D /r ",       2,     0,  Fmod,	Freg },

	{ "les",   "C4 /r",	   2,     0,  Fmod,     Freg },

	{ "lidt",  "0F 01 /3",     1,     0,  Fmod },
	{ "lgdt",  "0F 01 /2",     1,     0,  Fmod },

	{ "lodsb", "AC" },
	{ "lods",  "AD" },

	{ "loop",  "E2 cb",         1,    -1,  Fdisp },
	{ "loope", "E1 cb",         1,    -1,  Fdisp },
	{ "loopz", "E1 cb",         1,    -1,  Fdisp },
	{ "loopne","E0 cb",         1,    -1,  Fdisp },
	{ "loopnz","E0 cb",         1,    -1,  Fdisp },

	{ "movb",  "B0+r ib",       2,    -1,  Fimm,       Freg },
	{ "movb",  "C6 /0 ib",      2,     1,  Fimm,       Fmod },
	{ "movb",  "8A /r",         2,     0,  Fmod,       Freg },
	{ "movb",  "88 /r",         2,     1,  Freg,       Fmod },

	{ "mov",   "B8+r iw",       2,    -1,  Fimm,       Freg },
	{ "mov",   "C7 /0 iw",      2,     1,  Fimm,       Fmod },
	{ "mov",   "8B /r",         2,     0,  Fmod,       Freg },
	{ "mov",   "89 /r",         2,     1,  Freg,       Fmod },

	{ "mov",   "8C /r",         2,     1,  Fsreg,      Fmod },
	{ "mov",   "8E /r",         2,     0,  Fmod,       Fsreg },

	{ "movsb", "A4" },
	{ "movs",  "A5" },

	{ "movsbl", "0F BE /r",     2,     0,  Fmod,       Freg },
	{ "movsbw", "0F BF /r",     2,     0,  Fmod,       Freg },

	{ "movzbl", "0F B6 /r",     2,     0,  Fmod,       Freg },
	{ "movzbw", "0F B7 /r",     2,     0,  Fmod,       Freg },

	{ "mulb",    "F6 /4",        1,     0,  Fmod },
	{ "mul",     "F7 /4",        1,     0,  Fmod },

	{ "negb",    "F6 /3",        1,     0,  Fmod },
	{ "neg",     "F7 /3",        1,     0,  Fmod },

	{ "nop",     "90" },

	{ "notb",    "F6 /2",        1,     0,  Fmod },
	{ "notw",    "66 F7 /2",     1,     0,  Fmod },
	{ "notl",    "F7 /2",        1,     0,  Fmod },
	{ "opsz",   "66",           0,   -1 },

	{ "orb",     "0C ib",        2,   -1,   Fimm,    Fareg },
	{ "orb",     "80 /1 ib",     2,    1,   Fimm,	 Fmod },
	{ "orb",     "08 /r",        2,    1,   Freg,	 Fmod },
	{ "orb",     "0A /r",        2,    0,   Fmod,	 Freg },
	{ "or",      "0D iw",        2,   -1,   Fimm,	 Fareg },
	{ "or",      "81 /1 iw",     2,    1,   Fimm,	 Fmod },
	{ "or",      "09 /r",        2,    1,   Freg,	 Fmod },
	{ "or",      "0B /r",        2,    0,   Fmod,	 Freg },


	{ "outb",    "E6 ib",        1,   -1,   Fimm },
	{ "outb",    "EE" },
	{ "out",     "E7 ib",        1,   -1,   Fimm },
	{ "out",     "EF" },

	{ "outsb",   "6E" },
 	{ "outs",    "6F" },

	{ "popa",   "61",           0,   -1 },
	{ "popf",    "9D" },
	{ "pusha",   "60",           0,   -1 },
	{ "pushf",   "9C" },

	/* do rcl/rcr/rol/ror as pseudo op */

	{ "rep",    "F3" },
	{ "repe",   "F3" },
	{ "repne",  "F2" },

	{ "ret",    "C3" },
	{ "ret",    "C2 iw",         1,   -1,   Fimm },
	{ "lret",   "CB" },
	{ "lret",   "CA iw",         1,   -1,   Fimm },

	{ "sahf",   "9E" },

	/* do sal, sar, shal, shr as pseudo op */

	{ "sbbb",    "1C ib",        2,   -1,   Fimm,    Fareg },
	{ "sbbb",    "80 /3 ib",     2,    1,   Fimm,	 Fmod },
	{ "sbbb",    "18 /r",        2,    1,   Freg,	 Fmod },
	{ "sbbb",    "1A /r",        2,    0,   Fmod,	 Freg },
	{ "sbb",     "1D iw",        2,   -1,   Fimm,	 Fareg },
	{ "sbb",     "81 /3 iw",     2,    1,   Fimm,	 Fmod },
	{ "sbb",     "19 /r",        2,    1,   Freg,	 Fmod },
	{ "sbb",     "1B /r",        2,    0,   Fmod,	 Freg },

	{ "scasb",   "AE" },
	{ "scas",    "AF" },

	{ "seges",   "26" },
	{ "segcs",   "2e" },
	{ "segss",   "36" },
	{ "segds",   "3e" },

	{ "seta",    "0F 97 /0",     1,     0,  Fmod },
	{ "setae",   "0F 93 /0",     1,     0,  Fmod },
	{ "setb",    "0F 92 /0",     1,     0,  Fmod },
	{ "setbe",   "0F 96 /0",     1,     0,  Fmod },
	{ "setc",    "0F 92 /0",     1,     0,  Fmod },
	{ "sete",    "0F 94 /0",     1,     0,  Fmod },
	{ "setg",    "0F 9F /0",     1,     0,  Fmod },
	{ "setge",   "0F 9D /0",     1,     0,  Fmod },
	{ "setl",    "0F 9C /0",     1,     0,  Fmod },
	{ "setle",   "0F 9E /0",     1,     0,  Fmod },
	{ "setna",   "0F 96 /0",     1,     0,  Fmod },
	{ "setnae",  "0F 92 /0",     1,     0,  Fmod },
	{ "setnb",   "0F 93 /0",     1,     0,  Fmod },
	{ "setnbe",  "0F 97 /0",     1,     0,  Fmod },
	{ "setnc",   "0F 93 /0",     1,     0,  Fmod },
	{ "setne",   "0F 95 /0",     1,     0,  Fmod },
	{ "setng",   "0F 9E /0",     1,     0,  Fmod },
	{ "setnge",  "0F 9C /0",     1,     0,  Fmod },
	{ "setnl",   "0F 9D /0",     1,     0,  Fmod },
	{ "setnle",  "0F 9F /0",     1,     0,  Fmod },
	{ "setno",   "0F 91 /0",     1,     0,  Fmod },
	{ "setnp",   "0F 9B /0",     1,     0,  Fmod },
	{ "setns",   "0F 99 /0",     1,     0,  Fmod },
	{ "setnz",   "0F 95 /0",     1,     0,  Fmod },
	{ "seto",    "0F 90 /0",     1,     0,  Fmod },
	{ "setp",    "0F 9A /0",     1,     0,  Fmod },
	{ "setpe",   "0F 9A /0",     1,     0,  Fmod },
	{ "setpo",   "0F 9B /0",     1,     0,  Fmod },
	{ "sets",    "0F 98 /0",     1,     0,  Fmod },
	{ "setz",    "0F 94 /0",     1,     0,  Fmod },

	{ "stc",     "F9" },
	{ "std",     "FD" },
	{ "sti",     "FB" },

	{ "stosb",    "AA" },
	{ "stos",     "AB" },

	{ "str",      "0F 00 /1",   1,      0,  Fmod },

	{ "subb",    "2C ib",        2,   -1,   Fimm,    Fareg },
	{ "subb",    "80 /5 ib",     2,    1,   Fimm,	 Fmod },
	{ "subb",    "28 /r",        2,    1,   Freg,	 Fmod },
	{ "subb",    "2A /r",        2,    0,   Fmod,	 Freg },
	{ "sub",     "2D iw",        2,   -1,   Fimm,	 Fareg },
	{ "sub",     "81 /5 iw",     2,    1,   Fimm,	 Fmod },
	{ "sub",     "29 /r",        2,    1,   Freg,	 Fmod },
	{ "sub",     "2B /r",        2,    0,   Fmod,	 Freg },
	{ "subq",    "83 /5 ib",     2,    1,   Fimm,    Fmod },

	{ "testb",   "A8 ib",        2,   -1,   Fimm,    Fareg },
	{ "testb",   "F6 /0 ib",     2,    1,   Fimm,	 Fmod },
	{ "testb",   "84 /r",        2,    1,   Freg,	 Fmod },
	{ "test",    "A9 iw",        2,   -1,   Fimm,	 Fareg },
	{ "test",    "F7 /0 iw",     2,    1,   Fimm,	 Fmod },
	{ "test",    "85 /r",        2,    1,   Freg,	 Fmod },

	{ "xchgb",   "86 /r",        2,    1,   Freg,    Fmod},
	{ "xchgb",   "86 /r",        2,    0,   Fmod,    Freg },
	{ "xchgl",   "90+r",         2,   -1,   Freg,    Fareg },
	{ "xchgl",   "90+r",         2,   -1,   Fareg,   Freg },
	{ "xchgl",   "87 /r",        2,    1,   Freg,    Fmod },
	{ "xchgl",   "87 /r",        2,    0,   Fmod,    Freg },

	{ "xlatb",   "D7" },

	{ "xorb",    "34 ib",        2,   -1,   Fimm,    Fareg },
	{ "xorb",    "80 /6 ib",     2,    1,   Fimm,	 Fmod },
	{ "xorb",    "30 /r",        2,    1,   Freg,	 Fmod },
	{ "xorb",    "32 /r",        2,    0,   Fmod,	 Freg },
	{ "xorl",    "35 iw",        2,   -1,   Fimm,	 Fareg },
	{ "xorl",    "81 /6 iw",     2,    1,   Fimm,	 Fmod },
	{ "xorl",    "31 /r",        2,    1,   Freg,	 Fmod },
	{ "xorl",    "33 /r",        2,    0,   Fmod,	 Freg },

	{ 0 }
};

enum {
	TEOF,
	TCONST,
	TNAME,
	TLSHIFT,
	TRSHIFT,
};

enum {
	BLKLEN = 8*1024,
	NBLKPTRS = 2,
};

typedef struct Block Block;
struct Block {
	Block	*next;
	int	len;
	char	block[BLKLEN];
};

typedef struct Blkptr Blkptr;
struct Blkptr {
	Block	*first;
	Block	*last;
};

enum { NOPRN = 5, };

int	Lflag;
int	allow_reg;		/* flag: lookup will take names begining with % */
Blkptr	blkptrs[NBLKPTRS];
int	bytes_out;
Blkptr	**cur_bp;
int	*cur_loc;
R	**cur_relo;
int	debug;
R	*drelo;
jmp_buf env;
int	eof;
int	errcnt;
char	*filename;
int	in_offset;		/* offset into temp instruction buffer */
char	instr[12];
Sym	*last_sym;
Sym	*last_sys_sym;
char	*lexp;
int	lineno;
int	locs[3];
char	*name;
int	noprn;
Name	*nspace;
int	nxt_tmp;
Operand	oprn[NOPRN];
Biobuf	*outbp;
char	*outfname;
int	pass_no;
Relo	*rspace;
int	slen;
Biobuf	*src;
char	*sspace;
Sym	*symtab;
Blkptr	*tbp, *dbp;
Tmp	tmp[NTMP];
int	tok;
R	*trelo;
int	value;

int	block_copyout(Biobuf *, Blkptr *);
Blkptr	*block_open(void);
void	block_write(Blkptr *, void *, int);
int	cur_seg(int);
void	dumpb(char *, int);
void	error(char *, ...);
int	expr(int *, Sym **);
void	fatal(char *, ...);
void	fixup_undefs(void);
void	format(Itab *);
char	*get_ident(void);
int	get_num(void);
int	get_quoted(void);
char	*get_string(int);
void	init_kw(void);
void	install(char *, uintptr, int);
void	lex(void);
Sym	*lookup(char *);
int	lookup_tmp(int, int, int*);
void	lusyms(void);
int	mag(ulong, int);
int	map_type(int);
void	mk_code(char *);
int	mk_ea(int, ushort, Operand *, Operand*);
int	mk_extw(int, Operand *);
int	mk_mode(Operand *);
void	mk_tmp(int);
void	note_relo(Sym *, int, int, int);
int	number(Operand *);
Itab	*omatch(char *);
void	parseo(void);
void	pass1(void);
void	pass2(void);
void	print_undefs(void);
void	pr_oper(void);
void	pr_tok(void);
void	put(Operand *, int);
void	put_const(int, int);
void	put_string(void);
void	rlex(void);
void	store_image(void);
void	warning(char *, ...);
void	write_instr(void);
void	write_string(Blkptr *, char *, int);

static void
usage(void)
{
	fprint(2, "usage: %s [-XL][-o obj] [input]\n", argv0);
	exits("usage");
}

static void
vfybyteorder(void)
{
	union word {
		ulong	ul;
		char	ch[sizeof(ulong)];
	} wd;

	if (sizeof wd != sizeof wd.ul || sizeof wd.ul != sizeof wd.ch)
		sysfatal("possibly 64-bit struct alignment trouble\n");
	memset(&wd, 0, sizeof wd);
	wd.ch[0] = 'a';
	wd.ch[1] = 'b';
	wd.ch[2] = 'c';
	wd.ch[3] = 'd';
	switch (wd.ul) {
	case 'd'<<24 | 'c'<<16 | 'b'<<8 | 'a':
		return;			/* little endian, required */
	}
	if((wd.ul & ((1ull << 32) - 1)) != 0)
		sysfatal("gok");
	if (sizeof(uintptr) == sizeof(uvlong)) {
		wd.ul >>= 32;			/* on plan 9: "stupid shift" */
		switch (wd.ul) {
		case 'd'<<24 | 'c'<<16 | 'b'<<8 | 'a':
			return;		/* 64-bit little endian */
		}
	}
	sysfatal("must be run on a little-endian machine");
}

void
main(int argc, char *argv[])
{
	argv0 = argv[0];
	outfname = "a.out";
	ARGBEGIN{
	case 'o':
		outfname = EARGF(usage());
		break;
	case 'L':
		Lflag++;
		break;
	case 'X':
		debug++;
		break;
	default:
		usage();
	}ARGEND
	if (argc != 1)
		usage();
	filename = argv[0];

	vfybyteorder();
	init_kw();

	src = Bopen(filename, OREAD);
	if (src == nil)
		sysfatal("can't open %s: %r", filename);
	outbp = Bopen(outfname, OWRITE);
	if (outbp == nil)
		sysfatal("can't open %s: %r", outfname);

	tbp = block_open();
	dbp = block_open();
	cur_bp = &tbp;
	cur_relo = &trelo;
	pass1();
	fixup_undefs();
	if (errcnt)
		exits("errs");

	lexp = 0;
	Bseek(src, 0, 0);
	eof = 0;
	pass2();
	print_undefs();
	if (errcnt)
		exits("errs");

	store_image();
	exits(nil);
}

void
dump(void *vf, int n)
{
	int *f = vf;
	int i;

	while (n > 0) {
		print("%08p: ", f);
		for (i = 0; i < 8; i++)
			print("%08x ", *f++);
		print("\n");
		n -= 4*8;
	}
}


void
fixup_undefs(void)	/* make all undefined external */
{
	Sym *sp;

	for (sp = last_sys_sym->next; sp; sp = sp->next)
		if (sp->type == N_UNDEF)
			sp->type |= N_EXT;
}

void
print_undefs(void)
{
	Sym *sp;

	for (sp = last_sys_sym->next; sp; sp = sp->next)
		if ((sp->type&~N_EXT) == N_UNDEF)
			fprint(2, "%s: %s: undefined\n", argv0, sp->name);
}

int
xwrite(Biobuf *bp, void *where, int len)
{
	return Bwrite(bp, where, len);
}

/*
 * we can edit .Ls out because no relocation record points to them
 */

void
edit_names(void)	/* squeeze out .L */
{
	Sym *sp, **sq;

	sp = last_sys_sym->next;
	sq = &last_sys_sym->next;
	while (sp) {
		if (strncmp(sp->name, ".L", 2) == 0)
			*sq = sp->next;
		else
			sq = &sp->next;
		sp = sp->next;
	}
}

void
set_relo(Sym *sp, int snum)	/* find an set snum in relocation table */
{
	R *rp;

	for (rp = trelo; rp; rp = rp->next)
		if (sp == rp->what)
			rp->r.r_symbolnum = snum;
	for (rp = drelo; rp; rp = rp->next)
		if (sp == rp->what)
			rp->r.r_symbolnum = snum;
}

int
pack_names(int *nsize)	/* put strings into string space */
{
	char *cp;
	Sym *sp;
	Name *np;
	int ne = 0, slen = 0, cnt;

	/* scan symbol table counting entries and string sizes */
	for (sp = last_sys_sym->next; sp; sp = sp->next) {
		ne++;
		slen += strlen(sp->name)+1;
	}
	sspace = malloc(slen + 4);
	nspace = malloc(ne * sizeof(Name));
	if (sspace == nil || nspace == nil)
		sysfatal("out of memory");
	*nsize = ne * sizeof (Name);

	cp = sspace;
	cp += 4;	/* save room for total string length */
	np = nspace;
	cnt = 0;
	for (sp = last_sys_sym->next; sp; sp = sp->next) {
		strcpy(cp, sp->name);
		np->type = sp->type;
		np->value = sp->addr;
		np->name = cp - sspace;
		set_relo(sp, cnt++);
		cp += strlen(cp)+1;
		np++;
	}
	*(int *)sspace = cp - sspace;
	return cp - sspace;
}

int
copyout_relo(Biobuf *bp, R *r)	/* format and copyout relocation information */
{
	int nr = 0;
	R *rp;
	Relo *p;

	for (rp = r; rp; rp = rp->next) {
/*		if (rp->what && rp->r.r_symbolnum == 0)
			fatal("%s: relocation reference", rp->what->name); */
		nr++;
	}
	rspace = (Relo *)malloc(nr * sizeof *rspace);
	p = rspace;
	for (rp = r; rp; rp = rp->next)
		*p++ = rp->r;
	Bwrite(bp, rspace, sizeof *rspace * nr);
	free(rspace);
	return nr * sizeof *rspace;
}

void
store_image(void)
{
	Exec e;
	int i, nsyms;

	if (!Lflag)
		edit_names();	/* take out .L symbols */
	i = pack_names(&nsyms);	/* get strings packed up to go */
	e.magic = 0407;
	Bseek(outbp, sizeof e, 0);
	e.text = block_copyout(outbp, tbp);
	e.data = block_copyout(outbp, dbp);
	e.trelo = copyout_relo(outbp, trelo);
	e.drelo = copyout_relo(outbp, drelo);
	xwrite(outbp, nspace, nsyms);
	xwrite(outbp, sspace, i);
	e.bss = (locs[2] + 511) & ~0x1FF;
	e.entry = 0x1023;
	e.symsz = nsyms;
	Bseek(outbp, 0, 0);
	Bwrite(outbp, &e, sizeof e);
}


void dot_byte(char *);
void dot_word(char *);
void dot_long(char *);
void dot_text(char *);
void dot_bss(char *);
void dot_data(char *);
void dot_align(char *);
void dot_space(char *);
void dot_globl(char *);
void dot_lcomm(char *);
void dot_comm(char *);
void not_yet(char *);
void do_imul(char *);
void do_pop(char *);
void do_push(char *);
void do_rot(char *);
void do_shift(char *);
void dot_set(char *);
void do_ljmp(char *);
void do_lcall(char *);

void
init_kw(void)	/* put things in the symbol table */
{
	Itab *ip;

	/*
	 * registers use octal encoding of classes of register.
	 * The register value is the least sig 3 bits.
	 *
	 * 1	16
	 * 2	8 bit register
	 * 3	segment register
	 */

	install("ax",  010, N_REG);
	install("cx",  011, N_REG);
	install("dx",  012, N_REG);
	install("bx",  013, N_REG);
	install("sp",  014, N_REG);
	install("bp",  015, N_REG);
	install("si",  016, N_REG);
	install("di",  017, N_REG);

	install("al",  020, N_REG);
	install("cl",  021, N_REG);
	install("dl",  022, N_REG);
	install("bl",  023, N_REG);
	install("ah",  024, N_REG);
	install("ch",  025, N_REG);
	install("dh",  026, N_REG);
	install("bh",  027, N_REG);

	install("es",  030, N_REG);
	install("cs",  031, N_REG);
	install("ss",  032, N_REG);
	install("ds",  033, N_REG);
	install("fs",  034, N_REG);	/* with care */

	install(".align", (uintptr)dot_align, N_KW);
	install(".bss", (uintptr)dot_bss, N_KW);
	install(".byte", (uintptr)dot_byte, N_KW);
	install(".comm", (uintptr)dot_comm, N_KW);
	install(".data", (uintptr)dot_data, N_KW);
	install(".globl", (uintptr)dot_globl, N_KW);
	install(".lcomm", (uintptr)dot_lcomm, N_KW);
	install(".long", (uintptr)dot_long, N_KW);
	install(".set",  (uintptr)dot_set, N_KW);
	install(".size", (uintptr)not_yet, N_KW);
	install(".space", (uintptr)dot_space, N_KW);
	install(".text", (uintptr)dot_text, N_KW);
	install(".type", (uintptr)not_yet, N_KW);
	install(".ident", (uintptr)not_yet, N_KW);
	install(".word", (uintptr)dot_word, N_KW);

	/* instructions that don't fit the pattern matching algorithim */

	install("imul", (uintptr)do_imul, N_KW);
	install("imulb", (uintptr)do_imul, N_KW);
	install("lcall", (uintptr)do_lcall, N_KW);
	install("ljmp",  (uintptr)do_ljmp, N_KW);
	install("pop", (uintptr)do_pop, N_KW);
	install("pushb", (uintptr)do_push, N_KW);
	install("push", (uintptr)do_push, N_KW);
	install("salb", (uintptr) do_shift, N_KW);
	install("sal", (uintptr) do_shift, N_KW);
	install("sarb", (uintptr) do_shift, N_KW);
	install("sar", (uintptr) do_shift, N_KW);
	install("shlb", (uintptr) do_shift, N_KW);
	install("shl", (uintptr) do_shift, N_KW);
	install("shrb", (uintptr) do_shift, N_KW);
	install("shr", (uintptr) do_shift, N_KW);

	install("rclb", (uintptr) do_rot, N_KW);
	install("rcl", (uintptr) do_rot, N_KW);
	install("rcrb", (uintptr) do_rot, N_KW);
	install("rcr", (uintptr) do_rot, N_KW);
	install("rolb", (uintptr) do_rot, N_KW);
	install("rol", (uintptr) do_rot, N_KW);
	install("rorb", (uintptr) do_rot, N_KW);
	install("ror", (uintptr) do_rot, N_KW);

	for (ip = itab; ip->name; ip++)
		install(ip->name, (uintptr)ip, N_INST);

	last_sys_sym = last_sym;
}

void
install(char *name, uintptr value, int type)	/* insert symbol table entry */
{
	Sym *sp;

	allow_reg = 1;
	sp = lookup(name);
	allow_reg = 0;
	sp->type = type;
	sp->addr = value;
}

void
fatal(char *msg, ...)
{
	va_list arg;

	fprint(2, "Fatal error: %s:%d: ", filename, lineno);
	va_start(arg, msg);
	vfprint(2, msg, arg);
	va_end(arg);
	fprint(2, "\n");
	*(int *)0 = 0;
	exits("fatal");
}

void
error(char *msg, ...)		/* nonfatal error message */
{
	va_list arg;

	errcnt++;
	fprint(2, "Error: %s:%d: ", filename, lineno);
	va_start(arg, msg);
	vfprint(2, msg, arg);
	va_end(arg);
	fprint(2, "\n");
	longjmp(env, 1);
}

void
warning(char *msg, ...)
{
	va_list arg;

	fprint(2, "Warning: %s:%d: ", filename, lineno);
	va_start(arg, msg);
	vfprint(2, msg, arg);
	va_end(arg);
	fprint(2, "\n");
}

int
get_num(void)
{
	int val = 0;

	if (*lexp == '0') {
		lexp++;
		if (*lexp == 'x' || *lexp == 'X') {
			lexp++;
			while (isxdigit(*lexp)) {
				val <<= 4;
				if (*lexp >= '0' && *lexp <= '9')
					val += *lexp - '0';
				else if (*lexp >= 'a' && *lexp <= 'f')
					val += *lexp - 'a' + 10;
				else
					val += *lexp - 'A' + 10;
				lexp++;
			}
			return val;
		}
		while (isdigit(*lexp)) {
			val <<= 3;
			val += *lexp++ - '0';
		}
		return val;
	}
	while (isdigit(*lexp)) {
		val *= 10;
		val += *lexp++ - '0';
	}
	return val;
}

char *
get_ident(void)
{
	char *p, buf[512];

	p = buf;
	*p++ = *lexp++;
	while (isalnum(*lexp) || *lexp == '_' || *lexp == '.')
		*p++ = *lexp++;
	*p = 0;
	return strdup(buf);
}

char *
get_string(int closec)
{
	char *p, buf[512];
	int val = 0;

	p = buf;
	while (*lexp && *lexp != closec)
		if (*lexp != '\\')
			*p++ = *lexp++;
		else if (lexp[1] == '\\')
			*p++ = '\\';
		else
			switch (lexp[1]) {
			case 'b': *p++ = '\b';  lexp += 2; break;
			case 't': *p++ = '\t';  lexp += 2; break;
			case 'r': *p++ = '\r';  lexp += 2; break;
			case 'n': *p++ = '\n'; lexp += 2;  break;
			case '>': *p++ = '>'; lexp += 2; break;
			case '0':
				lexp += 2;
				while (isdigit(*lexp)) {
					val <<= 3;
					val += *lexp - '0';
				}
				*p++ = val;
				break;
			default:
				error("bad char in string");
			}
	*p = 0;
	slen = p - buf;
	if (*lexp == 0)
		error("newline in string");
	lexp++;
	return strdup(buf);
}

int
get_quoted(void)
{
	int val = 0;

	lexp++;
	if (*lexp != '\\')
		val = *lexp++;
	else if (lexp[1] == '\\') {
		lexp += 2;
		val = '\\';
	} else
		switch (lexp[1]) {
		case 'b': val = '\b';  lexp += 2; break;
		case 't': val = '\t';  lexp += 2; break;
		case 'r': val = '\r';  lexp += 2; break;
		case 'n': val = '\n'; lexp += 2;  break;
		case '"': val = '"'; lexp += 2; break;
		case '0':
			lexp += 2;
			while (isdigit(*lexp)) {
				val <<= 3;
				val += *lexp - '0';
				lexp++;
			}
			break;
		default:
			error("bad char in char const");
		}
	if (*lexp != '\'')
		error("buggered-up character constant");
	else
		lexp++;
	return val;
}

char line[256];

int	op;	/* true if we know we are in an operator; for '-' */

void
rlex(void)
{
	if (eof) {
		tok = TEOF;
		return;
	}
	if (lexp == 0) {
read_again:
		lexp = Brdline(src, '\n');
		if (lexp == 0) {
			tok = TEOF;
			eof = 1;
			return;
		}
		lineno++;
	}
again:
	while (*lexp == ' ' || *lexp == '\t')
		lexp++;
	switch (*lexp) {
	case 0:
		goto read_again;
	case '-':
	case ',':
	case '(':
	case ')':
	case ':':
	case '=':
	case '+':
	case '*':
	case '@':
	case '"':	/* XXX should parse string */
	case '$':
		tok = *lexp++;
		return;

	case '/':
		if (lexp[1] == '*') {		/* comments */
			lexp += 2;
			for (;;) {
				while (*lexp != '*' && *lexp != '\n')
					lexp++;
				if (*lexp == '\n') {
					lexp = Brdline(src, '\n');
					if (lexp == nil) {
						tok = TEOF;
						eof = 1;
						return;
					}
					lineno++;
				} else if (lexp[1] == '/') {
					lexp += 2;
					goto again;
				} else
					lexp++;
			}
		}
		tok = *lexp++;
		return;
	case '<':
		if (lexp[1] == '<') {
			lexp += 2;
			tok = TLSHIFT;
			return;
		}
		tok = *lexp++;
		return;

	case '>':
		if (lexp[1] == '>') {
			lexp += 2;
			tok = TRSHIFT;
			return;
		}
		tok = *lexp++;
		return;

	case '\'':
		value = get_quoted();
		tok = TCONST;
		return;

	case '\n':
		tok = *lexp;
		lexp = 0;
		return;

	case '\\':
		if (lexp[1] == '/') {
			lexp += 2;
			tok = '/';
			return;
		}
		goto bad_char;

	case ';':
		lexp++;
		tok = '\n';
		return;

	default:
		if (isdigit(*lexp)) {
			value = get_num();
			tok = TCONST;
			return;
		}
		if (isalpha(*lexp) || *lexp == '_' || *lexp == '.' || *lexp == '%') {
			name = get_ident();
			tok = TNAME;
			return;
		}
bad_char:
		fprint(2, "%s:%d illegal character %c\n",
			filename, lineno, *lexp);
		lexp++;
		goto again;
	}
}

void
pr_tok(void)
{
	print("%s:%d: ", filename, lineno);
	switch (tok) {
	case ',':
	case '(':
	case ')':
	case ':':
	case ';':
	case '=':
	case '+':
	case '-':
	case '*':
	case '#':
	case '$':
		print(" %c\n", tok);
		break;
	case TCONST:
		print(" TCONST %d\n", value);
		break;
	case TNAME:
		print(" TNAME %s\n", name);
		break;
	case TRSHIFT:
		print(" TRSHIFT\n");
		break;
	case TLSHIFT:
		print(" TLSHIFT\n");
		break;
	case '\n':
		print(" NEWLINE\n");
		break;
	case TEOF:
		print("EOF\n");
		break;
	default:
		print("??? %c\n", tok);
		break;
	}
}

void
lex(void)
{
	rlex();
	if (pass_no == 1 && lineno == 25000000)
		pr_tok();
}

Sym *
lookup(char *name)	/* find non-instruction */
{
	Sym *sp;

	for (sp = symtab; sp; sp = sp->next)
		if (strcmp(sp->name, name) == 0 && sp->type != N_INST &&
		    sp->type != N_KW)
			return sp;
	sp = (Sym *)malloc(sizeof *sp);
	if (last_sym)
		last_sym->next = sp;
	else
		symtab = sp;
	last_sym = sp;
	sp->name = strdup(name);
	sp->type = 0;
	sp->addr = 0;
	sp->next = 0;
	return sp;
}

Sym *
lookupi(char *name)	/* find instruction */
{
	Sym *sp;

	for (sp = symtab; sp; sp = sp->next)
		if (strcmp(sp->name, name) == 0 &&
		    (sp->type == N_INST || sp->type == N_KW))
			return sp;
	return 0;
}

Itab *
omatch(char *name)
{
	Operand *p;
	Itab *ip;

	for (ip = itab; ip->name; ip++)
		if (strcmp(ip->name, name) == 0)
			break;
	if (ip->name == 0)
		error("illegal opcode <%s>", name);
	for ( ; ip->name && strcmp(ip->name, name) == 0; ip++) {
		if (noprn != ip->noperands)
			continue;
		p = oprn;
		if (noprn > 0 &&
		   ((p->mode & ip->omask1) == 0 || p->mode & ~ip->omask1))
			continue;
		if ((p->mode & Fabs) && (ip->omask1 & Fabs) == 0)
			continue;
		if (noprn > 1 &&
		    ((p[1].mode & ip->omask2) == 0 || p[1].mode & ~ip->omask2))
			continue;
		if ((p[1].mode & Fabs) && (ip->omask2 & Fabs) == 0)
			continue;
		return ip;
	}
	pr_oper();
	error("illegal operand");
	return 0;
}


/*
 * build the symbol table
 */

void
pass1(void)
{
	char *np;
	Sym *sp, *sp2;
	int n;

	pass_no = 1;
	lineno = 0;
	locs[0] = locs[1] = locs[2] = 0;
	cur_loc = locs;
	if (setjmp(env)) {
		while (!eof && tok != '\n')
			lex();
	}
	lex();
loop:
	if (tok == TEOF)
		return;
	if (tok == '\n') {
		lex();
		goto loop;
	}
	if (tok == TNAME) {
		np = name;
		lex();
		if (tok == ':') {
			sp = lookup(np);
			if ((sp->type&N_TYPE) != N_UNDEF)
				error("symbol already defined!");
			sp->type = map_type(cur_loc - locs);
			sp->addr = *cur_loc;
			lex();
			goto loop;
		}
		if (tok == '=') {
			sp = lookup(np);
			if (sp->type != N_UNDEF)
				error("symbol already defined!");
			lex();
			sp->addr = expr(&sp->type, &sp2);
			goto loop;
		}
		sp = lookupi(np);
		if (sp == 0)
			error("unknown symbol for op-code: %s", np);
		if (sp->type == N_KW) {
			(*(void(*)(char *))sp->addr)(np);
			goto loop;
		}
		if (sp->type == N_INST) {
			parseo();
			mk_code(np);
			if (tok != '\n')
				error("extra stuff");
			goto loop;
		}
		error("unknown symbol type for op-code: %s", np);
	}
	if (tok == TCONST) {
		n = value;
		lex();
		if (value >= 0 && value < 10 && tok == ':') {
			mk_tmp(n);
			lex();
		} else
			*cur_loc += 4;
		goto loop;
	}
	if (tok == '<') {
		get_string('>');
		*cur_loc += slen;
		lex();
		goto loop;
	}
	error("syntax");
}

/*
 * pass over text again, this time generating the binary
 */

void
pass2(void)
{
	char *np;
	Sym *sp, *sp2;
	int n;

	pass_no = 2;
	lineno = 0;
	locs[0] = locs[1] = locs[2] = 0;
	cur_loc = locs;
	cur_bp = &tbp;
	cur_relo = &trelo;
	if (setjmp(env)) {
		while (tok != '\n')
			lex();
	}
	lex();
loop:
	if (tok == TEOF)
		return;
	if (tok == '\n') {
		lex();
		goto loop;
	}
	if (tok == TNAME) {
		np = name;
		lex();
		if (tok == ':') {
			sp = lookup(np);
			if (sp->addr != *cur_loc && errcnt == 0)
				fatal("out of phase: %08x %08x", sp->addr, *cur_loc);
			lex();
			goto loop;
		}
		if (tok == '=') {
			sp = lookup(np);
			lex();
			sp->addr = expr(&sp->type, &sp2);
			goto loop;
		}
		sp = lookupi(np);
		if (sp == 0)
			error("unknown symbol for op-code: %s", np);
		if (sp->type == N_KW) {
			(*(void(*)(char *))sp->addr)(np);
			goto loop;
		}
		if (sp->type == N_INST) {
			parseo();
			mk_code(np);
			if (tok != '\n')
				error("extra stuff");
			goto loop;
		}
	}
	if (tok == TCONST) {
		n = value;
		lex();
		if (value >= 0 && value < 10 && tok == ':')
			lex();	/* maybe I could check that the labels match */
		else {
			put_const(n, 4);
			write_instr();
		}
		goto loop;
	}
	if (tok == '<') {
		char *cp;

		cp = get_string('>');
		write_string(*cur_bp, cp, slen);
		*cur_loc += slen;
		lex();
		goto loop;
	}
	error("syntax");
}

void
mk_tmp(int n)
{
	Tmp *p;

	assert(nxt_tmp < NTMP);
	p = &tmp[nxt_tmp++];
	p->loc = *cur_loc;
	p->type = map_type(cur_loc - locs);
	p->label = n;
}

int
map_type(int loc_sub)	/* map location counter subscript into type */
{
	return (loc_sub << 1) + N_TEXT;
}

int
lookup_tmp(int n, int before, int *type)
{
	Tmp *p, *q, *e;

	q = 0;
	e = &tmp[nxt_tmp];
	if (before) {
		for (p = tmp; p < e; p++) {
			if (!cur_seg(p->type))
				continue;
			if (p->loc > *cur_loc)
				break;
			if (p->label == n)
				q = p;
		}
		if (q == 0)
			error("missing temporary label %db", n);
		*type = q->type;
		return q->loc;
	}
	/* else after */
	if (pass_no == 1) {
		*type = N_UNDEF;
		return 0;
	}
	for (p = tmp; p < e; p++) {
		if (!cur_seg(p->type))
			continue;
		if (p->loc >= *cur_loc)
			break;
	}
	if (p >= e)
		error("temporary %df not found", n);
	for (; p < e; p++)
		if (p->label == n) {
			*type = p->type;
			return p->loc;
		}
	error("temporary %df not found", n);
	return 0;
}

int
term(int *type_p, Sym **spp)	/* parse and evaluate expression */
{
	Sym *sp;
	int n;

	*spp = 0;
	if (tok == '-') {
		lex();
		if (tok != TCONST)
			error("can only do unary minus with constant for now");
		*type_p = N_ABS;
		n = value;
		lex();
		return -n;
	}
	if (tok == TNAME) {
		sp = lookup(name);
		*type_p = sp->type;
		switch (sp->type & N_TYPE) {
		case N_TEXT:
		case N_DATA:
		case N_BSS:
		case N_ABS:
		case N_UNDEF:
			break;
		default:
			if (pass_no > 1)
				error("illegal type in expression");
		}
		*spp = sp;
		lex();
		if (sp->type & N_EXT)
			return 0;
		return sp->addr;
	}
	if (tok == TCONST) {
		if (value >= 0 && value <= 9) {
			n = value;
			lex();
			if (tok == TNAME &&
			    (name[0] == 'f' || name[0] == 'b') && name[1] == 0){
				lex();
				if (pass_no != 2)
					return 0;
				return lookup_tmp(n, *name == 'b', type_p);
			}
			*type_p = N_ABS;
			return n;
		}
		*type_p = N_ABS;
		lex();
		return value;
	}
	pr_tok();
	error("expression syntax");
	return 0;
}

int
prop_type(int t1, int t2, int op)	/* propagate type */
{
	int a, b;

	if (pass_no == 1)
		return t1;
	a = t1 & N_TYPE;
	b = t2 & N_TYPE;
	if (a == N_UNDEF || b == N_UNDEF)
		return N_UNDEF;
	if (a == N_ABS && b == N_ABS)
		return N_ABS;
	switch (op) {
	case '+':
		if (a == N_TEXT || a == N_DATA || a == N_BSS || t1 == N_UNDEF+N_EXT) {
			if (b != N_ABS)
				error("illegal expression");
			return t1;
		}
		if (b == N_TEXT || b == N_DATA || b == N_BSS || t2 == N_UNDEF+N_EXT) {
			if (a != N_ABS)
				error("illegal expression");
			return t2;
		}
		error("illegal expression");
	case '-':
		if (a == N_TEXT || a == N_DATA || a == N_BSS) {
			if (b == N_ABS)
				return t1;
			if (b == N_TEXT || b == N_DATA || b == N_BSS)
				return N_ABS;
		}
		/* fall thru */
	default:
		error("illegal expression");
	}
	return 0;
}

/* for now, we cheat and use the type of the first term */
int
expr(int *type_p, Sym **spp)	/* evaluate simple expressions */
{
	int t1, t2, v1, v2;
	Sym *sp1, *sp2;

	v1 = term(&t1, &sp1);
	op = 1;
	for (;;)
		switch (tok) {
		case '+':
			lex();
			op = 0;
			v2 = term(&t2, &sp2);
			t1 = prop_type(t1, t2, '+');
			v1 += v2;
			if (sp1 && sp2)
				sp1 = 0;
			if (sp2)
				sp1 = sp2;
			break;

		case '-':
			lex();
			op = 0;
			v2 = term(&t2, &sp2);
			t1 = prop_type(t1, t2, '-');
			v1 -= v2;
			if (sp1 && sp2)
				sp1 = 0;
			if (sp2)
				sp1 = sp2;
			break;

		case TLSHIFT:
			lex();
			op = 0;
			v2 = term(&t2, &sp2);
			t1 = prop_type(t1, t2, 0);
			v1 <<= v2;
			sp1 = 0;
			break;

		case TRSHIFT:
			lex();
			op = 0;
			v2 = term(&t2, &sp2);
			t1 = prop_type(t1, t2, 0);
			v1 >>= v2;
			sp1 = 0;
			break;
		default:
			*type_p = t1;
			*spp = sp1;
			return v1;
		}
}

/*
 * Intel 486 syntax
 */

void
parseo(void)	/* parse zero, one or two operands */
{
	int i;
	Operand *p;
	Sym *sp;

	memset(oprn, 0, sizeof oprn);
	p = oprn;
	noprn = 0;
	i = 0;
	if (tok == '\n')
		return;
	for (;;) {
		if (tok == '*') {
			p->mode |= Fabs;
			lex();
		}
		switch (tok) {
		case TNAME:
			sp = lookup(name);
			if (sp->type != N_REG)
				goto offset;
			if (sp->type == N_UNDEF)
				error("illegal register name:%s", name);
			p->type = N_REG;
			switch ((sp->addr >> 3) & 7) {
			case 1:
			case 2:
				p->mode |= (sp->addr & 7) == 0? Fareg: Fxreg;
				break;
			case 3:
				p->mode |= Fsreg;
				break;
			default:
				error("illegal value for register (%s)", name);
			}
			p->reg = sp->addr;
			lex();
			break;

		case '$':
			lex();
			if (tok != TCONST && tok != TNAME && tok != '-')
				error("syntax");
			p->mode |= Fimm;
			p->offset = expr(&p->type, &p->sym);
			break;

		case TCONST:
		case '-':
		offset:
			p->mode |= Fdisp;
			p->offset = expr(&p->type, &p->sym);
			if (tok != '(')
				break;
			/* fall thru */
		case '(':
			p->index = 0;
			lex();
			if (tok == TNAME) {
				sp = lookup(name);
				if (sp->type != N_REG)
					error("register expected after (");
				if ((sp->addr & 070) != 010)
					error("syntax1");
				p->mode |= Fmodrm;
				p->base = sp->addr;
				lex();
				if (!((1<<p->base) & Oindex))
					error("base register may not be an index");
			}
			if (tok == ',') {
				lex();
				if (tok != TNAME)
					error("syntax2");
				sp = lookup(name);
				if (sp->type != N_REG)
					error("register expected after (name,");
				if ((sp->addr & 070) != 010)
					error("syntax4");
				p->index = sp->addr;
				if (!((1<<p->index) & Oindex))
					error("index register may not be an index");
				if (p->base == p->index)
					error("duplicate index registers");
				lex();
			}
			if (tok != ')')
				error("syntax");
			lex();
			break;
		default:
			error("syntax in operand");
		}
		if (p < &oprn[NOPRN])
			p++;
		i++;
		if (tok != ',')
			break;
		lex();
	}
	noprn = i;
}

void
dbwrite(int loc, char *where, int len)
{
	print("%8.8ux: %d: ", loc, len);
	while (len--)
		print("%2.2x", *where++ & 0xFF);
	print("\n");
}

void
mk_code(char *name)
{
	format(omatch(name));
	write_instr();
}

void
write_string(Blkptr *bp, char *where, int len)
{
	block_write(bp, where, len);
}

void
write_instr(void)
{
	if (pass_no == 2)
		block_write(*cur_bp, instr, in_offset);
	*cur_loc += in_offset;
	in_offset = 0;
}

int
cur_seg(int type)	/* is this type the same as the current segment? */
{
	return cur_loc == &locs[type - N_TEXT >> 1];
}

int
mag(ulong v, int bytes)
{
	ulong m;

	m = ~((1 << (bytes << (3-1))) - 1);	/* msb in mask */
	v &= m;
	if (v != 0 && (v ^ m) != 0)
		return 0;		/* too big */
	return 1;
}

void	/* make note of relocation requirement */
note_relo(Sym *sp, int type, int bytes, int pcrel)
{
	R *rp;

	if (pass_no != 2)
		return;
	rp = (R *)malloc(sizeof *rp);
	memset(rp, 0, sizeof *rp);
	assert (bytes == 1 || bytes == 2);
	rp->r.r_pcrel = pcrel;
	rp->r.r_length = (bytes == 1 ? 0 : bytes == 2 ? 1 : 3);
	rp->r.r_addr = *cur_loc + in_offset;
	rp->r.r_symbolnum = 0;
	if (sp && sp->type & N_EXT) {
		rp->what = sp;
		rp->r.r_extern = sp->type & N_EXT;
	} else
		rp->r.r_symbolnum = type;
	rp->next = *cur_relo;
	*cur_relo = rp;
}

void
put_const(int value, int bytes)	/* stick into instruction space */
{
	assert(in_offset < sizeof instr);
	switch (bytes) {	/* little endian */
	case 2:
		instr[in_offset++] = value;
		value >>= 8;
	case 1:
		instr[in_offset++] = value;
	}
}

void
rput(int offset, int type, int mode, Sym *sym, int n)
{
	assert (n == 1 || n == 2);
	if (pass_no == 1) {
		in_offset += n;
		return;
	}
	type &= N_TYPE;
	if (mode & (Fdisp | Fimm) &&
	    (type == N_TEXT || type == N_DATA || type == N_BSS || type == N_UNDEF))
		note_relo(sym, type, n, 0);
	put_const(offset, n);
}

void
put(Operand *p, int n)
{
	rput(p->offset, p->type, p->mode, p->sym, n);
}

void
disp8(Operand *p)		/* must be in same segment */
{
	int d, v;
	Sym *sp = p->sym;

	if (pass_no == 1) {
		put_const(0, 1);
		return;
	}
	if (sp) {
		if ((sp->type & N_TYPE) == N_UNDEF)
			error("disp8 undefined");
		if (!cur_seg(sp->type & N_TYPE))
			error("symbol in different segment");
		v = sp->addr;
	} else
		v = p->offset;
	d = v - (*cur_loc+in_offset+1);
	if (d < -128 || d > 127)
		error("too far");
	put_const(d, 1);
}

void
disp_16(Operand *p)
{
	int d, v;
	Sym *sp = p->sym;

	if (pass_no == 1) {
		put_const(0, 2);	/* just to advance location counter */
		return;
	}
	if (sp) {
		if (!(sp->type & N_EXT) && !cur_seg(sp->type & N_TYPE))
			error("symbol in different segment");
		v = sp->addr;
	} else
		v = p->offset;
	d = v - (*cur_loc+in_offset+2);
	if (d < -0x7fff || d > 0x7fff)
		error("disp too large");
	if (sp && sp->type & N_EXT) {
		note_relo(sp, sp->type, 2, 1);
		put_const(0, 2);
	} else
		put_const(d, 2);
}

typedef struct RM RM;
struct RM
{
	int	mask;
	int	rm;
};

RM rm[] = {
	{ 1<<BX|1<<SI,	0 },
	{ 1<<BX|1<<DI,	1 },
	{ 1<<BP|1<<SI,	2 },
	{ 1<<BP|1<<DI,	3 },
	{ 1<<SI,	4 },
	{ 1<<DI,	5 },
	{ 1<<BP,	6 },
	{ 1<<BX,	7 },
	{ 0, -1 },

};

int
put_modrm(int reg, Operand *p)
{
	uchar b0;
	int m;
	RM *rp;

	b0 = (reg << 3) & 070;
	if (!(p->mode & (Fdisp|Fmodrm))) {
		b0 |= p->reg & 7;
		b0 |= 0300;
		put_const(b0, 1);
		return 1;
	}
	if (!(p->mode & Fmodrm)) {
		b0 |= 6;
		put_const(b0, 1);
		put(p, 2);
		return 3;
	}

	/* [disp](base[,index]) */

	if (p->index == 0 && p->base == BP && !(p->mode & Fdisp)) {
		b0 |= 0106;
		put_const(b0, 1);
		put_const(0, 1);
		return 2;
	}

	m = 1<<p->base;
	if (p->index != 0)
		m |= 1<<p->index;
	for (rp = rm; rp->mask > 0; rp++)
		if (m == rp->mask)
			break;
	assert(rp->rm != -1);
	b0 |= rp->rm;
	if (p->mode & Fdisp) {
		b0 |= 0200;
		put_const(b0, 1);
		put(p, 2);
		return 3;
	}
	put_const(b0, 1);
	return 1;
}

int
map_size(char let)
{
	switch (let) {
	case 'b':
		return 1;
	case 'w':
		return 2;
	default:
		assert(0);
	}
	return 0;
}

/*
 * this function fills in the instruction buffer by
 * scanning the fmt string in the intstruction entry
 */

#define Fall_regs	(Freg|Fsreg)

void
format(Itab *ip)	/* format instruction using operands */
{
	Operand *reg = 0, *mod = 0, *imm, *disp = 0;
	char *cp;
	uchar byte;

	/*
	 * this is a little tricky.
	 * deduce where various things are.
	 */

	if (ip->mod > -1) {
		mod = &oprn[ip->mod];
		if (mod->mode & Fdisp)
			disp = mod;
		if (ip->noperands > 1 &&
		    oprn[ip->mod ^ 1].mode & Fall_regs)	/* assume only two operands */
			reg = &oprn[ip->mod ^ 1];
	} else
		disp = &oprn[ip->noperands - 1];	/* always last */
	imm = oprn;					/* always first */

	if (ip->noperands == 1)
		reg = oprn;
	if (ip->noperands == 2 && oprn->mode & Fimm && oprn[1].mode & Freg)
		reg = &oprn[1];
	if (reg == 0)
		if (ip->omask1 & Fxreg)
			reg = oprn;
		else if (ip->omask2 & Fxreg)
			reg = &oprn[1];

	/* now, loop over format string */

	for (cp = ip->fmt; *cp; cp++) {
		switch (*cp) {
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9': case 'A': case 'B':
		case 'C': case 'D': case 'E': case 'F':
			byte  = *cp   <= '9'? *cp - '0': *cp - 'A' + 10;
			byte <<= 4;
			byte |= *++cp <= '9'? *cp - '0': *cp - 'A' + 10;
			put_const(byte, 1);
			break;

		case ' ':
			break;

		case '/':
			assert(mod);
			switch (*++cp) {
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				put_modrm(*cp - '0', mod);
				break;

			case 'r':
				assert(reg);
				put_modrm(reg->reg, mod);
				break;

			default:
				assert(0);
			}
			break;

		case 'i':	/* immediate stuff at the end */
			assert(imm);
			switch (*++cp) {
			case 'b':
				if (imm->offset <0 || imm->offset > 0xff)
					error("immediate too large");
				put(imm, map_size(*cp));
				break;
			case 'w':
				if (imm->offset < -0x7fff || imm->offset > 0xffff)
					error("immediate too large");
				put(imm, map_size(*cp));
				break;
			case 'd':
				put(imm, map_size(*cp));
				break;

			default:
				assert(0);
			}
			break;

		case 'c':	/* call/jmp offsets */
			assert(disp);
			/*
			 * could make backward jmp's smaller by
			 * doing some sort of checks here
			 */
			switch (*++cp) {
			case 'b':
				if (disp->mode & Fabs)
					error("too short for abs");
				disp8(disp);
				break;

			case 'w':
				disp_16(disp);
				break;

			default:
				assert(0);
			}
			break;
		case '+':
			cp++;
			switch (*cp) {
			case 'r':
				assert(reg);
				instr[in_offset-1] += reg->reg & 7;
				break;

			default:
				assert(0);
			}
			break;
		default:
			assert(0);
		}
	}
}


/*
 * pseudo instruction
 */

void
dot_word(char *)
{
	int t, n;
	Sym *sp;

	for(;;) {
		n = expr(&t, &sp);
		rput(n, t, Fdisp, sp, 2);
		write_instr();
		if (tok != ',')
			break;
		lex();
	};
}

void
dot_byte(char *)
{
	int t, n;
	Sym *sp;

	for (;;) {
		n = expr(&t, &sp);
		put_const(n, 1);
		write_instr();
		if (tok != ',')
			break;
		lex();
	}
}

/*
 * by aligning with 0x90 (nop) we can reliabily dis-assemble the text
 * setments
 */

void
dot_align(char *)	/* .align <byte_align> */
{
	int n;

	if (tok != TCONST)
		error("align expects constant");
	if (value < 1 || value > 16 * 1024)
		error("align value out of range");
	n = value;
	if (n == 0)
		return;
	if (pass_no == 2 && cur_loc != &locs[2])
		while (*cur_loc % n) {
			block_write(*cur_bp, "\x90", 1);
			++*cur_loc;
		}
	else
		while (*cur_loc % n)
			++*cur_loc;
	lex();
}

void
dot_lcomm(char *)
{
	Sym *sp;

	if (cur_loc != &locs[2])
		error(".lcomm not in bss");
	if (tok != TNAME)
		error("missing name in .lcomm");
	sp = lookup(name);
	sp->addr = *cur_loc;
	sp->type = N_BSS;
	lex();
	if (tok != ',')
		error("syntax");
	lex();
	if (tok == TNAME) {
		sp = lookup(name);
		if ((sp->type & N_TYPE) == N_UNDEF)
			error(".lcomm size must be defined in first pass");
		*cur_loc += sp->addr;
		return;
	}
	if (tok == TCONST) {
		*cur_loc += value;
		lex();
		return;
	}
	error("syntax");
}

void
dot_set(char *)
{
	Sym *sp0, *sp;

	if (tok != TNAME)
		error("missing name in .set");
	sp = lookup(name);
	lex();
	if (tok != ',')
		error("syntax");
	lex();
	sp->addr = expr(&sp->type, &sp0);
}

void
dot_space(char *)
{
	Sym *sp;
	int len, size;

	SET(size);
	if (tok == TNAME) {
		sp = lookup(name);
		if ((sp->type & N_TYPE) == N_UNDEF)
			error(".space parameter must be defined in first pass");
		size = sp->addr;
	} else if (tok == TCONST)
		size = value;
	else
		error("syntax");
	if (pass_no == 2 && cur_loc != &locs[2]) {
		for (len = size; len > 4; len -= 4)
			block_write(*cur_bp, "\0\0\0\0", 4);
		if (len)
			block_write(*cur_bp, "\0\0\0\0", len);
	}
	*cur_loc += size;
	lex();
}

void
dot_long(char *)
{
	Sym *sp;
	ulong val;

	if (tok == TNAME) {
		sp = lookup(name);
		if ((sp->type & N_TYPE) == N_UNDEF)
			error(".space parameter must be defined in first pass");
		val = sp->addr;
	} else if (tok == TCONST)
		val = value;
	else
		error("syntax");
	if (pass_no == 2 && cur_loc != &locs[2])
		block_write(*cur_bp, &val, sizeof val); /* TODO correct byte order? */
	*cur_loc += sizeof val;
	lex();
}

void
dot_globl(char *)
{
	Sym *sp;

	while (tok == TNAME) {
		sp = lookup(name);
		sp->type |= N_EXT;
		lex();
		if (tok != ',')
			break;
		lex();
	}
}

void
dot_comm(char *)
{
	Sym *sp, *sp2;
	int type;

	if (pass_no == 2) {
		while (tok != '\n')
			lex();
		return;
	}
	if (tok != TNAME)
		error(".comm expected name");
	sp = lookup(name);
	if ((sp->type & N_TYPE) != N_UNDEF)
		error("%s already defined", name);
	lex();
	if (tok != ',')
		error(".comm expected ','");
	lex();
	sp->addr = expr(&type, &sp2);
	sp->type |= N_EXT;
}

void
dot_text(char *)
{
	cur_bp = &tbp;
	cur_relo = &trelo;
	cur_loc = locs;
}

void
dot_data(char *)
{
	cur_bp = &dbp;
	cur_relo = &drelo;
	cur_loc = &locs[1];
}

void
dot_bss(char *)
{
	cur_bp = 0;
	cur_loc = &locs[2];
}

void
not_yet(char *)
{
	while (tok != '\n')
		lex();
}


void
lusyms(void)		/* list the user's symbols */
{
	Sym *sp;

	for (sp = last_sys_sym->next; sp; sp = sp->next)
		print("%08p(%04ld): %08x %08p %s\n", sp, sp - symtab,
			sp->type, sp->addr, sp->name);
}

/* stuff to save temporary code and relocation bits */

Blkptr *
block_open(void)	/* get new open block */
{
	Blkptr *bp;

	for (bp = blkptrs; bp->first; bp++)
		;
	if (bp == &blkptrs[NBLKPTRS])
		abort();
	bp->last = bp->first = malloc(sizeof *bp->first);
	bp->last->len = 0;
	bp->last->next = 0;
	return bp;
}

void
block_write(Blkptr *bp, void *what, int len)	/* write into block */
{
	Block *p;

	p = bp->last;
	if (len <= BLKLEN - p->len) {
		memcpy(&p->block[p->len], what, len);
		p->len += len;
		return;
	}
	bp->last->next = malloc(sizeof *bp->last->next);
	bp->last = bp->last->next;
	p = bp->last;
	p->len = len;
	p->next = 0;
	memcpy(p->block, what, len);
}

int
block_copyout(Biobuf *bp, Blkptr *blkp)	/* put block into file */
{
	int size;
	Block *p;

	size = 0;
	for (p = blkp->first; p; p = p->next) {
		size += p->len;
		Bwrite(bp, p->block, p->len);
	}
	return size;
}

void
dumpb(char *p, int len)
{
	while (len--)
		print("%02x%c", *p++ & 0xff,
			((uintptr)p & 0xF) == 0? '\n': ' ');
	print("\n");
}

void
pr_oper(void)
{
	int n;
	Operand *p;

	for (n = noprn, p = oprn; n--; p++)
		print("operand %ld: mode %#x, type %#x, sym %s, offset %#x, "
			"reg %#o, base %#o, index %#o, scale %d\n",
			p - oprn + 1, p->mode, p->type,
			p->sym? p->sym->name: "nil", p->offset,
			p->reg, p->base, p->index, p->scale);
}

/*
 * instructions that don't fit well into the search algorithm
 */

void
do_imul(char *name)
{
	int size, m0, m1;

	if (strcmp(name, "imul") == 0)
		size = 'w';
	else
		size = name[strlen(name)-1];
	parseo();
	switch (noprn) {
	case 1:
		switch (size) {
		case 'b':
			put_const(0xF6, 1);
			put_modrm(5, oprn);
			break;
		case 'w':
			put_const(0xF7, 1);
			put_modrm(5, oprn);
			break;
		default:
			assert(0);
		}
		break;
	case 2:
		m0 = oprn[0].mode;
		m1 = oprn[1].mode;
		if (oprn->mode == Fimm &&
		    (oprn[1].mode == Fareg || oprn[1].mode == Fxreg)) {
			switch (size) {
			case 'w':
				put_const(0x69, 1);
				put_modrm(oprn[1].reg, &oprn[1]);
				put(oprn, size == 'l'? 4: 2);
				break;
			default:
				goto err;
			}
		} else if ((m1 == Fareg || m1 == Fxreg) && (m0 & Fmod)) {
			switch (size) {
			case 'w':
				put_const(0x0F, 1);
				put_const(0xAF, 1);
				put_modrm(oprn[1].reg, oprn);
				break;
			default:
				goto err;
			}
		} else
			goto err;
		break;
	case 3:
		if (oprn->mode != Fimm ||
		    (oprn[1].mode & Fmod) == 0 || (oprn[1].mode & ~Fmod) ||
		    oprn[2].mode != Fareg && oprn[2].mode != Fxreg ||
		    size != 'w' && size != 'l')
			goto err;
		put_const(0x69, 1);
		put_modrm(oprn[2].reg, &oprn[1]);
		put(oprn, size == 'l' ? 4 : 2);
		break;
	default:
err:
		pr_oper();
		error("illegal operand for imul");
	}
	write_instr();
}

void
do_pop(char *)
{
	int size;

	size = 'w';
	parseo();
	if (noprn != 1)
		error("illegal operands for pop: wrong number");
	if (oprn->mode == Fxreg || oprn->mode == Fareg)
		put_const(0x58 + (oprn->reg & 7), 1);
	else if ((oprn->mode & Fmod) && (oprn->mode & ~Fmod) == 0) {
		if (size == 'b')
			error("illegal operand for pop: not byte");
		put_const(0x8f, 1);
		put_modrm(0, oprn);
	} else if (oprn->mode == Fsreg) {
		switch (oprn->reg) {
		case 030:		/* ES */
			put_const(7, 1);
			break;
		case 031:		/* CS */
			error("illegal operand for pop: segment may not be cs");
		case 032:		/* SS */
			put_const(0x17, 1);
			break;
		case 033:		/* DS */
			put_const(0x1F, 1);
			break;
		case 034:		/* FS */
			put_const(0xF, 1);
			put_const(0xA1, 1);
			break;
		case 035:		/* GS */
			put_const(0xf, 1);
			put_const(0xA9, 1);
			break;
		default:
			error("illegal operand for pop: bad segment");
		}
	} else
		error("illegal operand for pop");
	write_instr();
}

void
do_push(char *name)
{
	int size;

	size = name[strlen(name)-1];
	if (size != 'b')
		size = 'w';
	parseo();
	if (noprn != 1)
		error("illegal operands for push: wrong number");
	if (oprn->mode == Fxreg || oprn->mode == Fareg) {
		put_const(0x50 + (oprn->reg & 7), 1);
	} else if ((oprn->mode & Fmod) && (oprn->mode & ~Fmod) == 0) {
		if (size == 'b')
			error("illegal operand for push: not byte");
		put_const(0xff, 1);
		put_modrm(6, oprn);
	} else if (oprn->mode == Fsreg) {
		if (size != 'l' && size != 'w')
			error("illegal operand for push: size %c: not long",
				size);
		switch (oprn->reg) {
		case 030:		/* ES */
			put_const(6, 1);
			break;
		case 031:		/* CS */
			put_const(0xe, 1);
			break;
		case 032:		/* SS */
			put_const(0x16, 1);
			break;
		case 033:		/* DS */
			put_const(0x1e, 1);
			break;
		case 034:		/* FS */
			put_const(0xF, 1);
			put_const(0xA0, 1);
			break;
		case 035:		/* GS */
			put_const(0xf, 1);
			put_const(0xA8, 1);
			break;
		default:
			error("illegal operand for push: bad segment");
		}
	} else if (oprn->mode == Fimm)
		switch (size) {
		case 'b':
			put_const(0x6a, 1);
			put(oprn, 1);
			break;
		case 'w':
			put_const(0x68, 1);
			put(oprn, size == 'w'? 2: 4);
			break;
		}

	else
		error("illegal operand for push");
	write_instr();
}

void
do_rot(char *name)
{
	int op, s;

	s = name[strlen(name)-1];
	if (s != 'b')
		s = 'w';
	parseo();
	if (noprn != 2)
		goto err;
	if (strncmp(name, "rcl", 3) == 0)
		op = 2;
	else if (strncmp(name, "rcr", 3) == 0)
		op = 3;
	else if (strncmp(name, "rol", 3) == 0)
		op = 0;
	else if (strncmp(name, "ror", 3) == 0)
		op = 1;
	else
		goto err;
	if ((oprn[1].mode & Fmod) == 0 || oprn[1].mode & ~Fmod)
		goto err;
	if (oprn->mode & Fimm) {
		if (oprn->offset == 1) {
			switch (s) {
			case 'b':
				put_const(0xd0, 1);
				put_modrm(op, &oprn[1]);
				break;
			case 'w':
				put_const(0xd1, 1);
				put_modrm(op, &oprn[1]);
				break;
			}
		} else if (oprn->offset < 128) {
			switch (s) {
			case 'b':
				put_const(0xc0, 1);
				put_modrm(op, &oprn[1]);
				put(oprn, 1);
				break;
			case 'w':
				put_const(0xc1, 1);
				put_modrm(op, &oprn[1]);
				put(oprn, 1);
				break;
			}
		} else
			goto err;
	} else if (oprn->mode & Fxreg && oprn->reg == 021)	/* CL */
		switch (s) {
		case 'b':
			put_const(0xd2, 1);
			put_modrm(op, &oprn[1]);
			break;
		case 'w':
			put_const(0xd3, 1);
			put_modrm(op, &oprn[1]);
			break;
		}
	else
		goto err;
	write_instr();
	return;
err:
	error("illegal operand for rotate");
}

void
do_shift(char *name)
{
	int op, s;

	s = name[strlen(name)-1];
	if (s != 'b')
		s = 'w';
	parseo();
	if (noprn != 2)
		goto err;
	if (strncmp(name, "sal", 3) == 0)
		op = 4;
	else if (strncmp(name, "sar", 3) == 0)
		op = 7;
	else if (strncmp(name, "shl", 3) == 0)
		op = 4;
	else if (strncmp(name, "shr", 3) == 0)
		op = 5;
	else
		goto err;
	if ((oprn[1].mode & Fmod) == 0 || oprn[1].mode & ~Fmod)
		goto err;
	if (oprn->mode & Fimm) {
		if (oprn->offset == 1) {
			switch (s) {
			case 'b':
				put_const(0xd0, 1);
				put_modrm(op, &oprn[1]);
				break;
			case 'w':
				put_const(0xd1, 1);
				put_modrm(op, &oprn[1]);
				break;
			}
		} else if (oprn->offset < 128) {
			switch (s) {
			case 'b':
				put_const(0xc0, 1);
				put_modrm(op, &oprn[1]);
				put(oprn, 1);
				break;
			case 'w':
				put_const(0xc1, 1);
				put_modrm(op, &oprn[1]);
				put(oprn, 1);
				break;
			}
		} else
			goto err;
	} else if (oprn->mode & Fxreg && oprn->reg == 021)	/* CL */
		switch (s) {
		case 'b':
			put_const(0xd2, 1);
			put_modrm(op, &oprn[1]);
			break;
		case 'w':
			put_const(0xd3, 1);
			put_modrm(op, &oprn[1]);
			break;
		}
	else
		goto err;
	write_instr();
	return;
err:
	error("illegal operand for shift");
}

void
do_ljmp(char *)
{
	int sel;
	Sym *sp;

	if (tok != TCONST)
		goto err;
	sel = value;
	lex();
	if (tok != ':')
		goto err;
	lex();
	if (tok != TNAME)
		goto err;
	sp = lookup(name);
	put_const(0xEA, 1);
	rput(sp->addr, sp->type, Fdisp, sp, 2);
	put_const(sel, 2);
	while (tok != '\n')
		lex();
	write_instr();
	return;
err:
	error("syntax");
}

void
do_lcall(char *)
{
	int sel;
	Sym *sp;

	if (tok != TCONST)
		goto err;
	sel = value;
	lex();
	if (tok != ':')
		goto err;
	lex();
	if (tok != TNAME)
		goto err;
	sp = lookup(name);
	put_const(0x9A, 1);
	rput(sp->addr, sp->type, Fdisp, sp, 2);
	put_const(sel, 2);
	while (tok != '\n')
		lex();
	write_instr();
	return;
err:
	error("syntax");
}
