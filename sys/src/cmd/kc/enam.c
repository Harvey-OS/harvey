/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

char* anames[] = {
    "XXX",    "ADD",      "ADDCC",   "ADDX",   "ADDXCC",   "AND",   "ANDCC",
    "ANDN",   "ANDNCC",   "BA",      "BCC",    "BCS",      "BE",    "BG",
    "BGE",    "BGU",      "BL",      "BLE",    "BLEU",     "BN",    "BNE",
    "BNEG",   "BPOS",     "BVC",     "BVS",    "CB0",      "CB01",  "CB012",
    "CB013",  "CB02",     "CB023",   "CB03",   "CB1",      "CB12",  "CB123",
    "CB13",   "CB2",      "CB23",    "CB3",    "CBA",      "CBN",   "CMP",
    "CPOP1",  "CPOP2",    "DATA",    "DIV",    "DIVL",     "FABSD", "FABSF",
    "FABSX",  "FADDD",    "FADDF",   "FADDX",  "FBA",      "FBE",   "FBG",
    "FBGE",   "FBL",      "FBLE",    "FBLG",   "FBN",      "FBNE",  "FBO",
    "FBU",    "FBUE",     "FBUG",    "FBUGE",  "FBUL",     "FBULE", "FCMPD",
    "FCMPED", "FCMPEF",   "FCMPEX",  "FCMPF",  "FCMPX",    "FDIVD", "FDIVF",
    "FDIVX",  "FMOVD",    "FMOVDF",  "FMOVDW", "FMOVDX",   "FMOVF", "FMOVFD",
    "FMOVFW", "FMOVFX",   "FMOVWD",  "FMOVWF", "FMOVWX",   "FMOVX", "FMOVXD",
    "FMOVXF", "FMOVXW",   "FMULD",   "FMULF",  "FMULX",    "FNEGD", "FNEGF",
    "FNEGX",  "FSQRTD",   "FSQRTF",  "FSQRTX", "FSUBD",    "FSUBF", "FSUBX",
    "GLOBL",  "GOK",      "HISTORY", "IFLUSH", "JMPL",     "JMP",   "MOD",
    "MODL",   "MOVB",     "MOVBU",   "MOVD",   "MOVH",     "MOVHU", "MOVW",
    "MUL",    "MULSCC",   "NAME",    "NOP",    "OR",       "ORCC",  "ORN",
    "ORNCC",  "RESTORE",  "RETT",    "RETURN", "SAVE",     "SLL",   "SRA",
    "SRL",    "SUB",      "SUBCC",   "SUBX",   "SUBXCC",   "SWAP",  "TA",
    "TADDCC", "TADDCCTV", "TAS",     "TCC",    "TCS",      "TE",    "TEXT",
    "TG",     "TGE",      "TGU",     "TL",     "TLE",      "TLEU",  "TN",
    "TNE",    "TNEG",     "TPOS",    "TSUBCC", "TSUBCCTV", "TVC",   "TVS",
    "UNIMP",  "WORD",     "XNOR",    "XNORCC", "XOR",      "XORCC", "END",
    "DYNT",   "INIT",     "SIGNAME", "LAST"};
