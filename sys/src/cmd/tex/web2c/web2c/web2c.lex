%{
/* web2c.l -- lexical analysis for Tangle output.  Public domain.  */

#include "web2c.h"
#include "y_tab.h"

/* For some reason flex wants to do a system call, so we must lose our
   definition of the Pascal read.  */
#undef read

char conditional[20], negbuf[2], temp[20];
extern boolean doing_statements;


/* We only read one input file.  This is the default definition, but
   giving it ourselves avoids the need to find -lfl or -ll at link time.
   This is a good thing, since libfl.a is often installed somewhere that
   the linker doesn't search by default.  */
int
yywrap P1H(void)
{
  return 1;
}
#define YY_SKIP_YYWRAP /* not that it matters */
%}
DIGIT		[0-9]
ALPHA		[a-zA-Z]
ALPHANUM	({DIGIT}|{ALPHA})
IDENTIFIER	({ALPHA}{ALPHANUM}*)
NUMBER		({DIGIT}+)
SIGN		("+"|"-")
SIGNED		({SIGN}?{NUMBER})
WHITE		[ \n\t]+
REAL		({NUMBER}"."{NUMBER}("e"{SIGNED})?)|({NUMBER}"e"{SIGNED})
COMMENT		(("{"[^}]*"}")|("(*"([^*]|"*"[^)])*"*)"))
W		({WHITE}|"packed ")+
WW		({WHITE}|{COMMENT}|"packed ")*
HHB0		("hh"{WW}"."{WW}"b0")
HHB1		("hh"{WW}"."{WW}"b1")

%%
{W}				;
"{"		{ while (input() != '}'); }

"#"		{
		    register int c;
		    putc('#', std);
		    while ((c = input()) && c != ';')
			putc(c, std);
		    putc('\n', std);
		}

"ifdef("	{register int c;
		 register char *cp=conditional;
		 new_line();
		 (void) input();
		 while ((c = input()) != '\'')
		    *cp++ = c;
		 *cp = '\0';
		 (void) input();
		 if (doing_statements) fputs("\t;\n", std);
		 fprintf(std, "#ifdef %s\n", conditional);
		}

"endif("	{register int c;
		 new_line();
		 fputs("#endif /* ", std);
		 (void) input();
		 while ((c = input()) != '\'')
			(void) putc(c, std);
		 (void) input();
		 conditional[0] = '\0';
		 fputs(" */\n", std);
		}

"ifndef("	{register int c;
		 register char *cp=conditional;
		 new_line();
		 (void) input();
		 while ((c = input()) != '\'')
		    *cp++ = c;
		 *cp = '\0';
		 (void) input();
		 if (doing_statements) fputs("\t;\n", std);
		 fprintf(std, "#ifndef %s\n", conditional);
		}

"endifn("	{register int c;
		 new_line();
		 fputs("#endif /* not ", std);
		 (void) input();
		 while ((c = input()) != '\'')
		   putc(c, std);
		 (void) input();
		 conditional[0] = '\0';
		 fputs(" */\n", std);
		}


"procedure "[a-z]+";"[ \n\t]*"forward;"	;

"function "[(),:a-z]+";"[ \n\t]*"forward;"	;

"@define"	return last_tok=define_tok;
"@field"	return last_tok=field_tok;
"and"		return last_tok=and_tok;
"array"		return last_tok=array_tok;
"begin"		return last_tok=begin_tok;
"case"		return last_tok=case_tok;
"const"		return last_tok=const_tok;
"div"		return last_tok=div_tok;
"break"		return last_tok=break_tok;
"do"		return last_tok=do_tok;
"downto"	return last_tok=downto_tok;
"else"		return last_tok=else_tok;
"end"		return last_tok=end_tok;
"file"		return last_tok=file_tok;
"for"		return last_tok=for_tok;
"function"	return last_tok=function_tok;
"goto"		return last_tok=goto_tok;
"if"		return last_tok=if_tok;
"label"		return last_tok=label_tok;
"mod"		return last_tok=mod_tok;
"not"		return last_tok=not_tok;
"of"		return last_tok=of_tok;
"or"		return last_tok=or_tok;
"procedure"	return last_tok=procedure_tok;
"program"	return last_tok=program_tok;
"record"	return last_tok=record_tok;
"repeat"	return last_tok=repeat_tok;
{HHB0}		return last_tok=hhb0_tok;
{HHB1}		return last_tok=hhb1_tok;
"then"		return last_tok=then_tok;
"to"		return last_tok=to_tok;
"type"		return last_tok=type_tok;
"until"		return last_tok=until_tok;
"var"		return last_tok=var_tok;
"while"		return last_tok=while_tok;
"others"	return last_tok=others_tok;

{REAL}		{		
		  sprintf (temp, "%s%s", negbuf, yytext);
		  negbuf[0] = '\0';
		  return last_tok=r_num_tok;
		}

{NUMBER}	{
		  sprintf (temp, "%s%s", negbuf, yytext);
		  negbuf[0] = '\0';
		  return last_tok=i_num_tok;
		}

("'"([^']|"''")"'")		return last_tok=single_char_tok;

("'"([^']|"''")*"'")		return last_tok=string_literal_tok;

"+"		{ if ((last_tok>=undef_id_tok &&
		      last_tok<=field_id_tok) ||
		      last_tok==i_num_tok ||
		      last_tok==r_num_tok ||
		      last_tok==')' ||
		      last_tok==']')
		   return last_tok='+';
		else return last_tok=unary_plus_tok; }

"-"		{ if ((last_tok>=undef_id_tok &&
		      last_tok<=field_id_tok) ||
		      last_tok==i_num_tok ||
		      last_tok==r_num_tok ||
		      last_tok==')' ||
		      last_tok==']')
		   return last_tok='-';
		else {
		  int c;
		  while ((c = input()) == ' ' || c == '\t')
                    ;
  		  unput(c);
		  if (c < '0' || c > '9') {
			return last_tok = unary_minus_tok;
		  }
		  negbuf[0] = '-';
		}}

"*"		return last_tok='*';
"/"		return last_tok='/';
"="		return last_tok='=';
"<>"		return last_tok=not_eq_tok;
"<"		return last_tok='<';
">"		return last_tok='>';
"<="		return last_tok=less_eq_tok;
">="		return last_tok=great_eq_tok;
"("		return last_tok='(';
")"		return last_tok=')';
"["		return last_tok='[';
"]"		return last_tok=']';
":="		return last_tok=assign_tok;
".."		return last_tok=two_dots_tok;
"."		return last_tok='.';
","		return last_tok=',';
";"		return last_tok=';';
":"		return last_tok=':';
"^"		return last_tok='^';

{IDENTIFIER}	{ strcpy (last_id, yytext);
		  l_s = search_table (last_id);
		  return
                    last_tok = (l_s == -1 ? undef_id_tok : sym_table[l_s].typ);
		}


.		{ /* Any bizarre token will do.  */
		  return last_tok = two_dots_tok; }
%%
