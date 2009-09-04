.HTML "A Manual for the Plan 9 assembler
.ft CW
.ta 8n +8n +8n +8n +8n +8n +8n
.ft
.TL
A Manual for the Plan 9 assembler
.AU
Rob Pike
rob@plan9.bell-labs.com
.SH
Machines
.PP
There is an assembler for each of the MIPS, SPARC, Intel 386,
Intel 960, AMD 29000, Motorola 68020 and 68000, Motorola Power PC,
AMD64, DEC Alpha, and Acorn ARM.
The 68020 assembler,
.CW 2a ,
is the oldest and in many ways the prototype.
The assemblers are really just variations of a single program:
they share many properties such as left-to-right assignment order for
instruction operands and the synthesis of macro instructions
such as
.CW MOVE
to hide the peculiarities of the load and store structure of the machines.
To keep things concrete, the first part of this manual is
specifically about the 68020.
At the end is a description of the differences among
the other assemblers.
.PP
The document, ``How to Use the Plan 9 C Compiler'', by Rob Pike,
is a prerequisite for this manual.
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
.CW USP ,
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
is the local stack pointer, where automatic variables are held
(SP is a pseudo-register only on the 68020);
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
.CW p+0(FP) ,
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
Addressing modes
.PP
The simple addressing modes are shared by all the assemblers.
Here, for completeness, follows a table of all the 68020 addressing modes,
since that machine has the richest set.
In the table,
.CW o
is an offset, which if zero may be elided, and
.CW d
is a displacement, which is a constant between -128 and 127 inclusive.
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
.IP
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
.in
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
(On most machines,
the only such operator is
.CW WORD
and it lays down 32-bit quantities.
The 386 has all three:
.CW LONG ,
.CW WORD ,
and
.CW BYTE .
The AMD64 adds
.CW QUAD
to that for 64-bit values.
The 960 has only one,
.CW LONG .)
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
or
.P1
	DATA    array+0(SB)/4, $"abc\ez"
	GLOBL   array(SB), $4
.P2
The
.CW /1
defines the number of bytes to define,
.CW GLOBL
makes the symbol global, and the
.CW $4
says how many bytes the symbol occupies.
Uninitialized data is zeroed automatically.
The character
.CW \ez
is equivalent to the C
.CW \e0.
The string in a
.CW DATA
statement may contain a maximum of eight bytes;
build larger strings piecewise.
Two pseudo-instructions,
.CW DYNT
and
.CW INIT ,
allow the (obsolete) Alef compilers to build dynamic type information during the load
phase.
The
.CW DYNT
pseudo-instruction has two forms:
.P1
	DYNT	, ALEF_SI_5+0(SB)
	DYNT	ALEF_AS+0(SB), ALEF_SI_5+0(SB)
.P2
In the first form,
.CW DYNT
defines the symbol to be a small unique integer constant, chosen by the loader,
which is some multiple of the word size.  In the second form,
.CW DYNT
defines the second symbol in the same way,
places the address of the most recently
defined text symbol in the array specified by the first symbol at the
index defined by the value of the second symbol,
and then adjusts the size of the array accordingly.
.PP
The
.CW INIT
pseudo-instruction takes the same parameters as a
.CW DATA
statement.  Its symbol is used as the base of an array and the
data item is installed in the array at the offset specified by the most recent
.CW DYNT
pseudo-instruction.
The size of the array is adjusted accordingly.
The
.CW DYNT
and
.CW INIT
pseudo-instructions are not implemented on the 68020.
.SH
Defining a procedure
.PP
Entry points are defined by the pseudo-operation
.CW TEXT ,
which takes as arguments the name of the procedure (including the ubiquitous
.CW (SB) )
and the number of bytes of automatic storage to pre-allocate on the stack,
which will usually be zero when writing assembly language programs.
On machines with a link register, such as the MIPS and SPARC,
the special value -4 instructs the loader to generate no PC save
and restore instructions, even if the function is not a leaf.
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
pseudo-op is a bit field of options to the loader.
Setting the 1 bit suspends profiling the function when profiling is enabled for the rest of
the program.
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
Setting the 2 bit allows multiple definitions of the same
.CW TEXT
symbol in a program; the loader will place only one such function in the image.
It was emitted only by the Alef compilers.
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
.PP
The MC68000 assembler,
.CW 1a ,
is essentially the same, honoring the appropriate subset of the instructions
and addressing modes.
The definitions of these are, nonetheless, part of
.CW 2.out.h .
.SH
Laying down instructions
.PP
The loader modifies the code produced by the assembler and compiler.
It folds branches,
copies short sequences of code to eliminate branches,
and discards unreachable code.
The first instruction of every function is assumed to be reachable.
The pseudo-instruction
.CW NOP ,
which you may see in compiler output,
means no instruction at all, rather than an instruction that does nothing.
The loader discards all
.CW NOP 's.
.PP
To generate a true
.CW NOP
instruction, or any other instruction not known to the assembler, use a
.CW WORD
pseudo-instruction.
Such instructions on RISCs are not scheduled by the loader and must have
their delay slots filled manually.
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
but in C (not Alef) the value is passed in
.CW R1
instead.
.PP
The loader uses
.CW R28
as a temporary.  The system uses
.CW R26
and
.CW R27
as interrupt-time temporaries.  Therefore none of these registers
should be used in user code.
.PP
The control registers are not known to the assembler.
Instead they are numbered registers
.CW M0 ,
.CW M1 ,
etc.
Use this trick to access, say,
.CW STATUS :
.P1
#define	STATUS	12
	MOVW	M(STATUS), R1
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
to 2.0;
this is done by the operating system.
.PP
The instructions and their syntax are different from those of the manufacturer's
manual.
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
on the 68020 but not as in MIPS documentation.
This means that the
.CW Bcond
instructions are reversed with respect to the book; for example, a
.CW va
.CW BGTZ
generates a MIPS
.CW bltz
instruction.
.PP
The assembler is for the R2000, R3000, and most of the R4000 and R6000 architectures.
It understands the 64-bit instructions
.CW MOVV ,
.CW MOVVL ,
.CW ADDV ,
.CW ADDVU ,
.CW SUBV ,
.CW SUBVU ,
.CW MULV ,
.CW MULVU ,
.CW DIVV ,
.CW DIVVU ,
.CW SLLV ,
.CW SRLV ,
and
.CW SRAV .
The assembler does not have any cache, load-linked, or store-conditional instructions.
.PP
Some assembler instructions are expanded into multiple instructions by the loader.
For example the loader may convert the load of a 32 bit constant into an
.CW lui
followed by an
.CW ori .
.PP
Assembler instructions should be laid out as if there
were no load, branch, or floating point compare delay slots;
the loader will rearrange\(em\f2schedule\f1\(emthe instructions
to guarantee correctness and improve performance.
The only exception is that the correct scheduling of instructions
that use control registers varies from model to model of machine
(and is often undocumented) so you should schedule such instructions
by hand to guarantee correct behavior.
The loader generates
.P1
	NOR	R0, R0, R0
.P2
when it needs a true no-op instruction.
Use exactly this instruction when scheduling code manually;
the loader recognizes it and schedules the code before it and after it independently.  Also,
.CW WORD
pseudo-ops are scheduled like no-ops.
.PP
The
.CW NOSCHED
pseudo-op disables instruction scheduling
(scheduling is enabled by default);
.CW SCHED
re-enables it.
Branch folding, code copying, and dead code elimination are
disabled for instructions that are not scheduled.
.SH
SPARC
.PP
Once you understand the Plan 9 model for the MIPS, the SPARC is familiar.
Registers have numerical names only:
.CW R0
through
.CW R31 .
Forget about register windows: Plan 9 doesn't use them at all.
The machine has 32 global registers, period.
.CW R1
[sic] is the stack pointer.
.CW R2
is the static base register, with value the address of
.CW setSB(SB) .
.CW R7
is the return register and also the register holding the first
argument to a C (not Alef) function, again with space reserved at
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
for the SPARC instruction
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
The SPARC's instruction scheduling is similar to the MIPS's.
The official no-op instruction is:
.P1
	ORN	R0, R0, R0
.P2
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
The stack pointer (not a pseudo-register) is
.CW SP
and the return register is
.CW AX .
There is no physical frame pointer but, as for the MIPS,
.CW FP
is a pseudo-register that acts as
a frame pointer.
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
There are many examples of illegal moves, for example,
.P1
	MOVB	BP, DI
.P2
that the loader actually implements as pseudo-operations.
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
stand-alone opcodes that precede the strings, for example
.P1
	CLD; REP; MOVSL
.P2
Segment override prefixes in
.CW MOD/RM
fields are not supported.
.SH
AMD64
.PP
The assembler assumes 64-bit mode unless a
.CW MODE
pseudo-operation is given:
.P1
	MODE $32
.P2
to change to 32-bit mode.
The effect is mainly to diagnose instructions that are illegal in
the given mode, but the loader will also assume 32-bit operands and addresses,
and 32-bit PC values for call and return.
The assembler's conventions are similar to those for the 386, above.
The architecture provides extra fixed-point registers
.CW R8
to
.CW R15 .
All registers are 64 bit, but instructions access low-order 8, 16 and 32 bits
as described in the processor handbook.
For example,
.CW MOVL
to
.CW AX
puts a value in the low-order 32 bits and clears the top 32 bits to zero.
Literal operands are limited to signed 32 bit values, which are sign-extended
to 64 bits in 64 bit operations; the exception is
.CW MOVQ ,
which allows 64-bit literals.
The external registers in Plan 9's C are allocated from
.CW R15
down.
There are many new instructions, including the MMX and XMM media instructions,
and conditional move instructions.
MMX registers are
.CW M0
to
.CW M7 ,
and
XMM registers are
.CW X0
to
.CW X15 .
As with the 386 instruction names,
all new 64-bit integer instructions, and the MMX and XMM instructions
uniformly use
.CW L
for `long word' (32 bits) and
.CW Q
for `quad word' (64 bits).
Some instructions use
.CW O
(`octword') for 128-bit values, where the processor handbook
variously uses
.CW O
or
.CW DQ .
The assembler also consistently uses
.CW PL
for `packed long' in
XMM instructions, instead of
.CW Q ,
.CW DQ
or
.CW PI .
Either
.CW MOVL
or
.CW MOVQ
can be used to move values to and from control registers, even when
the registers might be 64 bits.
The assembler often accepts the handbook's name to ease conversion
of existing code (but remember that the operand order is uniformly
source then destination).
C's
.CW "long long"
type is 64 bits, but passed and returned by value, not by reference.
More notably, C pointer values are 64 bits, and thus
.CW "long long"
and
.CW "unsigned long long"
are the only integer types wide enough to hold a pointer value.
The C compiler and library use the XMM floating-point instructions, not
the old 387 ones, although the latter are implemented by assembler and loader.
Unlike the 386, the first integer or pointer argument is passed in a register, which is
.CW BP
for an integer or pointer (it can be referred to in assembly code by the pseudonym
.CW RARG ).
.CW AX
holds the return value from subroutines as before.
Floating-point results are returned in
.CW X0 ,
although currently the first floating-point parameter is not passed in a register.
All parameters less than 8 bytes in length have 8 byte slots reserved on the stack
to preserve alignment and simplify variable-length argument list access,
including the first parameter when passed in a register,
even though bytes 4 to 7 are not initialized.
.SH
Alpha
.PP
On the Alpha, all registers are 64 bits.  The architecture handles 32-bit values
by giving them a canonical format (sign extension in the case of integer registers).
Registers are numbered
.CW R0
through
.CW R31 .
.CW R0
holds the return value from subroutines, and also the first parameter.
.CW R30
is the stack pointer,
.CW R29
is the static base,
.CW R26
is the link register, and
.CW R27
and
.CW R28
are linker temporaries.
.PP
Floating point registers are numbered
.CW F0
to
.CW F31 .
.CW F28
contains
.CW 0.5 ,
.CW F29
contains
.CW 1.0 ,
and
.CW F30
contains
.CW 2.0 .
.CW F31
is always
.CW 0.0
on the Alpha.
.PP
The extension character for
.CW MOV
follows DEC's notation:
.CW B
for byte (8 bits),
.CW W
for word (16 bits),
.CW L
for long (32 bits),
and
.CW Q
for quadword (64 bits).
Byte and ``word'' loads and stores may be made unsigned
by appending a
.CW U .
.CW S
and
.CW T
refer to IEEE floating point single precision (32 bits) and double precision (64 bits), respectively.
.SH
Power PC
.PP
The Power PC follows the Plan 9 model set by the MIPS and SPARC,
not the elaborate ABIs.
The 32-bit instructions of the 60x and 8xx PowerPC architectures are supported;
there is no support for the older POWER instructions.
Registers are
.CW R0
through
.CW R31 .
.CW R0
is initialized to zero; this is done by C start up code
and assumed by the compiler and loader.
.CW R1
is the stack pointer.
.CW R2
is the static base register, with value the address of
.CW setSB(SB) .
.CW R3
is the return register and also the register holding the first
argument to a C function, with space reserved at
.CW 0(FP)
as on the MIPS.
.CW R31
is the loader temporary.
The external registers in Plan 9's C are allocated from
.CW R30
down.
.PP
Floating point registers are called
.CW F0
through
.CW F31 .
By convention, several registers are initialized
to specific values; this is done by the operating system.
.CW F27
must be initialized to the value
.CW 0x4330000080000000
(used by float-to-int conversion),
.CW F28
to the value 0.0,
.CW F29
to 0.5,
.CW F30
to 1.0, and
.CW F31
to 2.0.
.PP
As on the MIPS and SPARC, the assembler accepts arbitrary literals
as operands to
.CW MOVW ,
and also to
.CW ADD
and others where `immediate' variants exist,
and the loader generates sequences
of
.CW addi ,
.CW addis ,
.CW oris ,
etc. as required.
The register indirect addressing modes use the same syntax as the SPARC,
including double indexing when allowed.
.PP
The instruction names are generally derived from the Motorola ones,
subject to slight transformation:
the
.CW . ' `
marking the setting of condition codes is replaced by
.CW CC ,
and when the letter
.CW o ' `
represents `OE=1' it is replaced by
.CW V .
Thus
.CW add ,
.CW addo.
and
.CW subfzeo.
become
.CW ADD ,
.CW ADDVCC
and
.CW SUBFZEVCC .
As well as the three-operand conditional branch instruction
.CW BC ,
the assembler provides pseudo-instructions for the common cases:
.CW BEQ ,
.CW BNE ,
.CW BGT ,
.CW BGE ,
.CW BLT ,
.CW BLE ,
.CW BVC ,
and
.CW BVS .
The unconditional branch instruction is
.CW BR .
Indirect branches use
.CW "(CTR)"
or
.CW "(LR)"
as target.
.PP
Load or store operations are replaced by
.CW MOV
variants in the usual way:
.CW MOVW
(move word),
.CW MOVH
(move halfword with sign extension), and
.CW MOVB
(move byte with sign extension, a pseudo-instruction),
with unsigned variants
.CW MOVHZ
and
.CW MOVBZ ,
and byte-reversing
.CW MOVWBR
and
.CW MOVHBR .
`Load or store with update' versions are
.CW MOVWU ,
.CW MOVHU ,
and
.CW MOVBZU .
Load or store multiple is
.CW MOVMW .
The exceptions are the string instructions, which are
.CW LSW
and
.CW STSW ,
and the reservation instructions
.CW lwarx
and
.CW stwcx. ,
which are
.CW LWAR
and
.CW STWCCC ,
all with operands in the usual data-flow order.
Floating-point load or store instructions are
.CW FMOVD ,
.CW FMOVDU ,
.CW FMOVS ,
and
.CW FMOVSU .
The register to register move instructions
.CW fmr
and
.CW fmr.
are written
.CW FMOVD
and
.CW FMOVDCC .
.PP
The assembler knows the commonly used special purpose registers:
.CW CR ,
.CW CTR ,
.CW DEC ,
.CW LR ,
.CW MSR ,
and
.CW XER .
The rest, which are often architecture-dependent, are referenced as
.CW SPR(n) . 
The segment registers of the 60x series are similarly
.CW SEG(n) ,
but
.I n
can also be a register name, as in
.CW SEG(R3) .
Moves between special purpose registers and general purpose ones,
when allowed by the architecture,
are written as
.CW MOVW ,
replacing
.CW mfcr ,
.CW mtcr ,
.CW mfmsr ,
.CW mtmsr ,
.CW mtspr ,
.CW mfspr ,
.CW mftb ,
and many others.
.PP
The fields of the condition register
.CW CR
are referenced as
.CW CR(0)
through
.CW CR(7) .
They are used by the
.CW MOVFL
(move field) pseudo-instruction,
which produces
.CW mcrf
or
.CW mtcrf .
For example:
.P1
	MOVFL	CR(3), CR(0)
	MOVFL	R3, CR(1)
	MOVFL	R3, $7, CR
.P2
They are also accepted in
the conditional branch instruction, for example
.P1
	BEQ	CR(7), label
.P2
Fields of the
.CW FPSCR
are accessed using
.CW MOVFL
in a similar way:
.P1
	MOVFL	FPSCR, F0
	MOVFL	F0, FPSCR
	MOVFL	F0, $7, FPSCR
	MOVFL	$0, FPSCR(3)
.P2
producing
.CW mffs ,
.CW mtfsf
or
.CW mtfsfi ,
as appropriate.
.SH
ARM
.PP
The assembler provides access to
.CW R0
through
.CW R14
and the
.CW PC .
The stack pointer is
.CW R13 ,
the link register is
.CW R14 ,
and the static base register is
.CW R12 .
.CW R0
is the return register and also the register holding
the first argument to a subroutine.
The assembler supports the
.CW CPSR
and
.CW SPSR
registers.
It also knows about coprocessor registers
.CW C0
through
.CW C15 .
Floating registers are
.CW F0
through
.CW F7 ,
.CW FPSR
and
.CW FPCR .
.PP
As with the other architectures, loads and stores are called
.CW MOV ,
e.g.
.CW MOVW
for load word or store word, and
.CW MOVM
for
load or store multiple,
depending on the operands.
.PP
Addressing modes are supported by suffixes to the instructions:
.CW .IA
(increment after),
.CW .IB
(increment before),
.CW .DA
(decrement after), and
.CW .DB
(decrement before).
These can only be used with the
.CW MOV
instructions.
The move multiple instruction,
.CW MOVM ,
defines a range of registers using brackets, e.g.
.CW [R0-R12] .
The special
.CW MOVM
addressing mode bits
.CW W ,
.CW U ,
and
.CW P
are written in the same manner, for example,
.CW MOVM.DB.W .
A
.CW .S
suffix allows a
.CW MOVM
instruction to access user
.CW R13
and
.CW R14
when in another processor mode.
Shifts and rotates in addressing modes are supported by binary operators
.CW <<
(logical left shift),
.CW >>
(logical right shift),
.CW ->
(arithmetic right shift), and
.CW @>
(rotate right); for example
.CW "R7>>R2" or
.CW "R2@>2" .
The assembler does not support indexing by a shifted expression;
only names can be doubly indexed.
.PP
Any instruction can be followed by a suffix that makes the instruction conditional:
.CW .EQ ,
.CW .NE ,
and so on, as in the ARM manual, with synonyms
.CW .HS
(for
.CW .CS )
and
.CW .LO
(for
.CW .CC ),
for example
.CW ADD.NE .
Arithmetic
and logical instructions
can have a
.CW .S
suffix, as ARM allows, to set condition codes.
.PP
The syntax of the
.CW MCR
and
.CW MRC
coprocessor instructions is largely as in the manual, with the usual adjustments.
The assembler directly supports only the ARM floating-point coprocessor
operations used by the compiler:
.CW CMP ,
.CW ADD ,
.CW SUB ,
.CW MUL ,
and
.CW DIV ,
all with
.CW F
or
.CW D
suffix selecting single or double precision.
Floating-point load or store become
.CW MOVF
and
.CW MOVD .
Conversion instructions are also specified by moves:
.CW MOVWD ,
.CW MOVWF ,
.CW MOVDW ,
.CW MOVWD ,
.CW MOVFD ,
and
.CW MOVDF .
.SH
AMD 29000
.PP
For details about this assembly language, which was built for the AMD 29240,
look at the sources or examine compiler output.
