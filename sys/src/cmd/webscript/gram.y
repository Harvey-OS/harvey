%{
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

%}
%union {
	char s[1024];
	Cmd *x;
	int i;
}

%left	','
%right	'!'
%token	'\n' ';' '{' '}' '(' ')'

%token	<s>	CELL
%token	<s>	CHECKBOX
%token	<s>	ELSE
%token	<s>	FIND
%token	<s>	FORM
%token	<s>	IF
%token	<s>	INNER
%token	<s>	INPUT
%token	<s>	LOAD
%token	<s>	NEXT
%token	<s>	OUTER
%token	<s>	PAGE
%token	<s>	PASSWORD
%token	<s>	PREV
%token	<s>	PRINT
%token	<s>	RADIOBUTTON
%token	<s>	ROW
%token	<s>	SELECT
%token	<s>	STRING
%token	<s>	SUBMIT
%token	<s>	TABLE
%token	<s>	TEXTAREA
%token	<s>	TEXTBOX

%type	<s>	cell
%type	<s>	checkbox
%type	<x>	cmd
%type	<s>	form
%type	<s>	inner
%type	<s>	next
%type	<s>	outer
%type	<s>	page
%type	<s>	password
%type	<s>	prev
%type	<s>	radiobutton
%type	<s>	row
%type	<s>	select
%type	<s>	string
%type	<s>	table
%type	<s>	textarea
%type	<s>	textbox
%type	<i>	thing
%type	<i>	where
%type	<x>	xcmd
%type	<x>	xcmdlist
%type	<s>	zstring
%type	<i>	zthing
%type	<i>	zwhere
%%

prog:	/* empty */
|	prog nl
|	prog xcmd
	{
		runcmd($2);
	}

cmd:
	cmd ',' cmd
	{
		$$ = mkcmd(Xcomma, 0, nil);
		$$->left = $1;
		$$->right = $3;
	}
|	'!' cmd
	{
		$$ = mkcmd(Xnot, 0, nil);
		$$->left = $2;
	}
|	input zthing string
	{
		$$ = mkcmd(Xinput, $2, $3);
	}
|	find zwhere zthing zstring
	{
		$$ = mkcmd(Xfind, $2, $4);
		$$->arg1 = $3;
	}
|	load string
	{
		$$ = mkcmd(Xload, Tnone, $2);
	}
|	print zstring
	{
		$$ = mkcmd(Xprint, Tnone, $2);
	}
|	submit
	{
		$$ = mkcmd(Xsubmit, Tnone, nil);
	}

thing:
	cell		{ $$ = Tcell; }
|	checkbox	{ $$ = Tcheckbox; }
|	form		{ $$ = Tform; }
|	page 		{ $$ = Tpage; }
|	password	{ $$ = Tpassword; }
|	radiobutton	{ $$ = Tradiobutton; }
|	row 		{ $$ = Trow; }
|	table		{ $$ = Ttable; }
|	textarea	{ $$ = Ttextarea; }
|	textbox		{ $$ = Ttextbox; }
|	select		{ $$ = Tselect; }

zthing:
	/* none */	{ $$ = Tnone; }
|	thing

where:
	inner		{ $$ = Winner; }
|	outer		{ $$ = Wouter; }
|	prev		{ $$ = Wprev; }
|	next		{ $$ = Wnext; }

zwhere:
	/* none */	{ $$ = Wnone; }
|	where

end:
	nl
|	';'

nl: '\n'

xcmd:
	';'	{ $$ = mkcmd(Xnop, Tnone, nil); }
|	cmd end
	{
		$$ = $1;
	}
|	if '(' cmd ')' znl xcmd znl
	{
		$$ = mkcmd(Xif, Tnone, nil);
		$$->cond = $3;
		$$->left = $6;
	}
|	if '(' cmd ')' znl xcmd znl else xcmd
	{
		$$ = mkcmd(Xif, Tnone, nil);
		$$->cond = $3;
		$$->left = $6;
		$$->right = $9;
	}
|	'{' znl xcmdlist znl '}'
	{
		$$ = $3;
	}

xcmdlist:
	xcmd
	{
		$$ = $1;
	}
|	xcmdlist xcmd
	{
		$$ = mkcmd(Xblock, Tnone, nil);
		$$->left = $1;
		$$->right = $2;
	}

zstring: 
	/* nothing */ { $$[0] = 0; }
|	string

znl:
|	znl nl

cell:			CELL
checkbox:		CHECKBOX
else:			ELSE
find:			FIND
form:			FORM
if:				IF
inner:			INNER
input:			INPUT
load:			LOAD
next:			NEXT
outer:			OUTER
page:			PAGE
password:		PASSWORD
prev:			PREV
print:			PRINT
radiobutton:	RADIOBUTTON
row:			ROW
select:			SELECT
string:			STRING
submit:			SUBMIT
table:			TABLE
textarea:		TEXTAREA
textbox:		TEXTBOX
