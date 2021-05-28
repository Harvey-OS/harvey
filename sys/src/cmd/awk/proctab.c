#include <stdio.h>
#include "awk.h"
#include "y.tab.h"

static char *printname[93] = {
	(char *) "FIRSTTOKEN",	/* 57346 */
	(char *) "PROGRAM",	/* 57347 */
	(char *) "PASTAT",	/* 57348 */
	(char *) "PASTAT2",	/* 57349 */
	(char *) "XBEGIN",	/* 57350 */
	(char *) "XEND",	/* 57351 */
	(char *) "NL",	/* 57352 */
	(char *) "ARRAY",	/* 57353 */
	(char *) "MATCH",	/* 57354 */
	(char *) "NOTMATCH",	/* 57355 */
	(char *) "MATCHOP",	/* 57356 */
	(char *) "FINAL",	/* 57357 */
	(char *) "DOT",	/* 57358 */
	(char *) "ALL",	/* 57359 */
	(char *) "CCL",	/* 57360 */
	(char *) "NCCL",	/* 57361 */
	(char *) "CHAR",	/* 57362 */
	(char *) "OR",	/* 57363 */
	(char *) "STAR",	/* 57364 */
	(char *) "QUEST",	/* 57365 */
	(char *) "PLUS",	/* 57366 */
	(char *) "EMPTYRE",	/* 57367 */
	(char *) "AND",	/* 57368 */
	(char *) "BOR",	/* 57369 */
	(char *) "APPEND",	/* 57370 */
	(char *) "EQ",	/* 57371 */
	(char *) "GE",	/* 57372 */
	(char *) "GT",	/* 57373 */
	(char *) "LE",	/* 57374 */
	(char *) "LT",	/* 57375 */
	(char *) "NE",	/* 57376 */
	(char *) "IN",	/* 57377 */
	(char *) "ARG",	/* 57378 */
	(char *) "BLTIN",	/* 57379 */
	(char *) "BREAK",	/* 57380 */
	(char *) "CLOSE",	/* 57381 */
	(char *) "CONTINUE",	/* 57382 */
	(char *) "DELETE",	/* 57383 */
	(char *) "DO",	/* 57384 */
	(char *) "EXIT",	/* 57385 */
	(char *) "FOR",	/* 57386 */
	(char *) "FUNC",	/* 57387 */
	(char *) "SUB",	/* 57388 */
	(char *) "GSUB",	/* 57389 */
	(char *) "IF",	/* 57390 */
	(char *) "INDEX",	/* 57391 */
	(char *) "LSUBSTR",	/* 57392 */
	(char *) "MATCHFCN",	/* 57393 */
	(char *) "NEXT",	/* 57394 */
	(char *) "NEXTFILE",	/* 57395 */
	(char *) "ADD",	/* 57396 */
	(char *) "MINUS",	/* 57397 */
	(char *) "MULT",	/* 57398 */
	(char *) "DIVIDE",	/* 57399 */
	(char *) "MOD",	/* 57400 */
	(char *) "ASSIGN",	/* 57401 */
	(char *) "ASGNOP",	/* 57402 */
	(char *) "ADDEQ",	/* 57403 */
	(char *) "SUBEQ",	/* 57404 */
	(char *) "MULTEQ",	/* 57405 */
	(char *) "DIVEQ",	/* 57406 */
	(char *) "MODEQ",	/* 57407 */
	(char *) "POWEQ",	/* 57408 */
	(char *) "PRINT",	/* 57409 */
	(char *) "PRINTF",	/* 57410 */
	(char *) "SPRINTF",	/* 57411 */
	(char *) "ELSE",	/* 57412 */
	(char *) "INTEST",	/* 57413 */
	(char *) "CONDEXPR",	/* 57414 */
	(char *) "POSTINCR",	/* 57415 */
	(char *) "PREINCR",	/* 57416 */
	(char *) "POSTDECR",	/* 57417 */
	(char *) "PREDECR",	/* 57418 */
	(char *) "VAR",	/* 57419 */
	(char *) "IVAR",	/* 57420 */
	(char *) "VARNF",	/* 57421 */
	(char *) "CALL",	/* 57422 */
	(char *) "NUMBER",	/* 57423 */
	(char *) "STRING",	/* 57424 */
	(char *) "REGEXPR",	/* 57425 */
	(char *) "GETLINE",	/* 57426 */
	(char *) "RETURN",	/* 57427 */
	(char *) "SPLIT",	/* 57428 */
	(char *) "SUBSTR",	/* 57429 */
	(char *) "WHILE",	/* 57430 */
	(char *) "CAT",	/* 57431 */
	(char *) "NOT",	/* 57432 */
	(char *) "UMINUS",	/* 57433 */
	(char *) "POWER",	/* 57434 */
	(char *) "DECR",	/* 57435 */
	(char *) "INCR",	/* 57436 */
	(char *) "INDIRECT",	/* 57437 */
	(char *) "LASTTOKEN",	/* 57438 */
};


Cell *(*proctab[93])(Node **, int) = {
	nullproc,	/* FIRSTTOKEN */
	program,	/* PROGRAM */
	pastat,	/* PASTAT */
	dopa2,	/* PASTAT2 */
	nullproc,	/* XBEGIN */
	nullproc,	/* XEND */
	nullproc,	/* NL */
	array,	/* ARRAY */
	matchop,	/* MATCH */
	matchop,	/* NOTMATCH */
	nullproc,	/* MATCHOP */
	nullproc,	/* FINAL */
	nullproc,	/* DOT */
	nullproc,	/* ALL */
	nullproc,	/* CCL */
	nullproc,	/* NCCL */
	nullproc,	/* CHAR */
	nullproc,	/* OR */
	nullproc,	/* STAR */
	nullproc,	/* QUEST */
	nullproc,	/* PLUS */
	nullproc,	/* EMPTYRE */
	boolop,	/* AND */
	boolop,	/* BOR */
	nullproc,	/* APPEND */
	relop,	/* EQ */
	relop,	/* GE */
	relop,	/* GT */
	relop,	/* LE */
	relop,	/* LT */
	relop,	/* NE */
	instat,	/* IN */
	arg,	/* ARG */
	bltin,	/* BLTIN */
	jump,	/* BREAK */
	closefile,	/* CLOSE */
	jump,	/* CONTINUE */
	awkdelete,	/* DELETE */
	dostat,	/* DO */
	jump,	/* EXIT */
	forstat,	/* FOR */
	nullproc,	/* FUNC */
	sub,	/* SUB */
	gsub,	/* GSUB */
	ifstat,	/* IF */
	sindex,	/* INDEX */
	nullproc,	/* LSUBSTR */
	matchop,	/* MATCHFCN */
	jump,	/* NEXT */
	jump,	/* NEXTFILE */
	arith,	/* ADD */
	arith,	/* MINUS */
	arith,	/* MULT */
	arith,	/* DIVIDE */
	arith,	/* MOD */
	assign,	/* ASSIGN */
	nullproc,	/* ASGNOP */
	assign,	/* ADDEQ */
	assign,	/* SUBEQ */
	assign,	/* MULTEQ */
	assign,	/* DIVEQ */
	assign,	/* MODEQ */
	assign,	/* POWEQ */
	printstat,	/* PRINT */
	awkprintf,	/* PRINTF */
	awksprintf,	/* SPRINTF */
	nullproc,	/* ELSE */
	intest,	/* INTEST */
	condexpr,	/* CONDEXPR */
	incrdecr,	/* POSTINCR */
	incrdecr,	/* PREINCR */
	incrdecr,	/* POSTDECR */
	incrdecr,	/* PREDECR */
	nullproc,	/* VAR */
	nullproc,	/* IVAR */
	getnf,	/* VARNF */
	call,	/* CALL */
	nullproc,	/* NUMBER */
	nullproc,	/* STRING */
	nullproc,	/* REGEXPR */
	awkgetline,	/* GETLINE */
	jump,	/* RETURN */
	split,	/* SPLIT */
	substr,	/* SUBSTR */
	whilestat,	/* WHILE */
	cat,	/* CAT */
	boolop,	/* NOT */
	arith,	/* UMINUS */
	arith,	/* POWER */
	nullproc,	/* DECR */
	nullproc,	/* INCR */
	indirect,	/* INDIRECT */
	nullproc,	/* LASTTOKEN */
};

char *tokname(int n)
{
	static char buf[100];

	if (n < FIRSTTOKEN || n > LASTTOKEN) {
		sprintf(buf, "token %d", n);
		return buf;
	}
	return printname[n-FIRSTTOKEN];
}
