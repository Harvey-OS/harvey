%{
#include "common.h"
#include <ctype.h>
#include "smtpd.h"

#define YYSTYPE yystype
typedef struct quux yystype;
struct quux {
	String	*s;
	int	c;
};
Biobuf *yyfp;
YYSTYPE *bang;
extern Biobuf bin;

YYSTYPE cat(YYSTYPE*, YYSTYPE*, YYSTYPE*, YYSTYPE*, YYSTYPE*, YYSTYPE*, YYSTYPE*);
int yyparse(void);
int yylex(void);
YYSTYPE anonymous(void);
%}

%term SPACE
%term CNTRL
%term CRLF
%start conversation
%%

conversation	: cmd
		| cmd conversation
		;
cmd		: error
		| 'h' 'e' 'l' 'o' SPACE domain CRLF
			{ hello($6.s); }
		| 'm' 'a' 'i' 'l' SPACE 'f' 'r' 'o' 'm' ':' path CRLF
			{ sender($11.s); }
		| 'r' 'c' 'p' 't' SPACE 't' 'o' ':' path CRLF
			{ receiver($9.s); }
		| 'd' 'a' 't' 'a' CRLF
			{ data(); }
		| 'r' 's' 'e' 't' CRLF
			{ reset(); }
		| 's' 'e' 'n' 'd' SPACE 'f' 'r' 'o' 'm' ':' path CRLF
			{ sender($11.s); }
		| 's' 'o' 'm' 'l' SPACE 'f' 'r' 'o' 'm'  ':' path CRLF
			{ sender($11.s); }
		| 's' 'a' 'm' 'l' SPACE 'f' 'r' 'o' 'm' ':' path CRLF
			{ sender($11.s); }
		| 'v' 'r' 'f' 'y' SPACE string CRLF
			{ verify($6.s); }
		| 'e' 'x' 'p' 'n' SPACE string CRLF
			{ verify($6.s); }
		| 'h' 'e' 'l' 'p' CRLF
			{ help(0); }
		| 'h' 'e' 'l' 'p' SPACE string CRLF
			{ help($6.s); }
		| 'n' 'o' 'o' 'p' CRLF
			{ noop(); }
		| 'q' 'u' 'i' 't' CRLF
			{ quit(); }
		| 't' 'u' 'r' 'n' CRLF
			{ turn(); }
		;
path		: '<' '>'		={ $$ = anonymous(); }
		| '<' mailbox '>'	={ $$ = $2; }
		| '<' a-d-l ':' mailbox '>'	={ $$ = cat(&$2, bang, &$4, 0, 0 ,0, 0); }
		;
a-d-l		: at-domain		={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| at-domain ',' a-d-l	={ $$ = cat(&$1, bang, &$3, 0, 0, 0, 0); }
		;
at-domain	: '@' domain		={ $$ = cat(&$2, 0, 0, 0, 0 ,0, 0); }
		;
domain		: element		={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| element '.' domain	={ $$ = cat(&$1, &$2, &$3, 0, 0 ,0, 0); }
		;
element		: name			={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| '#' number		={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		| '[' dotnum ']'	={ $$ = cat(&$1, &$2, &$3, 0, 0 ,0, 0); }
		;
mailbox		: local-part		={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| local-part '@' domain	={ $$ = cat(&$3, bang, &$1, 0, 0 ,0, 0); }
		;
local-part	: dot-string		={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| quoted-string		={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		;
name		: a			={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| a ld-str		={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		| a ldh-str ld-str	={ $$ = cat(&$1, &$2, &$3, 0, 0 ,0, 0); }
		;
ld-str		: let-dig
		| let-dig ld-str	={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		;
ldh-str		: '-'
		| ld-str '-'		={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		| ldh-str ld-str '-'	={ $$ = cat(&$1, &$2, &$3, 0, 0 ,0, 0); }
		;
let-dig		: a
		| d
		;
dot-string	: string		={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| string '.' dot-string	={ $$ = cat(&$1, &$2, &$3, 0, 0 ,0, 0); }
		;

string		: char
		| string char		={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		;

quoted-string	: '"' qtext '"'		={ $$ = cat(&$1, &$2, &$3, 0, 0 ,0, 0); }
		;
qtext		: '\\' x		={ $$ = cat(&$2, 0, 0, 0, 0 ,0, 0); }
		| qtext '\\' x		={ $$ = cat(&$1, &$3, 0, 0, 0 ,0, 0); }
		| q
		| qtext q		={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		;
char		: c
		| '\\' x		={ $$ = $2; }
		;
dotnum		: snum '.' snum '.' snum '.' snum ={ $$ = cat(&$1, &$2, &$3, &$4, &$5, &$6, &$7); }
		;
number		: d			={ $$ = cat(&$1, 0, 0, 0, 0 ,0, 0); }
		| number d		={ $$ = cat(&$1, &$2, 0, 0, 0 ,0, 0); }
		;
snum		: number		={ if(atoi(s_to_c($1.s)) > 255) print("bad snum\n"); } 
		;
special1	: CNTRL
		| '(' | ')' | ',' | '.'
		| ':' | ';' | '<' | '>' | '@'
		;
special		: special1 | '\\' | '"'
		;
notspecial	: '!' | '#' | '$' | '%' | ' &' | '\''
		| '*' | '+' | '-' | '/'
		| '=' | '?'
		| '[' | ']' | '^' | '_' | '`' | '{' | '} ' | '~'
		;

a		: 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i'
		| 'j' | 'k' | 'l' | 'm' | 'n' | 'o' | 'p' | 'q' | 'r'
		| 's' | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z'
		;
d		: '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
		;
c		: a | d | notspecial
		;		
q		: a | d | special1 | notspecial | SPACE
		;
x		: a | d | special | notspecial | SPACE
		;
%%

void
parseinit(void)
{
	bang = (YYSTYPE*)malloc(sizeof(YYSTYPE));
	bang->c = '!';
	bang->s = 0;
	yyfp = &bin;
}

yylex(void)
{
	int c;

	for(;;){
		c = Bgetc(yyfp);
		if(c == -1)
			return 0;
		yylval.c = c = c & 0x7F;
		if(c == '\n'){
			return CRLF;
		}
		if(c == '\r'){
			c = Bgetc(yyfp);
			if(c != '\n'){
				Bungetc(yyfp);
				c = '\r';
			} else
				return CRLF;
		}
		if(isalpha(c))
			return tolower(c);
		if(isspace(c))
			return SPACE;
		if(iscntrl(c))
			return CNTRL;
		return c;
	}
}

YYSTYPE
cat(YYSTYPE *y1, YYSTYPE *y2, YYSTYPE *y3, YYSTYPE *y4, YYSTYPE *y5, YYSTYPE *y6, YYSTYPE *y7)
{
	YYSTYPE rv;

	if(y1->s)
		rv.s = y1->s;
	else {
		rv.s = s_new();
		s_putc(rv.s, y1->c);
		s_terminate(rv.s);
	}
	if(y2){
		if(y2->s){
			s_append(rv.s, s_to_c(y2->s));
			s_free(y2->s);
		} else {
			s_putc(rv.s, y2->c);
			s_terminate(rv.s);
		}
	} else
		return rv;
	if(y3){
		if(y3->s){
			s_append(rv.s, s_to_c(y3->s));
			s_free(y3->s);
		} else {
			s_putc(rv.s, y3->c);
			s_terminate(rv.s);
		}
	} else
		return rv;
	if(y4){
		if(y4->s){
			s_append(rv.s, s_to_c(y4->s));
			s_free(y4->s);
		} else {
			s_putc(rv.s, y4->c);
			s_terminate(rv.s);
		}
	} else
		return rv;
	if(y5){
		if(y5->s){
			s_append(rv.s, s_to_c(y5->s));
			s_free(y5->s);
		} else {
			s_putc(rv.s, y5->c);
			s_terminate(rv.s);
		}
	} else
		return rv;
	if(y6){
		if(y6->s){
			s_append(rv.s, s_to_c(y6->s));
			s_free(y6->s);
		} else {
			s_putc(rv.s, y6->c);
			s_terminate(rv.s);
		}
	} else
		return rv;
	if(y7){
		if(y7->s){
			s_append(rv.s, s_to_c(y7->s));
			s_free(y7->s);
		} else {
			s_putc(rv.s, y7->c);
			s_terminate(rv.s);
		}
	} else
		return rv;
}

void
yyerror(char *x)
{
	print("501 %s\r\n", x);
}

/*
 *  an anonymous user
 */
YYSTYPE
anonymous(void)
{
	YYSTYPE rv;

	rv.s = s_copy("pOsTmAsTeR");
	return rv;
}
