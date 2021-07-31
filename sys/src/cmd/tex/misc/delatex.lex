/* Make this with:  lex delatex.lex;  cc lex.yy.c -ll -o delatex */
L [A-Za-z]
%Start Display Math Normal Tag
%%
<Normal>\'	{yyleng--; yymore(); /* ignore apostrophes */}
<Normal>{L}+\\- {yyleng-=2; yymore(); /* ignore hyphens */}
<Normal>[a-z]/[^A-Za-z] ; /* ignore single letter "words" */
<Normal>[A-Z]+	; /* ignore words all in uppercase */
<Normal>{L}+('{L}*)*{L}	{printf("%s\n",yytext); /* any other letter seq is a word */} 
<Normal>"%".*	; /* ignore comments */
<Normal>\\{L}+	; /* ignore other control sequences */
<Normal>"\\begin{"	BEGIN Tag; /* ignore this and up to next "}" */
<Normal>"\\bibitem{"	BEGIN Tag;
<Normal>"\\bibliography{"	BEGIN Tag;
<Normal>"\\bibstyle{"	BEGIN Tag;
<Normal>"\\cite{"	BEGIN Tag;
<Normal>"\\end{"		BEGIN Tag;
<Normal>"\\include{"	BEGIN Tag;
<Normal>"\\includeonly{"	BEGIN Tag;
<Normal>"\\input{"	BEGIN Tag;
<Normal>"\\label{"	BEGIN Tag;
<Normal>"\\pageref{"	BEGIN Tag;
<Normal>"\\ref{"		BEGIN Tag;
<Tag>[^}]		; /* ignore things up to next "}" */
<Tag>"}"		BEGIN Normal;
<Normal>[0-9]+	; /* ignore numbers */
<Normal>"\\("		BEGIN Math; /* begin latex math mode */
<Math>"\\)"		BEGIN Normal; /* end latex math mode */
<Math>.|\\[^)]|\n	; /* ignore anything else in latex math mode */
<Normal>"\\["		BEGIN Display; /* now in Latex display mode */
<Display>[^$]|\\[^\]]	; /* ignore most things in display math mode */
<Display>"\\]"	BEGIN Normal; /* get out of Display math mode */
<Normal>\\.	; /* ignore other single character control sequences */
<Normal>\\\n	; /* more of the same */
<Normal>\n|.	; /* ignore anything else, a character at a time */
%%
main(int argc, char **argv)
{
	int i;
	
	BEGIN Normal; /* Starts yylex off in the right state */
	if (argc==1) {
	    yyin = stdin;
	    yylex();
	    }
	else for (i=1; i<argc; i++) {
	    yyin = fopen(argv[i],"r");
	    if (yyin==NULL) {
		fprintf(stderr,"can't open %s\n",argv[i]);
		exit(1);
		}
	    yylex();
	    }
	exit(0);
}
yywrap(void)
{
	return 1;
}
