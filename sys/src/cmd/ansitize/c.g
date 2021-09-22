# fake operators used to resolve if/else ambiguity 
%left	DOSHIFT
%left	"else"

# real operators - usual c precedence
%left	":"
%left	","
%right	"=" "+=" "-=" "*=" "/=" "%=" "<<=" ">>=" "&=" "^=" "|="
%right	"?" ":"
%left	"||"
%left	"&&"
%left	"|"
%left	"^"
%left	"&"
%left	"=="
%left	"!="
%left	"<" ">" "<=" ">="
%left	"<<" ">>"
%left	"+" "-"
%left	"*" "/" "%"
%right	CAST
%left	sizeof UNARY "!" "~"
%right	"--" "++" "->" "." "[" "]" "(" ")"
%left	STRING LSTRING

# top-level loop
prog:
prog:	prog prog1

prog1:	lstmt
prog1:	xdecl
prog1:	include
prog1:	pragma
prog1:	vararg

cexpr:	expr,

expr:	name
expr:	number
expr:	string+
expr:	Lstring+
expr:	char
expr:	Lchar
expr:	expr "*" expr
expr:	expr "/" expr
expr:	expr "%" expr
expr:	expr "+" expr
expr:	expr "-" expr
expr:	expr ">>" expr
expr:	expr "<<" expr
expr:	expr "<" expr
expr:	expr ">" expr
expr:	expr "<=" expr
expr:	expr ">=" expr
expr:	expr "==" expr
expr:	expr "!=" expr
expr:	expr "&" expr
expr:	expr "^" expr
expr:	expr "|" expr
expr:	expr "&&" expr
expr:	expr "||" expr
expr:	expr "?" cexpr ":" expr
expr:	expr "=" expr
expr:	expr "+=" expr
expr:	expr "-=" expr
expr:	expr "*=" expr
expr:	expr "/=" expr
expr:	expr "%=" expr
expr:	expr "<<=" expr
expr:	expr ">>=" expr
expr:	expr "&=" expr
expr:	expr "^=" expr
expr:	expr "|=" expr
expr:	"*" expr	%prec UNARY
expr:	"&" expr	%prec UNARY
expr:	"+" expr	%prec UNARY
expr:	"-" expr	%prec UNARY
expr:	"!" expr
expr:	"~" expr
expr:	"++" expr
expr:	"--" expr
expr:	sizeof expr
expr:	sizeof "(" abtype ")"
expr:	"(" abtype ")" expr	%prec CAST
expr:	"(" abtype ")" "{" init, "}"	%prec CAST
expr:	"(" cexpr ")"
expr:	expr "(" expr,? ")"
expr:	expr "[" cexpr "]"
expr:	expr "++"
expr:	expr "--"
expr:	"va_arg" "(" expr "," abtype ")"

sizeof:	"sizeof"
sizeof:	"signof"
sizeof:	"alignof"

block1:	decl
block1:	lstmt

block1*:	
block1*:	block1* block1

block:	"{" block1* "}"

label:	"case" expr ":"
label:	"case" expr "..." expr ":"
label:	"default" ":"
label:	tag ":"

lstmt:	label* stmt

stmt:	";"
stmt:	block
stmt:	cexpr ";"
stmt:	"if" "(" cexpr ")" lstmt	%prec DOSHIFT
stmt:	"if" "(" cexpr ")" lstmt "else" lstmt	%prec "else"
stmt:	"for" "(" cexpr? ";" cexpr? ";" cexpr? ")" lstmt
stmt:	"while" "(" cexpr ")" lstmt
stmt:	"do" lstmt "while" "(" cexpr ")" ";"
stmt:	"return" cexpr? ";"
stmt:	"switch" "(" cexpr ")" lstmt
stmt:	"ARGBEGIN" stmt "ARGEND"
stmt:	"break" ";"
stmt:	"continue" ";"
stmt:	"goto" tag ";"

# can't happen in real code, but helps ignore some macros
# stmt:	expr block

# Abstract declarator - abdec1 includes the slot where the name would go
abdecor:
abdecor:	"*" qname* abdecor
abdecor:	abdec1

abdec1:	abdec1 "(" fnarg,? ")"
abdec1:	abdecor "[" expr? "]"
abdec1:	"(" abdecor ")"

# Concrete declarator
decor:	tag
decor:	"*" qname* decor
decor:	"(" decor ")"
decor:	decor "(" fnarg,? ")"
decor:	decor "[" expr? "]"

# Function argument
fnarg:	name
fnarg:	type abdecor
fnarg:	type decor
fnarg:	"..."

# Initialized declarator
idecor:	decor
idecor:	decor "=" init

# Class words
cname:	"auto"
cname:	"static"
cname:	"extern"
cname:	"typedef"
cname:	"typestr"
cname:	"register"
cname:	"inline"

# Qualifier words
qname:	"const"
qname:	"volatile"

# Type words
tname:	"char"
tname:	"short"
tname:	"int"
tname:	"long"
tname:	"signed"
tname:	"unsigned"
tname:	"float"
tname:	"double"
tname:	"void"

cqname:	cname
cqname:	qname

cqtname:	cqname
cqtname:	tname

# Type specifier but not a tname
typespec:	typename

# Types annotated with class info
#	typeclass:
#		cqname* typespec cqname*
#	|	cqname* tname cqtname*
#	|	cqname+
# except LR(1) can't seem to handle that (?)

typeclass:	cqname* typespec cqname*
typeclass:	cqname* tname cqtname*
typeclass:	cqname+

# Types without class info (check for class in higher level)
type:	typeclass

abtype:	type abdecor

# Declaration (finally)
decl:	typeclass idecor,? ";"

# External (top-level) declaration
xdecl:	decl
xdecl:	fndef

fndef:	typeclass decor decl* block

name:		id
typename:	id
tag:	id

# struct/union
structunion:	"struct"
structunion:	"union"

sudecor:	decor
sudecor:	tag? ":" expr

sudecl:	type sudecor,? ";"

typespec:	structunion tag
typespec:	structunion tag? "{" sudecl+ "}"

initprefix:	"." tag

expr:	expr "->" tag
expr:	expr "." tag

# enum
typespec:	"enum" tag
typespec:	"enum" tag? "{" edecl, comma? "}"
edecl:	tag eqexpr?
eqexpr:	"=" expr

# initializers
init:	expr
init:	"{" binit,? comma? "}"

binit:	init
binit:	initprefix+ eq? init

initprefix:	"[" expr "]"

eq?:
eq?: "="

comma?:
comma?: ","

initprefix+: initprefix
initprefix+: initprefix+ initprefix

init,:	init
init,:	init, "," init

binit,:	binit
binit,:	binit, "," binit
binit,?:
binit,?:	binit,


# the many number formats
numd:	/0|[1-9][0-9]*([Uu]?[Ll]?[Ll]?|[Ll][Ll]?[Uu]?)/
numo:	/0[0-7]*([Uu]?[Ll]?[Ll]?|[Ll][Ll]?[Uu]?)/
numx:	/0[Xx][0-9A-Fa-f]+([Uu]?[Ll]?[Ll]?|[Ll][Ll]?[Uu]?)/
numf:	/([0-9]+\.[0-9]*|[0-9]*\.[0-9]+)([eE][+\-]?[0-9]+)?[FfLl]?/
numfx:	/0[Xx]([0-9A-Fa-f]+\.[0-9A-Fa-f]*|[0-9A-Fa-f]*\.[0-9A-Fa-f]+)([pP][+\-]?[0-9]+)?[FfLl]?/

# catch-all for malformed numbers
numbad:	/[0-9][0-9A-Za-z_]+/

number:	numd
number:	numo
number:	numx
number:	numf
number:	numfx
number:	numbad

# constants strings and characters
string:	/"([^"\\]|\\.)*"/
Lstring:	/L"([^"\\]|\\.)*"/
char:	/'([^'\\]|\\.)*'/
Lchar:	/L'([^'\\]|\\.)*'/

# must be after all literal strings!
id:	/[_A-Za-z¡-￿][A-Za-z0-9_¡-￿]*/

# special notations - should be created implicitly
tag?:
tag?:	tag

cexpr?:
cexpr?:	cexpr

expr?:
expr?:	expr

expr,:	expr
expr,:	expr, "," expr

expr,?:
expr,?:	expr,

block?:
block?:	block

stmt*:
stmt*:	stmt* stmt

lstmt*:
lstmt*:	lstmt* lstmt

decl*:
decl*:	decl* decl

label*:
label*:	label* label

fnarg,:	fnarg
fnarg,:	fnarg, "," fnarg

fnarg,?:
fnarg,?:	fnarg,

idecor,:	idecor
idecor,:	idecor, "," idecor

idecor,?:
idecor,?:	idecor,

qname+:	qname
qname+:	qname+ qname

qname*:
qname*:	qname+

cqname+:	cqname
cqname+:	cqname+ cqname

cqname*:
cqname*:	cqname+

cqtname+:	cqtname
cqtname+:	cqtname+ cqtname

cqtname*:
cqtname*:	cqtname+

sudecor,:	sudecor
sudecor,:	sudecor, "," sudecor

sudecor,?:
sudecor,?:	sudecor,

sudecl+:	sudecl
sudecl+:	sudecl+ sudecl

eqexpr?:
eqexpr?:	eqexpr

edecl,: edecl
edecl,: edecl, "," edecl

include:	/#include[ \t].*/
vararg:	/(#pragma[ \t]+vararg.*\n+)*(#pragma[ \t]+vararg.*\n)/
pragma:	/#pragma.*/
%ignore	/[ \t\n]+/
%ignore	/#(.|\\\n)*/
%ignore	$//.*$
%ignore	$/\*(.|\n)*\*/$s
# %ignore	$/\*([^\*]|\*+([^/]|\n)|\n)*\*/$

string+: string
string+: string+ string

Lstring+: Lstring
Lstring+: Lstring+ Lstring

xxx:
