# include "ldefs.h"
void
phead1(void)
{
	fprintf(fout,"typedef unsigned char Uchar;\n");
	fprintf(fout,"# include <stdio.h>\n");
	fprintf(fout, "# define U(x) x\n");
	fprintf(fout, "# define NLSTATE yyprevious=YYNEWLINE\n");
	fprintf(fout,"# define BEGIN yybgin = yysvec + 1 +\n");
	fprintf(fout,"# define INITIAL 0\n");
	fprintf(fout,"# define YYLERR yysvec\n");
	fprintf(fout,"# define YYSTATE (yyestate-yysvec-1)\n");
	fprintf(fout,"# define YYOPTIM 1\n");
# ifdef DEBUG
	fprintf(fout,"# define LEXDEBUG 1\n");
# endif
	fprintf(fout,"# define YYLMAX 200\n");
	fprintf(fout,"# define output(c) putc(c,yyout)\n");
	fprintf(fout, "%s%d%s\n",
  "# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==",
	'\n',
 "?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)");
	fprintf(fout,
"# define unput(c) {yytchar= (c);if(yytchar=='\\n')yylineno--;*yysptr++=yytchar;}\n");
	fprintf(fout,"# define yymore() (yymorfg=1)\n");
	fprintf(fout,"# define ECHO fprintf(yyout, \"%%s\",yytext)\n");
	fprintf(fout,"# define REJECT { nstr = yyreject(); goto yyfussy;}\n");
	fprintf(fout,"int yyleng; extern char yytext[];\n");
	fprintf(fout,"int yymorfg;\n");
	fprintf(fout,"extern Uchar *yysptr, yysbuf[];\n");
	fprintf(fout,"int yytchar;\n");
	fprintf(fout,"FILE *yyin = {stdin}, *yyout = {stdout};\n");
	fprintf(fout,"extern int yylineno;\n");
	fprintf(fout,"struct yysvf { \n");
	fprintf(fout,"\tstruct yywork *yystoff;\n");
	fprintf(fout,"\tstruct yysvf *yyother;\n");
	fprintf(fout,"\tint *yystops;};\n");
	fprintf(fout,"struct yysvf *yyestate;\n");
	fprintf(fout,"extern struct yysvf yysvec[], *yybgin;\n");
	fprintf(fout,"int yylook(void), yywrap(void), yyback(int *, int);\n");
}

void
phead2(void)
{
	fprintf(fout,"while((nstr = yylook()) >= 0)\n");
	fprintf(fout,"yyfussy: switch(nstr){\n");
	fprintf(fout,"case 0:\n");
	fprintf(fout,"if(yywrap()) return(0); break;\n");
}

void
ptail(void)
{
	if(!pflag){
		fprintf(fout,"case -1:\nbreak;\n");		/* for reject */
		fprintf(fout,"default:\n");
		fprintf(fout,"fprintf(yyout,\"bad switch yylook %%d\",nstr);\n");
		fprintf(fout,"} return(0); }\n");
		fprintf(fout,"/* end of yylex */\n");
	}
	pflag = 1;
}

void
statistics(void)
{
	fprintf(errorf,"%d/%d nodes(%%e), %d/%d positions(%%p), %d/%d (%%n), %ld transitions\n",
		tptr, treesize, nxtpos-positions, maxpos, stnum+1, nstates, rcount);
	fprintf(errorf, ", %d/%d packed char classes(%%k)", pcptr-pchar, pchlen);
	fprintf(errorf,", %d/%d packed transitions(%%a)",nptr, ntrans);
	fprintf(errorf, ", %d/%d output slots(%%o)", yytop, outsize);
	putc('\n',errorf);
}
