#include <stdio.h>
#include "awk.h"
#include "y.tab.h"

static uchar *printname[92] = {
	(uchar *) "FIRSTTOKEN",	/* 57346 */
	(uchar *) "PROGRAM",	/* 57347 */
	(uchar *) "PASTAT",	/* 57348 */
	(uchar *) "PASTAT2",	/* 57349 */
	(uchar *) "XBEGIN",	/* 57350 */
	(uchar *) "XEND",	/* 57351 */
	(uchar *) "NL",	/* 57352 */
	(uchar *) "ARRAY",	/* 57353 */
	(uchar *) "MATCH",	/* 57354 */
	(uchar *) "NOTMATCH",	/* 57355 */
	(uchar *) "MATCHOP",	/* 57356 */
	(uchar *) "FINAL",	/* 57357 */
	(uchar *) "DOT",	/* 57358 */
	(uchar *) "ALL",	/* 57359 */
	(uchar *) "CCL",	/* 57360 */
	(uchar *) "NCCL",	/* 57361 */
	(uchar *) "CHAR",	/* 57362 */
	(uchar *) "OR",	/* 57363 */
	(uchar *) "STAR",	/* 57364 */
	(uchar *) "QUEST",	/* 57365 */
	(uchar *) "PLUS",	/* 57366 */
	(uchar *) "AND",	/* 57367 */
	(uchar *) "BOR",	/* 57368 */
	(uchar *) "APPEND",	/* 57369 */
	(uchar *) "EQ",	/* 57370 */
	(uchar *) "GE",	/* 57371 */
	(uchar *) "GT",	/* 57372 */
	(uchar *) "LE",	/* 57373 */
	(uchar *) "LT",	/* 57374 */
	(uchar *) "NE",	/* 57375 */
	(uchar *) "IN",	/* 57376 */
	(uchar *) "ARG",	/* 57377 */
	(uchar *) "BLTIN",	/* 57378 */
	(uchar *) "BREAK",	/* 57379 */
	(uchar *) "CLOSE",	/* 57380 */
	(uchar *) "CONTINUE",	/* 57381 */
	(uchar *) "DELETE",	/* 57382 */
	(uchar *) "DO",	/* 57383 */
	(uchar *) "EXIT",	/* 57384 */
	(uchar *) "FOR",	/* 57385 */
	(uchar *) "FUNC",	/* 57386 */
	(uchar *) "SUB",	/* 57387 */
	(uchar *) "GSUB",	/* 57388 */
	(uchar *) "IF",	/* 57389 */
	(uchar *) "INDEX",	/* 57390 */
	(uchar *) "LSUBSTR",	/* 57391 */
	(uchar *) "MATCHFCN",	/* 57392 */
	(uchar *) "NEXT",	/* 57393 */
	(uchar *) "ADD",	/* 57394 */
	(uchar *) "MINUS",	/* 57395 */
	(uchar *) "MULT",	/* 57396 */
	(uchar *) "DIVIDE",	/* 57397 */
	(uchar *) "MOD",	/* 57398 */
	(uchar *) "ASSIGN",	/* 57399 */
	(uchar *) "ASGNOP",	/* 57400 */
	(uchar *) "ADDEQ",	/* 57401 */
	(uchar *) "SUBEQ",	/* 57402 */
	(uchar *) "MULTEQ",	/* 57403 */
	(uchar *) "DIVEQ",	/* 57404 */
	(uchar *) "MODEQ",	/* 57405 */
	(uchar *) "POWEQ",	/* 57406 */
	(uchar *) "PRINT",	/* 57407 */
	(uchar *) "PRINTF",	/* 57408 */
	(uchar *) "SPRINTF",	/* 57409 */
	(uchar *) "ELSE",	/* 57410 */
	(uchar *) "INTEST",	/* 57411 */
	(uchar *) "CONDEXPR",	/* 57412 */
	(uchar *) "POSTINCR",	/* 57413 */
	(uchar *) "PREINCR",	/* 57414 */
	(uchar *) "POSTDECR",	/* 57415 */
	(uchar *) "PREDECR",	/* 57416 */
	(uchar *) "VAR",	/* 57417 */
	(uchar *) "IVAR",	/* 57418 */
	(uchar *) "VARNF",	/* 57419 */
	(uchar *) "CALL",	/* 57420 */
	(uchar *) "NUMBER",	/* 57421 */
	(uchar *) "STRING",	/* 57422 */
	(uchar *) "FIELD",	/* 57423 */
	(uchar *) "REGEXPR",	/* 57424 */
	(uchar *) "GETLINE",	/* 57425 */
	(uchar *) "RETURN",	/* 57426 */
	(uchar *) "SPLIT",	/* 57427 */
	(uchar *) "SUBSTR",	/* 57428 */
	(uchar *) "WHILE",	/* 57429 */
	(uchar *) "CAT",	/* 57430 */
	(uchar *) "NOT",	/* 57431 */
	(uchar *) "UMINUS",	/* 57432 */
	(uchar *) "POWER",	/* 57433 */
	(uchar *) "DECR",	/* 57434 */
	(uchar *) "INCR",	/* 57435 */
	(uchar *) "INDIRECT",	/* 57436 */
	(uchar *) "LASTTOKEN",	/* 57437 */
};


Cell *(*proctab[92])(Node **, int) = {
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
	delete,	/* DELETE */
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
	aprintf,	/* PRINTF */
	asprintf,	/* SPRINTF */
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
	nullproc,	/* FIELD */
	nullproc,	/* REGEXPR */
	getline,	/* GETLINE */
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

uchar *tokname(int n)
{
	static uchar buf[100];

	if (n < FIRSTTOKEN || n > LASTTOKEN) {
		sprintf(buf, "token %d", n);
		return buf;
	}
	return printname[n-FIRSTTOKEN];
}
