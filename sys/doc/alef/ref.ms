.am DS
.ft I
..
.ta 1i 2.3i 4.5i  (optional to set tabs)
.TL
ALEF Reference Manual
.AU
Phil Winterbottom
philw@research.att.com
.SH
Introduction
.PP
Alef is a concurrent programming language designed for
systems programming. Exception handling, process management and synchronisation
primitives are implemented by the language. Programs can be written using both
shared variable and message passing paradigms. Expressions use the same syntax
as C, but the type system is substantially different. The language does not
provide garbage collection, so programs are expected to manage their own memory.
This manual provides a bare description of the syntax and semantics of the current
implementation.
.NH
Lexical
.PP
Compilation starts with a preprocessing phase. An ANSI C preprocessor is used.
The preprocessor performs file inclusion and macro substitution. Comments and lines
beginning
with the # character are consumed by the preprocessor. The preprocessor
produces a sequence of tokens for the compiler. A token is a sequence of characters
separated by white space. The white space characters are space, new line, tab,
form feed and vertical tab.
.NH 2
Tokens
.PP
The lexical analyser classifies tokens as: identifiers, typenames, keywords,
constants and operators. Tokens are separated by white space.
White space is ignored in the source except as needed to separate sequences of tokens
which would otherwise be ambiguous.
The lexical analyser is greedy. If tokens
have been consumed up to a given character, then the next token will be the longest
string of characters which forms a legal token.
.NH 2
Reserved Words
.PP
The following keywords are reserved by the language and may not be used as identifiers:
.P1
	adt		aggr		alloc
	alt		break		case
	chan		char		check
	continue	default		do
	else		enum		extern
	float		for		goto
	if		int		intern
	nil		par		proc
	raise		rescue		return
	sint		sizeof		switch
	task		typedef		uint
	unalloc		union		usint
	void		while
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
Comments may not be nested.
.NH 2
Identifiers
.PP
An identifier is any sequence of alpha-numeric characters and the character
.CW _ .
Identifiers are case sensitive. All characters are significant.
Identifier names prefixed by
.CW ALEF
are reserved for use by the runtime system.
.NH 2
Constants
.PP
There are four types of constant:
.DS
	constant:
		integer-const
		character-const
		floating-const
		string-const
.DE
An integer constant is a sequence of digits. A prefix may be used to modify the base
of a number. The following prefixes are legal:
.P1
	none	decimal		0-9
	0x	hexadecimal	0-9 a-f A-F
	0	octal		0-7
.P2
A character constant may contain one or more characters surrounded by the single quote
mark
.CW ' .
If the constant contains two or more characters the first must be the escape character
\e. The following table shows valid characters after an escape and the value of the
constant:
.P1
	0	NUL	Null character
	n	NL	Newline
	r	CR	Carriage return
	t	HT	Horizontal tab
	b	BS	Backspace
	f	FF	Form feed
	a	BEL	Buzz
	v	VT	Vertical tab
	\e	\e	Backslash
	"	"	Double quote
.P2
Character constants have the type
.CW int .
.PP
A floating point constant consists of an integer part, a period, a fractional part,
the letter e or E and an exponent part. The integer, fraction and exponent parts 
must consist of decimal digits. Either the integer or fractional parts may be
omitted. Either the decimal point or the letter
.CW e (E)
and the exponent may be
omitted. The integer part or period and the exponent part may be preceded by
the unary
.CW +
or
.CW -
operators. Floating point constants have the type
.CW float .
.PP
A string constant is a sequence of characters between the double quote marks
.CW " .
A string has the type 'static array of char'. The NUL character is automatically
appended to the string by the compiler. The effect of modifying a string constant
is implementation dependent. Taking the
.CW sizeof
a string constant gives the
number of characters including the appended NUL.
.PP
.NH 2
Programs
.PP
An alef program is a list of declarations. The declarations introduce identifiers.
Identifiers may define variables, types, functions, function prototypes or enumerators.
Identifiers have storage classes which define their scope. A storage class applied
to a complex type controls the scope of the members. For functions and variables declared
at the file scope the storage class determines if a definition can be accessed
from another file.
.NH 2
Processes and Tasks
.PP
The term process is used to refer to a preemptively scheduled thread of execution.
A process may contain several tasks. A task is a coroutine within a process. The
memory model does not define the sharing of memory between processes. On a shared
memory computer processes will typically share the same address space.
On a multicomputer processes may be located on physically distant nodes with
access only to local memory. In such a system processes would not share
the same address space, and must communicate using message passing.
.PP
A group of tasks executing within the context of a process are defined
to be in the same address space. Tasks are scheduled during communication and
synchronisation operations. The term thread is used wherever the distinction between
a process and task is unimportant.
.NH
Definitions and Declarations
.PP
A declaration introduces an identifier and specifies its type. A definition is a
declaration that also reserves storage for an identifier.
An object is an area of memory of known type produced by a definition.
Function prototypes, variable declarations preceded by \f(CWextern\fP, and 
type specifiers are declarations.
Function declarations with bodies and variable declarations are
examples of definitions.
.NH 2
Scope
.PP
Identifiers within a program have scope. There are four levels of scope: file, type,
function and local.
.IP \(bu
A local identifier is declared at the start of a block. A local has scope starting
from its declaration to the end of the block in which it was declared.
.IP \(bu
Exception identifiers and labels have the scope of a function. These identifiers
can be referenced from the start of a function to its end, regardless of
position of the declaration.
.IP \(bu
A member of a complex type is in scope only when the dereference
operators
.CW .
and
.CW ->
are applied to the type.
Hidden type members have special scope and may only
be referenced by function members of the type.
.IP \(bu
All definitions outside of a function body have the scope of file. Unqualified
declarations at the file scope have static storage class.
.LP
.NH 2
Storage classes
.PP
There are three storage classes: automatic, parameter and static. Automatic objects
are created at entry to the block in which they were declared. After
creation the value of automatics is undefined. Automatic variables
are destroyed at block exit.
Parameters are created by function invocation and are
destroyed at function exit. Uninitialised static objects exist from invocation of
the program until termination. Static objects which have not been initialised
have the value 0.
.NH
Types
.PP
A small set of basic types are defined by the language. More complex types may
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
char	: 8  bits : unsigned character
_
sint	: 16 bits : signed short integer
_
usint	: 16 bits : unsigned short integer
_
int	: 32 bits : signed integer
_
uint	: 32 bits : unsigned integer
_
float	: 32 bits : floating point
_
long	: 64 bits : long signed integer
_
ulong	: 64 bits : unsigned long integer
_
chan	: 32 bits : channels
.TE
The size given for the basic types is the minimum number of bits required
to represent that type. The format and precision of
.CW float
is implementation
dependent. The
.CW float
type should be the highest precision available on the hardware.
The
.CW long
and
.CW ulong
types are optional and may not be implemented.
The alignment of the basic types is implementation dependent. The channel type
is special. Channels are implemented by the runtime system and must be initialised
before use.
.NH 2
Derived types
.PP
Operators applied in declarations use one of the basic types to derive a new type.
The deriving operators are:
.P1
        *	create a pointer to
        &	yield the address of
        ()	a function returning
        []	an array of
.P2
These operators bind to the name of each identifier in a declaration or definition.
Some examples are:
.P1
        int 	*ptr;		/* A pointer to an integer */
        char	c[10];		/* A vector of 10 characters */
        float	*pow();		/* A function returning a pointer to a float */
.P2
Complex types may be built from the basic types and the deriving operators. Complex
types may be either aggregates, unions or abstract data types. These complex types
contain sequences of basic types and other derived types. An aggregate is a simple
collection of basic and derived types. Each element of the aggregate has unique
storage. An abstract data type has the same storage allocation as an aggregate
but also has a set of functions to manipulate the type, and a set of protection
attributes for each of its members. A union type contains a sequence of basic and
derived types which occupy the same storage. The size of a union is determined by
the size of the largest member.
.PP
The declaration of complex types introduces
.I typenames
into the language. After
declaration a typename can be used wherever a basic type is permitted. Derived
types and basic types may be renamed using the
.CW typedef
statement.
.NH 2
Conversions and Promotions
.PP
The usual conversions are performed!
.NH
Declarations
.PP
A declaration attaches a type to an identifier, it need not reserve storage.
A declaration which reserves storage is called a definition. A program consists
of a list of declarations:
.DS
	program:
		declaration-list

	declaration-list:
		declaration
		declaration-list declaration
.DE
A declaration can define a simple variable, a function, a prototype to a function,
an adt function, a type specification or a type definition:
.DS
	declaration:
		simple-declarations
		type-declaration
		type-definition
		function-declaration
.DE
.NH 2
Simple declarations
.PP
A simple declaration consists of a type specifier and a list of identifiers. Each
identifier may be qualified by deriving operators. Simple declarations at the file
scope may be initialised.
.DS
	simple-declarations:
		type-specifier simple-decl-list \f(CW;\fP

	simple-decl-list:
		simple-declaration
		simple-decl-list , simple-declaration

	simple-declaration:
		pointer identifier array-spec
		pointer identifier array-spec \f(CW=\fP initialiser-list

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
.CW n-1 .
.NH 2
Type Specifiers
.PP
.DS
	type-specifier:
		type
		storage-class type

	type:
		\f(CWchar\fP
		\f(CWint\fP
		\f(CWuint\fP
		\f(CWsint\fP
		\f(CWusint\fP
		\f(CWlong\fP
		\f(CWulong\fP
		\f(CWvoid\fP
		\f(CWfloat\fP
		typename
		channel-specifier

	storage-class:
		\f(CWintern\fP
		\f(CWextern\fP

	channel-specifier:
		\f(CWchan\fP \f(CW(\fP type \f(CW)\fP buffer-spec

	buffer-spec:
		\f(CW[\fP constant-expression \f(CW]\fP
.DE
The storage class controls the scope of the declaration. Storage classes may only be
applied to declarations at the file scope. The scope of a definition qualified with
.CW intern
is file. A declaration qualified by
.CW extern
references a definition declared in this or another file.
.PP
A channel declaration cannot be a definition. A channel must have space reserved
by an
.CW alloc
statement. A channel declaration without a buffer specification produces a synchronous
communication channel.
Threads sending values on the channel will block until
some other thread receives from the channel. The two threads rendezvous and a value
is passed between sender and receiver. If buffers are specified
then an asynchronous channel is produced.
The constant-expression defines the number of buffers
to be allocated. A send operation will complete immediately while buffers
are available. A thread will block if all buffers are in use. A receive
operation will block if no value is buffered. If a value is buffered the receive
will complete and deallocate the buffer. Any senders waiting for buffers will then be
allowed to continue.
.PP
.I Typename
is an identifier defined as a complex type or derived from a
.CW typedef .
.NH 2
Initialisers
.PP
Only simple declarations at the file scope may be initialised.
.DS
	initialiser-list:
		constant-expression
		\f(CW[\fP constant-expression \f(CW]\fP constant-expression
		\f(CW{\fP initialiser-list \f(CW}\fP
		initialiser-list , initialiser-list
.DE
An initialisation consists of a
.I constant-expression
or a list of initialisations nested
in braces. An array or complex type requires an explicit set of braces for each level 
of nesting. If a complex or array is only partially initialised the remaining members
or elements are initialised to 0. Elements of sparse arrays can be initialised 
by supplying a bracketed index for an element. Successive elements without the
index notation continue to initialise the array in sequence. For example:
.P1
	char a[256] = {
			['a']	'A',	/* Set element 97 to 65 */
			['a'+1]	'B',	/* Set element 98 to 66 */
				'C'	/* Set element 99 to 67 */
	};
.P2
If the dimensions of the array are omitted from the
.I array-spec
the compiler sets the size of each dimension to be large enough to accommodate
the initialisation. The size of the array can be found using
.CW sizeof .
.NH 2
Type Declarations
.PP
A type declaration creates a new type and introduces an identifier representing that
type into the language.
.DS
	type-declaration:
		complex typename \f(CW{\fP memberlist \f(CW}\fP \f(CW;\fP
		complex \f(CW{\fP memberlist \f(CW}\fP decl-tag \f(CW;\fP
		complex typename \f(CW{\fP memberlist \f(CW}\fP decl-tag \f(CW;\fP
		enumeration-type

	complex:
		\f(CWadt\fP
		\f(CWaggr\fP
		\f(CWunion\fP

	decl-tag:
		identifier
.DE
A complex type is composed of a list of members. Each member may be a complex type,
a derived type or a basic type. Members are referenced by tag or by type. Members can
only be referenced by the
.CW .
and
.CW ->
operators. If a member is
declared with a tag it can only be referenced by tag. Members without tags are
called unnamed. Unnamed members are referenced by typename or by implicit promotion
when supplied as function arguments. A type declaration must have either a type name
or a tag.
.DS
	memberlist:
		member
		memberlist member

	member:
		tname pointer decl-tag array-spec \f(CW;\fP
		tname decl-tag \f(CW(\fP arglist \f(CW)\fP \f(CW;\fP
.DE
.NH 2
Abstract Data Types
.PP
An abstract data type defines both storage for members like an aggregate
and the operations which can be performed on that type.
Access to the members of an abstract data type is restricted
to enforce a policy of information hiding. The mechanism is designed to
encourage modular program design and provide clean library interfaces.
The scope of the members of an abstract data type depends on their
type. By default access to members which define data is limited to the member
functions. Members can be explicitly exported from the type using the
.CW extern
storage class in the member declaration. Member functions are visible by default,
that is the opposite behaviour of data members. Access to a member function
may be restricted to member functions only by qualifying the declaration with the
.CW intern
storage class.
The four combinations are:
.P1
	adt Point
	{
			int	x;		/* Access by member functions only */
		extern	int	y;		/* Access by everybody */

			Point set(Point*);	/* Access by everybody */
		intern	Point tst(Point);	/* Access only from Point.set */
	};
.P2
Member functions are defined by type and name. The pair form a unique name for
the function, so the same member function name can be used in many types.
Using the last example, the member function set could be defined as:
.P1
	Point
	Point.set(Point *a)
	{
		a->x = 0;	/* Set the value of the point to zero */
		a->y = 0;

		return *a;
	}
.P2
An implicit pointer to the abstract data type may be passed to a member function.
If the first argument of the member function declaration in the
.CW adt
specification is '* typename' the first parameter is passed implicitly.
.P1
	adt Point
	{
			int	x;
		extern	int	y;

			Point set(*Point);	/* Pass &Point as first argument */
		intern	Point tst(Point);
	};

	void
	func()
	{
		Point p;

		p.set();	/* Set receives &p as first argument */
	}
.P2
The implicit parameter passing mechanism is particularly useful when the
.CW adt
is an unnamed substructure.
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
The members of an enumeration are called enumerators.
Instantiation of the enumeration type, using
.I typename ,
produces a declaration of type
.CW int .
Enumeration constants may appear wherever an integer
constant is legal. If the values of the enumerators are undefined the compiler assigns
incrementing values from 0. If a value is given to an enumeration constant, values are
assigned to the following enumerators by incrementing the value for each
successive member until the next assigned value is reached.
.NH 2
Type Definition
.PP
Type definition allows derived types to be named, basic types to be renamed and
forward referencing between complex types.
.DS
	type-definition:
		\f(CWtypedef\fP tname identifier;
.DE
.NH 2
Function Declarations
.PP
There are three forms of function declaration: function definition,
prototype declaration and function pointer declaration.
.DS
	function-declaration:
		tname identifier \f(CW(\fP arglist \f(CW)\fP block
		tname decl-tag \f(CW(\fP arglist \f(CW)\fP block
		tname function-id \f(CW(\fP arglist \f(CW)\fP \f(CW;\fP
		tname \f(CW(\fP function-id \f(CW)\fP \f(CW(\fP arglist \f(CW)\fP \f(CW;\fP

	function-id:
		pointer identifier array-spec

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
.NH
Expressions
.PP
The order of expression evaluation is not defined except where noted. That is,
unless the definition of the operator guarantees evaluation order, an operator
may evaluate any of its operands first.
.PP
The behaviour of exceptional conditions such as divide by zero, arithmetic overflow
and floating point exceptions is not defined.
.NH 2
Pointer Generation
.PP
References to expressions of type 'function returning T' and 'array of T' 
are rewritten to produce pointers to either the function or first element of
array. That is 'function returning T' becomes 'pointer to function returning T'
and 'array of T' becomes 'pointer to array of T'.
.NH 2
Primary Expressions
.PP
Primary expressions are identifiers, constants or parenthesised expressions:
.DS
	primary-expression:
		identifier
		constant
		\f(CW...\fP
		\f(CWnil\fP
		\f(CW(\fP expression \f(CW)\fP
.DE
The parameters received by a function taking variable arguments are referenced using
'...'.
The primary-expression 
.CW '...'
yeilds an r-value of type 'pointer to void'. The r-value points at the first variable
parameter.
The primary-expression
.CW nil
returns a pointer of type 'pointer to void' of value 0
which is guaranteed not to point at an object.
.NH 2
Postfix Expressions
.DS
	postfix-expression:
		primary-expression
		postfix-expression \f(CW[\fP expression \f(CW]\fP
		postfix-expression \f(CW(\fP argument-list \f(CW)\fP
		postfix-expression \f(CW.\fP tag
		postfix-expression \f(CW->\fP tag 
		postfix-expression \f(CW++\fP
		postfix-expression \f(CW--\fP
		postfix-expression \f(CW?\fP

	tag:
		typename
		identifier
		decl-tag	

	argument-list:
		expression
		argument-list , expression
.DE
.NH 2
Array Reference
.PP
A primary expression followed by an expression enclosed in square bracket is an
array indexing operation. The expression is rewritten to be
.I *((postfix-expression)+(expression)) . 
One of the expressions must be of type
pointer the other of integral type.
.NH 2
Complex Type References
.PP
The operator . references a member of a complex type. The first part of the
expression must be of type 
.CW union ,
.CW aggr
or
.CW adt .
The member may be specified
by either name or type. Only one unnamed member of type
.I typename
is allowed when referencing members by type, 
otherwise the reference is ambiguous.
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
operator follows the search and
type rules as . and is equivalent to
.I (*postfix-expression).tag .
.NH 2
Function Calls
.PP
The
.I postfix-expression
must yield a value of type 'pointer to function'. A type
declaration for the function must be declared prior to a function call. The
declaration can be either the definition of the function or a function prototype.
The types of each argument in the specification must match the corresponding
expression type under the rules of promotion and conversion for assignment.
In addition unnamed complex type members will be promoted automatically.
For example:
.P1
	aggr Test
	{
		Lock;		/* Unnamed substructure */
	};

	Test yuk;		/* Definition of complex Test */ 
	void lock(Lock*);	/* Prototype for function lock */

	void
	main()
	{
		lock(&yuk);	/* address of Lock in Test is passed */
	}
.P2
.NH 2
Postfix Increment and Decrement
.PP
The postfix increment and decrement operators return the value of expression,
then add or subtract 1 to the expression. The expression must be
an l-value of integral type.
.NH 2
Unary Operators
.PP
The unary operators are:
.DS
	unary-expression:
		postfix-expression
		\f(CW++\fP unary-expression
		\f(CW--\fP unary-expression
		unary-operator cast-expression
		\f(CWsizeof\fP \f(CW(\fP cast-expression \f(CW)\fP

	unary-operator: one of
		\f(CW<-\fP \f(CW?\fP \f(CW*\fP \f(CW+\fP \f(CW-\fP ~ \f(CW!\fP
.DE
.NH 2
Prefix Increment and Decrement
.PP
The
.CW ++
(prefix increment) and
.CW --
(prefix decrement) operators add or subtract one to a
.I unary-expression
and return the new value. The
.I unary-expression
must be an l-value of integral or pointer type.
.NH 2
Receive and Can Receive
.PP
The operator
.CW <-
receives a value from a channel.
.I unary-expression
must be of type 'channel of T'.
The type of the result will be T. A process or task will block
until a value is available from the channel. The prefix operator
.CW ?
returns 1 if a channel
has a value available for receive, 0 otherwise.
.I unary-expression
must be of type 'channel of T'.
.NH 2
Can send
.PP
The postfix operator
.CW ?
returns 1 if a thread can send on a channel
without blocking, 0 otherwise.
.I unary-expression
must be of type 'channel of T'.
.PP
The blocking test operator
.CW ?
is only reliable when used on a channel shared between tasks.
A process may block after a successful
.CW ?
because the interleaving of the processes using the channel is undefined.
.NH 2
Indirection
.PP
The unary operator
.CW *
dereferences a pointer.
.I unary-expression
must be of type 'pointer to T'. The result of the indirection is a value of type T.
.NH 2
Unary Plus and Minus
.PP
Unary plus is equivalent to
.I (0+(unary-expression)) .
Unary minus is equivalent to
.I (0-(unary-expression)) .
An integral operand undergoes integral
promotion. The result is the type of the promoted operand.
.NH 2
Bitwise Negate
.PP
The operator
.CW ~
performs a bitwise negation of its operand.
.I unary-expression
must be of integral type.
.NH 2
Logical Negate
.PP
The operator 
.CW !
performs logical negation.
.I unary-expression
must of arithmetic or pointer type.
If the operand is a pointer and its value is nil the result is
integer 0, otherwise 1. If the operand is arithmetic and the value is 0 the
result is 0, otherwise 1.
.NH 2
Sizeof Operator
.PP
The
.CW sizeof
operator returns the number of bytes required to store an object of
type
.I cast-expression .
If
.CW sizeof
is applied to a string constant the result is the number of bytes required to
store the string and a NUL terminating byte.
.NH 2
Casts
.PP
A cast converts the result of an expression into a new type:
.DS
	cast-expression:
		unary-expression
		\f(CW(\fP type-cast \f(CW)\fP cast-expression

	cast-type:
		type pointer
.DE
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
must have arithmetic type. The operand of
.CW %
must be of integral type. The operator 
.CW /
yields the quotient, 
.CW %
the remainder and
.CW *
the product of the operands. If
.CW b
is non-zero then 
.CW "a = (a/b) + a%b
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
a pointer. If P is a pointer to type T then P+n is the same as p+(sizeof(T)*n).
The - operator computes the difference of its operands. The first operand may
be of pointer or arithmetic type. The second operand must be of arithmetic type.
If P is a pointer to type T then P-n is the same as p-(sizeof(T)*n).
Thus if P is a pointer to an array p+1 will point to the next object in the array and
P-1 will point to the previous object in the array.
.NH 2
Shift Operators
.PP
The shift Operators perform bitwise shifts:
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
performs an arithmetic shift left by
.I additive-expression bits.
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
(greater than or equal to). The operands must be of arithmetic of pointer type.
The value of the expression is 1 if the relation is true, otherwise 0.
The usual arithmetic conversions are performed. Pointer comparisons are allowed only
on pointers of the same type.
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
(not equal) follow the same rules as relational operators.
.NH 2
Bitwise Logic Operators
.PP
.DS
	AND-expression:
		equality-expression
		AND-expression \f(CW&\fP equality-expression

	XOR-expression:
		AND-expression
		XOR-expression ^ AND-expression

	OR-expression:
		XOR-expression
		OR-expression \f(CW|\fP XOR-expression
.DE
The operators perform bitwise logical operations and apply only to integral types.
The operators are
.CW &
(logical and),
.CW ^
(exclusive or) and
.CW |
(inclusive or).
.NH 2
Logical Operators
.PP
.DS
	logical-AND-expression:
		OR-expression
		logical-AND-expression && OR-expression

	logical-OR-expression:
		logical-AND-expression
		logical-OR-expression || logical-AND-expression
.DE
The
.CW &&
operator returns 1 if both of its operands evaluate to non-zero, otherwise 0.
The
.CW ||
operator returns 1 if either of its operand evaluates to non-zero, otherwise 0.
Both operators are guaranteed to evaluate strictly left to right.
The operands can be any mix of arithmetic and pointer types.
.NH 2
Constant expressions
.PP
A constant expression is any expression which can be completely evaluated by the
compiler.
.DS
	constant-expression:
		logical-OR-expression
.DE
.I constant-expression
appears as part of initialisation, channel buffer
specifications and array dimensions. The following operators may not be part of a
constant expression: function calls, assignment, send, receive, increment and decrement.
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
		unary-expression \f(CW=\fP \f(CW(\fP type-cast \f(CW)\fP compound-list

	compound-list:
		expression
		\f(CW[\fP expression \f(CW]\fP expression
		\f(CW{\fP compound-list \f(CW}\fP
		compound-list , compound-list

	assignment-operator: one of
		\f(CW=\fP  \f(CW+=\fP  \f(CW*=\fP  \f(CW/=\fP  \f(CW%=\fP  \f(CW&=\fP  \f(CW|=\fP  \f(CW^=\fP  \f(CW>>=\fP  \f(CW<<=\fP
.DE
The left side of the expression must be an l-value. Compound assignment allows the
members of a complex type to be assigned from a member list in a single statement.
The compound assignment list follows the rules of a complex initialiser list but allows
the use of abitary expressions. For example:
.P1
	aggr Readmesg				/* Read message to file system */
	{
		int	fd;
		void	*data;
		int	len;
	};

	int
	read(int fd, void *data, int len)
	{
		Readmesg m;

		m = (Readmesg) {fd, data, len};	/* Pack message parameters */
		filesys <-= m;
	}
.P2
.PP
The
.CW <-=
(assign send) operator sends the result of the right side into a channel.
The
.I unary-expression
must be of type 'channel of T'.
If the left side of the expression is of type 'channel of T', the value transmitted
down the channel is the same as if the expression were 'object of type T = expression'.
.NH 2
Binding and Precedence
.PP
The binding and precedence of the operators is as follows:
.TS
box, center, tab(:);
c | l
c | lFCW .
binding : operator
_
l to r : ()  []  ->  .
_
r to l : !  ~  ++  --  <-  ? +  -  *  &  (\f2cast\fP) sizeof
_
l to r : *  /  %
_
l to r : +  -
_
l to r : <<  >>
_
l to r : <  <=  >  >=
_
l to r : ==  !=
_
l to r : &
_
l to r : ^
_
l to r : |
_
l to r : &&
_
l to r : ||
_
l to r : <-=  =  +=  -=  *=  /=  %=  \&=  ^=  |=  <<=  >>=
.TE
.NH
Statements
.PP
Statements are executed for effect, and do not yield values. Statements fall
into one of several groups:
.DS
	statement:
		label-statement
		expression-statement
		block-statement
		selection-statement
		loop-statement
		jump-statement
		exception-statement
		process-statement
		allocation-statement
.DE
.NH 2
Label Statements
.PP
A statement may be prefixed by an identifier. The identifier labels the statement
and may be used as the destination of a goto. Label and exception identifiers have their
own namespace and do not conflict with other names. Labels have the scope of function.
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
		!{ autolist slist \f(CW}\fP

	autolist:
		declaration
		autolist declaration

	slist:
		statement
		slist statement
.DE
An identifier declared in
.I autolist
suspends any previous declaration.
An identifier may be declared only once per block. The declaration remains
in force until the end of the block, after which any suspended declaration
comes back into effect.
.PP
The value of identifiers declared in
.I autolist
is undefined at block entry and should be initialised after declaration but before use.
.PP
The symbol
.CW !{
introduces a guarded block. Only one thread may be executing
a guarded block at any instant.
.NH 2
Selection Statements
.PP
Selection statements alter the flow of control based on the value of an expression.
.DS
	selection-statement:
		\f(CWif\fP \f(CW(\fP expression \f(CW)\fP statement \f(CWelse\fP statement
		\f(CWif\fP \f(CW(\fP expression \f(CW)\fP statement
		\f(CWswitch\fP \f(CW(\fP expression \f(CW)\fP cbody
		\f(CWalt\fP cbody

	cbody:
		\f(CW{\fP caselist \f(CW}\fP
		!{ caselist \f(CW}\fP

	caselist:
		case-item
		caselist case-item

	case-item:
		\f(CWcase\fP expression : statement
		\f(CWdefault\fP : statement
.DE
An
.CW if
statement first evaluates expression. Expression must yield a value
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
clause is ambiguous. The ambiguity is
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
statement selects one of several statements. The
.I expression
is evaluated and converted into an integer. The integer is compared with the value
specified in each
.CW case .
If the integers compare the statement part of the
.CW case
is executed. The
.CW case
expression must yield an integer constant.
For a single
.CW switch
statement each case expression must yield a unique value.
If no
.CW case
is matched, the
.CW default
clause is executed. If the
.CW default
is omitted then none of the
.CW case
statements are executed.
.PP
The
.CW alt
statement allows threads to perform communication on several channels
simultaneously without polling. The
.CW alt
statement provides a fair select between ready channels.
A thread will remain blocked in
.CW alt
until one of the
.CW case
expressions can be evaluated without blocking. The
.CW case
expression may be evaluated more than once, therefore care should be taken when using
expressions which have side effects. If several of the
.CW case
expressions are ready for evaluation one is chosen at random.
The expression in each
.CW case
of an
.CW alt
must contain either a send or receive operation. For example:
.P1
	chan(Mesg) keyboard, mouse;
	Mesg m;

	alt {
	case m <-= keyboard:
		/* Process keyboard event */
		break;
	case m <-= mouse:
		/* Process mouse event */
		break;
	}
.P2
.PP
If a channel is shared by more than one thread an
.CW alt
on that channel may block if values are removed by another thread.
.PP
The symbol
.CW !{
introduces a guarded
.I caselist .
Only one thread may be executing a guarded caselist at any instant.
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
loop the
the expression is evaluated and tested before the statement. In the
.CW do
loop the statement is executed before expression is evaluated and tested.
.PP
In the
.CW for
loop the first expression is evaluated once before loop entry. The expression is
usually used to initialise the loop variable.
The second expression is evaluated
at the beginning of each loop iteration. The expression must yield either a pointer
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
		\f(CWcontinue\fP count;
		\f(CWbreak\fP count;
		\f(CWraise\fP identifier \f(CW;\fP
		\f(CWreturn\fP expression \f(CW;\fP

	count:
		integer-constant
.DE
.CW goto
transfers control to a label in the current function.
.NH 2
Continue Statements
.PP
The
.CW continue
statement may only appear as part of an iteration statement.
If
.I count
is omitted the
.CW continue
statement transfers control to the loop-continuation
portion of the smallest enclosing iteration statement, that is the end of the loop.
If
.I count
is supplied continue transfers control to the loop continuation of some
outer nested loop. count is the number of loops to skip. The statement
.CW continue
with no
.I count
is the same as
.CW continue
1 .
For example:
.P1
	while(1) {
		while(1) {
			continue 2;	/* Same as goto contin; */
		}
	contin:				/* Continue comes here */
	}
.P2
.NH 2
Break Statements
.PP
The
.CW break
statement may only appear as part of an iteration statement.
If
.I count 
is omitted the break statement terminates the statement portion of the iteration
loop and transfers control to the statement after the iteration statement.
If
.I count
is supplied break causes termination of the iteration statement
of some nested loop.
.I count
is the number of nested iteration loops to terminate.
.CW break
with no
.I count
is the same as
.CW break
1 .
.NH 2
Raise Statement
.PP
By default
.CW raise
transfers control to the last
.CW rescue
statement. If an
identifier is supplied, control is transferred to the named
.CW rescue
statement.
.CW raise
is intended for use in error recovery. For example, these two fragments are
equivalent:
.P1
	alloc p;			alloc p;
	rescue {			goto notrescue;
		unalloc p;			dorescue:
		raise;					unalloc p;
	}						goto nextrescue;
					notrescue:
	if(error)			if(error)
		raise;				goto dorescue;
.P2
.NH 2
Return Statement
.PP
A function returns to its caller using a
.CW return
statement. Expression is required
except for functions returning
.CW void .
The result of expression is
converted using the rules of assignment to the return type of the function.
Falling through the end of a typed function returns an undefined value.
.NH 2
Exception Statements
.PP
.CW rescue
and
.CW check
statements are provided for use in error recovery:
.DS
	exception-statement:
		\f(CWrescue\fP block
		\f(CWrescue\fP identifier block
		\f(CWcheck\fP expression \f(CW;\fP
.DE
.NH 2
Rescue Statement
.PP
Under normal execution block is not executed.  A
.CW raise
after a
.CW rescue
statement transfers
control to the closest previously defined
.CW rescue
statement. Execution flows through the end of the
.CW rescue
block by default.
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
.NH 2
Check Statement
.PP
The
.CW check
statement makes an assertion.  If the assertion fails a runtime error aborts
the program. A message is printed containing the file and line number in the source
of the
.CW check
statement. The expression is evaluated and compared to 0.
If the compare succeeds the assertion has failed. A compiler option allows check
statements to be omitted from trusted object code. For example:
.P1
	alloc ptr;
	check ptr != nil;	/* Program aborts if allocation fails */
.P2
.NH 2
Process Control Statements
.PP
Statements are used to create processes and coroutines:
.DS
	process-statement:
		\f(CWproc\fP function-call \f(CW;\fP
		\f(CWtask\fP function-call \f(CW;\fP
		\f(CWpar\fP block
.DE
The
.CW proc
statement creates a new process. The new process starts running the named
function. The arguments to
.I function-call
are evaluated by the parent of the new
process. Processes are scheduled preemptively and the interleaving of the
processes is undefined. The
.CW task
statement creates a coroutine within a process. Tasks
are non-preemptive and are scheduled during message passing and synchronisation
primitives. The scheduling primitives which can cause task switching are
.CW QLock.lock
and
.CW Rendez.sleep .
The communication operations which can cause task switching are
.CW alt ,
.CW <-=
(send) and
.CW <-
(receive).
All tasks within a process will block until a system call completes. A process which
contains several tasks will exist until all the tasks within the process have exited.
In turn, a program will exist until all of the processes in the program have exited.
A process or task may exit explicitly by calling the function
.CW exits
or by returning from the function in which they were invoked.
.PP
The
.CW par
statement implements fork/process/join. A new process is created for each
statement in block. The
.CW par
statement completes when all processes have
completed execution of their statements.
A
.CW par
with a single statement is the same as a block.
The process which entered the
.CW par
is guaranteed to be the same process which exits.
.NH 2
Allocation Statements
.PP
Memory management statements allocate and free memory for objects from the heap:
.DS
	allocation-statement:
		\f(CWalloc\fP alloclist \f(CW;\fP
		\f(CWunalloc\fP alloclist \f(CW;\fP

	alloclist:
		expr
		alloclist , expr
.DE
.NH 2
Alloc Statement
.PP
The
.CW alloc
statement reserves memory for an object. The expression argument to
.CW alloc
must yeild an l-value of type pointer or channel. If expression is of type
'pointer to T' then on return the pointer will point at an area of memory
large enough to hold and object of type T. The memory will be zeroed.
If expression yeilds a value of type channel a new channel will be allocated.
If the channel is asynchronous then the specified number of buffers will be
allocated.
If the allocation fails name is set to
.CW nil .
.NH 2
Unalloc Statement
.PP
The
.CW unalloc
statement returns memory to the heap. The argument to
.CW unalloc
must be of type pointer or channel and have been returned by a successful
.CW alloc
or be
.CW nil .
.PP
If an object is unallocated twice, or an invalid object is unallocated
the runtime system will abort the program.
.NH
Yacc Style Grammar
.PP
The following grammar is suitable for implementing a yacc parser. Uppercase words
and punctuation surrounded by single quotes are the terminal symbols.
.P1	
	prog		:=	decllist
	
	decllist	:=
			|	decllist decl
	
	decl		:=	tname vardecllist ';'
			|	tname vardecl '(' arglist ')' block
			|	tname adtfunc '(' arglist ')' block
			|	tname vardecl '(' arglist ')' ';'
			|	typespec ';'
			|	TYPDEF ztname name ';'
	
	adtfunc		:=	Ttypename '.' name
			|	indsp TYPENAME '.' name
	
	ztname		:=
			|	tname
			|	AGGREGATE
			|	ADT
			|	UNION
	
	typespec	:=	AGGREGATE ztag '{' memberlist '}' ztag
			|	UNION ztag '{' memberlist  '}' ztag
			|	ADT name '{' memberlist '}' ztag
			|	ENUM ztag '{' setlist '}'
	
	ztag		:=
			|	name
			|	TYPENAME
	
	setlist		:=	sname
			|	setlist ',' setlist
	
	sname		:=
			|	name
			|	name '=' expr
	
	name		:=	ID
	
	memberlist	:=	decl
			|	memberlist decl
	
	vardecllist	:=
			|	ivardecl
			|	vardecllist ',' ivardecl
	
	ivardecl	:=	vardecl zinit
	
	zinit		:=
			|	'=' zelist
	
	zelist		:=	zexpr
			|	'[' expr ']' expr
			|	'{' zelist '}'
			|	zelist ',' zelist
	
	vardecl		:=	ID arrayspec
			|	indsp ID arrayspec
			|	'(' indsp ID arrayspec ')' '(' arglist ')'
			|	indsp '(' indsp ID arrayspec ')' '(' arglist ')'
	
	arrayspec	:=
			|	arrayspec '[' zexpr ']'
	
	indsp		:=	'*'
			|	indsp '*'
	
	
	arglist		:=
			|	arg
			|	'*' xtname
			|	arglist ',' arg
	
	arg		:=	xtname
			|	xtname indsp
			|	xtname '(' '*' ')' '(' arglist ')'
			|	xtname vardecl
			|	'.' '.' '.'
	
	autolist	:= 
			|	autolist autodecl
	
	autodecl	:=	xtname vardecllist ';'
	
	block		:=	'{' autolist slist '}'
			|	'!{' autolist slist '}'

	slist		:=
			|	slist stmnt

	cbody		:=	'{' clist '}'
			|	'!{' clist '}'
	
	clist		:=
			|	clist case
	
	case		:=	CASE expr ':=' slist
			|	DEFAULT ':=' slist
	
	rbody		:=	stmnt
			|	ID block
	
	zlab		:=
			|	ID
	
	stmnt		:=	zexpr ';'
			|	block
			|	CHECK expr ';'
			|	ALLOC alloclist
			|	UNALLOC alloclist
			|	ID '='
			|	RESCUE rbody
			|	RAISE zlab ';'
			|	GOTO Tid ';'
			|	PROCESS expr ';'
			|	TASK expr ';'
			|	ALT  cbody
			|	RETURN zexpr ';'
			|	FOR '(' zexpr ';' zexpr ';' zexpr ')' stmnt
			|	WHILE '(' expr ')' stmnt
			|	DO stmnt WHILE '(' expr ')'
			|	IF '(' expr ')' stmnt
			|	IF '(' expr ')' stmnt ELSE stmnt
			|	PAR block
			|	SWITCH '(' expr ')' cbody
			|	CONTINUE zconst ';'
			|	BREAK zconst ';'
	
	alloclist	:=	name
			|	alloclist ',' alloclist
	
	zconst		:=
			|	CONST
	
	zexpr		:=
			|	expr
	
	expr		:=	castexpr
			|	expr '*' expr
			|	expr '/' expr
			|	expr '%' expr
			|	expr '+' expr
			|	expr '-' expr
			|	expr '<<' expr
			|	expr '>>' expr
			|	expr '<' expr
			|	expr '>' expr
			|	expr '<=' expr
			|	expr '>=' expr
			|	expr '==' expr
			|	expr '!=' expr
			|	expr '&' expr
			|	expr '^' expr
			|	expr '|' expr
			|	expr '&&' expr
			|	expr '||' expr
			|	expr '=' expr
			|	expr '+=' expr
			|	expr '-=' expr
			|	expr '*=' expr
			|	expr '/=' expr
			|	expr '%=' expr
			|	expr '>>=' expr
			|	expr '<<=' expr
			|	expr '&=' expr
			|	expr '|='  expr
			|	expr '^=' expr
	
	castexpr	:=	monexpr
			|	'[' typecast ']' castexpr
			|	'(' typecast ')' convexpr
	
	convexpr	:=	castexpr
			|	'{' zelist '}'
	
	typecast	:=	xtname
			|	xtname indsp
	
	monexpr		:=	term
			|	'*' castexpr 
			|	'&' castexpr
			|	'+' castexpr
			|	'-' castexpr
			|	'--' castexpr
			|	'++' castexpr
			|	'!' castexpr
			|	'~' castexpr
			|	SIZEOF monexpr
	
	term		:=	'(' expr ')'
			|	SIZEOF '(' typecast ')'
			|	term '(' zarlist ')'
			|	term '[' expr ']'
			|	'<-' term
			|	'?' term
			|	term '<-'
			|	term '?'
			|	term '.' stag
			|	term '->' stag
			|	term '--'
			|	term '++'
			|	name
			|	'.' '.' '.'
			|	CONST
			|	NIL
			|	ENUMNAME
			|	STRING
			;
	
	stag		:=	ID
			|	TYPENAME
	
	zarlist		:=
			|	elist
	
	elist		:=	expr
			|	elist ',' expr
	
	
	tname		:=	sclass xtname
	
	xtname		:=	TYPENAME
			|	INT
			|	UINT
			|	SINT
			|	SUINT
			|	CHAR
			|	FLOAT
			|	VOID
			|	CHAN '(' typecast ')' bufdim
	
	bufdim		:=
			|	'[' expr ']'
	
	sclass		:=
			|	EXTERN
			|	INTERN
.P2
