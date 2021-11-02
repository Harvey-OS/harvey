/****************************************************************
Copyright (C) Lucent Technologies 1997
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name Lucent Technologies or any of
its entities not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/

/*
 * this program makes the table to link function names
 * and type indices that is used by execute() in run.c.
 * it finds the indices in y.tab.h, produced by yacc.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "awk.h"
#include "y.tab.h"

struct xx
{	int token;
	char *name;
	char *pname;
} proc[] = {
	{ PROGRAM, "program", nil },
	{ BOR, "boolop", " || " },
	{ AND, "boolop", " && " },
	{ NOT, "boolop", " !" },
	{ NE, "relop", " != " },
	{ EQ, "relop", " == " },
	{ LE, "relop", " <= " },
	{ LT, "relop", " < " },
	{ GE, "relop", " >= " },
	{ GT, "relop", " > " },
	{ ARRAY, "array", nil },
	{ INDIRECT, "indirect", "$(" },
	{ SUBSTR, "substr", "substr" },
	{ SUB, "sub", "sub" },
	{ GSUB, "gsub", "gsub" },
	{ INDEX, "sindex", "sindex" },
	{ SPRINTF, "awksprintf", "sprintf" },
	{ ADD, "arith", " + " },
	{ MINUS, "arith", " - " },
	{ MULT, "arith", " * " },
	{ DIVIDE, "arith", " / " },
	{ MOD, "arith", " % " },
	{ UMINUS, "arith", " -" },
	{ POWER, "arith", " **" },
	{ PREINCR, "incrdecr", "++" },
	{ POSTINCR, "incrdecr", "++" },
	{ PREDECR, "incrdecr", "--" },
	{ POSTDECR, "incrdecr", "--" },
	{ CAT, "cat", " " },
	{ PASTAT, "pastat", nil },
	{ PASTAT2, "dopa2", nil },
	{ MATCH, "matchop", " ~ " },
	{ NOTMATCH, "matchop", " !~ " },
	{ MATCHFCN, "matchop", "matchop" },
	{ INTEST, "intest", "intest" },
	{ PRINTF, "awkprintf", "printf" },
	{ PRINT, "printstat", "print" },
	{ CLOSE, "closefile", "closefile" },
	{ DELETE, "awkdelete", "awkdelete" },
	{ SPLIT, "split", "split" },
	{ ASSIGN, "assign", " = " },
	{ ADDEQ, "assign", " += " },
	{ SUBEQ, "assign", " -= " },
	{ MULTEQ, "assign", " *= " },
	{ DIVEQ, "assign", " /= " },
	{ MODEQ, "assign", " %= " },
	{ POWEQ, "assign", " ^= " },
	{ CONDEXPR, "condexpr", " ?: " },
	{ IF, "ifstat", "if(" },
	{ WHILE, "whilestat", "while(" },
	{ FOR, "forstat", "for(" },
	{ DO, "dostat", "do" },
	{ IN, "instat", "instat" },
	{ NEXT, "jump", "next" },
	{ NEXTFILE, "jump", "nextfile" },
	{ EXIT, "jump", "exit" },
	{ BREAK, "jump", "break" },
	{ CONTINUE, "jump", "continue" },
	{ RETURN, "jump", "ret" },
	{ BLTIN, "bltin", "bltin" },
	{ CALL, "call", "call" },
	{ ARG, "arg", "arg" },
	{ VARNF, "getnf", "NF" },
	{ GETLINE, "getline", "getline" },
	{ 0, "", "" },
};

#define SIZE	(LASTTOKEN - FIRSTTOKEN + 1)
char *table[SIZE];
char *names[SIZE];

void main(int, char**)
{
	struct xx *p;
	int i, tok;
	Biobuf *fp;
	char *buf, *toks[3];

	print("#include <u.h>\n");
	print("#include <libc.h>\n");
	print("#include <bio.h>\n");
	print("#include \"awk.h\"\n");
	print("#include \"y.tab.h\"\n\n");
	for (i = SIZE; --i >= 0; )
		names[i] = "";

	if ((fp = Bopen("y.tab.h", OREAD)) == nil) {
		fprint(2, "maketab can't open y.tab.h!\n");
		exits("can't open y.tab.h");
	}
	print("static char *printname[%d] = {\n", SIZE);
	i = 0;
	while ((buf = Brdline(fp, '\n')) != nil) {
		buf[Blinelen(fp)-1] = '\0';
		if (tokenize(buf, toks, 3) != 3
		|| strcmp("#define", toks[0]) != 0)	/* not a valid #define */
			continue;
		tok = strtol(toks[2], nil, 10);
		if (tok < FIRSTTOKEN || tok > LASTTOKEN) {
			fprint(2, "maketab funny token %d %s\n", tok, buf);
			exits("funny token");
		}
		names[tok-FIRSTTOKEN] = (char *) malloc(strlen(toks[1])+1);
		strcpy(names[tok-FIRSTTOKEN], toks[1]);
		print("\t(char *) \"%s\",\t/* %d */\n", toks[1], tok);
		i++;
	}
	print("};\n\n");

	for (p=proc; p->token!=0; p++)
		table[p->token-FIRSTTOKEN] = p->name;
	print("\nCell *(*proctab[%d])(Node **, int) = {\n", SIZE);
	for (i=0; i<SIZE; i++)
		if (table[i]==0)
			print("\tnullproc,\t/* %s */\n", names[i]);
		else
			print("\t%s,\t/* %s */\n", table[i], names[i]);
	print("};\n\n");

	print("char *tokname(int n)\n");	/* print a tokname() function */
	print("{\n");
	print("	static char buf[100];\n\n");
	print("	if (n < FIRSTTOKEN || n > LASTTOKEN) {\n");
	print("		sprint(buf, \"token %%d\", n);\n");
	print("		return buf;\n");
	print("	}\n");
	print("	return printname[n-FIRSTTOKEN];\n");
	print("}\n");
	exits(0);
}
