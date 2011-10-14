.HTML "Plan 9 C Compilers
.TL
Plan 9 C Compilers
.AU
Ken Thompson
ken@plan9.bell-labs.com
.AB
.FS
Originally appeared, in a different form, in
.I
Proceedings of the Summer 1990 UKUUG Conference,
.R
pp. 41-51,
London, 1990.
.FE
This paper describes the overall structure and function of the Plan 9 C compilers.
A more detailed implementation document
for any one of the compilers
is yet to be written.
.AE
.NH
Introduction
.LP
There are many compilers in the series.
Five of the compilers (Intel 386, AMD64, PowerPC, PowerPC 64-bit, ARM)
are considered active and are used to compile
current versions of Plan 9.
Four of the compilers (MIPS 3000, SPARC, DEC Alpha, and Motorola 68020)
are maintained but are for older machines
for which we have no current ports of Plan 9;
other than the MIPS, we are unlikely to port to any such machines.
Several others (Motorola 68000, Intel 960, AMD 29000) have had only limited use, such as
to program peripherals or experimental devices.
.NH
Structure
.LP
The compiler is a single program that produces an
object file.
Combined in the compiler are the traditional
roles of preprocessor, lexical analyzer, parser, code generator,
local optimizer,
and first half of the assembler.
The object files are binary forms of assembly
language,
similar to what might be passed between
the first and second passes of an assembler.
.LP
Object files and libraries
are combined by a loader
program to produce the executable binary.
The loader combines the roles of second half
of the assembler, global optimizer, and loader.
The names of the compliers, loaders, and assemblers
are as follows:
.DS
.ta 1.5i
.de Ta
\\$1	\f(CW\\$2\fP  \f(CW\\$3\fP  \f(CW\\$4\fP
..
.Ta SPARC kc kl ka
.Ta PowerPC qc ql qa
.Ta MIPS vc vl va
.Ta MIPS\ little-endian 0c 0l 0a
.Ta Motorola\ 68000 1c 1l 1a
.Ta Motorola\ 68020 2c 2l 2a
.Ta ARM 5c 5l 5a
.Ta AMD64 6c 6l 6a
.Ta DEC\ Alpha 7c 7l 7a
.Ta Intel\ 386 8c 8l 8a
.Ta PowerPC\ 64-bit 9c 9l 9a
.DE
There is a further breakdown
in the source of the compilers into
object-independent and
object-dependent
parts.
All of the object-independent parts
are combined into source files in the
directory
.CW /sys/src/cmd/cc .
The object-dependent parts are collected
in a separate directory for each compiler,
for example
.CW /sys/src/cmd/vc .
All of the code,
both object-independent and
object-dependent,
is machine-independent
and may be cross-compiled and executed on any
of the architectures.
.NH
The Language
.LP
The compiler implements ANSI C with some
restrictions and extensions
[ANSI90].
Most of the restrictions are due to
personal preference, while
most of the extensions were to help in
the implementation of Plan 9.
There are other departures from the standard,
particularly in the libraries,
that are beyond the scope of this
paper.
.NH 2
Register, volatile, const
.LP
The keyword
.CW register
is recognized syntactically
but is semantically ignored.
Thus taking the address of a
.CW register
variable is not diagnosed.
The keyword
.CW volatile
disables all optimizations, in particular registerization, of the corresponding variable.
The keyword
.CW const
generates warnings (if warnings are enabled by the compiler's
.CW -w
option) of non-constant use of the variable,
but does not affect the generated code.
.NH 2
The preprocessor
.LP
The C preprocessor is probably the
biggest departure from the ANSI standard.
.LP
The preprocessor built into the Plan 9 compilers does not support
.CW #if ,
although it does handle
.CW #ifdef
and
.CW #include .
If it is necessary to be more standard,
the source text can first be run through the separate ANSI C
preprocessor,
.CW cpp .
.NH 2
Unnamed substructures
.LP
The most important and most heavily used of the
extensions is the declaration of an
unnamed substructure or subunion.
For example:
.DS
.CW
.ta .1i .6i 1.1i 1.6i
	typedef
	struct	lock
	{
		int    locked;
	} Lock;

	typedef
	struct	node
	{
		int	type;
		union
		{
			double dval;
			float  fval;
			long   lval;
		};
		Lock;
	} Node;

	Lock*	lock;
	Node*	node;
.DE
The declaration of
.CW Node
has an unnamed substructure of type
.CW Lock
and an unnamed subunion.
One use of this feature allows references to elements of the
subunit to be accessed as if they were in
the outer structure.
Thus
.CW node->dval
and
.CW node->locked
are legitimate references.
.LP
When an outer structure is used
in a context that is only legal for
an unnamed substructure,
the compiler promotes the reference to the
unnamed substructure.
This is true for references to structures and
to references to pointers to structures.
This happens in assignment statements and
in argument passing where prototypes have been
declared.
Thus, continuing with the example,
.DS
.CW
.ta .1i .6i 1.1i 1.6i
	lock = node;
.DE
would assign a pointer to the unnamed
.CW Lock
in
the
.CW Node
to the variable
.CW lock .
Another example,
.DS
.CW
.ta .1i .6i 1.1i 1.6i
	extern void lock(Lock*);
	func(...)
	{
		...
		lock(node);
		...
	}
.DE
will pass a pointer to the
.CW Lock
substructure.
.LP
Finally, in places where context is insufficient to identify the unnamed structure,
the type name (it must be a
.CW typedef )
of the unnamed structure can be used as an identifier.
In our example,
.CW &node->Lock
gives the address of the anonymous
.CW Lock
structure.
.NH 2
Structure displays
.LP
A structure cast followed by a list of expressions in braces is
an expression with the type of the structure and elements assigned from
the corresponding list.
Structures are now almost first-class citizens of the language.
It is common to see code like this:
.DS
.CW
.ta .1i
	r = (Rectangle){point1, (Point){x,y+2}};
.DE
.NH 2
Initialization indexes
.LP
In initializers of arrays,
one may place a constant expression
in square brackets before an initializer.
This causes the next initializer to assign
the indicated element.
For example:
.DS
.CW
.ta .1i .6i 1.6i
	enum	errors
	{
		Etoobig,
		Ealarm,
		Egreg
	};
	char* errstrings[] =
	{
		[Ealarm]	"Alarm call",
		[Egreg]	"Panic: out of mbufs",
		[Etoobig]	"Arg list too long",
	};
.DE
In the same way,
individual structures members may
be initialized in any order by preceding the initialization with
.CW .tagname .
Both forms allow an optional
.CW = ,
to be compatible with a proposed
extension to ANSI C.
.NH 2
External register
.LP
The declaration
.CW extern
.CW register
will dedicate a register to
a variable on a global basis.
It can be used only under special circumstances.
External register variables must be identically
declared in all modules and
libraries.
The feature is not intended for efficiency,
although it can produce efficient code;
rather it represents a unique storage class that
would be hard to get any other way.
On a shared-memory multi-processor,
an external register is
one-per-processor and neither one-per-procedure (automatic)
or one-per-system (external).
It is used for two variables in the Plan 9 kernel,
.CW u
and
.CW m .
.CW U
is a pointer to the structure representing the currently running process
and
.CW m
is a pointer to the per-machine data structure.
.NH 2
Long long
.LP
The compilers accept
.CW long
.CW long
as a basic type meaning 64-bit integer.
On all of the machines
this type is synthesized from 32-bit instructions.
.NH 2
Pragma
.LP
The compilers accept
.CW #pragma
.CW lib
.I libname
and pass the
library name string uninterpreted
to the loader.
The loader uses the library name to
find libraries to load.
If the name contains
.CW $O ,
it is replaced with
the single character object type of the compiler
(e.g.,
.CW v
for the MIPS).
If the name contains
.CW $M ,
it is replaced with
the architecture type for the compiler
(e.g.,
.CW mips
for the MIPS).
If the name starts with
.CW /
it is an absolute pathname;
if it starts with
.CW .
then it is searched for in the loader's current directory.
Otherwise, the name is searched from
.CW /$M/lib .
Such
.CW #pragma
statements in header files guarantee that the correct
libraries are always linked with a program without the
need to specify them explicitly at link time.
.LP
They also accept
.CW #pragma
.CW packed
.CW on
(or
.CW yes
or
.CW 1 )
to cause subsequently declared data, until
.CW #pragma
.CW packed
.CW off
(or
.CW no
or
.CW 0 ),
to be laid out in memory tightly packed in successive bytes, disregarding
the usual alignment rules.
Accessing such data can cause faults.
.LP
Similarly, 
.CW #pragma
.CW profile
.CW off
(or
.CW no
or
.CW 0 )
causes subsequently declared functions, until
.CW #pragma
.CW profile
.CW on
(or
.CW yes
or
.CW 1 ),
to be marked as unprofiled.
Such functions will not be profiled when 
profiling is enabled for the rest of the program.
.LP
Two
.CW #pragma
statements allow type-checking of
.CW print -like
functions.
The first, of the form
.P1
#pragma varargck argpos error 2
.P2
tells the compiler that the second argument to
.CW error
is a
.CW print
format string (see the manual page
.I print (2))
that specifies how to format
.CW error 's
subsequent arguments.
The second, of the form
.P1
#pragma varargck type "s" char*
.P2
says that the
.CW print
format verb
.CW s
processes an argument of
type
.CW char* .
If the compiler's
.CW -F
option is enabled, the compiler will use this information
to report type violations in the arguments to
.CW print ,
.CW error ,
and similar routines.
.NH
Object module conventions
.LP
The overall conventions of the runtime environment
are important
to runtime efficiency.
In this section,
several of these conventions are discussed.
.NH 2
Register saving
.LP
In the Plan 9 compilers,
the caller of a procedure saves the registers.
With caller-saves,
the leaf procedures can use all the
registers and never save them.
If you spend a lot of time at the leaves,
this seems preferable.
With callee-saves,
the saving of the registers is done
in the single point of entry and return.
If you are interested in space,
this seems preferable.
In both,
there is a degree of uncertainty
about what registers need to be saved.
Callee-saved registers make it difficult to
find variables in registers in debuggers.
Callee-saved registers also complicate
the implementation of
.CW longjmp .
The convincing argument is
that with caller-saves,
the decision to registerize a variable
can include the cost of saving the register
across calls.
For a further discussion of caller- vs. callee-saves,
see the paper by Davidson and Whalley [Dav91].
.LP
In the Plan 9 operating system,
calls to the kernel look like normal procedure
calls, which means
the caller
has saved the registers and the system
entry does not have to.
This makes system calls considerably faster.
Since this is a potential security hole,
and can lead to non-determinism,
the system may eventually save the registers
on entry,
or more likely clear the registers on return.
.NH 2
Calling convention
.LP
Older C compilers maintain a frame pointer, which is at a known constant
offset from the stack pointer within each function.
For machines where the stack grows towards zero,
the argument pointer is at a known constant offset
from the frame pointer.
Since the stack grows down in Plan 9,
the Plan 9 compilers
keep neither an
explicit frame pointer nor
an explicit argument pointer;
instead they generate addresses relative to the stack pointer.
.LP
On some architectures, the first argument to a subroutine is passed in a register.
.NH 2
Functions returning structures
.LP
Structures longer than one word are awkward to implement
since they do not fit in registers and must
be passed around in memory.
Functions that return structures
are particularly clumsy.
The Plan 9 compilers pass the return address of
a structure as the first argument of a
function that has a structure return value.
Thus
.DS
.CW
.ta .1i .6i 1.1i 1.6i
	x = f(...)
.DE
is rewritten as
.DS
.CW
.ta .1i .6i 1.1i 1.6i
	f(&x, ...)\f1.
.DE
This saves a copy and makes the compilation
much less clumsy.
A disadvantage is that if you call this
function without an assignment,
a dummy location must be invented.
.LP
There is also a danger of calling a function
that returns a structure without declaring
it as such.
With ANSI C function prototypes,
this error need never occur.
.NH
Implementation
.LP
The compiler is divided internally into
four machine-independent passes,
four machine-dependent passes,
and an output pass.
The next nine sections describe each pass in order.
.NH 2
Parsing
.LP
The first pass is a YACC-based parser
[Joh79].
Declarations are interpreted immediately,
building a block structured symbol table.
Executable statements are put into a parse tree
and collected,
without interpretation.
At the end of each procedure,
the parse tree for the function is
examined by the other passes of the compiler.
.LP
The input stream of the parser is
a pushdown list of input activations.
The preprocessor
expansions of
macros
and
.CW #include
are implemented as pushdowns.
Thus there is no separate
pass for preprocessing.
.NH 2
Typing
.LP
The next pass distributes typing information
to every node of the tree.
Implicit operations on the tree are added,
such as type promotions and taking the
address of arrays and functions.
.NH 2
Machine-independent optimization
.LP
The next pass performs optimizations
and transformations of the tree, such as converting
.CW &*x
and
.CW *&x
into
.CW x .
Constant expressions are converted to constants in this pass.
.NH 2
Arithmetic rewrites
.LP
This is another machine-independent optimization.
Subtrees of add, subtract, and multiply of integers are
rewritten for easier compilation.
The major transformation is factoring:
.CW 4+8*a+16*b+5
is transformed into
.CW 9+8*(a+2*b) .
Such expressions arise from address
manipulation and array indexing.
.NH 2
Addressability
.LP
This is the first of the machine-dependent passes.
The addressability of a processor is defined as the set of
expressions that is legal in the address field
of a machine language instruction.
The addressability of different processors varies widely.
At one end of the spectrum are the 68020 and VAX,
which allow a complex mix of incrementing,
decrementing,
indexing, and relative addressing.
At the other end is the MIPS,
which allows only registers and constant offsets from the
contents of a register.
The addressability can be different for different instructions
within the same processor.
.LP
It is important to the code generator to know when a
subtree represents an address of a particular type.
This is done with a bottom-up walk of the tree.
In this pass, the leaves are labeled with small integers.
When an internal node is encountered,
it is labeled by consulting a table indexed by the
labels on the left and right subtrees.
For example,
on the 68020 processor,
it is possible to address an
offset from a named location.
In C, this is represented by the expression
.CW *(&name+constant) .
This is marked addressable by the following table.
In the table,
a node represented by the left column is marked
with a small integer from the right column.
Marks of the form
.CW A\s-2\di\u\s0
are addressable while
marks of the form
.CW N\s-2\di\u\s0
are not addressable.
.DS
.B
.ta .1i 1.1i
	Node	Marked
.CW
	name	A\s-2\d1\u\s0
	const	A\s-2\d2\u\s0
	&A\s-2\d1\u\s0	A\s-2\d3\u\s0
	A\s-2\d3\u\s0+A\s-2\d1\u\s0	N\s-2\d1\u\s0 \fR(note that this is not addressable)\fP
	*N\s-2\d1\u\s0	A\s-2\d4\u\s0
.DE
Here there is a distinction between
a node marked
.CW A\s-2\d1\u\s0
and a node marked
.CW A\s-2\d4\u\s0
because the address operator of an
.CW A\s-2\d4\u\s0
node is not addressable.
So to extend the table:
.DS
.B
.ta .1i 1.1i
	Node	Marked
.CW
	&A\s-2\d4\u\s0	N\s-2\d2\u\s0
	N\s-2\d2\u\s0+N\s-2\d1\u\s0	N\s-2\d1\u\s0
.DE
The full addressability of the 68020 is expressed
in 18 rules like this,
while the addressability of the MIPS is expressed
in 11 rules.
When one ports the compiler,
this table is usually initialized
so that leaves are labeled as addressable and nothing else.
The code produced is poor,
but porting is easy.
The table can be extended later.
.LP
This pass also rewrites some complex operators
into procedure calls.
Examples include 64-bit multiply and divide.
.LP
In the same bottom-up pass of the tree,
the nodes are labeled with a Sethi-Ullman complexity
[Set70].
This number is roughly the number of registers required
to compile the tree on an ideal machine.
An addressable node is marked 0.
A function call is marked infinite.
A unary operator is marked as the
maximum of 1 and the mark of its subtree.
A binary operator with equal marks on its subtrees is
marked with a subtree mark plus 1.
A binary operator with unequal marks on its subtrees is
marked with the maximum mark of its subtrees.
The actual values of the marks are not too important,
but the relative values are.
The goal is to compile the harder
(larger mark)
subtree first.
.NH 2
Code generation
.LP
Code is generated by recursive
descent.
The Sethi-Ullman complexity completely guides the
order.
The addressability defines the leaves.
The only difficult part is compiling a tree
that has two infinite (function call)
subtrees.
In this case,
one subtree is compiled into the return register
(usually the most convenient place for a function call)
and then stored on the stack.
The other subtree is compiled into the return register
and then the operation is compiled with
operands from the stack and the return register.
.LP
There is a separate boolean code generator that compiles
conditional expressions.
This is fundamentally different from compiling an arithmetic expression.
The result of the boolean code generator is the
position of the program counter and not an expression.
The boolean code generator makes extensive use of De Morgan's rule.
The boolean code generator is an expanded version of that described
in chapter 8 of Aho, Sethi, and Ullman
[Aho87].
.LP
There is a considerable amount of talk in the literature
about automating this part of a compiler with a machine
description.
Since this code generator is so small
(less than 500 lines of C)
and easy,
it hardly seems worth the effort.
.NH 2
Registerization
.LP
Up to now,
the compiler has operated on syntax trees
that are roughly equivalent to the original source language.
The previous pass has produced machine language in an internal
format.
The next two passes operate on the internal machine language
structures.
The purpose of the next pass is to reintroduce
registers for heavily used variables.
.LP
All of the variables that can be
potentially registerized within a procedure are
placed in a table.
(Suitable variables are any automatic or external
scalars that do not have their addresses extracted.
Some constants that are hard to reference are also
considered for registerization.)
Four separate data flow equations are evaluated
over the procedure on all of these variables.
Two of the equations are the normal set-behind
and used-ahead
bits that define the life of a variable.
The two new bits tell if a variable life
crosses a function call ahead or behind.
By examining a variable over its lifetime,
it is possible to get a cost
for registerizing.
Loops are detected and the costs are multiplied
by three for every level of loop nesting.
Costs are sorted and the variables
are replaced by available registers on a greedy basis.
.LP
The 68020 has two different
types of registers.
For the 68020,
two different costs are calculated for
each variable life and the register type that
affords the better cost is used.
Ties are broken by counting the number of available
registers of each type.
.LP
Note that externals are registerized together with automatics.
This is done by evaluating the semantics of a ``call'' instruction
differently for externals and automatics.
Since a call goes outside the local procedure,
it is assumed that a call references all externals.
Similarly,
externals are assumed to be set before an ``entry'' instruction
and assumed to be referenced after a ``return'' instruction.
This makes sure that externals are in memory across calls.
.LP
The overall results are satisfactory.
It would be nice to be able to do this processing in
a machine-independent way,
but it is impossible to get all of the costs and
side effects of different choices by examining the parse tree.
.LP
Most of the code in the registerization pass is machine-independent.
The major machine-dependency is in
examining a machine instruction to ask if it sets or references
a variable.
.NH 2
Machine code optimization
.LP
The next pass walks the machine code
for opportunistic optimizations.
For the most part,
this is highly specific to a particular
processor.
One optimization that is performed
on all of the processors is the
removal of unnecessary ``move''
instructions.
Ironically,
most of these instructions were inserted by
the previous pass.
There are two patterns that are repetitively
matched and replaced until no more matches are
found.
The first tries to remove ``move'' instructions
by relabeling variables.
.LP
When a ``move'' instruction is encountered,
if the destination variable is set before the
source variable is referenced,
then all of the references to the destination
variable can be renamed to the source and the ``move''
can be deleted.
This transformation uses the reverse data flow
set up in the previous pass.
.LP
An example of this pattern is depicted in the following
table.
The pattern is in the left column and the
replacement action is in the right column.
.DS
.CW
.ta .1i .6i 1.6i 2.1i 2.6i
	MOVE	a->b		\fR(remove)\fP
.R
	(sequence with no mention of \f(CWa\fP)
.CW
	USE	b		USE	a
.R
	(sequence with no mention of \f(CWa\fP)
.CW
	SET	b		SET	b
.DE
.LP
Experiments have shown that it is marginally
worthwhile to rename uses of the destination variable
with uses of the source variable up to
the first use of the source variable.
.LP
The second transform will do relabeling
without deleting instructions.
When a ``move'' instruction is encountered,
if the source variable has been set prior
to the use of the destination variable
then all of the references to the source
variable are replaced by the destination and
the ``move'' is inverted.
Typically,
this transformation will alter two ``move''
instructions and allow the first transformation
another chance to remove code.
This transformation uses the forward data flow
set up in the previous pass.
.LP
Again,
the following is a depiction of the transformation where
the pattern is in the left column and the
rewrite is in the right column.
.DS
.CW
.ta .1i .6i 1.6i 2.1i 2.6i
	SET	a		SET	b
.R
	(sequence with no use of \f(CWb\fP)
.CW
	USE	a		USE	b
.R
	(sequence with no use of \f(CWb\fP)
.CW
	MOVE	a->b		MOVE	b->a
.DE
Iterating these transformations
will usually get rid of all redundant ``move'' instructions.
.LP
A problem with this organization is that the costs
of registerization calculated in the previous pass
must depend on how well this pass can detect and remove
redundant instructions.
Often,
a fine candidate for registerization is rejected
because of the cost of instructions that are later
removed.
.NH 2
Writing the object file
.LP
The last pass walks the internal assembly language
and writes the object file.
The object file is reduced in size by about a factor
of three with simple compression
techniques.
The most important aspect of the object file
format is that it is independent of the compiling machine.
All integer and floating numbers in the object
code are converted to known formats and byte
orders.
.NH
The loader
.LP
The loader is a multiple pass program that
reads object files and libraries and produces
an executable binary.
The loader also does some minimal
optimizations and code rewriting.
Many of the operations performed by the
loader are machine-dependent.
.LP
The first pass of the loader reads the
object modules into an internal data
structure that looks like binary assembly language.
As the instructions are read,
code is reordered to remove
unconditional branch instructions.
Conditional branch instructions are inverted
to prevent the insertion of unconditional branches.
The loader will also make a copy of a few instructions
to remove an unconditional branch.
.LP
The next pass allocates addresses for
all external data.
Typical of processors is the MIPS,
which can reference ±32K bytes from a
register.
The loader allocates the register
.CW R30
as the static pointer.
The value placed in
.CW R30
is the base of the data segment plus 32K.
It is then cheap to reference all data in the
first 64K of the data segment.
External variables are allocated to
the data segment
with the smallest variables allocated first.
If all of the data cannot fit into the first
64K of the data segment,
then usually only a few large arrays
need more expensive addressing modes.
.LP
For the MIPS processor,
the loader makes a pass over the internal
structures,
exchanging instructions to try
to fill ``delay slots'' with useful work.
If a useful instruction cannot be found
to fill a delay slot,
the loader will insert
``noop''
instructions.
This pass is very expensive and does not
do a good job.
About 40% of all instructions are in
delay slots.
About 65% of these are useful instructions and
35% are ``noops.''
The vendor-supplied assembler does this job
more effectively,
filling about 80%
of the delay slots with useful instructions.
.LP
On the 68020 processor,
branch instructions come in a variety of
sizes depending on the relative distance
of the branch.
Thus the size of branch instructions
can be mutually dependent.
The loader uses a multiple pass algorithm
to resolve the branch lengths
[Szy78].
Initially, all branches are assumed minimal length.
On each subsequent pass,
the branches are reassessed
and expanded if necessary.
When no more expansions occur,
the locations of the instructions in
the text segment are known.
.LP
On the MIPS processor,
all instructions are one size.
A single pass over the instructions will
determine the locations of all addresses
in the text segment.
.LP
The last pass of the loader produces the
executable binary.
A symbol table and other tables are
produced to help the debugger to
interpret the binary symbolically.
.LP
The loader places absolute source line numbers in the symbol table.
The name and absolute line number of all
.CW #include
files is also placed in the
symbol table so that the debuggers can
associate object code to source files.
.NH
Performance
.LP
The following is a table of the source size of the MIPS
compiler.
.DS
.ta .1i .6i
	lines	module
	\0509	machine-independent headers
	1070	machine-independent YACC source
	6090	machine-independent C source

	\0545	machine-dependent headers
	6532	machine-dependent C source

	\0298	loader headers
	5215	loader C source
.DE
.LP
The following table shows timing
of a test program
that plays checkers, running on a MIPS R4000.
The test program is 26 files totaling 12600 lines of C.
The execution time does not significantly
depend on library implementation.
Since no other compiler runs on Plan 9,
the Plan 9 tests were done with the Plan 9 operating system;
the other tests were done on the vendor's operating system.
The hardware was identical in both cases.
The optimizer in the vendor's compiler
is reputed to be extremely good.
.DS
.ta .1i .9i
	\0\04.49s	Plan 9 \f(CWvc\fP \f(CW-N\fP compile time (opposite of \f(CW-O\fP)
	\0\01.72s	Plan 9 \f(CWvc\fP \f(CW-N\fP load time
	148.69s	Plan 9 \f(CWvc\fP \f(CW-N\fP run time

	\015.07s	Plan 9 \f(CWvc\fP compile time (\f(CW-O\fP implicit)
	\0\01.66s	Plan 9 \f(CWvc\fP load time
	\089.96s	Plan 9 \f(CWvc\fP run time

	\014.83s	vendor \f(CWcc\fP compile time
	\0\00.38s	vendor \f(CWcc\fP load time
	104.75s	vendor \f(CWcc\fP run time

	\043.59s	vendor \f(CWcc\fP \f(CW-O\fP compile time
	\0\00.38s	vendor \f(CWcc\fP \f(CW-O\fP load time
	\076.19s	vendor \f(CWcc\fP \f(CW-O\fP run time

	\0\08.19s	vendor \f(CWcc\fP \f(CW-O3\fP compile time
	\035.97s	vendor \f(CWcc\fP \f(CW-O3\fP load time
	\071.16s	vendor \f(CWcc\fP \f(CW-O3\fP run time
.DE
.LP
To compare the Intel compiler,
a program that is about 40% bit manipulation and
about 60% single precision floating point was
run on the same 33 MHz 486, once under Windows
compiled with the Watcom compiler, version 10.0,
in 16-bit mode and once under
Plan 9 in 32-bit mode.
The Plan 9 execution time was 27 sec while the Windows
execution time was 31 sec.
.NH
Conclusions
.LP
The new compilers compile
quickly,
load slowly,
and produce
medium quality
object code.
The compilers are relatively
portable,
requiring but a couple of weeks' work to
produce a compiler for a different computer.
For Plan 9,
where we needed several compilers
with specialized features and
our own object formats,
this project was indispensable.
It is also necessary for us to
be able to freely distribute our compilers
with the Plan 9 distribution.
.LP
Two problems have come up in retrospect.
The first has to do with the
division of labor between compiler and loader.
Plan 9 runs on multi-processors and as such
compilations are often done in parallel.
Unfortunately,
all compilations must be complete before loading
can begin.
The load is then single-threaded.
With this model,
any shift of work from compile to load
results in a significant increase in real time.
The same is true of libraries that are compiled
infrequently and loaded often.
In the future,
we may try to put some of the loader work
back into the compiler.
.LP
The second problem comes from
the various optimizations performed over several
passes.
Often optimizations in different passes depend
on each other.
Iterating the passes could compromise efficiency,
or even loop.
We see no real solution to this problem.
.NH
References
.LP
[Aho87] A. V. Aho, R. Sethi, and J. D. Ullman,
.I
Compilers \- Principles, Techniques, and Tools,
.R
Addison Wesley,
Reading, MA,
1987.
.LP
[ANSI90] \f2American National Standard for Information Systems \-
Programming Language C\f1, American National Standards Institute, Inc.,
New York, 1990.
.LP
[Dav91] J. W. Davidson and D. B. Whalley,
``Methods for Saving and Restoring Register Values across Function Calls'',
.I
Software\-Practice and Experience,
.R
Vol 21(2), pp. 149-165, February 1991.
.LP
[Joh79] S. C. Johnson,
``YACC \- Yet Another Compiler Compiler'',
.I
UNIX Programmer's Manual, Seventh Ed., Vol. 2A,
.R
AT&T Bell Laboratories,
Murray Hill, NJ,
1979.
.LP
[Set70] R. Sethi and J. D. Ullman,
``The Generation of Optimal Code for Arithmetic Expressions'',
.I
Journal of the ACM,
.R
Vol 17(4), pp. 715-728, 1970.
.LP
[Szy78] T. G. Szymanski,
``Assembling Code for Machines with Span-dependent Instructions'',
.I
Communications of the ACM,
.R
Vol 21(4), pp. 300-308, 1978.
