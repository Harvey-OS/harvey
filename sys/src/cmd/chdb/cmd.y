%{
#include	"ddb.h"
%}
%union	{
	long	lval;
	Node*	node;
	String	str;
}

%token	<lval>	NUMB
%token	<str>	STRING

%type	<node>	pattern p1 p2 p3 p4
%type	<lval>	expr gexpr wtype pexpr pgexpr fields field

%left	'+' '-'

%%
prog:
	command
|	prog command

command:
	';'
|	'd' 'd' ';'
	{
		tgames -= ngames-1;
		lastgame = 0;
		memmove(str+1, str+ngames, (tgames-1)*sizeof(Str));
		ngames = tgames;
		sortit(0);
		sprint(chars, "     %ld games", ngames);
		prline(8);
	}
|	'f' 'f' ';'
	{
		dodupl();
	}
|	'f' NUMB NUMB ';'
	{
		cinitf1 = $2;
		cinitf2 = $3;
		chessinit('G', cinitf1, cinitf2);
		forcegame(gameno);
	}
|	'g' gexpr ';'
	{
		setgame($2);
	}
|	'k' NUMB ';'
	{
		if($2 >= 1 && $2 <= 7)
			setmark($2);
		else
			yyerror("mark must be 1-7");
	}
|	'p' pexpr ';'
	{
		setposn($2);
	}
|	pattern ';'
	{
		if(!yysyntax) {
			setmark(0);
			search($1, 1, tgames, 1, 0);
			sortit(0);
			if(!yysyntax) {
				sprint(chars, "     %ld games", ngames-1);
				prline(8);
			}
		}
	}
|	'w' wtype { yystring = -1; } STRING ';'
	{
		dowrite($2, &cmd[$4.beg]);
	}

wtype:
	'a'
	{
		$$ = 'a';
	}
|	'm'
	{
		$$ = 'm';
	}
|	'n'
	{
		$$ = 'n';
	}

gexpr:
	expr
|	pgexpr
	{
		$$ = gameno+$1;
	}
|	'.'
	{
		$$ = rellast();
	}

pexpr:
	expr
	{
		$$ = $1*2 - 1;
	}
|	expr '.'
	{
		$$ = $1*2 - 0;
	}
|	pgexpr
	{
		$$ = curgame.position+$1;
	}

pgexpr:
	{
		$$ = 1;
	}
|	'+'
	{
		$$ = 1;
	}
|	'-'
	{
		$$ = -1;
	}
|	'+' expr
	{
		$$ = $2;
	}
|	'-' expr
	{
		$$ = -$2;
	}

expr:
	NUMB
|	'(' expr ')'
	{
		$$ = $2;
	}
|	expr '+' expr
	{
		$$ = $1 + $3;
	}
|	expr '-' expr
	{
		$$ = $1 - $3;
	}

pattern:
	p1
|	'&' p1
	{
		$$ = new(Nand);
		$$->left = new(Nnull);
		$$->right = $2;
	}
|	'-' p1
	{
		$$ = new(Nsub);
		$$->left = new(Nnull);
		$$->right = $2;
	}
|	'|' p1
	{
		$$ = new(Nor);
		$$->left = new(Nnull);
		$$->right = $2;
	}
|	'+' p1
	{
		$$ = new(Nor);
		$$->left = new(Nnull);
		$$->right = $2;
	}

p1:
	p2
|	p1 '&' p2
	{
		$$ = new(Nand);
		$$->left = $1;
		$$->right = $3;
	}
|	p1 p2
	{
		$$ = new(Nand);
		$$->left = $1;
		$$->right = $2;
	}

p2:
	p3
|	p2 '|' p3
	{
		$$ = new(Nor);
		$$->left = $1;
		$$->right = $3;
	}
|	p2 '+' p3
	{
		$$ = new(Nor);
		$$->left = $1;
		$$->right = $3;
	}

p3:
	p4
|	p3 '-' p4
	{
		$$ = new(Nsub);
		$$->left = $1;
		$$->right = $3;
	}

p4:
	'/' { yystring = '/'; } STRING fields
	{
		int c;

		c = cmd[$3.end];
		cmd[$3.end] = 0;
		$$ = new(Nrex);
		$$->rex = regcomp(&cmd[$3.beg]);
		$$->field = $4;
		cmd[$3.end] = c;
	}
|	'\'' NUMB
	{
		$$ = new(Nmark);
		$$->mark = 0;
		if($2 >= 0 && $2 <= 7)
			$$->mark = $2;
		else
			yyerror("mark must be 0-7");
	}
|	'.'
	{
		$$ = new(Ngame);
		$$->gp = str[gameno].gp;
	}
|	'*'
	{
		$$ = new(Nall);
		$$->gp = str[gameno].gp;
	}
|	'[' ']'
	{
		$$ = new(Nposit);
		$$->xor = xorset(0);
	}
|	'[' expr ']'
	{
		$$ = new(Nposit);
		$$->xor = xorset($2);
	}
|	'!' p4
	{
		$$ = new(Nnot);
		$$->left = $2;
	}
|	'(' p1 ')'
	{
		$$ = $2;
	}

fields:
	{
		$$ = 0;
	}
|	field fields
	{
		$$ = $1 | $2;
	}

field:
	'f'	/* file */
	{
		$$ = 1<<Byfile;
	}	/* white */
|	'w'	{
		$$ = 1<<Bywhite;
	}
|	'b'	/* black */
	{
		$$ = 1<<Byblack;
	}
|	'e'	/* event */
	{
		$$ = 1<<Byevent;
	}
|	'r'	/* result */
	{
		$$ = 1<<Byresult;
	}
|	'o'	/* opening */
	{
		$$ = 1<<Byopening;
	}
|	'd'	/* date */
	{
		$$ = 1<<Bydate;
	}
|	'p'	/* player = black|white */
	{
		$$ = (1<<Bywhite) | (1<<Byblack);
	}
|	'a'	/* all */
	{
		$$ = ~0;
	}
%%
