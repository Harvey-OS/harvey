.am DS
.ft I
..
.ta 1i 2.3i 4.5i  (optional to set tabs)
.TL
ACID Manual
.AU
Phil Winterbottom
philw@research.att.com
.SH
Introduction
.PP
Acid is a general purpose, source level symbolic debugger.
The debugger is built around a simple command language. 
The command language provides a flexible user interface which allows the debugger
interface to be customized for a specific application or architecture.
Moreover, it provides an opportunity to write test and
verification code independently of a program's source code.
Acid is able to debug multiple
processes provided they share a common set of symbols (e.g., An ALEF program).
.PP
Like other language based solutions, acid presents a poor user interface but
provides a powerful debugging tool. Patience is required.
.PP
Acid allows the execution of a program to be controlled by operating on its
state while it is stopped and by monitoring and controlling its execution
when it is running. Each program action which causes a change 
of state is reflected by the execution
of an acid function. The action of the function may be user defined.
A library of default functions provides the functionality of a normal debugger.
.PP
A Plan9 process is controlled by writing messages to a control file in the
proc(4) file system. Each control message has a corresponding acid function which
sends the message to the process. These functions take a pid as an
argument. The memory and text file of the program may be manipulated using
the indirection operators. The symbol table, including source cross reference
are available to an acid program. The combination allows complex operations
to be performed both in terms of control flow and data manipulation.
.PP
Acid uses a garbage collector to free the programmer from storage management.
.SH
Using the Library Functions
.PP
The following example uses the standard debugging library to show how
language and program interact:
.P1
% acid /bin/ls
/bin/ls: mips plan 9 executable
/lib/acid/port
/lib/acid/mips
acid: new()
13006 : system call	_main	ADD	$-14,R29
acid: bpset(ls)
acid: cont()
13006 : breakpoint	ls	ADD	$-16c8,R29
acid: stk()
ls(s=0x0000004d ,multi=0x00000000 ) ls.c:84 called from main+0xfc ls.c:76 
main(argc=0x00000000 ,argv=0x7ffffff0 ) ls.c:46 called from _main+0x20 main9.s:10 
acid: print(PC)
0xc0000f60 
acid: print(*PC)
0x000013fc 
acid: print(ls)
0x000013fc 
.P2
After loading a mips binary, acid loads the portable and mips-specific
library functions  which form the standard debugging environment.
These files are acid source code and can be read. The function
.CW new()
creates a new process and stops it at the first instruction.
This change in state is reflected by a call to the
acid function
.CW stopped .
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
the function ls in the C program. Once again the
.CW stopped
routine is called to print the status of the program. The function
.CW stk
prints a C stack trace of the current process. It is implemented using
a builtin acid function which returns the stack trace as a list. The code
which formats the information is all written in acid. 
The variable PC is  a processor register pointing to the 
cell where the current value of the PC is stored. By indirecting through
the value of PC the address where the program is stopped can be found.
All of the processor registers are available by the same mechanism.
The implementation of all the support from very few primitives allows each
of the functions to be tailored to the users needs.
.SH
Interface
.PP
After loading the default program modules and reporting name conflicts acid
enters interactive mode and prints a prompt. In this state acid accepts
either function definitions or statements to be evaluated. Statements are
evaluated immediately, while function definitions are stored for later
invocation. In this mode a newline implies a semicolon for the purposes
of the grammar - so a newline terminates a statement. If a compound
statement (of the form \f(CW{\fP \f(CIstatement\fP \f(CW}\fP)
or function definition is in progress the newline characters are ignored
until a closing
.CW }
is found. This allows the result of a
.CW whatis
statement to be sent directly to the prompt without spurious semicolons
being inserted. This mode is indicated by a tab character being used as a
prompt. A syntax error or interrupt returns acid to the normal evaluation
mode. Any partially evaluated definitions are lost.
.SH
Types
.PP
An acid variable has one of four types:
.I integer ,
.I float ,
.I list
or
.I string .
The type of a variable is inferred from the type of the right-hand
side of an assignment expression. Referencing a variable which has not yet
been assigned draws a used but not set error. Many of the operators may
be applied to more than
one type; for these operators the action of the operator is determined by
the types of its operands. The action of each operator is defined in the
.B expressions
section of the manual.
.SH
Variables
.PP
Acid has three kinds of variables: variables defined by the symbol table
of the program being debugged, variables which are defined and maintained
by the interpreter as a program changes state and variables defined and used
by acid programs.
.PP
Variables maintained by the interpreter are the register pointers listed by
name in the acid list variable
.CW registers ,
the symbol table listed by name and contents in the acid variable
.CW symbols.
.PP
The variable
.CW pid
is updated by the interpreter to point at the most recently created process
or the process pointed at by the
.CW setproc
builtin function.
.SH 1
Formats
.PP
In addition to a type, variables have formats. The format is a code
letter that determines the printing style and the effect of some of the
operators on that variable. The format codes are derived from the format
letters used by adb. By default, symbol table variables and numeric constants
are assigned the format code 'X' which specifies 32-bit hexadecimal.
Printing a variable with this code yields the output
.CW 0x00123456 .
The format code of a variable may be changed from the default by using the 
builtin function
.CW fmt .
This function takes two arguments, an expression and a format code. After
the expression is evaluated the new format code is attached to the result
and forms the return value from
.CW fmt .
If the result is assigned to a variable the new format code is maintained
in the variable. For example:
.P1
acid: x=10
acid: print(x)
0x0000000a 
acid: x = fmt(x, 'D')
acid: print(x, fmt(x, 'X'))
10 0x0000000a
acid: print(x)
10
.P2
The supported format characters are:
.RS
.IP \f(CWo\fP
Print two-byte integer in octal.
.IP \f(CWO\fP
Print four-byte integer in octal.
.IP \f(CWq\fP
Print two-byte in signed octal.
.IP \f(CWQ\fP
Print four-byte in signed octal.
.IP \f(CWd\fP
Print two-byte in decimal.
.IP \f(CWD\fP
Print four-byte in decimal.
.IP \f(CWx\fP
Print two-byte in hexadecimal.
.IP \f(CWX\fP
Print four-byte in hexadecimal.
.IP \f(CWu\fP
Print two-byte in unsigned decimal.
.IP \f(CWU\fP
Print four-byte in unsigned decimal.
.IP \f(CWf\fP
Print single-precision floating point number.
.IP \f(CWF\fP
Print double-precision floating point.
.IP \f(CWb\fP
Print the byte in hexadecimal.
.IP \f(CWc\fP
Print the byte as an ASCII character.
.IP \f(CWC\fP
Print the byte as a character.
Printable ASCII characters are represented normally;
others are printed in the form \exnn.
.IP \f(CWs\fP
Print the characters, as a UTF string,
until a zero byte is reached.
.IP \f(CWS\fP
Print a string using the escape convention (see C
above).
.IP \f(CWr\fP
Print the integer as a rune.
.IP \f(CWR\fP
Print the addressed two-byte integers as runes
until a zero rune is reached.
.IP \f(CWY\fP
Print a four-byte integer in date format (see ctime(2)).
.IP \f(CWi\fP
Print as machine instructions.
.IP \f(CWI\fP
As i above, but print the machine instructions in
an alternate form if possible: sunsparc and mipsco
reproduce the manufacturers' syntax.
.IP \f(CWa\fP
Print the value in symbolic form.
.RE
.SH 1
Scope
.PP
Variables are global unless they are either parameters to functions
or are declared as
.CW local
in a function body. Parameters and local variables are available only in
the body of the function in which they were instantiated.
.SH 1
Addressing
.PP
Since the symbol table specifies addresses, an extra level of indirection
is required to access the value of program variables. The registers are
also maintained as pointers to be consistent.
.SH 1
Name Conflicts
.PP
Name conflicts between keywords in the acid language, symbols in the program
and previously defined functions are resolved when the interpreter starts up.
Each name is made unique by prepending enough
.CW $
characters to the front of the name to make it unique. Acid reports
a list of each name change at startup. The report is of the form:
.P1
/bin/sam: mips plan 9 executable
/lib/acid/port
/lib/acid/mips
Symbol renames:
	append=$append T/0xa4e40
acid:
.P2
Append is both a keyword and text symbol in the program. The message reports
that the text symbol is now named
.CW $append .
.SH
Expressions
.PP
Expressions are evaluated from left to right. Operators have the same
binding and precedence as 'C' or ALEF.
.SH 1
Boolean expressions
.PP
If an expression is evaluated for a boolean condition the test
performed depends on the type of the result. If the result is of
.I integer
or
.I floating
type the result is true if the value is non-zero. If the expression is a
.I List
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
An identifier may be any legal acid variable. The colon operator references
parameters or local variables in the current stack of a program. For example:
.P1
	acid: print(main:argc)
	3
.P2
Prints the number of arguments passed into main. Local variables and parameters
can only be referenced after the frame has been established. You may need to
step a program over the first few instructions of a function to properly set
the frame.
.PP
Constants follow the same rules
as 'C'. A list of expressions delimited by braces forms a list constructor.
A new list is produced by evaluating each expression whenever the constructor
is executed.
If an expression in the constructor list has a global variable
then each time the constructor
is executed the resulting list will have the current value of the variable.
The empty list is formed from
.CW {} .
.P1
	acid: x = 10
	acid: l = { 1, x, 2 }
	acid: x = 20
	acid: print(l)
	{0x00000001 , 0x0000000a , 0x00000002 }
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
for head and tail must yield a value of type
.I list .
If there are no elements in the list the value of
.I head
or
.I tail
will be the empty list. Otherwise
.CW head
evaluates to the first element of the list and
.CW tail
evaluates to the rest.
.P1
	acid: print(head {})
	{}
	acid: print(head {1, 2, 3, 4})
	0x00000001 
	acid: print(tail {1, 2, 3, 4})
	{0x00000002 , 0x00000003 , 0x00000004 }
.P2
The first operand of
.CW append 
and
.CW delete
must be an expression which yields a
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
removes the i'th item from the list. List indices are zero-based.
.P1
	acid: print(append {1, 2}, 3)
	{0x00000001 , 0x00000002 , 0x00000003 }
	acid: print(delete {1, 2, 3}, 1)
	{0x00000001 , 0x00000003 }
.P2
.PP
Assigning a list to a variable copies a point to the list; if a list variable
is copied it still points at the same list. A constructor may be used to make
a duplicate of a list.
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
.CW []
operator performs indexing.
The indexing expression must result in an integer type.
The operation depends on the type of
.I postfix-expression .
If the
.I postfix-expression
yields an
.I integer
it is assumed to be the base address of an array in the core file.
The index offsets into this array; the size of the array members is
determined by the format associated with the
.I postfix-expression .
If the 
.I postfix-expression
yields a
.I string
the index operator fetches the n'th character
of the string. If the index points beyond the end
of the string an zero is returned.
If the
.I postfix-expression
yields a
.I list
then the indexing operation returns the n'th item of the list.
If the list contains less than n items the empty list
.CW {}
is returned.
.PP
The
.CW ++
and
.CW --
operators increment and decrement integer variables.
The size of increment or decrement depends on the format code. These postfix
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
operator fetches a value from core, so there must be a running process. If either
operator appears on the left-hand side of an assignment statement, either the file
or memory will be written. The file can only be modified when acid is invoked
with the
.I -w
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
variable length instructions like the 68020 or 386:
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
perform bitwise and logical negation respectively. These operands must be of
.I integer
type.
.DS
	multiplicative-expression:
		multiplicative-expression \f(CW*\fP multiplicative-expression
		multiplicative-expression \f(CW/\fP multiplicative-expression
		multiplicative-expression \f(CW%\fP multiplicative-expression
.DE
These operators perform on
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
If both operands are of either
.I string
or
.I list
type  then addition is defined as concatenation. Subtraction is undefined for
these two types.
.DS
	shift-expression:
		additive-expression
		shift-expression << additive-expression
		shift-expression >> additive-expression
.DE
The
.CW >>
and
.CW <<
operators perform bitwise right and left shifts respectively. Both operands
must be of
.I integer
type.
.DS
	relational-expression:
		relational-expression \f(CW<\fP shift-expression
		relational-expression \f(CW>\fP shift-expression
		relational-expression <= shift-expression
		relational-expression >= shift-expression

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
type. The equality operators apply to all types. Two lists are equal if they
are of the same number of members and a pairwise comparison of the members results
in equality.
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
		logical-AND-expression && OR-expression

	logical-OR-expression:
		logical-AND-expression
		logical-OR-expression || logical-AND-expression
.DE
The
.CW &&
operator returns 1 if both of its operands evaluate to boolean true, otherwise 0.
The
.CW ||
operator returns 1 if either of its operand evaluates to boolean true, otherwise 0.
.SH
Complex Types
.PP
.SH
Statements
.PP
.DS
	\f(CWif\fP expression \f(CWthen\fP statement \f(CWelse\fP statement
	\f(CWif\fP expression \f(CWthen\fP statement
.DE
Expression is evaluated as a boolean. If the value is true the statement after
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
In a while loop, statement is executed while the boolean expression evaluates
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
is evaluated while the result of
.I startexpr
is less than
.I endexpr .
Both expressions must yield integer values. The result of startexpr is
incremented by one for each loop iteration.
.DS
	\f(CWreturn\fP expression
.DE
.CW return
terminates execution of the current function and returns to its caller.
The value of the function is given by expression. Since
.CW return
requires an argument nil-valued functions should return the empty list
.CW {} .
.DS
	\f(CWlocal\fP variable
.DE
The
.CW local
statement creates a local instance of variable which remains for the duration
of the instance of the function in which it is declared. The local variable,
rather than the previous value of variable is visible to called functions.
After a return from the current function the previous value of variable is
visible.
.PP
If acid is interrupted the value of all local variables are lost,
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
Source Code Management
.PP
Acid provides the means to manipulate source code. Source code is
represented by lists of strings. Builtin functions provide mapping
from address to lines and vice-versa. The default debugging environment
has the means to load and display source files.
.SH
Builtin Functions
.PP
Acid has a number of builtin functions which not be redefined. In the
following description the return type, the function name and
the type of the parameters are defined for each function.
The type
.CW {}
denotes the empty list as return value.
.de Ip
.IP "\f2\\$1\fP\ \f(CW\\$2(\f2\\$3\f(CW)\f1
.br
..
.Ip float atof string
Converts the string argument into a floating point value.
.Ip integer access string
Access returns the integer 1 if the file name in
.CW string can be read by
.CW file ,
otherwise 0. This builtin is used to track down source files in
.CW findsrc
without using file to read them.
.Ip {} error string
Error generates an error message and returns the interpreter to interactive
mode. If a program is running, it is aborted.
Processes are not affected. The values of local variables are lost.
.Ip integer atoi string
Converts the string argument to an integer value.
.Ip {} print "item, item, ...
This function prints the arguments using the format associated with each.
.P1
	acid: print(10, "decimal ", fmt(10, 'D'), "octal ", fmt(10, 'o'))
	0x0000000a decimal 10 octal 000000000012 
	acid: print({1, 2, 3})
	{0x00000001 , 0x00000002 , 0x00000003 }
	acid: print(main, fmt(main, 'a'), "\t", @fmt(main, 'i'))
	0x00001020 main	ADD	$-64,R29
.P2
.Ip list file string
This function opens and reads the file specified by
.I string .
Each line of the source file forms an element of the list returned by this
function. File breaks lines at the newline character and the newline
characters are not returned in the list.
.Ip item fmt "item, fmt
.CW Fmt
makes a copy of
.I item
and associates the specified format
.I fmt
with it.
.P1
	acid: main = fmt(main, 'i')	// interpret as instructions
	acid: print(fmt(main, 'X'), "\t", *main)
	0x00001020 ADD	$-64,R29
.P2
.Ip string pcfile integer
This function returns a string containing the name of the source file
corresponding to the text address supplied as the argument.
If the source file name cannot be found,
.CW pcfile
returns the string "?file?".
.Ip integer pcline integer 
This function returns an integer line number in the source file closest
to the text address supplied as the argument.
If the line number cannot be found, 0 is returned.
.P1
	acid: print("Stopped at ", pcfile(*PC), ":", pcline(*PC))
	Stopped at ls.c:46 
.P2
.Ip integer filepc string
This function returns the text address of the source line specified by 
.I string
of the form filename:line.
.P1
	acid: print(fmt(filepc("main.c:10"), 'a'))
	main+0x10
.P2
.Ip integer match "item, list
.I List
is searched for a member equal to
.I item .
.I Item
can be of any type.
If
.I item
is not found,
.CW match
returns -1, otherwise it returns the index of the matching member.
.Ip {} setproc integer
This function selects the process specified by the integer process id supplied
as the argument as the current process. The internal variable
.CW pid
is set to the argument.
.Ip {} strace "integer pc, integer sp
This function returns a list of entries for each active frame on the stack
defined by the program counter and stack pointer supplied as arguments.
Each entry in the list has four items:
the function address, the address from which the function was called,
a list of parameters and a list of automatic variables.
The parameter and automatic list contains pairs of values: the name of the
variable and its value.
.P1
	acid: print(strace(*PC, *SP))
	{{0x00001020 , 0x000068f8 , {{argc, 0x00000001 }, {argv, 0x7fffffec }},
	{{_argc, 0x00000000 }, {_args, 0x00000000 }, {fd, 0x00000000 },
	{buf, 0x00000000 }, {i, 0x00000000 }}}}
	acid:
.P2
.Ip {} follow integer
This function returns a list of text addresses the program counter may advance
to after a single instruction is executed. It is used to plant breakpoints on
all possible potential paths of execution. The following code fragment
plants breakpoints ahead of all potential following instructions.
.P1
	lst = follow(*PC);
	lpl = lst;
	while lpl do
	{
		*head lpl = bpinst;
		lpl = tail lpl;
	}
.P2
.Ip string reason integer
This function converts the architecture-dependent exception or status register
to a
.I string
describing the cause of the exception.
.Ip {} newproc string
This function starts a new process with arguments constructed from
.I string .
Arguments must be space separated. Internal variable
.CW pid
contains the process id of the newly created process.
The new process is halted at the first instruction and the debugger calls
.CW stopped .
.Ip {} startstop integer
This function starts the stopped process specified by the integer process id
supplied as its argument. The debugger waits until the process stops
or the debugger is interrupted.
.Ip string status integer
This function returns a string describing the state of a process specified
by the integer process id supplied as the argument.
The string corresponds to the status reported in the
sixth column of the ps(1) command.
.Ip {} kill integer
This function kills the process specified by the integer process id supplied
as its argument and
removes it from the active process list.
If it is the current process, internal variable
.CW pid
is set to zero.
.IP {} waitstop integer
This function causes the debugger to wait until the process specified by the
integer process id supplied as its argument enters the
.I stopped
state or the debugger is interrupted.
.Ip {} stop integer
This function causes the process specified by the integer process id supplied
as its argument to be stopped next time it enters the kernel.
.Ip {} start integer
This function causes the stopped process specified by the integer process id
to be started.
.Ip {} tag string
This function loads a tags file generate by the -s option of the C compiler
(see 2C(1)). A tag file defines offsets and types within complex structures
and is used by the
.CW .
and
.CW ->
operators.
.Ip {} rc string
This function forks a shell and executes the command contained in
.I string.
.de Iq
.IP "\f(CW\\$1(\f2\\$2\f(CW)\f1
.br
..
.SH
Library Functions
.PP
The following functions are defined by modules loaded from
.CW /lib/acid
at initialization.
These functions may be redefined by code in
.CW $home/lib/acid .
.Iq acidinit
This function is invoked by the interpreter after all
modules have been loaded at initialization time.
Its purpose is set up machine specific variables and the default source path.
.Iq stk
This function print a stack trace of the current process.
.Iq lstk
This function prints a long stack trace of the current process including the
values of local variables.
.Iq labstk integer
This function prints a long stack trace using a program counter and
stack value fetched from the contents of a label pointed to by the integer
supplied as its argument.
The label must have been set by setjmp(2) or from setlabel in the kernel.
.Iq gpr
This function prints the contents of the general purpose registers.
.Iq spr
This function prints the contents of the special purpose registers like
the program counter and stack pointer.
.Iq regs
Print both the special purpose and general purpose registers.
.Iq next
This function prints a list of possible instructions which may be executed
from the current program counter value.
.Iq line integer
This function prints the line of source closest to the integer address
specified as its argument. 
.Iq addsrcdir string
This function adds the directory specified by
.I string
to the list of directories searched when looking for source files.
The new directory is added to the head of the source search list.
.Iq source
This function prints the current source list followed by the list of
loaded files.
.Iq src integer
This function prints 10 lines of source around the integer text address
supplied as the argument.
The line closest to the text address is marked with a
.CW >
character.
.Iq Bsrc integer
This function
loads the source file containing the integer text address supplied as the
argument into the most recently started sam(1).
The current sam address is set to the source line closest to the text address.
.Iq step
This function executes a single machine instruction in the current process.
.Iq bpset integer
This function sets a breakpoint at the integer text address supplied as the
argument. In multi-threaded code the breakpoint is only seen by the
current process.
.Iq bptab
This function prints a list of breakpoints installed in the current process.
.Iq bpdel integer
This function removes a breakpoint installed at the integer text address supplied
as the argument.
.Iq cont
This function continues execution of the current process.
The debugger will wait until the process becomes stopped.
This function differs from the builtin
.CW start
by waiting until the process stops.
.Iq stopped pid
This function is called by the interpreter automatically when a process
stops. By default,
.CW stopped
calls the function
.CW pstop
to print the current pid, program counter and instruction.
.CW Stopped
is a stub so that is can be easily replaced by the user.
.Iq procs
This function prints a list of processes attached to the debugger.
The current process is marked by a
.CW >
character.
.Iq asm integer
This function prints 30 lines of disassembly from the integer text address
supplied as the argument.
.Iq new
This function creates a new process.
The process halts at the first executable instruction and the debugger calls
.CW stopped .
The new process is added to the process list and becomes the current process.
Arguments can be supplied to the program by setting the string variable
.CW progargs .
.Iq stmnt
This function single steps through a high-level language source line.
.CW Stmnt
prints a summary of the instructions executed and then uses
.CW src
to report the current position in the source file.
.Iq func
This function steps the current process until it leaves a
function.
.Iq dump "integer addr, integer n, string
This function prints the contents of 
.I n
items beginning at the integer text address
.I addr
in the current core image.
.I String
specifies the format controlling the printing of the data.
.P1
	acid: dump(main, 2, "XXbxD")
	main 0x0000000d 0xafbf0000 175 0xa100 1746928384 
	main+0xf 0x01afa300 0x08200500   1 0xafa5 789504 
.P2
.Iq  mem "integer addr, string
This function prints the contents of memory starting
at the integer text address
.I addr
in the current core image.
.I String
specifies the format controlling the printing of the data.
