.HTML "How to Use the Plan 9 C Compiler
.TL
How to Use the Plan 9 C Compiler
.AU
Rob Pike
rob@plan9.bell-labs.com
.SH
Introduction
.PP
The C compiler on Plan 9 is a wholly new program; in fact
it was the first piece of software written for what would
eventually become Plan 9 from Bell Labs.
Programmers familiar with existing C compilers will find
a number of differences in both the language the Plan 9 compiler
accepts and in how the compiler is used.
.PP
The compiler is really a set of compilers, one for each
architecture \(em MIPS, SPARC, Motorola 68020, Intel 386, etc. \(em
that accept a dialect of ANSI C and efficiently produce
fairly good code for the target machine.
There is a packaging of the compiler that accepts strict ANSI C for
a POSIX environment, but this document focuses on the
native Plan 9 environment, that in which all the system source and
almost all the utilities are written.
.SH
Source
.PP
The language accepted by the compilers is the core 1989 ANSI C language
with some modest extensions,
a greatly simplified preprocessor,
a smaller library that includes system calls and related facilities,
and a completely different structure for include files.
.PP
Official ANSI C accepts the old (K&R) style of declarations for
functions; the Plan 9 compilers
are more demanding.
Without an explicit run-time flag
.CW -B ) (
whose use is discouraged, the compilers insist
on new-style function declarations, that is, prototypes for
function arguments.
The function declarations in the libraries' include files are
all in the new style so the interfaces are checked at compile time.
For C programmers who have not yet switched to function prototypes
the clumsy syntax may seem repellent but the payoff in stronger typing
is substantial.
Those who wish to import existing software to Plan 9 are urged
to use the opportunity to update their code.
.PP
The compilers include an integrated preprocessor that accepts the familiar
.CW #include ,
.CW #define
for macros both with and without arguments,
.CW #undef ,
.CW #line ,
.CW #ifdef ,
.CW #ifndef ,
and
.CW #endif .
It
supports neither
.CW #if
nor
.CW ## ,
although it does
honor a few
.CW #pragmas .
The
.CW #if
directive was omitted because it greatly complicates the
preprocessor, is never necessary, and is usually abused.
Conditional compilation in general makes code hard to understand;
the Plan 9 source uses it sparingly.
Also, because the compilers remove dead code, regular
.CW if
statements with constant conditions are more readable equivalents to many
.CW #ifs .
To compile imported code ineluctably fouled by
.CW #if
there is a separate command,
.CW /bin/cpp ,
that implements the complete ANSI C preprocessor specification.
.PP
Include files fall into two groups: machine-dependent and machine-independent.
The machine-independent files occupy the directory
.CW /sys/include ;
the others are placed in a directory appropriate to the machine, such as
.CW /mips/include .
The compiler searches for include files
first in the machine-dependent directory and then
in the machine-independent directory.
At the time of writing there are thirty-one machine-independent include
files and two (per machine) machine-dependent ones:
.CW <ureg.h>
and
.CW <u.h> .
The first describes the layout of registers on the system stack,
for use by the debugger.
The second defines some
architecture-dependent types such as
.CW jmp_buf
for
.CW setjmp
and the
.CW va_arg
and
.CW va_list
macros for handling arguments to variadic functions,
as well as a set of
.CW typedef
abbreviations for
.CW unsigned
.CW short
and so on.
.PP
Here is an excerpt from
.CW /68020/include/u.h :
.P1
#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef   signed char	schar;
typedef	long long       vlong;

typedef long	jmp_buf[2];
#define	JMPBUFSP	0
#define	JMPBUFPC	1
#define	JMPBUFDPC	0
.P2
Plan 9 programs use
.CW nil
for the name of the zero-valued pointer.
The type
.CW vlong
is the largest integer type available; on most architectures it
is a 64-bit value.
A couple of other types in
.CW <u.h>
are
.CW u32int ,
which is guaranteed to have exactly 32 bits (a possibility on all the supported architectures) and
.CW mpdigit ,
which is used by the multiprecision math package
.CW <mp.h> .
The
.CW #define
constants permit an architecture-independent (but compiler-dependent)
implementation of stack-switching using
.CW setjmp
and
.CW longjmp .
.PP
Every Plan 9 C program begins
.P1
#include <u.h>
.P2
because all the other installed header files use the
.CW typedefs
declared in
.CW <u.h> .
.PP
In strict ANSI C, include files are grouped to collect related functions
in a single file: one for string functions, one for memory functions,
one for I/O, and none for system calls.
Each include file is protected by an
.CW #ifdef
to guarantee its contents are seen by the compiler only once.
Plan 9 takes a different approach.  Other than a few include
files that define external formats such as archives, the files in
.CW /sys/include
correspond to
.I libraries.
If a program is using a library, it includes the corresponding header.
The default C library comprises string functions, memory functions, and
so on, largely as in ANSI C, some formatted I/O routines,
plus all the system calls and related functions.
To use these functions, one must
.CW #include
the file
.CW <libc.h> ,
which in turn must follow
.CW <u.h> ,
to define their prototypes for the compiler.
Here is the complete source to the traditional first C program:
.P1
#include <u.h>
#include <libc.h>

void
main(void)
{
	print("hello world\en");
	exits(0);
}
.P2
The
.CW print
routine and its relatives
.CW fprint
and
.CW sprint
resemble the similarly-named functions in Standard I/O but are not
attached to a specific I/O library.
In Plan 9
.CW main
is not integer-valued; it should call
.CW exits ,
which takes a string argument (or null; here ANSI C promotes the 0 to a
.CW char* ).
All these functions are, of course, documented in the Programmer's Manual.
.PP
To use
.CW printf ,
.CW <stdio.h>
must be included to define the function prototype for
.CW printf :
.P1
#include <u.h>
#include <libc.h>
#include <stdio.h>

void
main(int argc, char *argv[])
{
	printf("%s: hello world; argc = %d\en", argv[0], argc);
	exits(0);
}
.P2
In practice, Standard I/O is not used much in Plan 9.  I/O libraries are
discussed in a later section of this document.
.PP
There are libraries for handling regular expressions, raster graphics,
windows, and so on, and each has an associated include file.
The manual for each library states which include files are needed.
The files are not protected against multiple inclusion and themselves
contain no nested
.CW #includes .
Instead the
programmer is expected to sort out the requirements
and to
.CW #include
the necessary files once at the top of each source file.  In practice this is
trivial: this way of handling include files is so straightforward
that it is rare for a source file to contain more than half a dozen
.CW #includes .
.PP
The compilers do their own register allocation so the
.CW register
keyword is ignored.
For different reasons,
.CW volatile
and
.CW const
are also ignored.
.PP
To make it easier to share code with other systems, Plan 9 has a version
of the compiler,
.CW pcc ,
that provides the standard ANSI C preprocessor, headers, and libraries
with POSIX extensions.
.CW Pcc
is recommended only
when broad external portability is mandated.  It compiles slower,
produces slower code (it takes extra work to simulate POSIX on Plan 9),
eliminates those parts of the Plan 9 interface
not related to POSIX, and illustrates the clumsiness of an environment
designed by committee.
.CW Pcc
is described in more detail in
.I
APE\(emThe ANSI/POSIX Environment,
.R
by Howard Trickey.
.SH
Process
.PP
Each CPU architecture supported by Plan 9 is identified by a single,
arbitrary, alphanumeric character:
.CW k
for SPARC,
.CW q
for Motorola Power PC 630 and 640,
.CW v
for MIPS,
.CW 0
for little-endian MIPS,
.CW 1
for Motorola 68000,
.CW 2
for Motorola 68020 and 68040,
.CW 5
for Acorn ARM 7500,
.CW 6
for AMD 64,
.CW 7
for DEC Alpha,
.CW 8
for Intel 386, and
.CW 9
for AMD 29000.
The character labels the support tools and files for that architecture.
For instance, for the 68020 the compiler is
.CW 2c ,
the assembler is
.CW 2a ,
the link editor/loader is
.CW 2l ,
the object files are suffixed
.CW \&.2 ,
and the default name for an executable file is
.CW 2.out .
Before we can use the compiler we therefore need to know which
machine we are compiling for.
The next section explains how this decision is made; for the moment
assume we are building 68020 binaries and make the mental substitution for
.CW 2
appropriate to the machine you are actually using.
.PP
To convert source to an executable binary is a two-step process.
First run the compiler,
.CW 2c ,
on the source, say
.CW file.c ,
to generate an object file
.CW file.2 .
Then run the loader,
.CW 2l ,
to generate an executable
.CW 2.out
that may be run (on a 680X0 machine):
.P1
2c file.c
2l file.2
2.out
.P2
The loader automatically links with whatever libraries the program
needs, usually including the standard C library as defined by
.CW <libc.h> .
Of course the compiler and loader have lots of options, both familiar and new;
see the manual for details.
The compiler does not generate an executable automatically;
the output of the compiler must be given to the loader.
Since most compilation is done under the control of
.CW mk
(see below), this is rarely an inconvenience.
.PP
The distribution of work between the compiler and loader is unusual.
The compiler integrates preprocessing, parsing, register allocation,
code generation and some assembly.
Combining these tasks in a single program is part of the reason for
the compiler's efficiency.
The loader does instruction selection, branch folding,
instruction scheduling,
and writes the final executable.
There is no separate C preprocessor and no assembler in the usual pipeline.
Instead the intermediate object file
(here a
.CW \&.2
file) is a type of binary assembly language.
The instructions in the intermediate format are not exactly those in
the machine.  For example, on the 68020 the object file may specify
a MOVE instruction but the loader will decide just which variant of
the MOVE instruction \(em MOVE immediate, MOVE quick, MOVE address,
etc. \(em is most efficient.
.PP
The assembler,
.CW 2a ,
is just a translator between the textual and binary
representations of the object file format.
It is not an assembler in the traditional sense.  It has limited
macro capabilities (the same as the integral C preprocessor in the compiler),
clumsy syntax, and minimal error checking.  For instance, the assembler
will accept an instruction (such as memory-to-memory MOVE on the MIPS) that the
machine does not actually support; only when the output of the assembler
is passed to the loader will the error be discovered.
The assembler is intended only for writing things that need access to instructions
invisible from C,
such as the machine-dependent
part of an operating system;
very little code in Plan 9 is in assembly language.
.PP
The compilers take an option
.CW -S
that causes them to print on their standard output the generated code
in a format acceptable as input to the assemblers.
This is of course merely a formatting of the
data in the object file; therefore the assembler is just
an
ASCII-to-binary converter for this format.
Other than the specific instructions, the input to the assemblers
is largely architecture-independent; see
``A Manual for the Plan 9 Assembler'',
by Rob Pike,
for more information.
.PP
The loader is an integral part of the compilation process.
Each library header file contains a
.CW #pragma
that tells the loader the name of the associated archive; it is
not necessary to tell the loader which libraries a program uses.
The C run-time startup is found, by default, in the C library.
The loader starts with an undefined
symbol,
.CW _main ,
that is resolved by pulling in the run-time startup code from the library.
(The loader undefines
.CW _mainp
when profiling is enabled, to force loading of the profiling start-up
instead.)
.PP
Unlike its counterpart on other systems, the Plan 9 loader rearranges
data to optimize access.  This means the order of variables in the
loaded program is unrelated to its order in the source.
Most programs don't care, but some assume that, for example, the
variables declared by
.P1
int a;
int b;
.P2
will appear at adjacent addresses in memory.  On Plan 9, they won't.
.SH
Heterogeneity
.PP
When the system starts or a user logs in the environment is configured
so the appropriate binaries are available in
.CW /bin .
The configuration process is controlled by an environment variable,
.CW $cputype ,
with value such as
.CW mips ,
.CW 68020 ,
.CW 386 ,
or
.CW sparc .
For each architecture there is a directory in the root,
with the appropriate name,
that holds the binary and library files for that architecture.
Thus
.CW /mips/lib
contains the object code libraries for MIPS programs,
.CW /mips/include
holds MIPS-specific include files, and
.CW /mips/bin
has the MIPS binaries.
These binaries are attached to
.CW /bin
at boot time by binding
.CW /$cputype/bin
to
.CW /bin ,
so
.CW /bin
always contains the correct files.
.PP
The MIPS compiler,
.CW vc ,
by definition
produces object files for the MIPS architecture,
regardless of the architecture of the machine on which the compiler is running.
There is a version of
.CW vc
compiled for each architecture:
.CW /mips/bin/vc ,
.CW /68020/bin/vc ,
.CW /sparc/bin/vc ,
and so on,
each capable of producing MIPS object files regardless of the native
instruction set.
If one is running on a SPARC,
.CW /sparc/bin/vc
will compile programs for the MIPS;
if one is running on machine
.CW $cputype ,
.CW /$cputype/bin/vc
will compile programs for the MIPS.
.PP
Because of the bindings that assemble
.CW /bin ,
the shell always looks for a command, say
.CW date ,
in
.CW /bin
and automatically finds the file
.CW /$cputype/bin/date .
Therefore the MIPS compiler is known as just
.CW vc ;
the shell will invoke
.CW /bin/vc
and that is guaranteed to be the version of the MIPS compiler
appropriate for the machine running the command.
Regardless of the architecture of the compiling machine,
.CW /bin/vc
is
.I always
the MIPS compiler.
.PP
Also, the output of
.CW vc
and
.CW vl
is completely independent of the machine type on which they are executed:
.CW \&.v
files compiled (with
.CW vc )
on a SPARC may be linked (with
.CW vl )
on a 386.
(The resulting
.CW v.out
will run, of course, only on a MIPS.)
Similarly, the MIPS libraries in
.CW /mips/lib
are suitable for loading with
.CW vl
on any machine; there is only one set of MIPS libraries, not one
set for each architecture that supports the MIPS compiler.
.SH
Heterogeneity and \f(CWmk\fP
.PP
Most software on Plan 9 is compiled under the control of
.CW mk ,
a descendant of
.CW make
that is documented in the Programmer's Manual.
A convention used throughout the
.CW mkfiles
makes it easy to compile the source into binary suitable for any architecture.
.PP
The variable
.CW $cputype
is advisory: it reports the architecture of the current environment, and should
not be modified.  A second variable,
.CW $objtype ,
is used to set which architecture is being
.I compiled
for.
The value of
.CW $objtype
can be used by a
.CW mkfile
to configure the compilation environment.
.PP
In each machine's root directory there is a short
.CW mkfile
that defines a set of macros for the compiler, loader, etc.
Here is
.CW /mips/mkfile :
.P1
</sys/src/mkfile.proto

CC=vc
LD=vl
O=v
AS=va
.P2
The line
.P1
</sys/src/mkfile.proto
.P2
causes
.CW mk
to include the file
.CW /sys/src/mkfile.proto ,
which contains general definitions:
.P1
#
# common mkfile parameters shared by all architectures
#

OS=v486xq7
CPUS=mips 386 power alpha
CFLAGS=-FVw
LEX=lex
YACC=yacc
MK=/bin/mk
.P2
.CW CC
is obviously the compiler,
.CW AS
the assembler, and
.CW LD
the loader.
.CW O
is the suffix for the object files and
.CW CPUS
and
.CW OS
are used in special rules described below.
.PP
Here is a
.CW mkfile
to build the installed source for
.CW sam :
.P1
</$objtype/mkfile
OBJ=sam.$O address.$O buffer.$O cmd.$O disc.$O error.$O \e
	file.$O io.$O list.$O mesg.$O moveto.$O multi.$O \e
	plan9.$O rasp.$O regexp.$O string.$O sys.$O xec.$O

$O.out:	$OBJ
	$LD $OBJ

install:	$O.out
	cp $O.out /$objtype/bin/sam

installall:
	for(objtype in $CPUS) mk install

%.$O:	%.c
	$CC $CFLAGS $stem.c

$OBJ:	sam.h errors.h mesg.h
address.$O cmd.$O parse.$O xec.$O unix.$O:	parse.h

clean:V:
	rm -f [$OS].out *.[$OS] y.tab.?
.P2
(The actual
.CW mkfile
imports most of its rules from other secondary files, but
this example works and is not misleading.)
The first line causes
.CW mk
to include the contents of
.CW /$objtype/mkfile
in the current
.CW mkfile .
If
.CW $objtype
is
.CW mips ,
this inserts the MIPS macro definitions into the
.CW mkfile .
In this case the rule for
.CW $O.out
uses the MIPS tools to build
.CW v.out .
The
.CW %.$O
rule in the file uses
.CW mk 's
pattern matching facilities to convert the source files to the object
files through the compiler.
(The text of the rules is passed directly to the shell,
.CW rc ,
without further translation.
See the
.CW mk
manual if any of this is unfamiliar.)
Because the default rule builds
.CW $O.out
rather than
.CW sam ,
it is possible to maintain binaries for multiple machines in the
same source directory without conflict.
This is also, of course, why the output files from the various
compilers and loaders
have distinct names.
.PP
The rest of the
.CW mkfile
should be easy to follow; notice how the rules for
.CW clean
and
.CW installall
(that is, install versions for all architectures) use other macros
defined in
.CW /$objtype/mkfile .
In Plan 9,
.CW mkfiles
for commands conventionally contain rules to
.CW install
(compile and install the version for
.CW $objtype ),
.CW installall
(compile and install for all
.CW $objtypes ),
and
.CW clean
(remove all object files, binaries, etc.).
.PP
The
.CW mkfile
is easy to use.  To build a MIPS binary,
.CW v.out :
.P1
% objtype=mips
% mk
.P2
To build and install a MIPS binary:
.P1
% objtype=mips
% mk install
.P2
To build and install all versions:
.P1
% mk installall
.P2
These conventions make cross-compilation as easy to manage
as traditional native compilation.
Plan 9 programs compile and run without change on machines from
large multiprocessors to laptops.  For more information about this process, see
``Plan 9 Mkfiles'',
by Bob Flandrena.
.SH
Portability
.PP
Within Plan 9, it is painless to write portable programs, programs whose
source is independent of the machine on which they execute.
The operating system is fixed and the compiler, headers and libraries
are constant so most of the stumbling blocks to portability are removed.
Attention to a few details can avoid those that remain.
.PP
Plan 9 is a heterogeneous environment, so programs must
.I expect
that external files will be written by programs on machines of different
architectures.
The compilers, for instance, must handle without confusion
object files written by other machines.
The traditional approach to this problem is to pepper the source with
.CW #ifdefs
to turn byte-swapping on and off.
Plan 9 takes a different approach: of the handful of machine-dependent
.CW #ifdefs
in all the source, almost all are deep in the libraries.
Instead programs read and write files in a defined format,
either (for low volume applications) as formatted text, or
(for high volume applications) as binary in a known byte order.
If the external data were written with the most significant
byte first, the following code reads a 4-byte integer correctly
regardless of the architecture of the executing machine (assuming
an unsigned long holds 4 bytes):
.P1
ulong
getlong(void)
{
	ulong l;

	l = (getchar()&0xFF)<<24;
	l |= (getchar()&0xFF)<<16;
	l |= (getchar()&0xFF)<<8;
	l |= (getchar()&0xFF)<<0;
	return l;
}
.P2
Note that this code does not `swap' the bytes; instead it just reads
them in the correct order.
Variations of this code will handle any binary format
and also avoid problems
involving how structures are padded, how words are aligned,
and other impediments to portability.
Be aware, though, that extra care is needed to handle floating point data.
.PP
Efficiency hounds will argue that this method is unnecessarily slow and clumsy
when the executing machine has the same byte order (and padding and alignment)
as the data.
The CPU cost of I/O processing
is rarely the bottleneck for an application, however,
and the gain in simplicity of porting and maintaining the code greatly outweighs
the minor speed loss from handling data in this general way.
This method is how the Plan 9 compilers, the window system, and even the file
servers transmit data between programs.
.PP
To port programs beyond Plan 9, where the system interface is more variable,
it is probably necessary to use
.CW pcc
and hope that the target machine supports ANSI C and POSIX.
.SH
I/O
.PP
The default C library, defined by the include file
.CW <libc.h> ,
contains no buffered I/O package.
It does have several entry points for printing formatted text:
.CW print
outputs text to the standard output,
.CW fprint
outputs text to a specified integer file descriptor, and
.CW sprint
places text in a character array.
To access library routines for buffered I/O, a program must
explicitly include the header file associated with an appropriate library.
.PP
The recommended I/O library, used by most Plan 9 utilities, is
.CW bio
(buffered I/O), defined by
.CW <bio.h> .
There also exists an implementation of ANSI Standard I/O,
.CW stdio .
.PP
.CW Bio
is small and efficient, particularly for buffer-at-a-time or
line-at-a-time I/O.
Even for character-at-a-time I/O, however, it is significantly faster than
the Standard I/O library,
.CW stdio .
Its interface is compact and regular, although it lacks a few conveniences.
The most noticeable is that one must explicitly define buffers for standard
input and output;
.CW bio
does not predefine them.  Here is a program to copy input to output a byte
at a time using
.CW bio :
.P1
#include <u.h>
#include <libc.h>
#include <bio.h>

Biobuf	bin;
Biobuf	bout;

main(void)
{
	int c;

	Binit(&bin, 0, OREAD);
	Binit(&bout, 1, OWRITE);

	while((c=Bgetc(&bin)) != Beof)
		Bputc(&bout, c);
	exits(0);
}
.P2
For peak performance, we could replace
.CW Bgetc
and
.CW Bputc
by their equivalent in-line macros
.CW BGETC
and
.CW BPUTC
but 
the performance gain would be modest.
For more information on
.CW bio ,
see the Programmer's Manual.
.PP
Perhaps the most dramatic difference in the I/O interface of Plan 9 from other
systems' is that text is not ASCII.
The format for
text in Plan 9 is a byte-stream encoding of 16-bit characters.
The character set is based on the Unicode Standard and is backward compatible with
ASCII:
characters with value 0 through 127 are the same in both sets.
The 16-bit characters, called
.I runes
in Plan 9, are encoded using a representation called
UTF,
an encoding that is becoming accepted as a standard.
(ISO calls it UTF-8;
throughout Plan 9 it's just called
UTF.)
UTF
defines multibyte sequences to
represent character values from 0 to 65535.
In
UTF,
character values up to 127 decimal, 7F hexadecimal, represent themselves,
so straight
ASCII
files are also valid
UTF.
Also,
UTF
guarantees that bytes with values 0 to 127 (NUL to DEL, inclusive)
will appear only when they represent themselves, so programs that read bytes
looking for plain ASCII characters will continue to work.
Any program that expects a one-to-one correspondence between bytes and
characters will, however, need to be modified.
An example is parsing file names.
File names, like all text, are in
UTF,
so it is incorrect to search for a character in a string by
.CW strchr(filename,
.CW c)
because the character might have a multi-byte encoding.
The correct method is to call
.CW utfrune(filename,
.CW c) ,
defined in
.I rune (2),
which interprets the file name as a sequence of encoded characters
rather than bytes.
In fact, even when you know the character is a single byte
that can represent only itself,
it is safer to use
.CW utfrune
because that assumes nothing about the character set
and its representation.
.PP
The library defines several symbols relevant to the representation of characters.
Any byte with unsigned value less than
.CW Runesync
will not appear in any multi-byte encoding of a character.
.CW Utfrune
compares the character being searched against
.CW Runesync
to see if it is sufficient to call
.CW strchr
or if the byte stream must be interpreted.
Any byte with unsigned value less than
.CW Runeself
is represented by a single byte with the same value.
Finally, when errors are encountered converting
to runes from a byte stream, the library returns the rune value
.CW Runeerror
and advances a single byte.  This permits programs to find runes
embedded in binary data.
.PP
.CW Bio
includes routines
.CW Bgetrune
and
.CW Bputrune
to transform the external byte stream
UTF
format to and from
internal 16-bit runes.
Also, the
.CW %s
format to
.CW print
accepts
UTF;
.CW %c
prints a character after narrowing it to 8 bits.
The
.CW %S
format prints a null-terminated sequence of runes;
.CW %C
prints a character after narrowing it to 16 bits.
For more information, see the Programmer's Manual, in particular
.I utf (6)
and
.I rune (2),
and the paper,
``Hello world, or
Καλημέρα κόσμε, or\ 
\f(Jpこんにちは 世界\f1'',
by Rob Pike and
Ken Thompson;
there is not room for the full story here.
.PP
These issues affect the compiler in several ways.
First, the C source is in
UTF.
ANSI says C variables are formed from
ASCII
alphanumerics, but comments and literal strings may contain any characters
encoded in the native encoding, here
UTF.
The declaration
.P1
char *cp = "abcÿ";
.P2
initializes the variable
.CW cp
to point to an array of bytes holding the
UTF
representation of the characters
.CW abcÿ.
The type
.CW Rune
is defined in
.CW <u.h>
to be
.CW ushort ,
which is also the  `wide character' type in the compiler.
Therefore the declaration
.P1
Rune *rp = L"abcÿ";
.P2
initializes the variable
.CW rp
to point to an array of unsigned short integers holding the 16-bit
values of the characters
.CW abcÿ .
Note that in both these declarations the characters in the source
that represent
.CW "abcÿ"
are the same; what changes is how those characters are represented
in memory in the program.
The following two lines:
.P1
print("%s\en", "abcÿ");
print("%S\en", L"abcÿ");
.P2
produce the same
UTF
string on their output, the first by copying the bytes, the second
by converting from runes to bytes.
.PP
In C, character constants are integers but narrowed through the
.CW char
type.
The Unicode character
.CW ÿ
has value 255, so if the
.CW char
type is signed,
the constant
.CW 'ÿ'
has value \-1 (which is equal to EOF).
On the other hand,
.CW L'ÿ'
narrows through the wide character type,
.CW ushort ,
and therefore has value 255.
.PP
Finally, although it's not ANSI C, the Plan 9 C compilers
assume any character with value above
.CW Runeself
is an alphanumeric,
so α is a legal, if non-portable, variable name.
.SH
Arguments
.PP
Some macros are defined
in
.CW <libc.h>
for parsing the arguments to
.CW main() .
They are described in
.I ARG (2)
but are fairly self-explanatory.
There are four macros:
.CW ARGBEGIN
and
.CW ARGEND
are used to bracket a hidden
.CW switch
statement within which
.CW ARGC
returns the current option character (rune) being processed and
.CW ARGF
returns the argument to the option, as in the loader option
.CW -o
.CW file .
Here, for example, is the code at the beginning of
.CW main()
in
.CW ramfs.c
(see
.I ramfs (1))
that cracks its arguments:
.P1
void
main(int argc, char *argv[])
{
	char *defmnt;
	int p[2];
	int mfd[2];
	int stdio = 0;

	defmnt = "/tmp";
	ARGBEGIN{
	case 'i':
		defmnt = 0;
		stdio = 1;
		mfd[0] = 0;
		mfd[1] = 1;
		break;
	case 's':
		defmnt = 0;
		break;
	case 'm':
		defmnt = ARGF();
		break;
	default:
		usage();
	}ARGEND
.P2
.SH
Extensions
.PP
The compiler has several extensions to 1989 ANSI C, all of which are used
extensively in the system source.
Some of these have been adopted in later ANSI C standards.
First,
.I structure
.I displays
permit 
.CW struct
expressions to be formed dynamically.
Given these declarations:
.P1
typedef struct Point Point;
typedef struct Rectangle Rectangle;

struct Point
{
	int x, y;
};

struct Rectangle
{
	Point min, max;
};

Point	p, q, add(Point, Point);
Rectangle r;
int	x, y;
.P2
this assignment may appear anywhere an assignment is legal:
.P1
r = (Rectangle){add(p, q), (Point){x, y+3}};
.P2
The syntax is the same as for initializing a structure but with
a leading cast.
.PP
If an
.I anonymous
.I structure
or
.I union
is declared within another structure or union, the members of the internal
structure or union are addressable without prefix in the outer structure.
This feature eliminates the clumsy naming of nested structures and,
particularly, unions.
For example, after these declarations,
.P1
struct Lock
{
	int	locked;
};

struct Node
{
	int	type;
	union{
		double  dval;
		double  fval;
		long    lval;
	};		/* anonymous union */
	struct Lock;	/* anonymous structure */
} *node;

void	lock(struct Lock*);
.P2
one may refer to
.CW node->type ,
.CW node->dval ,
.CW node->fval ,
.CW node->lval ,
and
.CW node->locked .
Moreover, the address of a
.CW struct
.CW Node
may be used without a cast anywhere that the address of a
.CW struct
.CW Lock
is used, such as in argument lists.
The compiler automatically promotes the type and adjusts the address.
Thus one may invoke
.CW lock(node) .
.PP
Anonymous structures and unions may be accessed by type name
if (and only if) they are declared using a
.CW typedef
name.
For example, using the above declaration for
.CW Point ,
one may declare
.P1
struct
{
	int	type;
	Point;
} p;
.P2
and refer to
.CW p.Point .
.PP
In the initialization of arrays, a number in square brackets before an
element sets the index for the initialization.  For example, to initialize
some elements in
a table of function pointers indexed by
ASCII
character,
.P1
void	percent(void), slash(void);

void	(*func[128])(void) =
{
	['%']	percent,
	['/']	slash,
};
.P2
.LP
A similar syntax allows one to initialize structure elements:
.P1
Point p =
{
	.y 100,
	.x 200
};
.P2
These initialization syntaxes were later added to ANSI C, with the addition of an
equals sign between the index or tag and the value.
The Plan 9 compiler accepts either form.
.PP
Finally, the declaration
.P1
extern register reg;
.P2
.I this "" (
appearance of the register keyword is not ignored)
allocates a global register to hold the variable
.CW reg .
External registers must be used carefully: they need to be declared in
.I all
source files and libraries in the program to guarantee the register
is not allocated temporarily for other purposes.
Especially on machines with few registers, such as the i386,
it is easy to link accidentally with code that has already usurped
the global registers and there is no diagnostic when this happens.
Used wisely, though, external registers are powerful.
The Plan 9 operating system uses them to access per-process and
per-machine data structures on a multiprocessor.  The storage class they provide
is hard to create in other ways.
.SH
The compile-time environment
.PP
The code generated by the compilers is `optimized' by default:
variables are placed in registers and peephole optimizations are
performed.
The compiler flag
.CW -N
disables these optimizations.
Registerization is done locally rather than throughout a function:
whether a variable occupies a register or
the memory location identified in the symbol
table depends on the activity of the variable and may change
throughout the life of the variable.
The
.CW -N
flag is rarely needed;
its main use is to simplify debugging.
There is no information in the symbol table to identify the
registerization of a variable, so
.CW -N
guarantees the variable is always where the symbol table says it is.
.PP
Another flag,
.CW -w ,
turns
.I on
warnings about portability and problems detected in flow analysis.
Most code in Plan 9 is compiled with warnings enabled;
these warnings plus the type checking offered by function prototypes
provide most of the support of the Unix tool
.CW lint
more accurately and with less chatter.
Two of the warnings,
`used and not set' and `set and not used', are almost always accurate but
may be triggered spuriously by code with invisible control flow,
such as in routines that call
.CW longjmp .
The compiler statements
.P1
SET(v1);
USED(v2);
.P2
decorate the flow graph to silence the compiler.
Either statement accepts a comma-separated list of variables.
Use them carefully: they may silence real errors.
For the common case of unused parameters to a function,
leaving the name off the declaration silences the warnings.
That is, listing the type of a parameter but giving it no
associated variable name does the trick.
.SH
Debugging
.PP
There are two debuggers available on Plan 9.
The first, and older, is
.CW db ,
a revision of Unix
.CW adb .
The other,
.CW acid ,
is a source-level debugger whose commands are statements in
a true programming language.
.CW Acid
is the preferred debugger, but since it
borrows some elements of
.CW db ,
notably the formats for displaying values, it is worth knowing a little bit about
.CW db .
.PP
Both debuggers support multiple architectures in a single program; that is,
the programs are
.CW db
and
.CW acid ,
not for example
.CW vdb
and
.CW vacid .
They also support cross-architecture debugging comfortably:
one may debug a 68020 binary on a MIPS.
.PP
Imagine a program has crashed mysteriously:
.P1
% X11/X
Fatal server bug!
failed to create default stipple
X 106: suicide: sys: trap: fault read addr=0x0 pc=0x00105fb8
% 
.P2
When a process dies on Plan 9 it hangs in the `broken' state
for debugging.
Attach a debugger to the process by naming its process id:
.P1
% acid 106
/proc/106/text:mips plan 9 executable

/sys/lib/acid/port
/sys/lib/acid/mips
acid: 
.P2
The
.CW acid
function
.CW stk()
reports the stack traceback:
.P1
acid: stk()
At pc:0x105fb8:abort+0x24 /sys/src/ape/lib/ap/stdio/abort.c:6
abort() /sys/src/ape/lib/ap/stdio/abort.c:4
	called from FatalError+#4e
		/sys/src/X/mit/server/dix/misc.c:421
FatalError(s9=#e02, s8=#4901d200, s7=#2, s6=#72701, s5=#1,
    s4=#7270d, s3=#6, s2=#12, s1=#ff37f1c, s0=#6, f=#7270f)
    /sys/src/X/mit/server/dix/misc.c:416
	called from gnotscreeninit+#4ce
		/sys/src/X/mit/server/ddx/gnot/gnot.c:792
gnotscreeninit(snum=#0, sc=#80db0)
    /sys/src/X/mit/server/ddx/gnot/gnot.c:766
	called from AddScreen+#16e
		/n/bootes/sys/src/X/mit/server/dix/main.c:610
AddScreen(pfnInit=0x0000129c,argc=0x00000001,argv=0x7fffffe4)
    /sys/src/X/mit/server/dix/main.c:530
	called from InitOutput+0x80
		/sys/src/X/mit/server/ddx/brazil/brddx.c:522
InitOutput(argc=0x00000001,argv=0x7fffffe4)
    /sys/src/X/mit/server/ddx/brazil/brddx.c:511
	called from main+0x294
		/sys/src/X/mit/server/dix/main.c:225
main(argc=0x00000001,argv=0x7fffffe4)
    /sys/src/X/mit/server/dix/main.c:136
	called from _main+0x24
		/sys/src/ape/lib/ap/mips/main9.s:8
.P2
The function
.CW lstk()
is similar but
also reports the values of local variables.
Note that the traceback includes full file names; this is a boon to debugging,
although it makes the output much noisier.
.PP
To use
.CW acid
well you will need to learn its input language; see the
``Acid Manual'',
by Phil Winterbottom,
for details.  For simple debugging, however, the information in the manual page is
sufficient.  In particular, it describes the most useful functions
for examining a process.
.PP
The compiler does not place
information describing the types of variables in the executable,
but a compile-time flag provides crude support for symbolic debugging.
The
.CW -a
flag to the compiler suppresses code generation
and instead emits source text in the
.CW acid
language to format and display data structure types defined in the program.
The easiest way to use this feature is to put a rule in the
.CW mkfile :
.P1
syms:   main.$O
        $CC -a main.c > syms
.P2
Then from within
.CW acid ,
.P1
acid: include("sourcedirectory/syms")
.P2
to read in the relevant definitions.
(For multi-file source, you need to be a little fancier;
see
.I 2c (1)).
This text includes, for each defined compound
type, a function with that name that may be called with the address of a structure
of that type to display its contents.
For example, if
.CW rect
is a global variable of type
.CW Rectangle ,
one may execute
.P1
Rectangle(*rect)
.P2
to display it.
The
.CW *
(indirection) operator is necessary because
of the way
.CW acid
works: each global symbol in the program is defined as a variable by
.CW acid ,
with value equal to the
.I address
of the symbol.
.PP
Another common technique is to write by hand special
.CW acid
code to define functions to aid debugging, initialize the debugger, and so on.
Conventionally, this is placed in a file called
.CW acid
in the source directory; it has a line
.P1
include("sourcedirectory/syms");
.P2
to load the compiler-produced symbols.  One may edit the compiler output directly but
it is wiser to keep the hand-generated
.CW acid
separate from the machine-generated.
.PP
To make things simple, the default rules in the system
.CW mkfiles
include entries to make
.CW foo.acid
from
.CW foo.c ,
so one may use
.CW mk
to automate the production of
.CW acid
definitions for a given C source file.
.PP
There is much more to say here.  See
.CW acid
manual page, the reference manual, or the paper
``Acid: A Debugger Built From A Language'',
also by Phil Winterbottom.
