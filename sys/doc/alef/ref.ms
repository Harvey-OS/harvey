.ds Op \s-2\dopt\u\s0
.am DS
.ft I
..
.ta 1i 2.3i 4.5i  (optional to set tabs)
.TL
Alef Language Reference Manual
.AU
Phil Winterbottom
philw@plan9.att.com
.SH
Introduction
.PP
Alef is a concurrent programming language designed for
systems software. Exception handling, process management, and synchronization
primitives are implemented by the language. Programs can be written using both
shared variable and message passing paradigms. Expressions use the same syntax
as C, but the type system is substantially different. Alef supports object-oriented
programming through static inheritance and information hiding. The language does not
provide garbage collection, so programs are expected to manage their own memory.
This manual provides a bare description of the syntax and semantics 
of the current implementation.
.PP
Much of the terminology used in this manual is borrowed from the ANSI C language
reference manual and the Plan 9 manual. The manual expects familiarity
with both.
.NH
Lexical
.PP
Compilation starts with a preprocessing phase. An ANSI C preprocessor is used.
The preprocessor performs file inclusion and macro substitution. Comments and lines
beginning
with the
.CW #
character are consumed by the preprocessor. The preprocessor
produces a sequence of tokens for the compiler.
.NH 2
Tokens
.PP
The lexical analyzer classifies tokens as: identifiers, typenames, keywords,
constants, and operators. Tokens are separated by white space,
which is ignored in the source except as needed to separate
sequences of tokens which would otherwise be ambiguous.
The lexical analyzer is greedy: if tokens have been consumed up to a
given character, then the next token will be the longest subsequent
string of characters that forms a legal token.
.NH 2
Reserved Words
.PP
The following keywords are reserved by the language and may
not be used as identifiers:
.P1
adt		aggr		alloc
alt		become		break
byte		case		chan
check		continue	default
do		else		enum
extern		float		for
goto		if		int
intern		lint		nil
par		proc		raise
rescue		return		sint
sizeof		switch		task
tuple		typedef		typeof
uint		ulint		unalloc
union		usint		void
while		zerox
.P2
The following symbols are used as separators and operators in the language:
.P1
+	-	/	=
>	<	!	%
&	|	?	.
"	'	{	}
[	]	(	)
*	;
.P2
The following multi-character sequences are used as operators:
.P1
+=	-=	/=	*=
%=	&=	|=	^=
<<=	>>=	==	!=
--	<-	->	++
::	:=
.P2
.NH 2
Comments
.PP
Comments are removed by the preprocessor. A comment starts with the characters
.CW /*
and finishes at the characters
.CW */ .
A comment may include any sequence of
characters including
.CW /* .
Comments do not nest.
.NH 2
Identifiers
.PP
An identifier, also called a lexical name, is any sequence of alpha-numeric characters and the
underscore character
.CW _ .
Identifiers may not start with a digit.
Identifiers are case sensitive. All characters are significant.
Identifiers beginning with the string
.CW ALEF
are reserved for use by the runtime system.
.NH 2
Constants
.PP
There are five types of constant:
.DS
	constant:
		integer-const
		character-const
		floating-const
		string-const
		rune-string-const
.DE
An integer constant is a sequence of digits.
A prefix may be used to modify the base of a number.
Defined prefixes, bases, and digit sets are:
.P1
none	decimal		0-9
0x	hexadecimal	0-9 a-f A-F
0	octal		0-7
.P2
A character constant contains one or more characters surrounded
by single quote marks
.CW ' .
If the constant contains two or more characters the first must be
the escape character
.CW \e .
The following table shows valid characters after an escape and the value of the
constant:
.P1
0	NUL	\f1Null character\fP
n	NL	\f1Newline\fP
r	CR	\f1Carriage return\fP
t	HT	\f1Horizontal tab\fP
b	BS	\f1Backspace\fP
f	FF	\f1Form feed\fP
a	BEL	\f1Beep\fP
v	VT	\f1Vertical tab\fP
\e	\e	\f1Backslash\fP
"	"	\f1Double quote\fP
.P2
Character constants have the type
.CW int .
The range of values they hold depends on the character set.
In Plan 9, the input text is in UTF
and character constants hold the 16-bit
representation of the Unicode character
(see
.I rune (6)
in Volume 1 of the Plan 9 Programmer's Manual).
.PP
A floating point constant consists of an integer part, a period,
a fractional part, the letter
.CW e
and an exponent part. The integer, fraction and exponent parts 
must consist of decimal digits. Either the integer or fractional parts may be
omitted. Either the decimal point or the letter
.CW e
and the exponent may be
omitted. The integer part or period and the exponent part may be preceded by
the unary
.CW +
or
.CW -
operators. Floating point constants have the type
.CW float .
.PP
A string constant is a sequence of characters between double quote marks
.CW \&" .
A string has the type `static array of byte'. A NUL (zero) character is automatically
appended to the string by the compiler.
The effect of modifying a string constant is implementation dependent.
The
.CW sizeof
operator applied to a string constant yields the number of bytes including
the appended NUL.
.PP
A rune string constant is a sequence of Unicode characters introduced by
.CW $\&" 
and terminated by
.CW \&" .
A rune string has the type `static array of usint'.
A zero rune character is automatically
appended to the string by the compiler.
The
.CW sizeof
operator applied to a rune string constant yields the number of runes, including
the appended zero, times
.CW sizeof
.CW (usint) .
.PP
.NH 2
Programs
.PP
An Alef program is a list of declarations stored in one or more source
.I files.
The declarations introduce identifiers.
Identifiers may define variables, types, functions,
function prototypes, or enumerators.
Identifiers have associated
.I "storage classes
and
.I scope
(see Section 2).
For functions and variables declared at the file scope the storage class
determines if a definition can be accessed from another file.
.NH 2
Processes and Tasks
.PP
The term
.I process
is used to refer to a preemptively scheduled
.I thread 
of execution.
A process may contain several tasks.
A
.I task
is a non-preemptively scheduled coroutine within a process. The
memory model does not define the sharing of memory between processes.
On a shared
memory computer processes will typically share the same address space.
On a multicomputer processes may be located on physically distant nodes with
access only to local memory. In such a system processes would not share
the same address space, and must communicate using message passing.
.PP
A group of tasks executing within the context of a process are defined
to be in the same address space. Tasks are scheduled during communication and
synchronization operations.
The term 
.I thread 
is used wherever the distinction between
a process and a task is unimportant.
.NH
Definitions and Declarations
.PP
A declaration introduces an identifier and specifies its type.
A definition is a
declaration that also reserves storage for an identifier.
An object is an area of memory of known type produced by a definition.
Function prototypes, variable declarations preceded by
.CW  extern ,
and 
type specifiers are declarations.
Function declarations with bodies and variable declarations are
examples of definitions.
.NH 2
Scope
.PP
Identifiers within a program have scope.
There are four levels of scope: local, function, type, and file:
.IP \(bu
A local identifier is declared at the start of a block. A local has scope starting
from its declaration to the end of the block in which it was declared.
.IP \(bu
Exception identifiers and labels have the scope of a function. These identifiers
can be referenced from the start of a function to its end, regardless of
position of the declaration.
.IP \(bu
A member of a complex type is in scope only when a dereferencing
operator
.CW .
or
.CW ->
is applied to an object of the type.
Hidden type members have special scope and may only
be referenced by function members of the type.
.IP \(bu
All definitions outside of a function body have the scope of file. Unqualified
declarations at the file scope have static storage class.
.LP
.NH 2
Storage classes
.PP
There are three storage classes: automatic, parameter and static.
Automatic objects
are created at entry to the block in which they were declared. The
value of an automatic is undefined upon creation. Automatic variables
are destroyed at block exit.
Parameters are created by function invocation and are
destroyed at function exit.
Static objects exist from invocation of
the program until termination.
Static objects which have not been initialized have the value 0.
.NH
Types
.PP
A small set of basic types is defined by the language. More complex types may
be derived from the basic types.
.NH 2
Basic types
.PP
The basic types are:
.TS
box, center, tab(:);
c | l | l
cFCW | l | l .
name : size : type
_
byte	: 8  bits : unsigned byte
_
sint	: 16 bits : signed short integer
_
usint	: 16 bits : unsigned short integer
_
int	: 32 bits : signed integer
_
uint	: 32 bits : unsigned integer
_
float	: 64 bits : floating point
_
lint	: 64 bits : long signed integer
_
ulint	: 64 bits : unsigned long integer
_
chan	: 32 bits : channel
_
\f1poly\fP	: 64 bits : polymorphic type
.TE
The size given for the basic types is the minimum number of bits required
to represent that type. The format and precision of
.CW float
is implementation
dependent. The
.CW float
type should be the highest precision floating point provided by the hardware.
The
.CW lint
and
.CW ulint
types are not part of the current implementation but have been defined.
The alignment of the basic types is implementation dependent.
Channels are implemented by the runtime system and must
be allocated before use. They are the size of a pointer.
Polymorphic types are represented by a pointer and a tag representing the type.
For a given implementation the polymorphic type has the same size as the
following aggregate definition:
.P1
aggr Polytype
{
	void*	ptr;
	int	tag;
};
.P2
The
.CW void
type performs the special task of declaring procedures returning
no value and as part of a derived type to form generic pointers. The
.CW void
type may not be used as a basic type.
.NH 2
Derived types
.PP
Types are derived in the same way as in C.
Operators applied in declarations use one of the basic types to derive a
new type. The deriving operators are:
.P1
*	\f1create a pointer to\fP
&	\f1yield the address of\fP
()	\f1a function returning\fP
[]	\f1an array of\fP
.P2
These operators bind to the name of each identifier in a declaration or
definition.
Some examples are:
.P1
int 	*ptr;	/* A pointer to an integer */
byte	c[10];	/* A vector of 10 bytes */
float	*pow();	/* A function returning a pointer to float */
.P2
Complex types may be built from the basic types and the deriving operators.
Complex types may be either aggregates, unions, tuples, or abstract data types (ADT).
These complex types contain sequences of basic types and other derived types.
An aggregate is a simple
collection of basic and derived types. Each element of the aggregate has unique
storage. An abstract data type has the same storage allocation as an aggregate
but also has a set of functions to manipulate the type, and a set of protection
attributes for each of its members. A union type contains a sequence of basic and
derived types that occupy the same storage. The size of a union is determined by
the size of the largest member.
.PP
The declaration of complex types introduces
.I typenames
into the language. After
declaration a typename can be used wherever a basic type is permitted. New
.I typenames
may be defined from derived and basic types using the
.CW typedef
statement.
.PP
The integral types are
.CW int ,
.CW uint ,
.CW sint ,
.CW usint ,
.CW byte ,
.CW lint
and
.CW ulint .
The arithmetic types are the integral types and the type
.CW float .
The pointer type is a type derived from the
.CW &
(address of) operator or derived from a pointer declaration.
The complex types are
.CW aggr ,
.CW adt ,
and
.CW union .
.NH 2
Conversions and Promotions
.PP
Alef performs the same implicit conversions and promotions as ANSI C with the
addition of complex type promotion: under assignment, function parameter
evaluation, or function returns, Alef will promote an unnamed member of a
complex type into the type of the left-hand side, formal parameter, or
function.
.NH
Declarations
.PP
A declaration attaches a type to an identifier; it need not reserve storage.
A declaration which reserves storage is called a definition. A program consists
of a list of declarations:
.DS
	program:
		declaration-list

	declaration-list:
		declaration
		declaration-list declaration
.DE
A declaration can define a simple variable, a function, a prototype to a
function, an ADT function, a type specification, or a type definition:
.DS
	declaration:
		simple-declarations
		type-declaration
		function-declaration
		type-definition
.DE
.NH 2
Simple declarations
.PP
A simple declaration consists of a type specifier and a list of identifiers. Each
identifier may be qualified by deriving operators. Simple declarations at the file
scope may be initialized.
.DS
	simple-declarations:
		type-specifier simple-decl-list \f(CW;\fP

	simple-decl-list:
		simple-declaration
		function-prototype
		simple-decl-list , simple-declaration

	function-prototype:
		pointer\*(Op identifier array-spec\*(Op \f(CW(\fP arglist \f(CW)\fP \f(CW;\fP
		\f(CW(\fP pointer\*(Op identifier array-spec\*(Op \f(CW)\fP \f(CW(\fP arglist \f(CW)\fP \f(CW;\fP

	simple-declaration:
		pointer\*(Op identifier array-spec\*(Op
		pointer\*(Op identifier array-spec\*(Op \f(CW=\fP initializer-list

	pointer:
		\f(CW*\fP
		pointer \f(CW*\fP

	array-spec:
		\f(CW[\fP constant-expression \f(CW]\fP
		\f(CW[\fP constant-expression \f(CW]\fP array-spec
.DE
.NH 2
Array Specifiers
.PP
The dimension of an array must be non-zero positive constant. Arrays
have a lower bound of 0 and an upper bound of 
.CW n-1 ,
where
.CW n
is the value of the constant expression.
.NH 2
Type Specifiers
.PP
.DS
	type-specifier:
		scope\*(Op type

	type:
		\f(CWbyte\fP
		\f(CWint\fP
		\f(CWuint\fP
		\f(CWsint\fP
		\f(CWusint\fP
		\f(CWlint\fP
		\f(CWulint\fP
		\f(CWvoid\fP
		\f(CWfloat\fP
		typename
		polyname
		tupletype
		channel-specifier
.DE
.DS
	scope:
		\f(CWintern\fP
		\f(CWextern\fP

	channel-specifier:
		\f(CWchan\fP \f(CW(\fP typelist \f(CW)\fP buffer-spec\*(Op

	tupletype:
		\f(CWtuple\fP\*(Op  \f(CW(\fP typelist \f(CW)\fP

	buffer-spec:
		\f(CW[\fP constant-expression \f(CW]\fP

	typelist:
		ptr-type
		ptr-type \f(CW,\fP typelist

	ptr-type:
		type-specifier
		ptr-type pointer\*(Op

	polyname:
		identifier

.DE
The keywords
.CW intern
and
.CW extern
control the scope of declarations.
When applied to a definition or declaration at the file scope,
.CW intern
narrows the scope to the current file;
.CW extern
makes the declared identifier visible to other compilation
units.
By default declarations at the file scope default to
.CW extern .
The control of access to members of abstract data types
is defined in the discussion of ADT's below.
.PP
.I Typename
is an identifier defined by a complex type declaration or a
.CW typedef
statement.
.NH 3
Channel Type Specification
.PP
The type specified by a
.CW chan
declaration is actually a pointer to
an internal object with an anonymous type specifier.
Because of their anonymity,
objects of this special type cannot be defined in declarations;
instead they must be created by an
.CW alloc
statement referring to a
.CW chan .
A channel declaration without a buffer specification produces a synchronous
communication channel.
Threads sending values on the channel will block until
some other thread receives from the channel.
The two threads rendezvous and a value
is passed between sender and receiver. If buffers are specified
then an asynchronous channel is produced.
The
.I constant-expression
defines the number of buffers
to be allocated. A send operation will complete immediately while buffers
are available, and will block if all buffers are in use. A receive
operation will block if no value is buffered. If a value is buffered, the receive
will complete and make the buffer available for a new send operation.
Any senders waiting for buffers will then be allowed to continue.
.PP
Values of
.I chan-type
are passed between threads using the channel for communication. If
.I chan-type
is a comma-separated list of types the channel supplies a
.I variant
protocol.
A variant protocol allows messages to be demultiplexed by type during
a receive operation.
A form of the
.CW alt
statement allows the control flow to be modified based on the type
of a value received from a channel supplying a variant protocol.
.NH 3
Polymorphic Type
.PP
The polymorphic type can be used to dynamically represent a value of any type.
A polymorphic type is identified by a lexical name defined in a polymorphic
type definition (see the section
on Type Definition) or as a parameter to a polymorphic abstract
data type (see the section on Polymorphic and Parameterized Abstract Data Types).
Distinct lexical names
represent a value of the same structure but are different for the purposes
of type checking.
A polymorphic value is represented by a
.I fat
.I pointer .
The pointer consists of an
.I integer
tag and a
.I pointer
to a value.
Like channels, storage for the data must be allocated by the runtime.
.NH 2
Initializers
.PP
Only simple declarations at the file scope may be initialized.
.DS
	initializer-list:
		constant-expression
		\f(CW[\fP constant-expression \f(CW]\fP constant-expression
		\f(CW{\fP initializer-list \f(CW}\fP
		initializer-list \f(CW,\fP initializer-list
.DE
An initialization consists of a
.I constant-expression
or a list of constant-expressions separated by commas and enclosed by braces.
An array or complex type requires an explicit set of braces for each level 
of nesting.
Unions may not be initialized.
All the components of a variable need not be explicitly initialized;
uninitialized elements are set to zero. ADT types are initialized in the same
way as aggregates with the exception of ADT function members which are ignored
for the purposes of initialization.
Elements of sparse arrays can be initialized 
by supplying a bracketed index for an element. Successive elements without the
index notation continue to initialize the array in sequence. For example:
.P1
byte a[256] = {
		['a']	'A',	/* Set element 97 to 65 */
		['a'+1]	'B',	/* Set element 98 to 66 */
			'C'	/* Set element 99 to 67 */
};
.P2
If the dimensions of the array are omitted from the
.I array-spec
the compiler sets the size of each dimension to be large enough to accommodate
the initialization. The size of the array in bytes can be found using
.CW sizeof .
.NH 2
Type Declarations
.PP
A type declaration creates a new type and introduces an identifier representing that
type into the language.
.DS
	type-declaration:
		complex \f(CW{\fP memberlist \f(CW}\fP \f(CW;\fP
		enumeration-type
		tupletype

	complex:
		\f(CWadt\fP typename poly-spec\*(Op
		\f(CWaggr\fP typename
		\f(CWunion\fP typename

	poly-spec:
		\f(CW[\fP typelist \f(CW]\fP
.DE
A complex type is composed of a list of members.
Each member may be a complex type,
a derived type or a basic type.
Members are referenced by tag or by type. 
Members without tags are called
.I unnamed.
Arithmetic types, channel types, tuples, and complex types may be
unnamed. Derived types may not be left unnamed.
Complex unnamed members are referenced by type or by implicit promotion
during assignment or when supplied as function arguments. Other unnamed
members allocate storage but may not be referenced.
Complex types are compared by structural rather than name equivalence.
A type declaration must have either a type name or a tag.
.DS
	memberlist:
		member
		memberlist member

	member:
		type \f(CW;\fP
		tname pointer\*(Op decl-tag array-spec\*(Op \f(CW;\fP
		tname decl-tag \f(CW(\fP arglist \f(CW)\fP \f(CW;\fP

	decl-tag:
		identifier
.DE
.I tname
is one of the basic types or a new type introduced by
.CW aggr ,
.CW adt ,
.CW union ,
or
.CW typedef .
.NH 2
Tuples
.PP
A
.CW tuple
is a collection of types forming a single object which can be used in the
place of an unnamed complex type. The individual members of a tuple
can only be accessed by assignment.
.DS
	tuple:
		\f(CW(\fP tlist \f(CW)\fP

	tlist:
		tlist \f(CW,\fP expression
.DE
When the declaration of a tuple would be ambiguous
because of the parenthesis (for instance in the declaration of an automatic variable)
use the keyword
.CW tuple :
.P1
void
f()
{
	int a;
	tuple (int, byte, Rectangle) b;
	int c;
}
.P2
Type checking
of tuple expressions is performed by matching the
.I shape
of each of the component types.
Tuples may only be addressed by assignment into other complex
types or l-valued tuple expressions. A bracketed list of expressions
forms a tuple constructor, while a list of l-valued expressions on the left
hand side forms a destructor.
For example, to make a function return multiple values:
.P1
(int, byte*, byte)
func()
{
	return (10, "hello", 'c');
}

void
main()
{
	int	a;
	byte*	str;
	byte	c;
	(a, str, c) = func();
}
.P2
When a tuple appears as the left-hand side of an assignment, type
checking proceeds
as if each individual member of the tuple were an assignment statement to the
corresponding member of the complex type on the right-hand side. If a tuple
appears on the right hand side of an assignment where the left-hand side
yields a complex type then the types of each individual member of the tuple
must match the
corresponding types of the complex type exactly. If a tuple is cast into
a complex type then each member of the tuple will be converted into the
type of the corresponding member of the complex type under the rules of
assignment.
.P1
aggr X
{
	int	a;
	byte	b;
};

void
main()
{
	X x;
	byte c;

	x = (10, c);      /* Members match exactly */
	x = (X)(10, 1.5); /* float is converted to byte */	
}
.P2
.NH 2
Abstract Data Types
.PP
An abstract data type (ADT) defines both storage for members, as in an aggregate,
and the operations that can be performed on that type.
Access to the members of an abstract data type is restricted
to enable information hiding.
The scope of the members of an abstract data type depends on their
type. By default access to members that define data is limited to the member
functions. Members can be explicitly exported from the type using the
.CW extern
storage class in the member declaration. Member functions are visible by default,
the opposite behavior of data members. Access to a member function
may be restricted to other member functions by qualifying the declaration
with the
.CW intern
storage class.
The four combinations are:
.P1
adt Point
{
		int	x; /* Access only by member fns */
	extern	int	y; /* by everybody */

		Point set(Point*); /* by everybody */
	intern	Point tst(Point);  /* only by member fns */
};
.P2
Member functions are defined by type and name. The pair forms a unique name for
the function, so the same member function name can be used in many types.
Using the last example, the member function
.CW set
could be defined as:
.P1
Point
Point.set(Point *a)
{
	a->x = 0;	/* Set Point value to zero */
	a->y = 0;

	return *a;
}
.P2
An implicit argument of either a pointer to the ADT or the value of the ADT 
may be passed to a member function.
If the first argument of the member function declaration in the ADT
specification is
.CW *
.I typename
(with the
.CW *
preceding the name),
then a
.I pointer
to the ADT is automatically passed as the first parameter, similarly to the
.CW self
construct
in Smalltalk.
If the declaration is of the form
.CW .
.I typename
then the
.I value
of the ADT will be passed to the member function.
.P1
adt Point
{
		int	x;
	extern	int	y;

 		/* Pass &Point as 1st arg */
		Point set(*Point);
		/* Pass Point as 1st arg */
		Point clr(.Point);
	intern	Point tst(Point);
};

void
func()
{
	Point p;

	p.set();	/* Set receives &p as 1st arg */
}
.P2
The receiving function is defined as:
.P1
Point
Point.set(Point *p)
{
	...
}
.P2
.NH 2
Polymorphic and Parameterized Abstract Data Types
.PP
Alef allows the construction of type parameterized abstract data types,
similar to
.I generic
abstract data types in Ada and Eiffel.
An ADT is parameterized by supplying type parameter names in the declaration.
The type parameters may be used to specify the types of members of the ADT.
The argument type names have the same effect as a
.CW typedef
to the polymorphic type. The scope of the types supplied as arguments is the same
as the ADT
.I typename 
and can therefore be used as a type specifier in simple declarations.
For example the definition of a stack type of parameter type
.CW T
may be defined as:
.P1
adt Stack[T]
{
	int	tos;
	T	data[100];
	void	push(*Stack, T);
	T	pop(*Stack);
};
.P2
Member functions of
.CW Stack
are written in terms of the parameter type
.CW T .
The implementation of
.CW push
might be:
.P1
void
Stack.push(Stack *s, T v)
{
	s->data[s->tos++] = v;
}
.P2
The
.CW Stack
type can be instantiated in two forms. In the
.I bound
form, a type is specified for
.CW T .
The program is type checked as if the supplied type were substituted for
.CW T 
in the ADT declaration. For example:
.P1
Stack[int] stack;
.P2
declares a stack where each element is an
.CW int .
In the bound form a type must be supplied for each parameter type.
In the
.I unbound
form no parameter types are specified. This allows values of any type to be stored
in the stack. For example:
.P1
Stack poly;
.P2
declares a stack where each element has polymorphic type.
.NH 2
Enumeration Types
.PP
.DS
	enumeration-type:
		\f(CWenum\fP typename \f(CW{\fP enum-list \f(CW}\fP \f(CW;\fP

	enum-list:
		identifier
		identifier \f(CW=\fP constant-expression
		enum-list , enum-list
.DE
Enumerations are types whose value is limited to a set of integer constants.
These constants, the members of the enumeration, are called enumerators.
Enumeration variables are equivalent to integer variables.
Enumerators may appear wherever an integer
constant is legal. If the values of the enumerators are not
defined explicitly, the compiler assigns
incrementing values starting from 0. If a value is given to an enumerator, values are
assigned to the following enumerators by incrementing the value for each
successive member until the next assigned value is reached.
.NH 2
Type Definition
.PP
Type definition allows derived types to be named, basic types to be renamed,
polymorphic types to be named, and forward referencing between complex types.
.DS
	type-definition:
		\f(CWtypedef\fP tname identifier \f(CW;\fP
		\f(CWtypedef\fP polyname \f(CW;\fP
.DE
If
.I tname
is omitted then the identifier,
.I polyname ,
becomes a polymorphic type specifier.
To declare complex types
with mutually dependent pointers,
it is necessary to use a
.CW typedef
to predefine one of the types.
Alef does not permit mutually dependent complex types, only references between
them. For example:
.P1
typedef aggr A;

aggr B
{
	A	*aptr;
	B	*bptr;
};

aggr A
{
	A	*aptr;
	B	*bptr;
};
.P2
.NH 2
Function Declarations
.PP
There are three forms of function declaration: function definition,
prototype declaration, and function pointer declaration.
.DS
	function-declaration:
		type-specifier identifier \f(CW(\fP arglist \f(CW)\fP block

	function-id:
		pointer\*(Op identifier array-spec\*(Op
		adt-function

	adt-function:
		typename \f(CW.\fP decl-tag

	arglist:
		arg
		pointer type
		arglist , arg

	arg:
		type
		type pointer
		type \f(CW(\fP pointer \f(CW)\fP \f(CW(\fP arglist \f(CW)\fP
		type simple-declaration
		\f(CW...\fP
.DE
If a formal parameter is declared without an identifier, no variable
corresponding to the actual parameter is produced.
.NH
Expressions
.PP
The order of expression evaluation is not defined except where noted. That is,
unless the definition of the operator guarantees evaluation order, an operator
may evaluate any of its operands first.
.PP
The behavior of exceptional conditions such as divide by zero,
arithmetic overflow, and floating point exceptions is not defined.
.NH 2
Pointer Generation
.PP
References to expressions of type `function returning T' and `array of T' 
are rewritten to produce pointers to either the function or the first element of
the array. That is `function returning T'
becomes `pointer to function returning T'
and `array of T' becomes `pointer to the first element of array of T'.
.NH 2
Primary Expressions
.PP
Primary expressions are identifiers, constants, or parenthesized expressions:
.DS
	primary-expression:
		identifier
		constant
		\f(CW...\fP
		\f(CWnil\fP
		\f(CW(\fP expression \f(CW)\fP
		tuple
.DE
The parameters received by a function taking variable
arguments are referenced using the ellipsis
.CW ... .
The primary-expression 
.CW ...
yields a value of type `pointer to void'.
The value points at the first location after the formal parameters.
The primary-expression
.CW nil
returns a pointer of type `pointer to void' of value 0
which is guaranteed not to point at an object.
.CW nil
may also be used to initialize channels and polymorphic types
to a known value. The only legal operation on these types after
such an assignment is a test with one of the equality test operators
and the
.CW nil
value.
.NH 2
Postfix Expressions
.DS
	postfix-expression:
		primary-expression
		postfix-expression \f(CW[\fP expression \f(CW]\fP
		postfix-expression \f(CW(\fP argument-list \f(CW)\fP
		\f(CW.\fP typename \f(CW.\fP tag \f(CW(\fP argument-list \f(CW)\fP
		postfix-expression \f(CW.\fP tag
		postfix-expression \f(CW->\fP tag 
		postfix-expression \f(CW++\fP
		postfix-expression \f(CW--\fP
		postfix-expression \f(CW?\fP

	tag:
		typename
		identifier

	argument-list:
		expression
		argument-list \f(CW,\fP expression
.DE
.NH 3
Array Reference
.PP
A primary expression followed by an expression enclosed in square brackets is an
array indexing operation. The expression is rewritten to be
\f(CW*((\fIpostfix-expression\f(CW)+(\fIexpression\f(CW))\fR.
One of the expressions must be of type
pointer, the other of integral type.
.NH 3
Function Calls
.PP
The
.I postfix-expression
must yield a value of type `pointer to function'. A type
declaration for the function must be declared prior to a function call. The
declaration can be either the definition of the function or a function prototype.
The types of each argument in the prototype must match the corresponding
expression type under the rules of promotion and conversion for assignment.
In addition unnamed complex type members will be promoted automatically.
For example:
.P1
aggr Test
{
	int	t;
	Lock;		/* Unnamed substructure */
};

Test yuk;		/* Definition of complex object yuk */ 
void lock(Lock*);	/* Prototype for function lock */

void
main()
{
	lock(&yuk);	/* address of yuk.Lock is passed */
}
.P2
.PP
Calls to member functions may use the type name instead of
an expression to identify the ADT.  If the function has an implicit
first parameter,
.CW nil
is passed.  Given the following definition of
.CW X
these two calls are equivalent:
.P1
adt X
{
	int	i;
	void	f(*X);
};

X val;

	((X*)nil)->f();
	.X.f();
.P2
This form is illegal if the implicit parameter is declared by value
rather than by reference.
.PP
Calls to member functions of polymorphic ADT's have special promotion
rules for function arguments. If a polymorphic type
.CW P
has been bound to an actual type
.CW T
then an actual parameter
.CW v
of type
.CW T
corresponding to a formal parameter of type
.CW P
will be promoted into type
.CW P
automatically. The promotion is equivalent to
.CW "(alloc P)v"
as described in the Casts section. For example:
.P1
adt X[T]
{
	void	f(*X, T);
};

X[int] bound;

bound.f(3);		/* 3 is promoted as if (alloc T)3 */
bound.f((alloc T)3);	/* illegal: int not same as poly */
.P2
In the unbound case values must be explicitly converted into the polymorphic
type using the cast syntax:
.P1
X unbound;

unbound.f((alloc T)3);	/* 3 is converted into poly */
unbound.f(3);		/* illegal: int not same as poly */
.P2
In either case the actual parameter must have the same type as the
formal parameter after any binding has taken place.
.NH 3
Complex Type References
.PP
The operator
.CW .
references a member of a complex type. The first part of the expression must yield
.CW union ,
.CW aggr ,
or
.CW adt .
Named members must be specified by name, unnamed members by type.
Only one unnamed member of type
.I typename
is permitted in the complex type when referencing members by type, 
otherwise the reference would be ambiguous.
If the reference is by
.I typename
and no members of
.I typename
exist in the complex,
unnamed substructures will be searched breadth first. The operation
.CW ->
uses a
pointer to reference a complex type member. The
.CW ->
operator follows the same search and
type rules as
.CW .
and is equivalent to the expression
.I (*postfix-expression).tag .
.PP
References to polymorphic members of unbound polymorphic ADT's behave as
normal members: they yield an unbound polymorphic type. Bound polymorphic
ADT's have special rules. Consider a polymorphic type
.CW P
that is bound to an actual type
.CW T .
If a reference to a member or function return value of type
.CW P
is assigned to a variable
.CW v
of type
.CW T
using the assignment operator
.CW = ,
then the type of
.CW P
will be narrowed to
.CW T ,
assigned to
.CW v ,
and the storage used by the polymorphic value will be unallocated.
The value assignment operator
.CW :=
performs the same type narrowing but does not unallocate the storage
used by the polymorphic value. For example:
.P1
adt Stack[T]
{
	int	tos;
	T	data[100];
};

Stack[int] s;
int i, j, k;

i := s.data[s->tos];
j = s.data[s->tos];
k = s.data[s->tos];	/* illegal */
.P2
The first assignment copies the value at the top of the stack into
.CW i
without altering the data structure.
The second assignment moves the value into
.CW j
and unallocates the storage used in the stack data structure.
The third assignment is illegal since
.CW data[s->tos]
has been unallocated.
.NH 3
Postfix Increment and Decrement
.PP
The postfix increment (
.CW ++
) and decrement (
.CW --
) operators return the value of
.I expression ,
then increment it or decrement it by 1. The expression must be
an l-value of integral or pointer type.
.NH 2
Unary Operators
.PP
The unary operators are:
.DS
	unary-expression:
		postfix-expression
		\f(CW<-\fP unary-expression
		\f(CW++\fP unary-expression
		\f(CW--\fP unary-expression
		unary-operator cast-expression
		\f(CWsizeof\fP cast-expression
		\f(CWzerox\fP unary-expression

	unary-operator: one of
		\f(CW?\fP   \f(CW*\fP   \f(CW!\fP
		\f(CW+\fP   \f(CW-\fP   \f(CW~\fP
.DE
.NH 2
Prefix Increment and Decrement
.PP
The prefix increment (
.CW ++
) and prefix decrement (
.CW --
) operators add or subtract one to a
.I unary-expression
and return the new value. The
.I unary-expression
must be an l-value of integral or pointer type.
.NH 2
Receive and Can Receive
.PP
The prefix operator
.CW <-
receives a value from a channel.
The
.I unary-expression
must be of type `channel of T'.
The type of the result will be T. A process or task will block
until a value is available from the channel. The prefix operator
.CW ?
returns 1 if a channel
has a value available for receive, 0 otherwise.
.NH 2
Send and Can send
.PP
The postfix operator
.CW <- ,
on the left-hand side of an assignment (see the section called Assignment),
sends a value to a channel, for example:
.P1
	chan(int) c;

	c <-= 1;	/* send 1 on channel c */
.P2
The postfix operator
.CW ?
returns 1 if a thread can send on a channel
without blocking, 0 otherwise.
.PP
The prefix or postfix blocking test operator
.CW ?
is only reliable when used on a channel shared between tasks in a single
process.
A process may block after a successful
.CW ?
because there may be a race between processes competing for the same channel.
.NH 2
Indirection
.PP
The unary operator
.CW *
retrieves the value pointed to by its operand.
The operand must be of type `pointer to T'.
The result of the indirection is a value of type T.
.NH 2
Unary Plus and Minus
.PP
Unary plus is equivalent to
\f(CW(0+(\fIunary-expression\f(CW))\fR.
Unary minus is equivalent to
\f(CW(0-(\fIunary-expression\f(CW))\fR.
An integral operand undergoes integral
promotion. The result has the type of the promoted operand.
.NH 2
Bitwise Negate
.PP
The operator
.CW ~
performs a bitwise negation of its operand,
which must be of integral type.
.NH 2
Logical Negate
.PP
The operator 
.CW !
performs logical negation of its operand, which
must of arithmetic or pointer type.
If the operand is a pointer and its value is
.CW nil
the result is
integer 1, otherwise 0. If the operand is arithmetic and the value is 0 the
result is 1, otherwise the result is 0.
.NH 2
Zerox
.PP
The
.CW zerox
operator may only be applied to an expression of polymorphic type.
The result of
.CW zerox
is a new fat pointer, which points at a copy of the result of evaluating
.I unary-expression .
For example:
.P1
	typedef Poly;
	Poly a, b, c;

	a = (alloc Poly)10;
	b = a;
	c = zerox a;	
.P2
causes
.CW a
and
.CW b
to point to the same storage for the value 10 and
.CW c
to point to distinct storage containing another copy of the value 10.
.NH 2
Sizeof Operator
.PP
The
.CW sizeof
operator yields the size in bytes of its operand, which may be an
expression or the parenthesized name of a type. The size is determined
from the type of the operand, which is not itself evaluated. The result
is a signed integer constant.
If
.CW sizeof
is applied to a string constant the result is the number of bytes required to
store the string including its terminating NUL byte or zero rune.
.NH 2
Casts
.PP
A cast converts the result of an expression into a new type:
.DS
	cast-expression:
		unary-expression
		\f(CW(\fP type-cast \f(CW)\fP cast-expression
		\f(CW( alloc\fP polyname \f(CW)\fP cast-expression

	type-cast:
		type pointer
		function-prototype
		\f(CWtuple\fP tuple
.DE
A value of any type may be converted into a polymorphic type
by adding the keyword
.CW alloc
before the polymorphic type name.
This has the effect of allocating storage for the value, assigning the value
of
.I cast-expression
into the storage, and yielding a fat pointer as the result.
For example, to create a polymorphic variable with integer value 10:
.P1
	typedef Poly;
	Poly p;

	p = (alloc Poly) 10;
.P2
The only other legal cast involving a polymorphic type converts one
.I polyname
into another.
.NH 2		
Multiply, Divide and Modulus
.PP
The multiplicative operators are:
.DS
	multiplicative-expression:
		cast-expression
		multiplicative-expression \f(CW*\fP multiplicative-expression
		multiplicative-expression \f(CW/\fP multiplicative-expression
		multiplicative-expression \f(CW%\fP multiplicative-expression
.DE
The operands of
.CW *
and
.CW /
must have arithmetic type. The operands of
.CW %
must be of integral type. The operator 
.CW /
yields the quotient, 
.CW %
the remainder, and
.CW *
the product of the operands. If
.CW b
is non-zero then 
.CW "a == (a/b) + a%b
should always be true.
.NH 2
Add and Subtract
.PP
The additive operators are:
.DS
	additive-expression:
		multiplicative-expression
		additive-expression \f(CW+\fP multiplicative-expression
		additive-expression \f(CW-\fP multiplicative-expression
.DE
The
.CW +
operator computes the sum of its operands. Either one of the operands may be
a pointer. If
.I P
is an expression yielding a pointer to type
.I T
then
.CW P+\fIn\fP
is the same as
\fIp\f(CW+(sizeof(T)*\fIn\f(CW)\fR.
The
.CW -
operator computes the difference of its operands. The first operand may
be of pointer or arithmetic type. The second operand must be of arithmetic type.
If
.I P
is an expression yielding a pointer of type
.I T
then
.CW P-\fIn\fP
is the same as
\fIp\f(CW-(sizeof(T)*\fIn\f(CW)\fR.
Thus if
.I P
is a pointer to an element of an array,
\fIP\f(CW+1\fR
will point to the next object in the array and
\fIP\f(CW-1\fR
will point to the previous object in the array.
.NH 2
Shift Operators
.PP
The shift operators perform bitwise shifts:
.DS
	shift-expression:
		additive-expression
		shift-expression << additive-expression
		shift-expression >> additive-expression
.DE
If the first operand is unsigned,
.CW <<
performs a logical left shift by
.I additive-expression
bits. If the first operand is signed,
.CW <<
performs an arithmetic left shift by
.I additive-expression
bits.
The
.I shift-expression
must be of integral type.
The
.CW >>
operator is a right shift and follows the same rules as left shift.
.NH 2
Relational Operators
.PP
The values of expressions can be compared as follows:
.DS
	relational-expression:
		shift-expression
		relational-expression \f(CW<\fP shift-expression
		relational-expression \f(CW>\fP shift-expression
		relational-expression <= shift-expression
		relational-expression >= shift-expression
.DE
The operators are
.CW <
(less than),
.CW >
(greater than),
.CW <=
(less than or equal to) and
.CW >=
(greater than or equal to). The operands must be of arithmetic or pointer type.
The value of the expression is 1 if the relation is true, otherwise 0.
The usual arithmetic conversions are performed.
Pointers may only be compared to pointers of the same
type or of type
.CW void* .
.NH 2
Equality operators
.PP
The equality operators are:
.DS
	equality-expression:
		relational-expression
		relational-expression \f(CW==\fP equality-expression
		relational-expression \f(CW!=\fP equality-expression
.DE
The operators
.CW ==
(equal to) and
.CW !=
(not equal) follow the same rules as relational operators. The equality
operations may be applied to expressions yielding channels and polymorphic types
for comparison with the value
.CW nil .
A pointer of value
.CW nil
or type
.CW void*
may be compared to any pointer. 
.NH 2
Bitwise Logic Operators
.PP
.DS
	AND-expression:
		equality-expression
		AND-expression \f(CW&\fP equality-expression

	XOR-expression:
		AND-expression
		XOR-expression \f(CW^\fP AND-expression

	OR-expression:
		XOR-expression
		OR-expression \f(CW|\fP XOR-expression
.DE
The operators perform bitwise logical operations and apply only to
integral types. The operators are
.CW &
(bitwise and),
.CW ^
(bitwise exclusive or) and
.CW |
(bitwise inclusive or).
.NH 2
Logical Operators
.PP
.DS
	logical-AND-expression:
		OR-expression
		logical-AND-expression \f(CW&&\fP OR-expression

	logical-OR-expression:
		logical-AND-expression
		logical-OR-expression \f(CW||\fP logical-AND-expression
.DE
The
.CW &&
operator returns 1 if both of its operands evaluate to non-zero, otherwise 0.
The
.CW ||
operator returns 1 if either of its operand evaluates to non-zero, otherwise 0.
Both operators are guaranteed to evaluate strictly left to right. Evaluation
of the expression will cease as soon the final value is determined.
The operands can be any mix of arithmetic and pointer types.
.NH 2
Constant expressions
.PP
A constant expression is an expression which can be fully evaluated by the
compiler during
translation
rather than at
runtime.
.DS
	constant-expression:
		logical-OR-expression
.DE
.I Constant-expression
appears as part of initialization, channel buffer specifications, and array
dimensions. The following operators may not be part of a
constant expression:
function calls, assignment, send, receive, increment and decrement.
Address computations using the
.CW &
(address of) operator on static declarations is permitted. 
.NH 2
Assignment
.PP
The assignment operators are:
.DS	
	assignment-expression:
		logical-OR-expression
		unary-expression \f(CW<-=\fP assignment-expression
		unary-expression assignment-operator assignment-expression
		unary-expression \f(CW=\fP \f(CW(\fP type-cast \f(CW)\fP tuple

	assignment-operator: one of
		\f(CW=\fP  \f(CW:=\fP  \f(CW+=\fP  \f(CW*=\fP  \f(CW/=\fP  \f(CW-=\fP \f(CW%=\fP  \f(CW&=\fP  \f(CW|=\fP  \f(CW^=\fP  \f(CW>>=\fP  \f(CW<<=\fP
.DE
The left side of the expression must be an l-value.
Compound assignment allows the members of a complex type to be assigned
from a member list in a single statement. A compound assignment is formed
by casting a tuple into the complex type.
Each element of the tuple is evaluated in turn and assigned to its
corresponding element in the complex types. The usual conversions are
performed for each assignment.
.P1
/* Encoding of read message to send to file system */
aggr Readmsg
{
	int	fd;
	void	*data;
	int	len;
};

chan (Readmsg) filesys;

int
read(int fd, void *data, int len)
{
	/* Pack message parameters and send to file system */
	filesys <-= (Readmsg)(fd, data, len);	
}
.P2
If the left side of an assignment is a tuple, selected members may be
discarded by placing 
.CW nil
in the corresponding position in the tuple list. In the following
example only the first and third integers returned from
.CW func
are assigned.
.P1
(int, int, int)	func();

void
main()
{
	int a, c;

	(a, nil, c) = func();
}
.P2
.PP
The
.CW <-=
(assign send) operator sends the value of the right side into a channel.
The
.I unary-expression
must be of type `channel of T'.
If the left side of the expression is of type `channel of T', the value transmitted
down the channel is the same as if the expression were `object of type T = expression'.
.NH 3
Promotion
.PP
If the two sides of an assignment yield different complex types then assignment
promotion is performed. The type of the right hand side is searched for
an unnamed complex type under the same rules as the
.CW .
operator.
If a matching type is found it is assigned to the left side. This promotion
is also performed for function arguments.
.NH 3
Polymorphic Assignment
.PP
There are two operators for assigning polymorphic values. The reference assignment
operator
.CW =
copies the fat pointer. For example:
.P1
	typedef Poly;
	Poly a, b;
	int i;

	a = (alloc Poly)i;
	b = a;
.P2
causes
.CW a
to be given a fat pointer to a copy of the variable
.CW i
and
.CW b
to have a distinct fat pointer pointing to the same copy of
.CW i .
Polymorphic variables assigned with the
.CW =
operator must be of the same polymorphic name.
.PP
The value assignment operator
.CW :=
copies the value of one polymorphic variable to another.
The variable and value must be of the same polymorphic name and
must represent values of the same type; there is no implicit type promotion.
In particular, the variable being
assigned to must already be defined, as it must have both a type and storage.
For example:
.P1
	typedef Poly;
	Poly a, b, c;
	int i, j;

	a = (alloc Poly)i;
	b = (alloc Poly)j;
	b := a;
	c := a;			/* illegal */
.P2
causes
.CW a
to be given a fat pointer to a copy of the variable
.CW i
and
.CW b
to be given a fat pointer to a copy of the variable
.CW j .
The value assignment
.CW b:=a
copies the value of
.CW i
from the storage referenced by the fat pointer of
.CW a
to the storage referenced by
.CW b ,
with the result being that
.CW a
and
.CW b
point to distinct copies of the value of
.CW i ;
the reference to the value of
.CW j
is lost.
The assignment
.CW c:=a
is illegal because
.CW c
has no storage to hold the value;
.CW c
is in effect an uninitialized pointer.
.PP
A polymorphic variable may be assigned the value
.CW nil .
This assigns the value 0 to the pointer element of the
fat pointer but leaves the type field unmodified.
.NH 2
Iterators
.PP
The iteration operator causes repeated execution of
the statement that contains
the iterating expression
by constructing a loop surrounding that statement.
.DS
	expression:
		assignment-expression
		assignment-expression \f(CW::\fP assignment-expression
.DE
The operands of the iteration operator are the integral bounds of the loop.
The iteration counter may be made explicit by assigning the value of
the iteration expression to an integral variable; otherwise it is implicit.
The two expressions are evaluated before iteration begins. The iteration is
performed while the iteration counter is less than the value of the second
expression (the same convention as array bounds).
When the counter is explicit, its value is available throughout the statement.
For example, here are two
implementations of a string copy function:
.P1
void
copy(byte *to, byte *from)
{
	to[0::strlen(from)+1] = *from++;
}
.P2
.P1
void
copy(byte *to, byte *from)
{
	int i;

	to[i] = from[i=0::strlen(from)+1];
}
.P2
If iterators are nested, the order
of iteration is undefined.
.NH 2
Binding and Precedence
.PP
The binding and precedence of the operators is in decreasing order:
.TS
box, center, tab(@);
c | l
c | lFCW .
binding @ operator
_
l to r @ ()  []  ->  .
_
r to l @ !  ~  ++  --  <-  ? +  -  *  &  (\f2cast\fP) sizeof zerox
_
l to r @ *  /  %
_
l to r @ +  -
_
l to r @ <<  >>
_
r to l @ ::
_
l to r @ <  <=  >  >=
_
l to r @ ==  !=
_
l to r @ &
_
l to r @ ^
_
l to r @ |
_
l to r @ &&
_
l to r @ ||
_
l to r @ <-=  = :=  +=  -=  *=  /=  %=  \&=  ^=  |=  <<=  >>=
.TE
.NH
Statements
.PP
Statements are executed for effect, and do not yield values. Statements fall
into several groups:
.DS
	statement:
		expression \f(CW;\fP
		label-statement \f(CW:\fP
		block-statement
		selection-statement \f(CW;\fP
		loop-statement
		jump-statement
		exception-statement
		process-statement \f(CW;\fP
		allocation-statement \f(CW;\fP
.DE
.NH 2
Label Statements
.PP
A statement may be prefixed by an identifier.
The identifier labels the statement and may be used as the destination of a
.CW goto .
Label and exception identifiers have their own name space and do not
conflict with other names. Labels have function scope.
.NH 2
Expression Statements
.PP
Most expressions statements are function calls or assignments. Expressions may be
null. Null expressions are often useful as empty bodies to labels or iteration
statements.
.NH 2
Block Statements
.PP
Several statements may be grouped together to form a block. The body of a function is
a block.
.DS
	block:
		\f(CW{\fP autolist slist \f(CW}\fP
		\f(CW!{\fP autolist slist \f(CW}\fP

	autolist:
		declaration
		autolist declaration

	slist:
		statement
		slist statement
.DE
An identifier declared in
.I autolist
suspends any previous declaration of the same identifier.
An identifier may be declared only once per block. The declaration remains
in force until the end of the block, after which any suspended declaration
comes back into effect.
.PP
The value of identifiers declared in
.I autolist
is undefined at block entry and should be assigned to a known value after
declaration but before use.
.PP
The symbol
.CW !{
introduces a guarded block. Only one thread may be executing the statements
contained in the guarded block at any instant.
.NH 2
Selection Statements
.PP
Selection statements alter the flow of control based on the value of an expression.
.DS
	selection-statement:
		\f(CWif\fP \f(CW(\fP expression \f(CW)\fP statement \f(CWelse\fP statement
		\f(CWif\fP \f(CW(\fP expression \f(CW)\fP statement
		\f(CWswitch\fP expression cbody
		\f(CWtypeof\fP expression cbody
		\f(CWalt\fP cbody

	cbody:
		\f(CW{\fP caselist \f(CW}\fP
		\f(CW!{\fP caselist \f(CW}\fP

	caselist:
		case-item
		alt-item
		type-item
		caselist case-item

	case-item:
		\f(CWcase\fP constant-expression \f(CW:\fP statement
		\f(CWdefault\fP \f(CW:\fP statement

	alt-item:
		\f(CWcase\fP expression \f(CW:\fP statement
	
	type-item:
		\f(CWcase\fP ptr-type \f(CW:\fP statement	
.DE
An
.CW if
statement first evaluates
.I expression,
which must yield a value
of arithmetic or pointer type.
The value of
.I expression
is compared with 0. If it compares unequal
.I statement
is executed. If an
.CW else
clause is supplied and the value compares equal the
.CW else
statement will be executed. The
.CW else
clause shows an ambiguity in the grammar. The ambiguity is
resolved by matching an
.CW else
with the nearest
.CW if
without an
.CW else
at the same block level.
.PP
The
.CW switch
statement selects one of several statements based on the value of
.I expression .
The
.I expression
is evaluated and converted into an integer. The integer is compared with the value
specified in each
.CW case .
If the integers compare, control is transferred to the statement after the matching
.CW case .
If no
.CW case
is matched, the
.CW default
clause is executed. If the
.CW default
is omitted then none of the
.CW case
statements is executed.
The
.CW case
expression must yield an integer constant.
For a single
.CW switch
statement each case expression must yield a unique value.
.PP
Within a
.CW switch ,
.CW alt ,
or
.CW typeof
execution proceeds normally except that a
.CW break
statement will terminate the selection statement.
.PP
The
.CW typeof
statement selects one of several statements based on the type of
.I expression .
The
.I expression
must be of polymorphic type.
The expression is evaluated and the resulting type is compared with the type
specified by each
.CW case .
If the types match, the statement part of the corresponding
.CW case
is executed.
All the cases must have a distinct type within a single
.CW typeof
statement. If no
.CW case
is matched, the
.CW default
clause is executed, if one exists; otherwise
none of the
.CW case
statements is executed. If the
.I expression
is a simple variable, then within the statement supplied by the
.CW case ,
the value is narrowed to the type of that
.CW case .
In the
.CW default
case where
.I expression
is a simple variable the type remains polymorphic.
.P1
	typeof v {
	case int:
		print("int=%d", v);   /* v passed as int */
		break;
	case float:
		print("float=%f", v); /* v passed as float */
		break;
	default:
		usertype(v);	  /* still polymorphic */
		break;
	}
.P2
The
.CW typeof
statement is the only way to narrow the type
of a polymorphic value.
.PP
The
.CW alt
statement allows threads to perform communication on several channels
simultaneously without polling. The expression in each
.CW case
of an
.CW alt
must contain either a send or receive operation. The
.CW alt
statement provides a fair select between ready channels.
A thread will remain blocked in
.CW alt
until one of the
.CW case
expressions can be evaluated without blocking. The
.CW case
expression may be evaluated more than once, therefore care should be
taken when using expressions which have side effects. If several of the
.CW case
expressions are ready for evaluation one is chosen at random. A
.CW break
statement terminates each case of the
.CW alt .
If the
.CW break
statement is omitted execution will proceed to execute the communication
of the next case regardless of its readiness to communicate.
For example:
.P1
	chan(Mesg) keyboard, mouse;
	Mesg m;

	alt {
	case m = <-keyboard:
		/* Process keyboard event */
		break;
	case m = <-mouse:
		/* Process mouse event */
		break;
	}
.P2
.PP
The
.CW alt
statement is also used to discriminate between the type of values received
from channels of variant protocols. In this form each
.I case-item
of the
.CW alt
must be a simple assignment.
The right hand side must contain a communication operation on a channel which
supplies a variant protocol. The type of the l-value is used to match a
type in the variant protocol.
An
.CW alt
may be performed on an arbitrary set of variant protocol channels so long
as no type is supplied by more than one channel. There must be a
.CW case
clause for each type supplied by the union of all channel types;
Alef requires the match against types to be exhaustive.
For example:
.P1
	Aggr1 a;
	Aggr2 b;
	Aggr3 c;

	chan (Aggr1, Aggr2, Aggr3) ch;

	alt {
	case a = <-ch:
		print("received Aggr1");
		break;
	case b = <-ch:
		print("received Aggr2");
		break;
	case c = <-ch:
		print("received Aggr3");
		break;
	}
.P2
.PP
If an
.CW alt
is pending on a channel the programmer must ensure that other
threads do not perform an operation of the same type on the channel
until the
.CW alt
is complete.
Otherwise the
.CW alt
on that channel may block if values are removed by the other thread.
.PP
The symbol
.CW !{
introduces a guarded
.I caselist .
Only one thread may be executing the statements contained in the guarded
caselist at any instant.
.NH 2
Loop Statements
.PP
Several loop constructs are provided:
.DS
	loop-statement:
		\f(CWwhile\fP \f(CW(\fP expression \f(CW)\fP statement
		\f(CWdo\fP statement \f(CWwhile\fP \f(CW(\fP expression \f(CW)\fP \f(CW;\fP
		\f(CWfor\fP \f(CW(\fP expression \f(CW;\fP expression \f(CW;\fP expression \f(CW)\fP statement
.DE
In
.CW while
and
.CW do
loops the statement is repeated until the expression evaluates to 0.
The expression must yield either an arithmetic or pointer type. In the
.CW while
loop the expression is evaluated and tested before the statement. In the
.CW do
loop the statement is executed before the expression is evaluated and tested.
.PP
In the
.CW for
loop the first expression is evaluated once before loop entry.
The expression is usually used to initialize the loop variable.
The second expression is evaluated at the beginning of each loop iteration,
including the first. The expression must yield either a pointer
or arithmetic type. The statement is executed while the evaluation of the second
expression does not compare to 0. The third expression is evaluated after the
statement on each loop iteration. The first and third expressions have no type
restrictions. All of the expressions are optional. If the second expression is
omitted an expression returning a non-zero value is implied.
.NH 2
Jump Statements
.PP
Jump statements transfer control unconditionally.
.DS
	jump-statement:
		\f(CWgoto\fP identifier \f(CW;\fP
		\f(CWcontinue\fP count\*(Op \f(CW;\fP
		\f(CWbreak\fP count\*(Op \f(CW;\fP
		\f(CWreturn\fP expression\*(Op \f(CW;\fP
		\f(CWbecome\fP expression \f(CW;\fP

	count:
		integer-constant
.DE
.CW goto
transfers control to the label
.I identifier ,
which must be in the current function.
.NH 3
Continue Statements
.PP
The
.CW continue
statement may only appear as part of a loop statement.
If
.I count
is omitted the
.CW continue
statement transfers control to the loop-continuation
portion of the smallest enclosing iteration statement,
that is, the end of that loop.
If
.I count
is supplied
.CW continue
transfers control to the loop continuation of some
outer nested loop.
.I Count
specifies the number of loops to skip. The statement
.CW continue
with no
.I count
is the same as
.CW continue
.CW 1 .
For example:
.P1
while(1) {
	while(1) {
		continue 2;	/* Same as goto contin; */
	}
contin:				/* Continue comes here */
}
.P2
.NH 3
Break Statements
.PP
Define
.I compound
to be a selection or loop statement.
The
.CW break
statement may only appear as part of such a compound.
If
.I count 
is omitted, then the break terminates
the statement portion of the compound
and transfers control to the statement after the compound.
If
.I count
is supplied, break causes termination of the some nested compound.
.I Count
is the number of nested compounds to terminate.
A
.CW break
with no
.I count
is the same as
.CW break
1.
In a selection statement,
.CW break
terminates execution of the case in the selection, and thus
prevents `falling through' to the next case.
.NH 3
Return Statement
.PP
A function returns to its caller using a
.CW return
statement. An expression is required
unless the function is declared as returning the type
.CW void .
The result of
.I expression
is
evaluated using the rules of assignment to the return type of the function.
.NH 3
Become Statement
.PP
The
.CW become
statement transforms the execution of the
current function into the calculation of the
.I expression 
given as its argument.
If
.I expression
is not itself a function call, then it must have the same type as the
return value of the caller
and the behavior is analogous to a
.CW return
statement.
If, however,
it is a function call, then it need only have the same return
type; the argument list may be different.
When a function
.I P
executes a
.CW become
whose expression is a call of
.I Q ,
the effect is exactly as if the caller of
.I P
had instead called
.I Q
with the appropriate arguments from
.I P .
In particular, the stack frame for
.I P
is overwritten by the frame for
.I Q ;
functions that invoke one another with
.CW become
will execute in
constant stack space.
.NH 2
Exception Statements
.PP
The
.CW rescue ,
.CW raise ,
and
.CW check
statements are provided for use in error recovery:
.DS
	exception-statement:
		\f(CWraise\fP identifier\*(Op \f(CW;\fP
		\f(CWrescue\fP identifier\*(Op block
		\f(CWcheck\fP expression \f(CW;\fP
		\f(CWcheck\fP expression \f(CW,\fP string-constant \f(CW;\fP
.DE
.NH 3
Raise and Rescue Statement
.PP
Under normal execution a
.CW rescue 
block is not executed.  A
.CW raise
after a
.CW rescue
statement transfers
control to the closest previously defined
.CW rescue
statement in the same function. Execution flows through the end of the
.CW rescue
block by default.
.PP
Execution has no effect on the connection
between
.CW raise
and
.CW rescue
statements.
If an identifier is supplied in a
.CW raise
statement, control is transferred to the named
.CW rescue
statement.
For example, these two fragments are
equivalent:
.P1
alloc p;		alloc p;
rescue {		goto notrescue;
	unalloc p;		dorescue:
	raise;				unalloc p;
}					goto nextrescue;
			notrescue:
if(error)		if(error)
	raise;			goto dorescue;
.P2
Multiple
.CW rescue
statements may be cascaded to perform complex error recovery actions:
.P1
alloc a, b;
rescue {
	unalloc a, b;
	return 0;
}

alloc c;
rescue {
	unalloc c;
	raise;
}

dostuff();

if(error)
	raise;
.P2
.NH 3
Check Statement
.PP
The
.CW check
statement tests an assertion.  If the assertion fails, the runtime calls
an error handler. By default the handler writes a message to standard error
and exits with the status
.CW ALEFcheck .
A user-supplied error handler may be installed by setting a handler vector.
The prototype for the vector is:
.P1
void	(*ALEFcheck)(byte*, byte*);
.P2
The first string argument, supplied by the compiler, is of the form
.CW "file:line: func()"
The second argument passed to the handler will be the
.I string-constant
or the default string
.CW "check"
if no string is supplied by the statement.
The expression is evaluated and compared to 0.
If the compare succeeds the assertion has failed.
For example, the runtime checks the return from memory allocation like this:
.P1
ptr = malloc(n);
check ptr != nil, "no memory";
.P2
.NH 2
Process Control Statements
.PP
An Alef program consists of one or more preemptively scheduled processes
called
.I procs ,
each of which consists of one or more coroutines
called
.I tasks .
An Alef program is always executing within some task.
.PP
These statements create procs and tasks:
.DS
	process-statement:
		\f(CWproc\fP function-call \f(CW;\fP
		\f(CWtask\fP function-call \f(CW;\fP
		\f(CWpar\fP block
.DE
The
.CW proc
statement creates a new proc, which starts running the named
function. The arguments to
.I function-call
are evaluated by the original proc.
Since procs are scheduled preemptively, the interleaving of their
execution is undefined. What resources a proc
shares with its parent is defined by the implementation.
A
proc
is initially created with a single task, which begins execution at the function
call.
.PP
The
.CW task
statement creates a coroutine within the same proc,
which begins execution at the function
call. The proc
is maintained until all the tasks have completed. A task completes
when it returns from its initial function or calls the library function
.CW terminate .
All of the tasks within a proc share memory, including access to the
stack of each task.
Tasks are non-preemptive: they are scheduled during message passing
and synchronization primitives only.
In both the
.CW proc
and
.CW task
statements, the function call parameters are evaluated in the original
task.
.PP
The synchronization primitives that can cause task switching are
defined by a library. They are
.CW QLock.lock
and
.CW Rendez.sleep .
The communication operations which can cause task switching are
.CW alt ,
.CW <-=
(send) and
.CW <-
(receive).
A process that contains several tasks will exist until all the tasks within
the process have exited.
In turn, a program will exist until all of the processes in the
program have exited.
A process or task may exit explicitly by calling the function
.CW exits
or by returning from the function in which it was invoked.
.PP
The
.CW par
statement implements fork/process/join. A new process is created for each
statement in the block. The
.CW par
statement completes when all processes have
completed execution of their statements.
A
.CW par
with a single statement is the same as a block.
The processes within a
.CW par
have the same memory sharing model as
procs
and share all automatics and parameters of the function
executing the
.CW par .
.CW Locks
or
.CW QLocks
may be used to synchronize write access to the shared variables.
The process that entered the
.CW par
is guaranteed to be the same process that exits.
.NH 2
Allocation Statements
.PP
Memory management statements allocate and free memory for objects:
.DS
	allocation-statement:
		\f(CWalloc\fP alloclist \f(CW;\fP
		\f(CWunalloc\fP alloclist \f(CW;\fP

	alloclist:
		expression
		alloclist \f(CW,\fP expression
.DE
.NH 3
Alloc Statement
.PP
The
.CW alloc
statement
takes a list of pointers, which must also be l-values.
In strictly left to right order, for each pointer,
memory is reserved for an object of
appropriate type and its address is assigned to the pointer.
The memory is guaranteed to be filled with zeros.
If the allocation fails because there is insufficient memory a
.CW check
condition will be generated with the argument string
.CW "no memory" .
.PP
If the
.I expression
has
.CW chan
type, the runtime system will
also initialize the new channel.
Buffers will be allocated for asynchronous channels.
For example:
.P1
chan(int) a;
chan(int)[10] *p;

alloc a, p, *p;
.P2
To allocate a polymorphic value a cast expression
is used as defined in the section on Casts.
.NH 3
Unalloc Statement
.PP
The
.CW unalloc
statement releases memory. The argument to
.CW unalloc
must have been returned by a successful
.CW alloc
or be
.CW nil .
Unalloc of
.CW nil
has no effect.
If an object is unallocated twice, or an invalid object is unallocated,
the runtime system will generate a
.CW check
condition with the message string
.CW arena
.CW corrupted .
.NH 2
Lock
.PP
The
.CW Lock
ADT provides spin locks.
Two operations are provided.
.CW Lock.lock
claims the lock if free, otherwise it busy loops until the lock becomes
free.
.CW Lock.unlock
releases a lock after it has been claimed.
.PP
Lock ADTs have no runtime state and may be dynamically allocated.
The thread which claimed the lock need not be the thread which unlocks it.
.NH 2
QLock
.PP
The
.CW QLock
ADT provides blocking mutual exclusion.
If the lock is free
.CW QLock.lock
claims the lock.
Further attempts to gain the lock will cause the thread
to be suspended until the lock becomes free. The lock is
released by calling
.CW QLock.unlock .
.PP
The thread which claimed the lock need not be the thread which unlocks it.
.PP
.CW QLock
ADTs have runtime state and may be dynamically allocated provided
they are unallocated only when unlocked.
.NH
The Plan 9 Implementation
.PP
The runtime, support functions, and basic library for Alef are contained in a
single library documented in section 2 of the Plan 9 Programmer's Manual. The
include file
.CW <alef.h>
contains prototypes for the library. A
.CW "pragma lib"
directive tells
.I vl (1)
to load the correct library elements automatically.
The
.CW pragma
directives supported by the compiler
are documented by the compiler manual page
.I alef(1).
The compiler ignores unrecognized
.CW pragma
directives.
.PP
In Plan 9 all procs in an Alef program share the same address space.
When a task performs a system call, all tasks within its proc will
be blocked until the system call completes.
.PP
Programs should not receive notes unless the programmer can guarantee
the runtime system will not be interrupted.
.PP
A channel may be involved in no more than one
.CW alt
at any time.
.PP
The compiler does not support
.CW lint
and
.CW ulint
even though they are defined.
.PP
Stack bounds checking is not implemented.
The external variable
.CW ALEFstack
defines the number of bytes allocated for
the stack of each task.
The
.CW ALEFstack
variable can be changed any time, and is global to all
task creation. The default stack size is 16000 bytes.
The library function
.CW doprint
has a 1024 byte automatic array so
.CW ALEFstack
should not be less than 2048 bytes.
.PP
The runtime system uses the
.I rendezvous (2)
system call to synchronize procs within an Alef program.
The rendezvous tag space is part of the name space group,
so care must be taken in forking the name space with RFNAMEG.
For example after an
.CW rfork(RFNAMEG)
a task cannot synchronize or exchange messages with another proc.
A good example of the code necessary to perform this operation
can be found in the source to
.I acme (1).
.PP
A related issue is that programs that do not fork their name space
may collide with other programs sharing the space, so unless there is strong
reason not to, an Alef program should call
.CW rfork(RFNAMEG)
early.
.PP
The runtime uses the variable
.CW ALEFrfflag
as the argument to
.I rfork (2)
when creating a proc.
By default
.CW ALEFrfflag
is set to
.CW RFNOWAIT|RFMEM|RFPROC .
It may be modified, for example by clearing
.CW RFNOWAIT
to permit a proc to
.I wait (2)
for a child proc to exit.
The value of
.CW ALEFrfflag
should be restored immediately after the
.CW rfork .
.PP
A unique identifier for each task may be found by calling the function
.CW ALEF_tid ,
declared
.P1
	uint	ALEF_tid();
.P2
in
.CW <alef.h> .
The identifier is useful for debugging; for example, it is used in the
.I acid (1)
alef library.
.PP
In the symbol table of the executable,
member functions of ADT types are named by concatenating the ADT name, an underscore, and the member name.
.NH
Yacc Style Grammar
.PP
The following grammar is suitable for implementing a yacc parser. Upper case words
and punctuation surrounded by single quotes are the terminal symbols.
.P1
prog:		decllist

decllist	:
		| decllist decl

decl		: tname vardecllist ';'
		| tname vardecl '(' arglist ')' block
		| tname adtfunc '(' arglist ')' block
		| tname vardecl '(' arglist ')' ';'
		| typespec ';'
		| TYPEDEF ztname vardecl zargs ';'
		| TYPEDEF IDENTIFIER ';'

zargs		:
		| '(' arglist ')'

ztname		: tname
		| AGGR
		| ADT
		| UNION

adtfunc		: TYPENAME '.' name
		| indsp TYPENAME '.' name

typespec	: AGGR ztag '{' memberlist '}' ztag
		| UNION ztag '{' memberlist  '}' ztag
		| ADT ztag zpolytype '{' memberlist '}' ztag
		| ENUM ztag '{' setlist '}'

ztag		:
		| name
		| TYPENAME

zpolytype	:
		| '[' polytype ']'

polytype	: name
		| name ',' polytype

setlist		: sname
		| setlist ',' setlist

sname		:
		| name
		| name '=' expr

name		: IDENTIFIER

memberlist	: decl
		| memberlist decl

vardecllist	:
		| ivardecl
		| vardecllist ',' ivardecl

ivardecl	: vardecl zinit

zinit		:
		| '=' zelist

zelist		: zexpr
		| '[' expr ']' expr
		| '.' stag expr
		| '{' zelist '}'
		| '[' expr ']' '{' zelist '}'
		| zelist ',' zelist

vardecl		: IDENTIFIER arrayspec
		| indsp IDENTIFIER arrayspec
		| '(' indsp IDENTIFIER arrayspec ')'
				'(' arglist ')'
		| indsp '(' indsp IDENTIFIER arrayspec ')'
				'(' arglist ')'

arrayspec	:
		| arrayspec '[' zexpr ']'

indsp		: '*'
		| indsp '*'

arglist		:
		| arg
		| '*' xtname
		| '.' xtname
		| arglist ',' arg

arg		: xtname
		| xtname indsp arrayspec
		| xtname '(' indsp ')' '(' arglist ')'
		| xtname indsp '(' indsp ')' '(' arglist ')'
		| TUPLE tuplearg
		| xtname vardecl
		| '.' '.' '.'

tuplearg	: tname
		| tname '(' indsp ')' '(' arglist ')'
		| tname vardecl

autolist	: 
		| autolist autodecl

autodecl	: xtname vardecllist ';'
		| TUPLE tname vardecllist ';'

block		: '{' autolist slist '}'
		| '!' '{' autolist slist '}'

slist		:
		| slist stmnt

tbody		: '{' ctlist '}'
		| '!' '{' clist '}'

ctlist		:
		| ctlist tcase

tcase		: CASE typecast ':' slist
		| DEFAULT ':' slist

cbody		: '{' clist '}'
		| '!' '{' clist '}'

clist		:
		| clist case

case		: CASE expr ':' slist
		| DEFAULT ':' slist

rbody		: stmnt
		| IDENTIFIER block

zlab		:
		| IDENTIFIER

stmnt		: nlstmnt
		| IDENTIFIER ':' stmnt

info		:
		| ',' STRING_CONST

nlstmnt		: zexpr ';'
		| block
		| CHECK expr info ';'
		| ALLOC elist ';'
		| UNALLOC elist ';'
		| RESCUE rbody
		| RAISE zlab ';'
		| GOTO IDENTIFIER ';'
		| PROC elist ';'
		| TASK elist ';'
		| BECOME expr ';'
		| ALT cbody
		| RETURN zexpr ';'
		| FOR '(' zexpr ';' zexpr ';' zexpr ')' stmnt
		| WHILE '(' expr ')' stmnt
		| DO stmnt WHILE '(' expr ')'
		| IF '(' expr ')' stmnt
		| IF '(' expr ')' stmnt ELSE stmnt
		| PAR block
		| SWITCH expr cbody
		| TYPEOF expr tbody
		| CONTINUE zconst ';'
		| BREAK zconst ';'

zconst		:
		| CONSTANT

zexpr		:
		| expr

expr		: castexpr
		| expr '*' expr
		| expr '/' expr
		| expr '%' expr
		| expr '+' expr
		| expr '-' expr
		| expr '>>' expr
		| expr '<<' expr
		| expr '<' expr
		| expr '>' expr
		| expr '<=' expr
		| expr '>=' expr
		| expr '==' expr
		| expr '!=' expr
		| expr '&' expr
		| expr '^' expr
		| expr '|' expr
		| expr '&&' expr
		| expr '||' expr
		| expr '=' expr
		| expr ':=' expr
		| expr '<-' '=' expr
		| expr '+=' expr
		| expr '-=' expr
		| expr '*=' expr
		| expr '/=' expr
		| expr '%=' expr
		| expr '>>=' expr
		| expr '<<=' expr
		| expr '&=' expr
		| expr '|='  expr
		| expr '^=' expr
		| expr '::' expr

castexpr	: monexpr
		| '(' typecast ')' castexpr
		| '(' ALLOC typecast ')' castexpr

typecast	: xtname
		| xtname indsp
		| xtname '(' indsp ')' '(' arglist ')'
		| TUPLE tname

monexpr		: term
		| '*' castexpr 
		| '&' castexpr
		| '+' castexpr
		| '-' castexpr
		| '--' castexpr
		| ZEROX castexpr
		| '++' castexpr
		| '!' castexpr
		| '~' castexpr
		| SIZEOF monexpr
		| '<-' castexpr
		| '?' castexpr

ztelist		:
		| telist

tcomp		: expr
		| '{' ztelist '}'

telist		: tcomp
		| telist ',' tcomp

term		: '(' telist ')'
		| SIZEOF '(' typecast ')'
		| term '(' zarlist ')'
		| term '[' expr ']'
		| term '.' stag
		| '.' TYPENAME '.' stag
		| term '->' stag
		| term '--'
		| term '++'
		| term '?'
		| name
		| '.' '.' '.'
		| ARITHMETIC_CONST
		| NIL
		| ENUM_MEMBER
		| STRING_CONST
		| '$' STRING_CONST

stag		: IDENTIFIER
		| TYPENAME

zarlist		:
		| elist

elist		: expr
		| elist ',' expr

tlist		: typecast
		| typecast ',' tlist

tname		: sclass xtname
		| sclass TUPLE '(' tlist ')'
		| sclass '(' tlist ')'

variant		: typecast
		| typecast ',' variant

xtname		: INT
		| UINT
		| SINT
		| SUINT
		| BYTE
		| FLOAT
		| VOID
		| TYPENAME
		| TYPENAME '[' variant ']'
		| CHAN '(' variant ')' bufdim

bufdim		:
		| '[' expr ']'

sclass		:
		| EXTERN
		| INTERN
		| PRIVATE
.P2
