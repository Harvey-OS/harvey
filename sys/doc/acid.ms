.HTML "Acid Manual
.am DS
.ft I
..
.ta 1i 2.3i 4.5i  (optional to set tabs)
.TL
Acid Manual
.AU
Phil Winterbottom
philw@plan9.bell-labs.com
.SH
Introduction
.PP
Acid is a general purpose, source level symbolic debugger.
The debugger is built around a simple command language. 
The command language, distinct from the language of the program being debugged,
provides a flexible user interface that allows the debugger
interface to be customized for a specific application or architecture.
Moreover, it provides an opportunity to write test and
verification code independently of a program's source code.
Acid is able to debug multiple
processes provided they share a common set of symbols, such as the processes in
a threaded program.
.PP
Like other language-based solutions, Acid presents a poor user interface but
provides a powerful debugging tool.
Application of Acid to hard problems is best approached by writing functions off-line
(perhaps loading them with the
.CW include
function or using the support provided by
.I acme (1)),
rather than by trying to type intricate Acid operations
at the interactive prompt.
.PP
Acid allows the execution of a program to be controlled by operating on its
state while it is stopped and by monitoring and controlling its execution
when it is running. Each program action that causes a change 
of execution state is reflected by the execution
of an Acid function, which may be user defined.
A library of default functions provides the functionality of a normal debugger.
.PP
A Plan 9 process is controlled by writing messages to a control file in the
.I proc (3)
file system. Each control message has a corresponding Acid function, which
sends the message to the process. These functions take a process id
.I pid ) (
as an
argument. The memory and text file of the program may be manipulated using
the indirection operators. The symbol table, including source cross reference,
is available to an Acid program. The combination allows complex operations
to be performed both in terms of control flow and data manipulation.
.SH
Input format and \f(CWwhatis\fP
.PP
Comments start with
.CW //
and continue to the end of the line.
Input is a series of statements and expressions separated by semicolons.
At the top level of the interpreter, the builtin function
.CW print
is called automatically to display the result of all expressions except function calls.
A unary
.CW +
may be used as a shorthand to force the result of a function call to be printed.
.PP
Also at the top level, newlines are treated as semicolons
by the parser, so semicolons are unnecessary when evaluating expressions.
.PP
When Acid starts, it loads the default program modules,
enters interactive mode, and prints a prompt. In this state Acid accepts
either function definitions or statements to be evaluated.
In this interactive mode
statements are evaluated immediately, while function definitions are
stored for later invocation.
.PP
The
.CW whatis
operator can be used to report the state of identifiers known to the interpreter.
With no argument,
.CW whatis
reports the name of all defined Acid functions; when supplied with an identifier
as an argument it reports any variable, function, or type definition
associated with the identifier.
Because of the way the interpreter handles semicolons,
the result of a
.CW whatis
statement can be returned directly to Acid without adding semicolons.
A syntax error or interrupt returns Acid to the normal evaluation
mode; any partially evaluated definitions are lost.
.SH
Using the Library Functions
.PP
After loading the program binary, Acid loads the portable and architecture-specific
library functions  that form the standard debugging environment.
These files are Acid source code and are human-readable.
The following example uses the standard debugging library to show how
language and program interact:
.P1
% acid /bin/ls
/bin/ls:mips plan 9 executable

/sys/lib/acid/port
/sys/lib/acid/mips
acid: new()
75721: system call  _main ADD  $-0x14,R29
75721: breakpoint   main+0x4   MOVW  R31,0x0(R29)
acid: bpset(ls)
acid: cont()
75721: breakpoint   ls    ADD  $-0x16c8,R29
acid: stk()
At pc:0x0000141c:ls /sys/src/cmd/ls.c:87
ls(s=0x0000004d,multi=0x00000000) /sys/src/cmd/ls.c:87
    called from main+0xf4 /sys/src/cmd/ls.c:79
main(argc=0x00000000,argv=0x7ffffff0) /sys/src/cmd/ls.c:48
    called from _main+0x20 /sys/src/libc/mips/main9.s:10
acid: PC
0xc0000f60
acid: *PC
0x0000141c
acid: ls
0x0000141c
.P2
The function
.CW new()
creates a new process and stops it at the first instruction.
This change in state is reported by a call to the
Acid function
.CW stopped ,
which is called by the interpreter whenever the debugged program stops.
.CW Stopped
prints the status line giving the pid, the reason the program stopped
and the address and instruction at the current PC.
The function
.CW bpset
makes an entry in the breakpoint table and plants a breakpoint in memory.
The
.CW cont
function continues the process, allowing it to run until some condition
causes it to stop. In this case the program hits the breakpoint placed on
the function
.CW ls
in the C program. Once again the
.CW stopped
routine is called to print the status of the program. The function
.CW stk
prints a C stack trace of the current process. It is implemented using
a builtin Acid function that returns the stack trace as a list; the code
that formats the information is all written in Acid. 
The Acid variable
.CW PC
holds the address of the 
cell where the current value of the processor register
.CW PC
is stored. By indirecting through
the value of
.CW PC
the address where the program is stopped can be found.
All of the processor registers are available by the same mechanism.
.SH
Types
.PP
An Acid variable has one of four types:
.I integer ,
.I float ,
.I list ,
or
.I string .
The type of a variable is inferred from the type of the right-hand
side of the assignment expression which last set its value.
Referencing a variable that has not yet
been assigned draws a "used but not set" error. Many of the operators may
be applied to more than
one type; for these operators the action of the operator is determined by
the types of its operands. The action of each operator is defined in the
.I Expressions
section of this manual.
.SH
Variables
.PP
Acid has three kinds of variables: variables defined by the symbol table
of the debugged program, variables that are defined and maintained
by the interpreter as the debugged program changes state, and variables
defined and used by Acid programs.
.PP
Some examples of variables maintained by the interpreter are the register
pointers listed by name in the Acid list variable
.CW registers ,
and the symbol table listed by name and contents in the Acid variable
.CW symbols .
.PP
The variable
.CW pid
is updated by the interpreter to select the most recently created process
or the process selected by the
.CW setproc
builtin function.
.SH 1
Formats
.PP
In addition to a type, variables have formats. The format is a code
letter that determines the printing style and the effect of some of the
operators on that variable. The format codes are derived from the format
letters used by
.I db (1).
By default, symbol table variables and numeric constants
are assigned the format code
.CW X ,
which specifies 32-bit hexadecimal.
Printing a variable with this code yields the output
.CW 0x00123456 .
The format code of a variable may be changed from the default by using the 
builtin function
.CW fmt .
This function takes two arguments, an expression and a format code. After
the expression is evaluated the new format code is attached to the result
and forms the return value from
.CW fmt .
The backslash operator is a short form of
.CW fmt .
The format supplied by the backslash operator must be the format character
rather than an expression.
If the result is assigned to a variable the new format code is maintained
in the variable. For example:
.P1
acid: x=10
acid: print(x)
0x0000000a 
acid: x = fmt(x, 'D')
acid: print(x, fmt(x, 'X'))
10 0x0000000a
acid: x
10
acid: x\eo
12
.P2
The supported format characters are:
.RS
.IP \f(CWo\fP
Print two-byte integer in octal.
.IP \f(CWO\fP
Print four-byte integer in octal.
.IP \f(CWq\fP
Print two-byte integer in signed octal.
.IP \f(CWQ\fP
Print four-byte integer in signed octal.
.IP \f(CWB\fP
Print four-byte integer in binary.
.IP \f(CWd\fP
Print two-byte integer in signed decimal.
.IP \f(CWD\fP
Print four-byte integer in signed decimal.
.IP \f(CWV\fP
Print eight-byte integer in signed decimal.
.IP \f(CWZ\fP
Print eight-byte integer in unsigned decimal.
.IP \f(CWx\fP
Print two-byte integer in hexadecimal.
.IP \f(CWX\fP
Print four-byte integer in hexadecimal.
.IP \f(CWY\fP
Print eight-byte integer in hexadecimal.
.IP \f(CWu\fP
Print two-byte integer in unsigned decimal.
.IP \f(CWU\fP
Print four-byte integer in unsigned decimal.
.IP \f(CWf\fP
Print single-precision floating point number.
.IP \f(CWF\fP
Print double-precision floating point number.
.IP \f(CWg\fP
Print a single precision floating point number in string format.
.IP \f(CWG\fP
Print a double precision floating point number in string format.
.IP \f(CWb\fP
Print byte in hexadecimal.
.IP \f(CWc\fP
Print byte as an ASCII character.
.IP \f(CWC\fP
Like
.CW c ,
with
printable ASCII characters represented normally and
others printed in the form \f(CW\ex\fInn\fR.
.IP \f(CWs\fP
Interpret the addressed bytes as UTF characters
and print successive characters until a zero byte is reached.
.IP \f(CWr\fP
Print a two-byte integer as a rune.
.IP \f(CWR\fP
Print successive two-byte integers as runes
until a zero rune is reached.
.IP \f(CWi\fP
Print as machine instructions.
.IP \f(CWI\fP
As
.CW i
above, but print the machine instructions in
an alternate form if possible:
.CW sunsparc
and
.CW mipsco
reproduce the manufacturers' syntax.
.IP \f(CWa\fP
Print the value in symbolic form.
.RE
.SH
Complex types
.PP
Acid permits the definition of the layout of memory.
The usual method is to use the
.CW -a
flag of the compilers to produce Acid-language descriptions of data structures (see
.I 2c (1))
although such definitions can be typed interactively.
The keywords
.CW complex ,
.CW adt ,
.CW aggr ,
and
.CW union
are all equivalent; the compiler uses the synonyms to document the declarations.
A complex type is described as a set of members, each containing a format letter,
an offset in the structure, and a name.  For example, the C structure
.P1
struct List {
	int         type;
	struct List *next;
};
.P2
is described by the Acid statement
.P1
complex List {
	'D'	0	type;
	'X'	4	next;
};
.P2
.SH
Scope
.PP
Variables are global unless they are either parameters to functions
or are declared as
.CW local
in a function body. Parameters and local variables are available only in
the body of the function in which they are instantiated.
Variables are dynamically bound: if a function declares a local variable
with the same name as a global variable, the global variable will be hidden
whenever the function is executing.
For example, if a function
.CW f
has a local called
.CW main ,
any function called below
.CW f
will see the local version of
.CW main ,
not the external symbol.
.SH 1
Addressing
.PP
Since the symbol table specifies addresses,
to access the value of program variables
an extra level of indirection
is required relative to the source code.
For consistency, the registers are maintained as pointers as well; Acid variables with the names
of processor registers point to cells holding the saved registers.
.PP
The location in a file or memory image associated with
an address is calculated from a map
associated with the file.
Each map contains one or more quadruples (\c
.I t ,
.I b ,
.I e ,
.I f \|),
defining a segment named
.I t
(usually 
.CW text ,
.CW data ,
.CW regs ,
or
.CW fpregs )
mapping addresses in the range
.I b
through
.I e
to the part of the file
beginning at
offset
.I f .
The memory model of a Plan 9 process assumes
that segments are disjoint.  There
can be more than one segment of a given type (e.g., a process
may have more than one text segment) but segments
may not overlap.
An address
.I a
is translated
to a file address
by finding a segment
for which
.I b
+
.I a
<
.I e ;
the location in the file
is then
.I address
+
.I f
\-
.I b .
.PP
Usually,
the text and initialized data of a program
are mapped by segments called 
.CW text
and
.CW data .
Since a program file does not contain bss, stack, or register data,
these data are
not mapped by the data segment.
The text segment is mapped similarly in the memory image of
a normal (i.e., non-kernel) process.
However, the segment called 
.CW *data
maps memory from the beginning to the end of the program's data space.
This region contains the program's static data, the bss, the
heap and the stack.  A segment
called
.CW *regs
maps the registers;
.CW *fpregs
maps the floating point registers.
.PP
Sometimes it is useful to define a map with a single segment
mapping the region from 0 to 0xFFFFFFFF; such a map
allows the entire file to be examined
without address translation.  The builtin function
.CW map
examines and modifies Acid's map for a process.
.SH 1
Name Conflicts
.PP
Name conflicts between keywords in the Acid language, symbols in the program,
and previously defined functions are resolved when the interpreter starts up.
Each name is made unique by prefixing enough
.CW $
characters to the front of the name to make it unique. Acid reports
a list of each name change at startup. The report looks like this:
.P1
/bin/sam: mips plan 9 executable
/lib/acid/port
/lib/acid/mips
Symbol renames:
	append=$append T/0xa4e40
acid:
.P2
The symbol
.CW append
is both a keyword and a text symbol in the program. The message reports
that the text symbol is now named
.CW $append .
.SH
Expressions
.PP
Operators have the same
binding and precedence as in C.
For operators of equal precedence, expressions are evaluated from left to right. 
.SH 1
Boolean expressions
.PP
If an expression is evaluated for a boolean condition the test
performed depends on the type of the result. If the result is of
.I integer
or
.I floating
type the result is true if the value is non-zero. If the expression is a
.I list
the result is true if there are any members in the list.
If the expression is a
.I string
the result is true if there are any characters in the string.
.DS
	primary-expression:
		identifier
		identifier \f(CW:\fP identifier
		constant
		\f(CW(\fP expression \f(CW)\fP
		\f(CW{\fP elist \f(CW}\fP

	elist:
		expression
		elist , expression
.DE
An identifier may be any legal Acid variable. The colon operator returns the
address of parameters or local variables in the current stack of a program.
For example:
.P1
*main:argc
.P2
prints the number of arguments passed into main. Local variables and parameters
can only be referenced after the frame has been established. It may be necessary to
step a program over the first few instructions of a breakpointed function to properly set
the frame.
.PP
Constants follow the same lexical rules as C.
A list of expressions delimited by braces forms a list constructor.
A new list is produced by evaluating each expression when the constructor is executed.
The empty list is formed from
.CW {} .
.P1
acid: x = 10
acid: l = { 1, x, 2\eD }
acid: x = 20
acid: l
{0x00000001 , 0x0000000a , 2 }
.P2
.SH 1
Lists
.PP
Several operators manipulate lists.
.DS
	list-expression:
		primary-expression
		\f(CWhead\fP primary-expression
		\f(CWtail\fP primary-expression
		\f(CWappend\fP expression \f(CW,\fP primary-expression
		\f(CWdelete\fP expression \f(CW,\fP primary-expression
.DE
The
.I primary-expression
for
.CW head
and
.CW tail
must yield a value of type
.I list .
If there are no elements in the list the value of
.CW head
or
.CW tail
will be the empty list. Otherwise
.CW head
evaluates to the first element of the list and
.CW tail
evaluates to the rest.
.P1
acid: head {}
{}
acid: head {1, 2, 3, 4}
0x00000001 
acid: tail {1, 2, 3, 4}
{0x00000002 , 0x00000003 , 0x00000004 }
.P2
The first operand of
.CW append 
and
.CW delete
must be an expression that yields a
.I list .
.CW Append
places the result of evaluating
.I primary-expression
at the end of the list.
The
.I primary-expression
supplied to
.CW delete
must evaluate to an integer;
.CW delete
removes the 
.I n 'th
item from the list, where
.I n
is integral value of
.I primary-expression.
List indices are zero-based.
.P1
	acid: append {1, 2}, 3
	{0x00000001 , 0x00000002 , 0x00000003 }
	acid: delete {1, 2, 3}, 1
	{0x00000001 , 0x00000003 }
.P2
.PP
Assigning a list to a variable copies a reference to the list; if a list variable
is copied it still points at the same list.  To copy a list, the elements must
be copied piecewise using
.CW head
and
.CW append .
.SH 1
Operators
.PP
.DS
	postfix-expression:
		list-expression
		postfix-expression \f(CW[\fP expression \f(CW]\fP
		postfix-expression \f(CW(\fP argument-list \f(CW)\fP
		postfix-expression \f(CW.\fP tag
		postfix-expression \f(CW->\fP tag 
		postfix-expression \f(CW++\fP
		postfix-expression \f(CW--\fP

	argument-list:
		expression
		argument-list , expression
.DE
The
.CW [
.I expression
.CW ]
operator performs indexing.
The indexing expression must result in an expression of
.I integer
type, say
.I n .
The operation depends on the type of
.I postfix-expression .
If the
.I postfix-expression
yields an
.I integer
it is assumed to be the base address of an array in the memory image.
The index offsets into this array; the size of the array members is
determined by the format associated with the
.I postfix-expression .
If the 
.I postfix-expression
yields a
.I string
the index operator fetches the
.I n 'th
character
of the string. If the index points beyond the end
of the string, a zero is returned.
If the
.I postfix-expression
yields a
.I list
then the indexing operation returns the
.I n 'th
item of the list.
If the list contains less than
.I n
items the empty list
.CW {}
is returned.
.PP
The
.CW ++
and
.CW --
operators increment and decrement integer variables.
The amount of increment or decrement depends on the format code. These postfix
operators return the value of the variable before the increment or decrement
has taken place.
.DS
	unary-expression:
		postfix-expression
		\f(CW++\fP unary-expression
		\f(CW--\fP unary-expression

	unary-operator: one of
		\f(CW*\fP \f(CW@\fP \f(CW+\fP \f(CW-\fP ~ \f(CW!\fP
.DE
The operators
.CW *
and
.CW @
are the indirection operators.
.CW @
references a value from the text file of the program being debugged.
The size of the value depends on the format code. The
.CW *
operator fetches a value from the memory image of a process. If either
operator appears on the left-hand side of an assignment statement, either the file
or memory will be written. The file can only be modified when Acid is invoked
with the
.CW -w
option.
The prefix
.CW ++
and
.CW --
operators perform the same operation as their postfix counterparts but
return the value after the increment or decrement has been performed. Since the
.CW ++
and
.CW *
operators fetch and increment the correct amount for the specified format,
the following function prints correct machine instructions on a machine with
variable length instructions, such as the 68020 or 386:
.P1
	defn asm(addr)
	{
		addr = fmt(addr, 'i');
		loop 1, 10 do
			print(*addr++, "\en");
	}
.P2
The operators
.CW ~
and
.CW !
perform bitwise and logical negation respectively. Their operands must be of
.I integer
type.
.DS
	cast-expression:
		unary-expression
		unary-expression \f(CW\e\fP format-char
		\f(CW(\fP complex-name \f(CW)\fP unary-expression		
.DE
A unary expression may be preceded by a cast. The cast has the effect of
associating the value of 
.I unary-expression
with a complex type structure.
The result may then be dereferenced using the
.CW .
and
.CW ->
operators.
.PP
An Acid variable may be associated with a complex type
to enable accessing the type's members:
.P1
acid: complex List {
	'D'	0	type;
	'X'	4	next;
};
acid: complex List lhead
acid: lhead.type
10
acid: lhead = ((List)lhead).next
acid: lhead.type
-46
.P2
Note that the
.CW next
field cannot be given a complex type automatically.
.PP
When entered at the top level of the interpreter,
an expression of complex type
is treated specially.
If the type is called
.CW T
and an Acid function also called
.CW T
exists,
then that function will be called with the expression as its argument.
The compiler options
.CW -a
and
.CW -aa
will generate Acid source code defining such complex types and functions; see
.I 2c (1).
.PP
A
.I unary-expression
may be qualified with a format specifier using the
.CW \e
operator. This has the same effect as passing the expression to the
.CW fmt
builtin function.
.DS
	multiplicative-expression:
		cast-expression
		multiplicative-expression \f(CW*\fP multiplicative-expression
		multiplicative-expression \f(CW/\fP multiplicative-expression
		multiplicative-expression \f(CW%\fP multiplicative-expression
.DE
These operate on
.I integer
and 
.I float
types and perform the expected operations:
.CW *
multiplication,
.CW /
division,
.CW %
modulus.
.DS
	additive-expression:
		multiplicative-expression
		additive-expression \f(CW+\fP multiplicative-expression
		additive-expression \f(CW-\fP multiplicative-expression
.DE
These operators perform as expected for
.I integer
and 
.I float
operands.
Unlike in C,
.CW +
and
.CW -
do not scale the addition based on the format of the expression.
This means that
.CW i=i+1
will always add 1 but
.CW i++
will add the size corresponding to the format stored with
.CW i .
If both operands are of either
.I string
or
.I list
type then addition is defined as concatenation. 
Adding a string and an integer is treated as concatenation
with the Unicode character corresponding to the integer.
Subtraction is undefined for strings and lists.
.DS
	shift-expression:
		additive-expression
		shift-expression \f(CW<<\fP additive-expression
		shift-expression \f(CW>>\fP additive-expression
.DE
The
.CW >>
and
.CW <<
operators perform bitwise right and left shifts respectively. Both
require operands of
.I integer
type.
.DS
	relational-expression:
		relational-expression \f(CW<\fP shift-expression
		relational-expression \f(CW>\fP shift-expression
		relational-expression \f(CW<=\fP shift-expression
		relational-expression \f(CW>=\fP shift-expression

	equality-expression:
		relational-expression
		relational-expression \f(CW==\fP equality-expression
		relational-expression \f(CW!=\fP equality-expression
.DE
The comparison operators are
.CW <
(less than),
.CW >
(greater than),
.CW <=
(less than or equal to),
.CW >=
(greater than or equal to),
.CW ==
(equal to) and
.CW !=
(not equal to). The result of a comparison is 0
if the condition is false, otherwise 1. The relational operators can only be
applied to operands of
.I integer
and
.I float
type. The equality operators apply to all types.  Comparing mixed types is legal.
Mixed integer and float compare on the integral value.  Other mixtures are always unequal.
Two lists are equal if they
have the same number of members and a pairwise comparison of the members results
in equality.
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
These operators perform bitwise logical operations and apply only to the
.I integer
type.
The operators are
.CW &
(logical and),
.CW ^
(exclusive or) and
.CW |
(inclusive or).
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
operator returns 1 if both of its operands evaluate to boolean true, otherwise 0.
The
.CW ||
operator returns 1 if either of its operands evaluates to boolean true,
otherwise 0.
.SH
Statements
.PP
.DS
	\f(CWif\fP expression \f(CWthen\fP statement \f(CWelse\fP statement
	\f(CWif\fP expression \f(CWthen\fP statement
.DE
The
.I expression
is evaluated as a boolean. If its value is true the statement after
the
.CW then
is executed, otherwise the statement after the
.CW else
is executed. The 
.CW else
portion may be omitted.
.DS
	\f(CWwhile\fP expression \f(CWdo\fP statement
.DE
In a while loop, the
.I statement
is executed while the boolean
.I expression
evaluates
true.
.DS
	\f(CWloop\fP startexpr, endexpr \f(CWdo\fP statement
.DE
The two expressions
.I startexpr
and
.I endexpr
are evaluated prior to loop entry.
.I Statement
is evaluated while the value of
.I startexpr
is less than or equal to
.I endexpr .
Both expressions must yield
.I integer
values. The value of
.I startexpr
is
incremented by one for each loop iteration.
Note that there is no explicit loop variable; the
.I expressions
are just values.
.DS
	\f(CWreturn\fP expression
.DE
.CW return
terminates execution of the current function and returns to its caller.
The value of the function is given by expression. Since
.CW return
requires an argument, nil-valued functions should return the empty list
.CW {} .
.DS
	\f(CWlocal\fP variable
.DE
The
.CW local
statement creates a local instance of
.I variable ,
which exists for the duration
of the instance of the function in which it is declared. Binding is dynamic: the local variable,
rather than the previous value of
.I variable ,
is visible to called functions.
After a return from the current function the previous value of
.I variable
is
restored.
.PP
If Acid is interrupted, the values of all local variables are lost,
as if the function returned.
.DS
	\f(CWdefn\fP function-name \f(CW(\fP parameter-list \f(CW)\fP body

	parameter-list:
		variable
		parameter-list , variable

	body:
		\f(CW{\fP statement \f(CW}\fP
.DE
Functions are introduced by the
.CW defn
statement. The definition of parameter names suppresses any variables
of the same name until the function returns. The body of a function is a list
of statements enclosed by braces.
.SH
Code variables
.PP
Acid permits the delayed evaluation of a parameter to a function.  The parameter
may then be evaluated at any time with the
.CW eval
operator.  Such parameters are called
.I "code variables
and are defined by prefixing their name with an asterisk in their declaration.
.PP
For example, this function wraps up an expression for later evaluation:
.P1
acid: defn code(*e) { return e; }
acid: x = code(v+atoi("100")\eD)
acid: print(x)
(v+atoi("100"))\eD;
acid: eval x
<stdin>:5: (error) v used but not set
acid: v=5
acid: eval x
105
.P2
.SH
Source Code Management
.PP
Acid provides the means to examine source code. Source code is
represented by lists of strings. Builtin functions provide mapping
from address to lines and vice-versa. The default debugging environment
has the means to load and display source files.
.SH
Builtin Functions
.PP
The Acid interpreter has a number of builtin functions, which cannot be redefined.
These functions perform machine- or operating system-specific functions such as
symbol table and process management.
The following section presents a description of each builtin function.
The notation
.CW {}
is used to denote the empty list, which is the default value of a function that
does not execute a
.CW return 
statement.
The type and number of parameters for each function are specified in the
description; where a parameter can be of any type it is specified as type
.I item .
.de Ip
.KS
.in 0
.LP
.ie h \&\f2\\$1\fP\ \ \f(CW\\$2(\f2\\$3\f(CW)\f1\ \ \ \ \ \ \ \ \\$4
.el .tl '\f2\\$1\fP\ \ \f(CW\\$2(\f2\\$3\f(CW)\f1''\\$4'
.IP
..
.de Ex
.KE
.KS
.IP
.ft CW
.ta 4n +4n +4n +4n +4n +4n +4n +4n +4n +4n +4n +4n +4n +4n +4n +4n
.nf
.in +4n
.br
..
.de Ee
.fi
.ft 1
.br
.KE
..
.\"
.\"
.\"
.Ip integer access string "Check if a file can be read
.CW Access
returns the integer 1 if the file name in
.I string
can be read by the builtin functions
.CW file ,
.CW readfile ,
or
.CW include ,
otherwise 0. A typical use of this function is to follow
a search path looking for a source file; it is used by
.CW findsrc .
.Ex
if access("main.c") then
	return file("main.c");
.Ee
.\"
.\"
.\"
.Ip float atof string "Convert a string to float
.CW atof
converts the string supplied as its argument into a floating point
number. The function accepts strings in the same format as the C
function of the same name. The value returned has the format code
.CW f .
.CW atof
returns the value 0.0 if it is unable to perform the conversion.
.Ex
acid: +atof("10.4e6")
1.04e+07
.Ee
.\"
.\"
.\"
.Ip integer atoi string "Convert a string to an integer
.CW atoi
converts the argument
.i string
to an integer value.
The function accepts strings in the same format as the C function of the
same name. The value returned has the format code
.CW D .
.CW atoi
returns the integer 0 if it is unable to perform a conversion.
.Ex
acid: +atoi("-1255")
-1255
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP error string "Generate an interpreter error
.CW error
generates an error message and returns the interpreter to interactive
mode. If an Acid program is running, it is aborted.
Processes being debugged are not affected. The values of all local variables are lost.
.CW error
is commonly used to stop the debugger when some interesting condition arises
in the debugged program.
.Ex
while 1 do {
	step();
	if *main != @main then
		error("memory corrupted");
}
.Ee
.\"
.\"
.\"
.Ip list file string "Read the contents of a file into a list
.CW file
reads the contents of the file specified by
.I string
into a list.
Each element in the list is a string corresponding to a line in the file.
.CW file
breaks lines at the newline character, but the newline
characters are not returned as part each string.
.CW file
returns the empty list if it encounters an error opening or reading the data.
.Ex
acid: print(file("main.c")[0])
#include	<u.h>
.Ee
.\"
.\"
.\"
.Ip integer filepc string "Convert source address to text address
.CW filepc
interprets its
.I string
argument as a source file address in the form of a file name and line offset.
.CW filepc
uses the symbol table to map the source address into a text address
in the debugged program. The
.I integer
return value has the format
.CW X .
.CW filepc
returns an address of -1 if the source address is invalid.
The source file address uses the same format as
.I acme (1).
This function is commonly used to set breakpoints from the source text.
.Ex
acid: bpset(filepc("main:10"))
acid: bptab()
	0x00001020 usage  ADD	$-0xc,R29
.Ee
.\"
.\"
.\"
.Ip item fmt item,fmt "Set print, \f(CW@\fP and \f(CW*\fP formats
.CW fmt
evaluates the expression
.I item
and sets the format of the result to
.I fmt .
The format of a value determines how it will be printed and
what kind of object will be fetched by the
.CW *
and
.CW @
operators. The
.CW \e
operator is a short-hand form of the
.CW fmt
builtin function. The
.CW fmt
function leaves the format of the
.I item
unchanged.
.Ex
acid: main=fmt(main, 'i') // as instructions
acid: print(main\eX, "\et", *main)
0x00001020 ADD	$-64,R29
.Ee
.\"
.\"
.\"
.Ip fmt fmtof item "Get format
.CW fmtof
evaluates the expression
.I item
and returns the format of the result.
.Ex
acid: +fmtof(33)
W
acid: +fmtof("string")
s
.Ee
.\"
.\"
.\"
.Ip integer fmtsize item "Get format size
.CW fmtsize
evaluates the expression
.I item
and returns the size in bytes of a single element of result's format.
.Ex
acid: +fmtsize('c')
8
acid: +fmtsize('c'\ec)
1
acid: +fmtsize(0\eX)
4
acid: +fmtsize('c'\e3)
10
.Ee
.\"
.\"
.\"
.Ip list fnbound integer "Find start and end address of a function
.CW fnbound
interprets its
.I integer
argument as an address in the text of the debugged program.
.CW fnbound
returns a list containing two integers corresponding to
the start and end addresses of the function containing the supplied address.
If the
.I integer
address is not in the text segment of the program then the empty list is returned.
.CW fnbound
is used by
.CW next
to detect stepping into new functions.
.Ex
acid: print(fnbound(main))
{0x00001050, 0x000014b8}
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP follow integer "Compute follow set
The follow set is defined as the set of program counter values that could result
from executing an instruction.
.CW follow
interprets its
.I integer
argument as a text address, decodes the instruction at
that address and, with the current register set, builds a list of possible
next program counter values. If the instruction at the specified address
cannot be decoded
.CW follow
raises an error.
.CW follow
is used to plant breakpoints on
all potential paths of execution. The following code fragment
plants breakpoints on top of all potential following instructions.
.Ex
lst = follow(*PC);
while lst do
{
	*head lst = bpinst;
	lst = tail lst;
}
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP include string "Take input from a new file
.CW include
opens the file specified by
.I string
and uses its contents as command input to the interpreter.
The interpreter restores input to its previous source when it encounters
either an end of file or an error.
.CW include
can be used to incrementally load symbol table information without
leaving the interpreter.
.Ex
acid: include("/sys/src/cmd/acme/syms")
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP interpret string "Take input from a string
.CW interpret
evaluates the
.I string
expression and uses its result as command input for the interpreter.
The interpreter restores input to its previous source when it encounters
either the end of string or an error. The
.CW interpret
function allows Acid programs to write Acid code for later evaluation.
.Ex
acid: interpret("main+10;")
0x0000102a
.Ee
.\"
.\"
.\"
.Ip string itoa integer[,string] "Convert integer to string
.CW itoa
takes an integer argument and converts it into an ASCII string
in the
.CW D
format.
an alternate format string
may be provided in the
.CW %
style of
.I print (2).
This function is commonly used to build
.CW rc
command lines.
.Ex
acid: rc("cat /proc/"+itoa(pid)+"/segment")
Stack    7fc00000 80000000    1
Data     00001000 00009000    1
Data     00009000 0000a000    1
Bss      0000a000 0000c000    1
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP kill integer "Kill a process
.CW kill
writes a kill control message into the control file of the process
specified by the
.I integer
pid.
If the process was previously installed by
.CW setproc
it will be removed from the list of active processes.
If the
.I integer
has the same value as
.CW pid ,
then
.CW pid
will be set to 0.
To continue debugging, a new process must be selected using
.CW setproc .
For example, to kill all the active processes:
.Ex
while proclist do {
	kill(head proclist);
	proclist = tail proclist;
}
.Ee
.\"
.\"
.\"
.Ip list map list "Set or retrieve process memory map
.CW map
either retrieves all the mappings associated with a process or sets a single
map entry to a new value.
If the
.I list
argument is omitted then
.CW map
returns a list of lists. Each sublist has four values and describes a
single region of contiguous addresses in the
memory or file image of the debugged program. The first entry is the name of the
mapping. If the name begins with
.CW *
it denotes a map into the memory of an active process.
The second and third values specify the base and end
address of the region and the fourth number specifies the offset in the file
corresponding to the first location of the region.
A map entry may be set by supplying a list in the same format as the sublist
described above. The name of the mapping must match a region already defined
by the current map.
Maps are set automatically for Plan 9 processes and some kernels; they may
need to be set by hand for other kernels and programs that run on bare hardware.
.Ex
acid: map({"text", _start, end, 0x30})
.Ee
.\"
.\"
.\"
.Ip integer match item,list "Search list for matching value
.CW match
compares each item in
.I list
using the equality operator
.CW ==
with
.I item .
The
.I item
can be of any type. If the match succeeds the result is the integer index
of the matching value, otherwise -1.
.Ex
acid: list={8,9,10,11}
acid: print(list[match(10, list)]\eD)
10
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP newproc string "Create a new process
.CW newproc
starts a new process with an argument vector constructed from
.I string .
The argument vector excludes the name of the program to execute and
each argument in
.I string
must be space separated. A new process can accept no more
than 512 arguments. The internal variable
.CW pid
is set to the pid of the newly created process. The new pid
is also appended to the list of active processes stored in the variable
.CW proclist .
The new process is created then halted at the first instruction, causing
the debugger to call
.CW stopped .
The library functions
.CW new
and
.CW win
should be used to start processes when using the standard debugging
environment.
.Ex
acid: newproc("-l .")
56720: system call	_main	ADD	$-0x14,R29
.Ee
.\"
.\"
.\"
.Ip string pcfile integer "Convert text address to source file name
.CW pcfile
interprets its
.I integer
argument as a text address in the debugged program. The address and symbol table
are used to generate a string containing the name of the source file
corresponding to the text address. If the address does not lie within the
program the string
.CW ?file?
is returned.
.Ex
acid: print("Now at ", pcfile(*PC), ":", pcline(*PC))
Now at ls.c:46 
.Ee
.\"
.\"
.\"
.Ip integer pcline integer "Convert text address to source line number
.CW pcline
interprets its
.I integer
argument as a text address in the debugged program. The address and symbol table
are used to generate an integer containing the line number in the source file
corresponding to the text address. If the address does not lie within the
program the integer 0 is returned.
.Ex
acid: +file("main.c")[pcline(main)]
main(int argc, char *argv[])
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP print item,item,... "Print expressions
.CW print
evaluates each
.I item
supplied in its argument list and prints it to standard output. Each
argument will be printed according to its associated format character.
When the interpreter is executing, output is buffered and flushed every
5000 statements or when the interpreter returns to interactive mode.
.CW print
accepts a maximum of 512 arguments.
.Ex
acid: print(10, "decimal ", 10\eD, "octal ", 10\eo)
0x0000000a decimal 10 octal 000000000012 
acid: print({1, 2, 3})
{0x00000001 , 0x00000002 , 0x00000003 }
acid: print(main, main\ea, "\et", @main\ei)
0x00001020 main	ADD	$-64,R29
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP printto string,item,item,... "Print expressions to file
.CW printto
offers a limited form of output redirection. The first
.I string
argument is used as the path name of a new file to create.
Each
.I item
is then evaluated and printed to the newly created file. When all items
have been printed the file is closed.
.CW printto
accepts a maximum of 512 arguments.
.Ex
acid: printto("/env/foo", "hello")
acid: rc("echo -n $foo")
hello
.Ee
.\"
.\"
.\"
.Ip string rc string "Execute a shell command
.CW rc
evaluates
.I string
to form a shell command. A new command interpreter is started
to execute the command. The Acid interpreter blocks until the command
completes. The return value is the empty string
if the command succeeds, otherwise the exit status of the failed command.
.Ex
acid: rc("B "+itoa(-pcline(addr))+" "+pcfile(addr));
.Ee
.\"
.\"
.\"
.Ip string readfile string "Read file contents into a string
.CW readfile
takes the contents of the file specified by
.I string
and returns its contents as a new string.
If
.CW readfile
encounters a zero byte in the file, it terminates.
If
.CW readfile
encounters an error opening or reading the file then the empty list
is returned.
.CW readfile
can be used to read the contents of device files whose lines are not
terminated with newline characters.
.Ex
acid: ""+readfile("/dev/label")
helix
.Ee
.\"
.\"
.\"
.Ip string reason integer "Print cause of program stoppage
.CW reason
uses machine-dependent information to generate a string explaining
why a process has stopped. The
.I integer
argument is the value of an architecture dependent status register,
for example
.CW CAUSE
on the MIPS.
.Ex
acid: print(reason(*CAUSE))
system call
.Ee
.\"
.\"
.\"
.Ip integer regexp pattern,string "Regular expression match
.CW regexp
matches the
.I pattern
string supplied as its first argument with the 
.I string
supplied as its second.
If the pattern matches the result is the value 1, otherwise 0.
.Ex
acid: print(regexp(".*bar", "foobar"))
1
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP setproc integer "Set debugger focus
.CW setproc
selects the default process used for memory and control operations. It effectively
shifts the focus of control between processes. The 
.I integer
argument specifies the pid of the process to look at.
The variable
.CW pid
is set to the pid of the selected process. If the process is being
selected for the first time its pid is added to the list of active
processes
.CW proclist .
.Ex
acid: setproc(68382)
acid: procs()
>68382: Stopped at main+0x4 setproc(68382)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP start integer "Restart execution
.CW start
writes a
.CW start
message to the control file of the process specified by the pid
supplied as its
.I integer
argument.
.CW start
draws an error if the process is not in the
.CW Stopped
state.
.Ex
acid: start(68382)
acid: procs()
>68382: Running at main+0x4 setproc(68382)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP startstop integer "Restart execution, block until stopped
.CW startstop
performs the same actions as a call to
.CW start
followed by a call to
.CW stop .
The
.I integer
argument specifies the pid of the process to control. The process
must be in the
.CW Stopped
state.
Execution is restarted, the debugger then waits for the process to
return to the
.CW Stopped
state. A process will stop if a startstop message has been written to its control
file and any of the following conditions becomes true: the process executes or returns from
a system call, the process generates a trap or the process receives a note.
.CW startstop
is used to implement single stepping.
.Ex
acid: startstop(pid)
75374: breakpoint	ls	ADD	$-0x16c8,R29
.Ee
.\"
.\"
.\"
.Ip string status integer "Return process state
.CW status
uses the pid supplied by its
.I integer
argument to generate a string describing the state of the process.
The string corresponds to the state returned by the
sixth column of the
.I ps (1)
command.
A process must be in the
.CW Stopped
state to modify its memory or registers.
.Ex
acid: ""+status(pid)
Stopped
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP stop integer "Wait for a process to stop
.CW stop
writes a
.CW stop
message to the control file of the process specified by the
pid supplied as its
.I integer
argument.
The interpreter blocks until the debugged process enters the
.CW Stopped
state.
A process will stop if a stop message has been written to its control
file and any of the following conditions becomes true: the process executes or returns from
a system call, the process generates a trap, the process is scheduled or the
process receives a note.
.CW stop
is used to wait for a process to halt before planting a breakpoint since Plan 9
only allows a process's memory to be written while it is in the
.CW Stopped
state.
.Ex
defn bpset(addr) {
	if (status(pid)!="Stopped") then {
		print("Waiting...\en");
		stop(pid);
	}
	...
}
.Ee
.\"
.\"
.\"
.Ip list strace pc,sp,linkreg "Stack trace
.CW strace
generates a list of lists corresponding to procedures called by the debugged
program. Each sublist describes a single stack frame in the active process.
The first element is an
.I integer
of format
.CW X
specifying the address of the called function. The second element is the value
of the program counter when the function was called. The third and fourth elements
contain lists of parameter and automatic variables respectively.
Each element of these lists
contains a string with the name of the variable and an
.I integer
value of format
.CW X
containing the current value of the variable.
The arguments to
.CW strace
are the current value of the program counter, the current value of the
stack pointer, and the address of the link register. All three parameters
must be integers.
The setting of 
.I linkreg
is architecture dependent. On the MIPS linkreg is set to the address of saved
.CW R31 ,
on the SPARC to the address of saved
.CW R15 .
For the other architectures
.I linkreg
is not used, but must point to valid memory.
.Ex
acid: print(strace(*PC, *SP, linkreg))
{{0x0000141c, 0xc0000f74,
{{"s", 0x0000004d}, {"multi", 0x00000000}}, 
{{"db", 0x00000000}, {"fd", 0x000010a4},
{"n", 0x00000001}, {"i", 0x00009824}}}}
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP waitstop integer "Wait for a process to stop
.CW waitstop
writes a waitstop message to the control file of the process specified by the
pid supplied as its
.I integer
argument.
The interpreter will remain blocked until the debugged process enters the
.CW Stopped
state.
A process will stop if a waitstop message has been written to its control
file and any of the following conditions becomes true: the process generates a trap
or receives a note. Unlike
.CW stop ,
the
.CW waitstop
function is passive; it does not itself cause the program to stop.
.Ex
acid: waitstop(pid)
75374: breakpoint	ls	ADD	$-0x16c8,R29
.Ee
.\"
.\"
.\"
.SH
Library Functions
.PP
A standard debugging environment is provided by modules automatically
loaded when
Acid is started.
These modules are located in the directory
.CW /sys/lib/acid .
These functions may be overridden, personalized, or added to by code defined in
.CW $home/lib/acid .
The implementation of these functions can be examined using the
.CW whatis
operator and then modified during debugging sessions.
.\"
.\"
.\"
.Ip \f(CW{}\fP Bsrc integer "Load editor with source
.CW Bsrc
interprets the
.I integer
argument as a text address. The text address is used to produce a pathname
and line number suitable for the
.CW B
command
to send to the text editor
.I sam (1)
or
.I acme (1).
.CW Bsrc
builds an
.I rc (1)
command to invoke
.CW B ,
which either selects an existing source file or loads a new source file into the editor.
The line of source corresponding to the text address is then selected.
In the following example
.CW stopped
is redefined so that the editor
follows and displays the source line currently being executed.
.Ex
defn stopped(pid) {
	pstop(pid);
	Bsrc(*PC);
}
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP Fpr "" "Display double precision floating registers
For machines equipped with floating point,
.CW Fpr
displays the contents of the floating point registers as double precision
values.
.Ex
acid: Fpr()
F0   0.	F2   0.
F4   0.	F6   0.
F8   0.	F10  0.
\&...
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP Ureg integer "Display contents of Ureg structure
.CW Ureg
interprets the integer passed as its first argument as the address of a
kernel
.CW Ureg
structure. Each element of the structure is retrieved and printed.
The size and contents of the
.CW Ureg
structure are architecture dependent.
This function can be used to decode the first argument passed to a
.I notify (2)
function after a process has received a note.
.Ex
acid: Ureg(*notehandler:ur)
	status	0x3000f000
	pc	0x1020
	sp	0x7ffffe00
	cause	0x00004002
\&...
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP acidinit "" "Interpreter startup
.CW acidinit
is called by the interpreter after all
modules have been loaded at initialization time.
It is used to set up machine specific variables and the default source path.
.CW acidinit
should not be called by user code.
.KE
.\"
.\"
.\"
.Ip \f(CW{}\fP addsrcdir string "Add element to source search path
.CW addsrcdir
interprets its string argument as a new directory
.CW findsrc
should search when looking for source code files.
.CW addsrcdir
draws an error if the directory is already in the source search path. The search
path may be examined by looking at the variable
.CW srcpath .
.Ex
acid: rc("9fs fornax")
acid: addsrcpath("/n/fornax/sys/src/cmd")
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP asm integer "Disassemble machine instructions
.CW asm
interprets its integer argument as a text address from which to disassemble
machine instructions.
.CW asm
prints the instruction address in symbolic and hexadecimal form, then prints
the instructions with addressing modes. Up to twenty instructions will
be disassembled.
.CW asm
stops disassembling when it reaches the end of the current function.
Instructions are read from the file image using the
.CW @
operator.
.Ex
acid: asm(main)
main     0x00001020 ADD    $-0x64,R29
main+0x4 0x00001024 MOVW   R31,0x0(R29)
main+0x8 0x00001028 MOVW   R1,argc+4(FP)
main+0xc 0x0000102c MOVW   $bin(SB),R1
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP bpdel integer "Delete breakpoint
.CW bpdel
removes a previously set breakpoint from memory.
The
.I integer
supplied as its argument must be the address of a previously set breakpoint.
The breakpoint address is deleted from the active breakpoint list
.CW bplist ,
then the original instruction is copied from the file image to the memory
image so that the breakpoint is removed.
.Ex
acid: bpdel(main+4)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP bpset integer "Set a breakpoint
.CW bpset
places a breakpoint instruction at the address specified
by its
.I integer
argument, which must be in the text segment.
.CW bpset
draws an error if a breakpoint has already been set at the specified address.
A list of current breakpoints is maintained in the variable
.CW bplist .
Unlike in
.I db (1),
breakpoints are left in memory even when a process is stopped, and
the process must exist, perhaps by being
created by either
.CW new
or
.CW win ,
in order to place a breakpoint.
.CW Db "" (
accepts breakpoint commands before the process is started.)
On the
MIPS and SPARC architectures,
breakpoints at function entry points should be set 4 bytes into the function
because the
instruction scheduler may fill
.CW JAL
branch delay slots with the first instruction of the function.
.Ex
acid: bpset(main+4)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP bptab "" "List active breakpoints
.CW bptab
prints a list of currently installed breakpoints. The list contains the
breakpoint address in symbolic and hexadecimal form as well as the instruction
the breakpoint replaced. Breakpoints are not maintained across process creation
using
.CW new
and
.CW win .
They are maintained across a fork, but care must be taken to keep control of
the child process.
.Ex
acid: bpset(ls+4)
acid: bptab()
	0x00001420 ls+0x4  MOVW	R31,0x0(R29)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP casm "" "Continue disassembly
.CW casm
continues to disassemble instructions from where the last
.CW asm
or
.CW casm
command stopped. Like
.CW asm ,
this command stops disassembling at function boundaries.
.Ex
acid: casm()
main+0x10 0x00001030	MOVW	$0x1,R3
main+0x14 0x00001034	MOVW	R3,0x8(R29)
main+0x18 0x00001038	MOVW	$0x1,R5
main+0x1c 0x0000103c	JAL	Binit(SB)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP cont "" "Continue program execution
.CW cont
restarts execution of the currently active process.
If the process is stopped on a breakpoint, the breakpoint is first removed,
the program is single stepped, the breakpoint is replaced and the program
is then set executing. This may cause
.CW stopped()
to be called twice.
.CW cont
causes the interpreter to block until the process enters the
.CW Stopped
state.
.Ex
acid: cont()
95197: breakpoint	ls+0x4	MOVW	R31,0x0(R29)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP dump integer,integer,string "Formatted memory dump
.CW dump
interprets its first argument as an address, its second argument as a
count and its third as a format string.
.CW dump
fetches an object from memory at the current address and prints it according
to the format. The address is incremented by the number of bytes specified by
the format and the process is repeated count times. The format string is any
combination of format characters, each preceded by an optional count.
For each object,
.CW dump
prints the address in hexadecimal, a colon, the object and then a newline.
.CW dump
uses
.CW mem
to fetch each object.
.Ex
acid: dump(main+35, 4, "X2bi")
0x00001043: 0x0c8fa700 108 143 lwc2 r0,0x528f(R4) 
0x0000104d: 0xa9006811   0   0 swc3 r0,0x0(R24) 
0x00001057: 0x2724e800   4  37 ADD  $-0x51,R23,R31 
0x00001061: 0xa200688d   6   0 NOOP
0x0000106b: 0x2710c000   7   0 BREAK
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP findsrc string "Use source path to load source file
.CW findsrc
interprets its
.I string
argument as a source file. Each directory in the source path is searched
in turn for the file. If the file is found, the source text is loaded using
.CW file
and stored in the list of active source files called
.CW srctext .
The name of the file is added to the source file name list
.CW srcfiles .
Users are unlikely to call
.CW findsrc
from the command line, but may use it from scripts to preload source files
for a debugging session. This function is used by
.CW src
and
.CW line
to locate and load source code. The default search path for the MIPS
is
.CW ./ ,
.CW /sys/src/libc/port ,
.CW /sys/src/libc/9sys ,
.CW /sys/src/libc/mips .
.Ex
acid: findsrc(pcfile(main));
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP fpr "" "Display single precision floating registers
For machines equipped with floating point,
.CW fpr
displays the contents of the floating point registers as single precision
values. When the interpreter stores or manipulates floating point values
it converts into double precision values.
.Ex
acid: fpr()
F0   0.	F1   0.
F2   0.	F3   0.
F4   0.	F5   0.
\&...
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP func "" "Step while in function
.CW func
single steps the active process until it leaves the current function
by either calling another function or returning to its caller.
.CW func
will execute a single instruction after leaving the current function.
.Ex
acid: func()
95197: breakpoint	ls+0x8	MOVW	R1,R8
95197: breakpoint	ls+0xc	MOVW	R8,R1
95197: breakpoint	ls+0x10	MOVW	R8,s+4(FP)
95197: breakpoint	ls+0x14	MOVW	$0x2f,R5
95197: breakpoint	ls+0x18	JAL	utfrrune(SB)
95197: breakpoint	utfrrune	ADD	$-0x18,R29
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP gpr "" "Display general purpose registers
.CW gpr
prints the values of the general purpose processor registers.
.Ex
acid: gpr()
R1	0x00009562 R2	0x000010a4 R3	0x00005d08
R4	0x0000000a R5	0x0000002f R6	0x00000008
\&...
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP labstk integer "Print stack trace from label
.CW labstk
performs a stack trace from a Plan 9
.I label.
The kernel,
C compilers store continuations in a common format. Since the
compilers all use caller save conventions a continuation may be saved by
storing a
.CW PC
and
.CW SP
pair. This data structure is called a label and is used by the
the C function
.CW longjmp
and the kernel to schedule threads and processes.
.CW labstk
interprets its
.I integer
argument as the address of a label and produces a stack trace for
the thread of execution. The value of the function
.CW ALEF_tid
is a suitable argument for
.CW labstk .
.Ex
acid: labstk(*mousetid)
At pc:0x00021a70:Rendez_Sleep+0x178 rendez.l:44
Rendez_Sleep(r=0xcd7d8,bool=0xcd7e0,t=0x0) rendez.l:5
	called from ALEF_rcvmem+0x198 recvmem.l:45
ALEF_rcvmem(c=0x000cd764,l=0x00000010) recvmem.l:6
\&...
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP lstk "" "Stack trace with local variables
.CW lstk
produces a long format stack trace.
The stack trace includes each function in the stack,
where it was called from, and the value of the parameters and automatic
variables for each function.
.CW lstk
displays the value rather than the address of each variable and all
variables are assumed to be an integer in format
.CW X .
To print a variable in its correct format use the
.CW :
operator to find the address and apply the appropriate format before indirection
with the
.CW *
operator. It may be necessary to single step a couple of instructions into
a function to get a correct stack trace because the frame pointer adjustment
instruction may get scheduled down into the body of the function.
.Ex
acid: lstk()
At pc:0x00001024:main+0x4 ls.c:48
main(argc=0x00000001,argv=0x7fffefec) ls.c:48
	called from _main+0x20 main9.s:10
	_argc=0x00000000
	_args=0x00000000
	fd=0x00000000
	buf=0x00000000
	i=0x00000000
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP mem integer,string "Print memory object
.CW mem
interprets its first
.I integer
argument as the address of an object to be printed according to the
format supplied in its second
.I string
argument.
The format string can be any combination of format characters, each preceded
by an optional count.
.Ex
acid: mem(bdata+0x326, "2c2Xb")
P = 0xa94bc464 0x3e5ae44d  19 
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP new "" "Create new process
.CW new
starts a new copy of the debugged program. The new program is started
with the program arguments set by the variable
.CW progargs .
The new program is stopped in the second instruction of
.CW main .
The breakpoint list is reinitialized.
.CW new
may be used several times to instantiate several copies of a program
simultaneously. The user can rotate between the copies using
.CW setproc .
.Ex
acid: progargs="-l"
acid: new()
60: external interrupt	_main	ADD	$-0x14,R29
60: breakpoint	main+0x4	MOVW	R31,0x0(R29)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP next "" "Step through language statement
.CW next
steps through a single language level statement without tracing down
through each statement in a called function. For each statement,
.CW next
prints the machine instructions executed as part of the statement. After
the statement has executed, source lines around the current program
counter are displayed.
.Ex
acid: next()
60: breakpoint	Binit+0x4 MOVW	R31,0x0(R29)
60: breakpoint	Binit+0x8 MOVW	f+8(FP),R4
binit.c:93
 88	
 89	int
 90	Binit(Biobuf *bp, int f, int mode)
 91	{
>92		return Binits(bp, f, mode, bp->b, BSIZE);
 93	}
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP notestk integer "Stack trace after receiving a note
.CW notestk
interprets its
.I integer
argument as the address of a
.CW Ureg
structure passed by the kernel to a
.I notify (2)
function during note processing.
.CW notestk
uses the
.CW PC ,
.CW SP ,
and link register from the
.CW Ureg
to print a stack trace corresponding to the point in the program where the note
was received.
To get a valid stack trace on the MIPS and SPARC architectures from a notify
routine, the program must stop in a new function called from the notify routine
so that the link register is valid and the notify routine's parameters are
addressable.
.Ex
acid: notestk(*notify:ur)
Note pc:0x00001024:main+0x4 ls.c:48
main(argc=0x00000001,argv=0x7fffefec) ls.c:48
	called from _main+0x20 main9.s:10
	_argc=0x00000000
	_args=0x00000000
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP pfl integer "Print source file and line
.CW pfl
interprets its argument as a text address and uses it to print
the source file and line number corresponding to the address. The output
has the same format as file addresses in
.I acme (1).
.Ex
acid: pfl(main)
ls.c:48
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP procs "" "Print active process list
.CW procs
prints a list of active process attached to the debugger. Each process
produces a single line of output giving the pid, process state, the address
the process is currently executing, and the
.CW setproc
command required to make that process current.
The current process is marked in the first column with a
.CW >
character. The debugger maintains a list of processes in the variable
.CW proclist .
.Ex
acid: procs()
>62: Stopped at main+0x4 setproc(62)
 60: Stopped at Binit+0x8 setproc(60)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP pstop integer "Print reason process stopped
.CW pstop
prints the status of the process specified by the
.I integer
pid supplied as its argument.
.CW pstop
is usually called from
.CW stopped
every time a process enters the
.CW Stopped
state.
.Ex
acid: pstop(62)
0x0000003e: breakpoint	main+0x4	MOVW	R31,0x0(R29)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP regs "" "Print registers
.CW regs
prints the contents of both the general and special purpose registers.
.CW regs
calls
.CW spr
then
.CW gpr
to display the contents of the registers.
.KE
.\"
.\"
.\"
.Ip \f(CW{}\fP source "" "Summarize source data base
.CW source
prints the directory search path followed by a list of currently loaded
source files. The source management functions
.CW src
and
.CW findsrc
use the search path to locate and load source files. Source files are
loaded incrementally into a source data base during debugging. A list
of loaded files is stored in the variable
.CW srcfiles
and the contents of each source file in the variable
.CW srctext .
.Ex
acid: source()
/n/bootes/sys/src/libbio/
./
/sys/src/libc/port/
/sys/src/libc/9sys/
/sys/src/libc/mips/
	binit.c
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP spr "" "Print special purpose registers
.CW spr
prints the contents of the processor control and memory management
registers. Where possible, the contents of the registers are decoded
to provide extra information; for example the
.CW CAUSE
register on the MIPS is
printed both in hexadecimal and using the
.CW reason
function.
.Ex
acid: spr()
PC	0x00001024 main+0x4  ls.c:48
SP	0x7fffef68 LINK	0x00006264 _main+0x28 main9.s:12
STATUS	0x0000ff33 CAUSE	0x00000024 breakpoint
TLBVIR	0x000000d3 BADVADR	0x00001020
HI	0x00000004 LO		0x00001ff7
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP src integer "Print lines of source
.CW src
interprets its
.I integer
argument as a text address and uses this address to print 5 lines
of source before and after the address. The current line is marked with a
.CW >
character.
.CW src
uses the source search path maintained by
.CW source
and
.CW addsrcdir
to locate the required source files.
.Ex
acid: src(*PC)
ls.c:47
 42	Biobuf	bin;
 43	
 44	#define		HUNK	50
 45	
 46	void
>47	main(int argc, char *argv[])
 48	{
 49		int i, fd;
 50		char buf[64];
 51	
 52		Binit(&bin, 1, OWRITE);
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP step "" "Single step process
.CW step
causes the debugged process to execute a single machine level instruction.
If the program is stopped on a breakpoint set by
.CW bpset
it is first removed, the single step executed, and the breakpoint replaced.
.CW step
uses
.CW follow
to predict the address of the program counter after the current instruction
has been executed. A breakpoint is placed at each of these predicted addresses
and the process is started. When the process stops the breakpoints are removed.
.Ex
acid: step()
62: breakpoint	main+0x8	MOVW	R1,argc+4(FP)
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP stk "" "Stack trace
.CW stk
produces a short format stack trace. The stack trace includes each function
in the stack, where it was called from, and the value of the parameters.
The short format omits the values of automatic variables.
Parameters are assumed to be integer values in the format
.CW X ;
to print a parameter in the correct format use the
.CW :
to obtain its address, apply the correct format, and use the
.CW *
indirection operator to find its value.
It may be necessary to single step a couple of instructions into
a function to get a correct stack trace because the frame pointer adjustment
instruction may get scheduled down into the body of the function.
.Ex
acid: stk()
At pc:0x00001028:main+0x8 ls.c:48
main(argc=0x00000002,argv=0x7fffefe4) ls.c:48
	called from _main+0x20 main9.s:10
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP stmnt "" "Execute a single statement
.CW stmnt
executes a single language level statement.
.CW stmnt
displays each machine level instruction as it is executed. When the executed
statement is completed the source for the next statement is displayed.
Unlike
.CW next ,
the
.CW stmnt
function will trace down through function calls.
.Ex
acid: stmnt()
62: breakpoint	main+0x18 MOVW	R5,0xc(R29)
62: breakpoint	main+0x1c JAL	Binit(SB)
62: breakpoint	Binit     ADD	$-0x18,R29
binit.c:91
 89	int
 90	Binit(Biobuf *bp, int f, int mode)
>91	{
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP stopped integer "Report status of stopped process
.CW stopped
is called automatically by the interpreter
every time a process enters the
.CW Stopped
state, such as when it hits a breakpoint.
The pid is passed as the
.I integer
argument.  The default implementation just calls
.CW pstop ,
but the function may be changed to provide more information or perform fine control
of execution.  Note that
.CW stopped
should return; for example, calling
.CW step
in
.CW stopped
will recur until the interpreter runs out of stack space.
.Ex
acid: defn stopped(pid) {
	if *lflag != 0 then error("lflag modified");
	}
acid: progargs = "-l"
acid: new();
acid: while 1 do step();
<stdin>:7: (error) lflag modified
acid: stk()
At pc:0x00001220:main+0x200 ls.c:54
main(argc=0x00000001,argv=0x7fffffe8) ls.c:48
	called from _main+0x20 main9.s:10
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP symbols string "Search symbol table
.CW symbols
uses the regular expression supplied by
.I string
to search the symbol table for symbols whose name matches the
regular expression.
.Ex
acid: symbols("main")
main	T	0x00001020
_main	T	0x0000623c
.Ee
.\"
.\"
.\"
.Ip \f(CW{}\fP win "" "Start new process in a window
.CW win
performs exactly the same function as
.CW new
but uses the window system to create a new window for the debugged process.
The variable
.CW progargs
supplies arguments to the new process.
The environment variable
.CW $8½srv
must be set to allow the interpreter to locate the mount channel for the
window system.
The window is created in the top left corner of the screen and is
400x600 pixels in size. The
.CW win
function may be modified to alter the geometry.
The window system will not be able to deliver notes in the new window
since the pid of the created process is not passed when the server is
mounted to create a new window.
.Ex
acid: win()
.Ee
