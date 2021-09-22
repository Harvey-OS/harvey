%{
#include "tca.h"
%}

%union	{
	long	lval;
	double	numb;
	int	op;
	Pair	pair;
	Node*	node;
	String	string;
}

%type	<node>		point circle line
%type	<numb>		value expr
%type	<op>		digit alph char other dir mchar
%type	<pair>		numb
%type	<string>	string
%left	'm' 't'
%left	'+' '-'
%left	'*' '/'

%%
prog:
|	prog stmt

stmt:
	'p' mchar point ';'
	{
		definep($2, $3);
	}
|	'v' mchar expr ';'
	{
		definev($2, $3);
	}
|	'n' string ';'
	{
		outstring(&$2);
	}
|	point ';'
|	segment ';'
|	';'

point:
	expr ',' expr
	{
		$$ = new(Opoint);
		$$->vleft = $1;
		$$->vright = $3;
	}
|	'p' mchar
	{
		$$ = lookupp($2);
	}
|	'd' point ',' expr ',' expr
	{
		$$ = new(Odme);
		$$->nleft = $2;
		$$->nright = new(Olist);
		$$->nright->vleft = $4;
		$$->nright->vright = $6;
	}
|	'd' point ',' 'a' point ',' expr
	{
		$$ = new(Odme);
		$$->nleft = $2;
		$$->nright = new(Olist);
		$$->nright->vleft = angl(eval($2), eval($5));
		$$->nright->vright = $7;
	}
|	'x' 'c' 'c' dir circle ',' circle
	{
		$$ = new(Oxcc);
		$$->ileft = $4;
		$$->nright = new(Olist);
		$$->nright->nleft = $5;
		$$->nright->nright = $7;
	}
|	'x' 'c' 'l' dir circle ',' line
	{
		$$ = new(Oxcl);
		$$->ileft = $4;
		$$->nright = new(Olist);
		$$->nright->nleft = $5;
		$$->nright->nright = $7;
	}
|	'x' 'l' 'l' line ',' line
	{
		$$ = new(Oxll);
		$$->nleft = $4;
		$$->nright = $6;
	}

segment:
	's' point ',' point
	{
		if(!outline)
			outsegment($2, $4);
	}
|	'S' point ',' point
	{
		outsegment($2, $4);
	}
|	'a' dir point ',' point ',' point
	{
		if(!outline)
			outarc($2, $3, $5, $7);
	}
|	'A' dir point ',' point ',' point
	{
		outarc($2, $3, $5, $7);
	}
|	'c' point ',' expr
	{
		if(!outline)
			outcirc($2, $4);
	}
|	'C' point ',' expr
	{
		outcirc($2, $4);
	}

		

line:
	point ',' expr
	{
		$$ = new(Oline);
		$$->nleft = $1;
		$$->vright = $3;
	}
|	point ',' 'a' point
	{
		$$ = new(Oline);
		$$->nleft = $1;
		$$->vright = angl(eval($1), eval($4));
	}

circle:
	point ',' expr
	{
		$$ = new(Ocirc);
		$$->nleft = $1;
		$$->vright = $3;
	}

dir:
	'l' { $$ = 'l'; }
|	'r' { $$ = 'r'; }
|	'n' { $$ = 'n'; }
|	's' { $$ = 's'; }
|	'e' { $$ = 'e'; }
|	'w' { $$ = 'w'; }

alph:
	'a' { $$ = 'a'; }
|	'b' { $$ = 'b'; }
|	'c' { $$ = 'c'; }
|	'd' { $$ = 'd'; }
|	'e' { $$ = 'e'; }
|	'f' { $$ = 'f'; }
|	'g' { $$ = 'g'; }
|	'h' { $$ = 'h'; }
|	'i' { $$ = 'i'; }
|	'j' { $$ = 'j'; }
|	'k' { $$ = 'k'; }
|	'l' { $$ = 'l'; }
|	'm' { $$ = 'm'; }
|	'n' { $$ = 'n'; }
|	'o' { $$ = 'o'; }
|	'p' { $$ = 'p'; }
|	'q' { $$ = 'q'; }
|	'r' { $$ = 'r'; }
|	's' { $$ = 's'; }
|	't' { $$ = 't'; }
|	'u' { $$ = 'u'; }
|	'v' { $$ = 'v'; }
|	'w' { $$ = 'w'; }
|	'x' { $$ = 'x'; }
|	'y' { $$ = 'y'; }
|	'z' { $$ = 'z'; }

|	'A' { $$ = 'A'; }
|	'B' { $$ = 'B'; }
|	'C' { $$ = 'C'; }
|	'D' { $$ = 'D'; }
|	'E' { $$ = 'E'; }
|	'F' { $$ = 'F'; }
|	'G' { $$ = 'G'; }
|	'H' { $$ = 'H'; }
|	'I' { $$ = 'I'; }
|	'J' { $$ = 'J'; }
|	'K' { $$ = 'K'; }
|	'L' { $$ = 'L'; }
|	'M' { $$ = 'M'; }
|	'N' { $$ = 'N'; }
|	'O' { $$ = 'O'; }
|	'P' { $$ = 'P'; }
|	'Q' { $$ = 'Q'; }
|	'R' { $$ = 'R'; }
|	'S' { $$ = 'S'; }
|	'T' { $$ = 'T'; }
|	'U' { $$ = 'U'; }
|	'V' { $$ = 'V'; }
|	'W' { $$ = 'W'; }
|	'X' { $$ = 'X'; }
|	'Y' { $$ = 'Y'; }
|	'Z' { $$ = 'Z'; }

digit:
	'0' { $$ = '0'; }
|	'1' { $$ = '1'; }
|	'2' { $$ = '2'; }
|	'3' { $$ = '3'; }
|	'4' { $$ = '4'; }
|	'5' { $$ = '5'; }
|	'6' { $$ = '6'; }
|	'7' { $$ = '7'; }
|	'8' { $$ = '8'; }
|	'9' { $$ = '9'; }

other:
	'_' { $$ = '_'; }
|	'[' { $$ = '['; }
|	']' { $$ = ']'; }
|	'-' { $$ = '-'; }
|	'/' { $$ = '/'; }
|	'\\' { $$ = '\\'; }

char:
	alph
|	digit
|	other

mchar:
	char
|	'*' mchar
	{
		$$ = $2 + 256;
	}

expr:
	value
|	'v' mchar
	{
		$$ = lookupv($2);
	}
|	'm' value
	{
		$$ = $2 + lookupv('m');
	}
|	't' value	/* statute */
	{
		$$ = $2 * .868976;
	}
|	expr '+' expr
	{
		$$ = $1 + $3;
	}
|	expr '-' expr
	{
		$$ = $1 - $3;
	}
|	expr '*' expr
	{
		$$ = $1 * $3;
	}
|	expr '/' expr
	{
		$$ = $1 / $3;
	}
value:

	'(' expr ')'
	{
		$$ = $2;
	}
|	numb
	{
		$$ = $1.val;
	}
|	numb '.' numb
	{
		$$ = $1.val + $3.val / pow10($3.len);
	}
|	numb '.'
	{
		$$ = $1.val;
	}
|	'.' numb
	{
		$$ = $2.val / pow10($2.len);
	}

numb:
	digit
	{
		$$.val = $1-'0';
		$$.len = 1;
	}
|	numb digit
	{
		$$.val = $1.val*10 + ($2-'0');
		$$.len = $1.len + 1;
	}

string:
	char
	{
		$$.val[0] = $1;
		$$.len = 1;
	}
|	string char
	{
		$$.val[$$.len] = $2;
		$$.len = $1.len + 1;
	}
