.ft CW
.ta 8n +8n +8n +8n +8n +8n +8n
.ft
.TL
A manual for the Plan 9 assembler
.AU
Rob Pike
.SH
Disclaimer
.PP
I wrote this manual to document some of the assembler's strange properties
but I am not responsible for any of them.
.SH
Machines
.PP
There is an assembler for each of the MIPS 3000, SPARC, Intel 386,
Intel 960, AT&T Hobbit, and Motorola 68020.
The 68020 assembler,
.CW 2a ,
is the oldest and in many ways the prototype.
To keep things concrete, the first part of this manual is
specifically about the 68020.
At the end is a brief description of the differences among
the other assemblers.
.SH
Registers
.PP
All pre-defined symbols in the assembler are upper-case.
Data registers are
.CW R0
through
.CW R7 ;
address registers are
.CW A0
through
.CW A7 ;
floating-point registers are
.CW F0
through
.CW F7 .
.PP
A pointer in
.CW A6
is used by the C compiler to point to data, enabling short addresses to
be used more often.
The value of
.CW A6
is constant and must be set during C program initialization
to the address of the externally-defined symbol
.CW a6base .
.PP
The following hardware registers are defined in the assembler; their
meaning should be obvious given a 68020 manual:
.CW CAAR ,
.CW CACR ,
.CW CCR ,
.CW DFC ,
.CW ISP ,
.CW MSP ,
.CW SFC ,
.CW SR ,
.CW USP
and
.CW VBR .
.PP
The assembler also defines several pseudo-registers that
manipulate the stack:
.CW FP ,
.CW SP ,
and
.CW TOS .
.CW FP
is the frame pointer, so
.CW 0(FP)
is the first argument,
.CW 4(FP)
is the second, and so on.
.CW SP
is the local stack pointer, where automatic variables are held;
.CW 0(SP)
is the first automatic, and so on as with
.CW FP .
Finally,
.CW TOS
is the top-of-stack register, used for pushing parameters to procedures,
saving temporary values, and so on.
.PP
The assembler and loader track these pseudo-registers so
the above statements are true regardless of what has been
pushed on the hardware stack, pointed to by
.CW A7 .
The name
.CW A7
refers to the hardware stack pointer, but beware of mixed use of
.CW A7
and the above stack-related pseudo-registers, which will cause trouble.
Note, too, that the
.CW PEA
instruction is observed by the loader to
alter SP and thus will insert a corresponding pop before all returns.
The assembler accepts a label-like name to be attached to
.CW FP
and
.CW SP
uses, such as
.CW p+0(FP)
to help document that
.CW p
is the first argument to a routine.
The name goes in the symbol table but has no significance to the result
of the program.
.SH
Referring to data
.PP
All external references must be made relative to some pseudo-register,
either
.CW PC
(the virtual program counter) or
.CW SB
(the ``static base'' register).
.CW PC
counts instructions, not bytes of data.
For example, to branch to the second following instruction, that is,
to skip one instruction, one may write
.P1
	BRA	2(PC)
.P2
Labels are also allowed, as in
.P1
	BRA	return
	NOP
return:
	RTS
.P2
When using labels, there is no
.CW (PC)
annotation.
.PP
The pseudo-register
.CW SB
refers to the beginning of the address space of the program.
Thus, references to global data and procedures are written as
offsets to
.CW SB ,
as in
.P1
	MOVL	$array(SB), TOS
.P2
to push the address of a global array on the stack, or
.P1
	MOVL	array+4(SB), TOS
.P2
to push the second (4-byte) element of the array.
Note the use of an offset; the complete list of addressing modes is given below.
Similarly, subroutine calls must use
.CW SB :
.P1
	BSR	exit(SB)
.P2
File-static variables have syntax
.P1
	local<>+4(SB)
.P2
The
.CW <>
will be filled in at load time by a unique integer.
.PP
When a program starts, it must execute
.P1
	MOVL	$a6base(SB), A6
.P2
before accessing any global data.
(On machines such as the MIPS and SPARC that cannot load a register
in a single instruction, constants are loaded through the static base
register.  The loader recognizes code that initializes the static
base register and treats it specially.  You must be careful, however,
not to load large constants on such machines when the static base
register is not set up, such as early in interrupt routines.)
.SH
Expressions
.PP
Expressions are mostly what one might expect.
Where an offset or a constant is expected,
a primary expression with unary operators is allowed.
A general C constant expression is allowed in parentheses.
.PP
Source files are preprocessed exactly as in the C compiler, so
.CW #define
and
.CW #include
work.
.SH
Laying down data
.PP
Placing data in the instruction stream, say for interrupt vectors, is easy:
the pseudo-instructions
.CW LONG
and
.CW WORD
(but not
.CW BYTE )
lay down the value of their single argument, of the appropriate size,
as if it were an instruction:
.P1
	LONG	$12345
.P2
places the long 12345 (base 10)
in the instruction stream.
(On all machines except the 68020, the only such operator is
.CW WORD
and it lays down 32-bit quantities.)
.PP
Placing information in the data section is more painful.
The pseudo-instruction
.CW DATA
does the work, given two arguments: an address at which to place the item,
including its size,
and the value to place there.  For example, to define a character array
.CW array
containing the characters
.CW abc
and a terminating null:
.P1
	DATA    array+0(SB)/1, $'a'
	DATA    array+1(SB)/1, $'b'
	DATA    array+2(SB)/1, $'c'
	GLOBL   array(SB), $4
.P2
Here, the
.CW /1
defines the number of bytes to define,
.CW GLOBL
makes the symbol global, and the
.CW $4
says how many bytes the symbol occupies.
Uninitialized data is zeroed automatically.
.SH
Defining a procedure
.PP
Entry points are defined by the pseudo-operation
.CW TEXT ,
which takes as arguments the name of the procedure (including the ubiquitous
.CW (SB) )
and the number of bytes of automatic storage to pre-allocate on the stack,
which will usually be zero when writing assembly language programs.
Here is a complete procedure that returns the sum
of its two arguments:
.P1
TEXT	sum(SB), $0
	MOVL	arg1+0(FP), R0
	ADDL	arg2+4(FP), R0
	RTS
.P2
An optional middle argument
to the
.CW TEXT
pseudo-op causes the loader
.I not
to insert profiling code into the subroutine, if it has the value
.CW 1 .
For example,
.P1
TEXT	sum(SB), 1, $0
	MOVL	arg1+0(FP), R0
	ADDL	arg2+4(FP), R0
	RTS
.P2
will not be profiled; the first version above would be.
Subroutines with peculiar state, such as system call routines,
should not be profiled.
.PP
Subroutines to be called from C should place their result in
.CW R0 ,
even if it is an address.
Floating point values are returned in
.CW F0 .
Functions that return a structure to a C program
receive as their first argument the address of the location to
store the result;
.CW R0
is unused in the calling protocol for such procedures.
A subroutine is responsible for saving its own registers,
and therefore is free to use any registers without saving them (``caller saves'').
.CW A6
and
.CW A7
are the exceptions as described above.
.SH
When in doubt
.PP
If you get confused, try using the
.CW -S
option to
.CW 2c
and compiling a sample program.
The standard output is valid input to the assembler.
.SH
Instructions
.PP
The instruction set of the assembler is not identical to that
of the machine.
It is chosen to match what the compiler generates, augmented
slightly by specific needs of the operating system.
For example,
.CW 2a
does not distinguish between the various forms of
.CW MOVE
instruction: move quick, move address, etc.  Instead the context
does the job.  For example,
.P1
	MOVL	$1, R1
	MOVL	A0, R2
	MOVW	SR, R3
.P2
generates official
.CW MOVEQ ,
.CW MOVEA ,
and
.CW MOVESR
instructions.
A number of instructions do not have the syntax necessary to specify
their entire capabilities.  Notable examples are the bitfield
instructions, the
multiply and divide instructions, etc.
For a complete set of generated instruction names (in
.CW 2a
notation, not Motorola's) see the file
.CW /sys/src/cmd/2c/2.out.h .
Despite its name, this file contains an enumeration of the
instructions that appear in the intermediate files generated
by the compiler, which correspond exactly to lines of assembly language.
.SH
Addressing modes
.PP
In this table,
.CW o
is an offset, which if zero may be elided.
Many of the modes listed have the same name;
scrutiny of the format will show what default is being applied.
For instance, indexed mode with no address register supplied operates
as though a zero-valued register were used.
For "offset" read "displacement."
For "\f(CW.s\fP" read one of
.CW .L ,
or
.CW .W
followed by
.CW *1 ,
.CW *2 ,
.CW *4 ,
or
.CW *8
to indicate the size and scaling of the data.
.TS
l lfCW.
data register	R0
address register	A0
floating-point register	F0
special names	CAAR, CACR, etc.
constant	$con
floating point constant	$fcon
external symbol	name+o(SB)
local symbol	name<>+o(SB)
automatic symbol	name+o(SP)
argument	name+o(FP)
address of external	$name+o(SB)
address of local	$name<>+o(SB)
indirect post-increment	(A0)+
indirect pre-decrement	-(A0)
indirect with offset	o(A0)
indexed with offset	o()(R0.s)
indexed with offset	o(A0)(R0.s)
external indexed	name+o(SB)(R0.s)
local indexed	name<>+o(SB)(R0.s)
automatic indexed	name+o(SP)(R0.s)
parameter indexed	name+o(FP)(R0.s)
offset indirect post-indexed	d(o())(R0.s)
offset indirect post-indexed	d(o(A0))(R0.s)
external indirect post-indexed	d(name+o(SB))(R0.s)
local indirect post-indexed	d(name<>+o(SB))(R0.s)
automatic indirect post-indexed	d(name+o(SP))(R0.s)
parameter indirect post-indexed	d(name+o(FP))(R0.s)
offset indirect pre-indexed	d(o()(R0.s))
offset indirect pre-indexed	d(o(A0))
offset indirect pre-indexed	d(o(A0)(R0.s))
external indirect pre-indexed	d(name+o(SB))
external indirect pre-indexed	d(name+o(SB)(R0.s))
local indirect pre-indexed	d(name<>+o(SB))
local indirect pre-indexed	d(name<>+o(SB)(R0.s))
automatic indirect pre-indexed	d(name+o(SP))
automatic indirect pre-indexed	d(name+o(SP)(R0.s))
parameter indirect pre-indexed	d(name+o(FP))
parameter indirect pre-indexed	d(name+o(FP)(R0.s))
.TE
.SH
MIPS
.PP
The registers are only addressed by number:
.CW R0
through
.CW R31 .
.CW R29
is the stack pointer;
.CW R30
is used as the static base pointer, the analogue of
.CW A6
on the 68020.
Its value is the address of the global symbol
.CW setR30(SB) .
The register holding returned values from subroutines is
.CW R1 .
When a function is called, space for the first argument
is reserved at
.CW 0(FP)
but the value is actually passed in
.CW R1.
.PP
The loader uses
.CW R28
as a temporary.  The system uses
.CW R26
and
.CW R27
as interrupt-time temporaries.  None of these registers should
therefore be used in user code.
.PP
The control registers are not known to the assembler.
Instead they are numbered registers
.CW M0 ,
.CW M1 ,
etc.
Use this trick to access, say,
.CW TLBVIRT :
.P1
#define	TLBVIRT	10
	MOVW	M(TLBVIRT), R1
.P2
.PP
Floating point registers are called
.CW F0
through
.CW F31 .
By convention,
.CW F24
must be initialized to the value 0.0,
.CW F26
to 0.5,
.CW F28
to 1.0, and
.CW F30
to 2.0.
.PP
The instructions are quite different from the book.
There are no
.CW lui
and kin; instead there are
.CW MOVW
(move word),
.CW MOVH
(move halfword),
and
.CW MOVB
(move byte) pseudo-instructions.  If the operand is unsigned, the instructions
are
.CW MOVHU
and
.CW MOVBU .
The order of operands is from left to right in dataflow order, just as
on the 68020 but not as in the book.  This means that the
.CW Bcond
instructions are reversed with respect to the book; for example, a
.CW va
.CW BGTZ
generates a MIPS
.CW bltz
instruction.
.PP
Instruction scheduling is handled by the loader.  Place the instructions
down as you would if there were no such things
as delay slots, except that, because the information to solve
the problem is not published,
you must insert your own NOOPS after instructions that modify
control registers.  Use exactly
.P1
	NOR	R0, R0, R0
.P2
as a no-op.
.SH
SPARC
.PP
Once you understand the Plan 9 model for the MIPS, the SPARC is familiar.
Registers have numerical names only:
.CW R0
through
.CW R31 .
Forget about register windows.  Plan 9 doesn't use them at all.
The machine has 32 global registers, period.
.CW R1
(sic) is the stack pointer.
.CW R2
is the static base register, with value the address of
.CW setSB(SB) .
.CW R7
is the return register and also the register holding the first
argument to a function, again with space reserved at
.CW 0(FP) .
.CW R14
is the loader temporary.
.PP
Floating-point registers are exactly as on the MIPS.
.PP
The control registers are known by names such as
.CW FSR .
The instructions to access these registers are
.CW MOVW
instructions, for example
.P1
	MOVW	Y, R8
.P2
for the Sparc instruction
.P1
	rdy	%r8
.P2
.PP
Move instructions are similar to those on the MIPS: pseudo-operations
that turn into appropriate sequences of
.CW sethi
instructions, adds, etc.
Instructions read from left to right.  Because the arguments are
flipped to
.CW SUBCC ,
the condition codes are not inverted as on the MIPS.
.PP
The syntax for the ASI stuff is, for example to move a word from ASI 2:
.P1
	MOVW	(R7, 2), R8
.P2
The syntax for double indexing is
.P1
	MOVW	(R7+R8), R9
.P2
.PP
Again, ignore scheduling and delay slots when laying down instructions; the loader
will do the scheduling.
.SH
i960
.PP
Registers are numbered
.CW R0
through
.CW R31 .
Stack pointer is
.CW R29 ;
return register is
.CW R4 ;
static base is
.CW R28 ;
it is initialized to the address of
.CW setSB(SB) .
.CW R3
must be zero; this should be done manually early in execution by
.P1
	SUBO	R3, R3
.P2
.CW R27
is the loader temporary.
.PP
There is no support for floating point.
.PP
The Intel calling convention is not supported and cannot be used; use
.CW BAL
instead.
Instructions are mostly as in the book.  The major change is that
.CW LOAD
and
.CW STORE
are both called
.CW MOV .
The extension character for
.CW MOV
is as in the manual:
.CW O
for ordinal,
.CW W
for signed, etc.
.SH
i386
.PP
The assembler assumes 32-bit protected mode.
The register names are
.CW SP ,
.CW AX ,
.CW BX ,
.CW CX ,
.CW DX ,
.CW BP ,
.CW DI ,
and
.CW SI .
The stack pointer is
.CW SP
and the return register is
.CW AX .
There is no physical frame pointer but, as for the 68020,
.CW FP
is a pseudo-register that acts as
a floating frame pointer.
.PP
Opcode names are mostly the same as those listed in the Intel manual
with an
.CW L ,
.CW W ,
or
.CW B
appended to identify 32-bit, 
16-bit, and 8-bit operations.
The exceptions are loads, stores, and conditionals.
All load and store opcodes to and from general registers, special registers
(such as
.CW CR0,
.CW CR3,
.CW GDTR,
.CW IDTR,
.CW SS,
.CW CS,
.CW DS,
.CW ES,
.CW FS,
and
.CW GS )
or memory are written
as
.P1
	MOV\f2x\fP	src,dst
.P2
where
.I x
is
.CW L ,
.CW W ,
or
.CW B .
Thus to get
.CW AL
use a
.CW MOVB
instruction.  If you need to access
.CW AH ,
you must mention it explicitly in a
.CW MOVB :
.P1
	MOVB	AH, BX
.P2
There are many examples of illegal moves, for example
.P1
	MOVB	BP, DI
.P2
that the loader actually implements as psuedo-operations.
.PP
The names of conditions in all conditional instructions
.CW J , (
.CW SET )
follow the conventions of the 68020 instead of those of the Intel
assembler:
.CW JOS ,
.CW JOC ,
.CW JCS ,
.CW JCC ,
.CW JEQ ,
.CW JNE ,
.CW JLS ,
.CW JHI ,
.CW JMI ,
.CW JPL ,
.CW JPS ,
.CW JPC ,
.CW JLT ,
.CW JGE ,
.CW JLE ,
and
.CW JGT
instead of
.CW JO ,
.CW JNO ,
.CW JB ,
.CW JNB ,
.CW JZ ,
.CW JNZ ,
.CW JBE ,
.CW JNBE ,
.CW JS ,
.CW JNS ,
.CW JP ,
.CW JNP ,
.CW JL ,
.CW JNL ,
.CW JLE ,
and
.CW JNLE .
.PP
The addressing modes have syntax like
.CW AX ,
.CW (AX) ,
.CW (AX)(BX*4) ,
.CW 10(AX) ,
and
.CW 10(AX)(BX*4) .
The offsets from
.CW AX
can be replaced by offsets from
.CW FP
or
.CW SB
to access names, for example
.CW extern+5(SB)(AX*2) .
.PP
Other notes: Non-relative
.CW JMP
and
.CW CALL
have a
.CW *
added to the syntax.
Only
.CW LOOP ,
.CW LOOPEQ ,
and
.CW LOOPNE
are legal loop instructions.  Only
.CW REP
and
.CW REPN
are recognized repeaters.  These are not prefixes, but rather
standalone opcodes that precede the strings, for example
.P1
	CLD; REP; MOVSL
.P2
.SH
Hobbit
.PP
The Hobbit book and data sheet are actually based on this assembler,
so the correspondence between the hardware and the assembler is
much closer than with the other machines.  There is no static
base register.
.PP
The one syntactic difference is that data operand suffices
use a colon
in the book, but a period in the assembler:
.P1
	MOV	$0, R4.UB
.P2
Unlike the other assemblers, these suffices are applied to
the operands rather than the instructions.

