/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdio.h>
#include "awk.h"
#include "y.tab.h"

static int8_t *printname[92] = {
	(int8_t *) "FIRSTTOKEN",	/* 57346 */
	(int8_t *) "PROGRAM",	/* 57347 */
	(int8_t *) "PASTAT",	/* 57348 */
	(int8_t *) "PASTAT2",	/* 57349 */
	(int8_t *) "XBEGIN",	/* 57350 */
	(int8_t *) "XEND",	/* 57351 */
	(int8_t *) "NL",	/* 57352 */
	(int8_t *) "ARRAY",	/* 57353 */
	(int8_t *) "MATCH",	/* 57354 */
	(int8_t *) "NOTMATCH",	/* 57355 */
	(int8_t *) "MATCHOP",	/* 57356 */
	(int8_t *) "FINAL",	/* 57357 */
	(int8_t *) "DOT",	/* 57358 */
	(int8_t *) "ALL",	/* 57359 */
	(int8_t *) "CCL",	/* 57360 */
	(int8_t *) "NCCL",	/* 57361 */
	(int8_t *) "CHAR",	/* 57362 */
	(int8_t *) "OR",	/* 57363 */
	(int8_t *) "STAR",	/* 57364 */
	(int8_t *) "QUEST",	/* 57365 */
	(int8_t *) "PLUS",	/* 57366 */
	(int8_t *) "AND",	/* 57367 */
	(int8_t *) "BOR",	/* 57368 */
	(int8_t *) "APPEND",	/* 57369 */
	(int8_t *) "EQ",	/* 57370 */
	(int8_t *) "GE",	/* 57371 */
	(int8_t *) "GT",	/* 57372 */
	(int8_t *) "LE",	/* 57373 */
	(int8_t *) "LT",	/* 57374 */
	(int8_t *) "NE",	/* 57375 */
	(int8_t *) "IN",	/* 57376 */
	(int8_t *) "ARG",	/* 57377 */
	(int8_t *) "BLTIN",	/* 57378 */
	(int8_t *) "BREAK",	/* 57379 */
	(int8_t *) "CLOSE",	/* 57380 */
	(int8_t *) "CONTINUE",	/* 57381 */
	(int8_t *) "DELETE",	/* 57382 */
	(int8_t *) "DO",	/* 57383 */
	(int8_t *) "EXIT",	/* 57384 */
	(int8_t *) "FOR",	/* 57385 */
	(int8_t *) "FUNC",	/* 57386 */
	(int8_t *) "SUB",	/* 57387 */
	(int8_t *) "GSUB",	/* 57388 */
	(int8_t *) "IF",	/* 57389 */
	(int8_t *) "INDEX",	/* 57390 */
	(int8_t *) "LSUBSTR",	/* 57391 */
	(int8_t *) "MATCHFCN",	/* 57392 */
	(int8_t *) "NEXT",	/* 57393 */
	(int8_t *) "NEXTFILE",	/* 57394 */
	(int8_t *) "ADD",	/* 57395 */
	(int8_t *) "MINUS",	/* 57396 */
	(int8_t *) "MULT",	/* 57397 */
	(int8_t *) "DIVIDE",	/* 57398 */
	(int8_t *) "MOD",	/* 57399 */
	(int8_t *) "ASSIGN",	/* 57400 */
	(int8_t *) "ASGNOP",	/* 57401 */
	(int8_t *) "ADDEQ",	/* 57402 */
	(int8_t *) "SUBEQ",	/* 57403 */
	(int8_t *) "MULTEQ",	/* 57404 */
	(int8_t *) "DIVEQ",	/* 57405 */
	(int8_t *) "MODEQ",	/* 57406 */
	(int8_t *) "POWEQ",	/* 57407 */
	(int8_t *) "PRINT",	/* 57408 */
	(int8_t *) "PRINTF",	/* 57409 */
	(int8_t *) "SPRINTF",	/* 57410 */
	(int8_t *) "ELSE",	/* 57411 */
	(int8_t *) "INTEST",	/* 57412 */
	(int8_t *) "CONDEXPR",	/* 57413 */
	(int8_t *) "POSTINCR",	/* 57414 */
	(int8_t *) "PREINCR",	/* 57415 */
	(int8_t *) "POSTDECR",	/* 57416 */
	(int8_t *) "PREDECR",	/* 57417 */
	(int8_t *) "VAR",	/* 57418 */
	(int8_t *) "IVAR",	/* 57419 */
	(int8_t *) "VARNF",	/* 57420 */
	(int8_t *) "CALL",	/* 57421 */
	(int8_t *) "NUMBER",	/* 57422 */
	(int8_t *) "STRING",	/* 57423 */
	(int8_t *) "REGEXPR",	/* 57424 */
	(int8_t *) "GETLINE",	/* 57425 */
	(int8_t *) "RETURN",	/* 57426 */
	(int8_t *) "SPLIT",	/* 57427 */
	(int8_t *) "SUBSTR",	/* 57428 */
	(int8_t *) "WHILE",	/* 57429 */
	(int8_t *) "CAT",	/* 57430 */
	(int8_t *) "NOT",	/* 57431 */
	(int8_t *) "UMINUS",	/* 57432 */
	(int8_t *) "POWER",	/* 57433 */
	(int8_t *) "DECR",	/* 57434 */
	(int8_t *) "INCR",	/* 57435 */
	(int8_t *) "INDIRECT",	/* 57436 */
	(int8_t *) "LASTTOKEN",	/* 57437 */
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

int8_t *tokname(int n)
{
	static int8_t buf[100];

	if (n < FIRSTTOKEN || n > LASTTOKEN) {
		sprintf(buf, "token %d", n);
		return buf;
	}
	return printname[n-FIRSTTOKEN];
}
