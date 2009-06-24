.HTML "Adding Application Support for a New Architecture in Plan 9
.TL
Adding Application Support for a New Architecture in Plan 9
.AU
Bob Flandrena
bobf@plan9.bell-labs.com
.SH
Introduction
.LP
Plan 9 has five classes of architecture-dependent software:
headers, kernels, compilers and loaders, the
.CW libc
system library, and a few application programs.  In general,
architecture-dependent programs
consist of a portable part shared by all architectures and a
processor-specific portion for each supported architecture.
The portable code is often compiled and stored in a library
associated with
each architecture.  A program is built by
compiling the architecture-specific code and loading it with the
library.  Support for a new architecture is provided
by building a compiler for the architecture, using it to
compile the portable code into libraries,
writing the architecture-specific code, and
then loading that code with
the libraries.
.LP
This document describes the organization of the architecture-dependent
code and headers on Plan 9.
The first section briefly discusses the layout of
the headers and the source code for the kernels, compilers, loaders, and the
system library, 
.CW libc .
The second section provides a detailed
discussion of the structure of
.CW libmach ,
a library containing almost
all architecture-dependent code
used by application programs.
The final section describes the steps required to add
application program support for a new architecture.
.SH
Directory Structure
.PP
Architecture-dependent information for the new processor
is stored in the directory tree rooted at \f(CW/\fP\fIm\fP
where
.I m
is the name of the new architecture (e.g.,
.CW mips ).
The new directory should be initialized with several important
subdirectories, notably
.CW bin ,
.CW include ,
and
.CW lib .
The directory tree of an existing architecture
serves as a good model for the new tree.
The architecture-dependent
.CW mkfile
must be stored in the newly created root directory
for the architecture.  It is easiest to copy the
mkfile for an existing architecture and modify
it for the new architecture.  When the mkfile
is correct, change the
.CW OS
and
.CW CPUS
variables in the
.CW /sys/src/mkfile.proto
to reflect the addition of the new architecture.
.SH
Headers
.LP
Architecture-dependent headers are stored in directory
.CW /\fIm\fP/include
where
.I m
is the name of the architecture (e.g.,
.CW mips ).
Two header files are required:
.CW u.h 
and
.CW ureg.h .
The first defines fundamental data types,
bit settings for the floating point
status and control registers, and
.CW va_list
processing which depends on the stack
model for the architecture.  This file
is best built by copying and modifying the
.CW u.h
file from an architecture
with a similar stack model.
The
.CW ureg.h
file
contains a structure describing the layout
of the saved register set for
the architecture; it is defined by the kernel.
.LP
Header file
.CW /sys/include/a.out.h
contains the definitions of the magic
numbers used to identify executables for
each architecture.  When support for a new
architecture is added, the magic number
for the architecture must be added to this file.
.LP
The header format of a bootable executable is defined by
each manufacturer.  Header file
.CW /sys/include/bootexec.h
contains structures describing the headers currently
supported.  If the new architecture uses a common header
such as COFF,
the header format is probably already defined,
but if the bootable header format is non-standard,
a structure defining the format must be added to this file.
.LP
.SH
Kernel
.LP
Although the kernel depends critically on the properties of the underlying
hardware, most of the
higher-level kernel functions, including process
management, paging, pseudo-devices, and some
networking code, are independent of processor
architecture.  The portable kernel code
is divided into two parts: that implementing kernel
functions and that devoted to the boot process.
Code in the first class is stored in directory
.CW /sys/src/9/port
and the portable boot code is stored in
.CW /sys/src/9/boot .
Architecture-dependent kernel code is stored in the
subdirectories of
.CW /sys/src/9
named for each architecture.
.LP
The relationship between the kernel code and the boot code
is convoluted and subtle.  The portable boot code
is compiled into a library for each architecture.  An architecture-specific
main program is loaded with the appropriate library and the resulting
executable is compiled into the kernel where it is executed as
a user process during the final stages of kernel initialization.  The boot process
performs authentication, attaches the name space root to the appropriate
file system and starts the
.CW init
process.
.LP
The organization of the portable kernel source code differs from that
of most other architecture-specific code.
Instead of storing the portable code in a library
and loading it with the architecture-specific
code, the portable code is compiled directly into
the directory containing the architecture-specific code
and linked with the object files built from the source in that directory.
.LP
.SH
Compilers and Loaders
.LP
The compiler source code conforms to the usual
organization: portable code is compiled into a library
for each architecture
and the architecture-dependent code is loaded with
that library.
The common compiler code is stored in
.CW /sys/src/cmd/cc .
The
.CW mkfile
in this directory compiles the portable source and
archives the objects in a library for each architecture.
The architecture-specific compiler source
is stored in a subdirectory of
.CW /sys/src/cmd
with the same name as the compiler (e.g.,
.CW /sys/src/cmd/vc ).
.LP
There is no portable code shared by the loaders.
Each directory of loader source
code is self-contained, except for
a header file and an instruction name table
included from the
directory of the associated
compiler.
.LP
.SH
Libraries
.LP
Most C library modules are
portable; the source code is stored in
directories
.CW /sys/src/libc/port
and
.CW /sys/src/libc/9sys .
Architecture-dependent library code
is stored in the subdirectory of
.CW /sys/src/libc
named the same as the target processor.
Non-portable functions not only
implement architecture-dependent operations
but also supply assembly language implementations
of functions where speed is critical.
Directory
.CW /sys/src/libc/9syscall
is unusual because it
contains architecture-dependent information
for all architectures.
It holds only a header file defining
the names and numbers of system calls
and a
.CW mkfile .
The
.CW mkfile
executes an
.CW rc
script that parses the header file, constructs
assembler language functions implementing the system
call for each architecture, assembles the code,
and archives the object files in
.CW libc .
The assembler language syntax and the system interface
differ for each architecture.
The
.CW rc
script in this
.CW mkfile
must be modified to support a new architecture.
.LP
.SH
Applications
.LP
Application programs process two forms of architecture-dependent
information: executable images and intermediate object files.
Almost all processing is on executable files.
System library
.CW libmach
provides functions that convert
architecture-specific data
to a portable format so application programs
can process this data independent of its
underlying representation.
Further, when a new architecture is implemented
almost all code changes
are confined to the library;
most affected application programs need only be reloaded.
The source code for the library is stored in
.CW /sys/src/libmach .
.LP
An application program running on one type of
processor must be able to interpret
architecture-dependent information for all
supported processors.
For example, a debugger must be able to debug
the executables of
all architectures, not just the
architecture on which it is executing, since
.CW /proc
may be imported from a different machine.
.LP
A small part of the application library
provides functions to
extract symbol references from object files.
The remainder provides the following processing
of executable files or memory images:
.IP \(bu
Header interpretation.
.IP \(bu
Symbol table interpretation.
.IP \(bu
Execution context interpretation, such as stack traces
and stack frame location.
.IP \(bu
Instruction interpretation including disassembly and
instruction size and follow-set calculations.
.IP \(bu
Exception and floating point number interpretation.
.IP \(bu
Architecture-independent read and write access through a
relocation map.
.LP
Header file
.CW /sys/include/mach.h
defines the interfaces to the
application library.  Manual pages
.I mach (2),
.I symbol (2),
and
.I object (2)
describe the details of the
library functions.
.LP
Two data structures, called
.CW Mach
and
.CW Machdata ,
contain architecture-dependent  parameters and
a jump table of functions.
Global variables
.CW mach
and
.CW machdata
point to the
.CW Mach
and
.CW Machdata
data structures associated with the target architecture.
An application determines the target architecture of
a file or executable image, sets the global pointers
to the data structures associated with that architecture,
and subsequently performs all references indirectly through the
pointers.
As a result, direct references to the tables for each
architecture are avoided and the application code intrinsically
supports all architectures (though only one at a time).
.LP
Object file processing is handled similarly: architecture-dependent
functions identify and
decode the intermediate files for the processor.
The application indirectly
invokes a classification function to identify
the architecture of the object code and to select the
appropriate decoding function.  Subsequent calls
then use that function to decode each record.  Again,
the layer of indirection allows the application code
to support all architectures without modification.
.LP
Splitting the architecture-dependent information
between the
.CW Mach
and
.CW Machdata
data structures
allows applications to choose
an appropriate level of service.  Even though an application
does not directly reference the architecture-specific data structures,
it must load the
architecture-dependent tables and code 
for all architectures it supports.  The size of this data
can be substantial and many applications do not require
the full range of architecture-dependent functionality.
For example, the
.CW size
command does not require the disassemblers for every architecture;
it only needs to decode the header.
The
.CW Mach
data structure contains a few architecture-specific parameters
and a description of the processor register set.
The size of the structure
varies with the size of the register
set but is generally small.
The
.CW Machdata
data structure contains
a jump table of architecture-dependent functions;
the amount of code and data referenced by this table
is usually large.
.SH
Libmach Source Code Organization
.LP
The
.CW libmach
library provides four classes of functionality:
.LP
.IP "Header and Symbol Table Decoding\ -\ "
Files
.CW executable.c
and
.CW sym.c
contain code to interpret the header and
symbol tables of
an executable file or executing image.
Function
.CW crackhdr
decodes the header,
reformats the
information into an
.CW Fhdr
data structure, and points
global variable
.CW mach
to the
.CW Mach
data structure of the target architecture.
The symbol table processing
uses the data in the
.CW Fhdr
structure to decode the symbol table.
A variety of symbol table access functions then support
queries on the reformatted table.
.IP "Debugger Support\ -\ "
Files named
.CW \fIm\fP.c ,
where
.I m
is the code letter assigned to the architecture,
contain the initialized
.CW Mach
data structure and the definition of the register
set for each architecture.
Architecture-specific debugger support functions and
an initialized
.CW Machdata
structure are stored in
files named
.CW \fIm\fPdb.c .
Files
.CW machdata.c 
and
.CW setmach.c
contain debugger support functions shared
by multiple architectures.
.IP "Architecture-Independent Access\ -\ "
Files
.CW map.c ,
.CW access.c ,
and
.CW swap.c
provide accesses through a relocation map
to data in an executable file or executing image.
Byte-swapping is performed as needed.  Global variables
.CW mach
and
.CW machdata
must point to the
.CW Mach
and
.CW Machdata
data structures of the target architecture.
.IP "Object File Interpretation\ -\ "
These files contain functions to identify the
target architecture of an
intermediate object file
and extract references to symbols.  File
.CW obj.c
contains code common to all architectures;
file
.CW \fIm\fPobj.c
contains the architecture-specific source code
for the machine with code character
.I m .
.LP
The
.CW Machdata
data structure is primarily a jump
table of architecture-dependent debugger support
functions. Functions select the
.CW Machdata
structure for a target architecture based
on the value of the
.CW type
code in the
.CW Fhdr
structure or the name of the architecture.
The jump table provides functions to swap bytes, interpret
machine instructions,
perform stack
traces, find stack frames, format floating point
numbers, and decode machine exceptions.  Some functions, such as
machine exception decoding, are idiosyncratic and must be
supplied for each architecture.  Others depend
on the compiler run-time model and several
architectures may share code common to a model.  For
example, many architectures share the code to
process the fixed-frame stack model implemented by
several of the compilers.
Finally, some
functions, such as byte-swapping, provide a general capability and
the jump table need only select an implementation appropriate
to the architecture.
.LP
.SH
Adding Application Support for a New Architecture
.LP
This section describes the
steps required to add application-level
support for a new architecture.
We assume
the kernel, compilers, loaders and system libraries
for the new architecture are already in place.  This
implies that a code-character has been assigned and
that the architecture-specific headers have been
updated.
With the exception of two programs,
application-level changes are confined to header
files and the source code in
.CW /sys/src/libmach .
.LP
.IP 1.
Begin by updating the application library
header file in
.CW /sys/include/mach.h .
Add the following symbolic codes to the
.CW enum
statement near the beginning of the file:
.RS
.IP \(bu
The processor type code, e.g., 
.CW MSPARC .
.IP \(bu
The type of the executable.  There are usually
two codes needed: one for a bootable
executable (i.e., a kernel) and one for an
application executable.
.IP \(bu
The disassembler type code.  Add one entry for
each supported disassembler for the architecture.
.IP \(bu
A symbolic code for the object file.
.RE
.LP
.IP 2.
In a file name
.CW /sys/src/libmach/\fIm\fP.c
(where
.I m
is the identifier character assigned to the architecture),
initialize
.CW Reglist
and
.CW Mach
data structures with values defining
the register set and various system parameters.
The source file for a similar architecture
can serve as template.
Most of the fields of the
.CW Mach
data structure are obvious
but a few require further explanation.
.RS
.IP "\f(CWkbase\fP\ -\ "
This field
contains the address of the kernel 
.CW ublock .
The debuggers
assume the first entry of the kernel
.CW ublock
points to the
.CW Proc
structure for a kernel thread.
.IP "\f(CWktmask\fP\ -\ "
This field
is a bit mask used to calculate the kernel text address from
the kernel 
.CW ublock
address.
The first page of the
kernel text segment is calculated by
ANDing
the negation of this mask with
.CW kbase .
.IP "\f(CWkspoff\fP\ -\ "
This field
contains the byte offset in the
.CW Proc
data structure to the saved kernel
stack pointer for a suspended kernel thread.  This
is the offset to the 
.CW sched.sp
field of a
.CW Proc
table entry.
.IP "\f(CWkpcoff\fP\ -\ "
This field contains the byte offset into the
.CW Proc
data structure
of
the program counter of a suspended kernel thread.
This is the offset to
field
.CW sched.pc
in that structure.
.IP "\f(CWkspdelta\fP and \f(CWkpcdelta\fP\ -\ "
These fields
contain corrections to be added to
the stack pointer and program counter, respectively,
to properly locate the stack and next
instruction of a kernel thread.  These
values bias the saved registers retrieved
from the
.CW Label
structure named
.CW sched
in the
.CW Proc
data structure.
Most architectures require no bias
and these fields contain zeros.
.IP "\f(CWscalloff\fP\ -\ "
This field
contains the byte offset of the
.CW scallnr
field in the
.CW ublock
data structure associated with a process.
The
.CW scallnr
field contains the number of the
last system call executed by the process.
The location of the field varies depending on
the size of the floating point register set
which precedes it in the
.CW ublock .
.RE
.LP
.IP 3.
Add an entry to the initialization of the
.CW ExecTable
data structure at the beginning of file
.CW /sys/src/libmach/executable.c .
Most architectures
require two entries: one for
a normal executable and
one for a bootable
image.  Each table entry contains:
.RS
.IP \(bu
Magic Number\ \-\ 
The big-endian magic number assigned to the architecture in
.CW /sys/include/a.out.h .
.IP \(bu
Name\ \-\ 
A string describing the executable.
.IP \(bu
Executable type code\ \-\ 
The executable code assigned in
.CW /sys/include/mach.h .
.IP \(bu
\f(CWMach\fP pointer\ \-\ 
The address of the initialized
.CW Mach
data structure constructed in Step 2.
You must also add the name of this table to the
list of
.CW Mach
table definitions immediately preceding the
.CW ExecTable
initialization.
.IP \(bu
Header size\ \-\ 
The number of bytes in the executable file header.
The size of a normal executable header is always
.CW sizeof(Exec) .
The size of a bootable header is
determined by the size of the structure
for the architecture defined in
.CW /sys/include/bootexec.h .
.IP \(bu
Byte-swapping function\ \-\ 
The address of
.CW beswal
or
.CW leswal
for big-endian and little-endian
architectures, respectively.
.IP \(bu
Decoder function\ -\ 
The address of a function to decode the header.
Function
.CW adotout
decodes the common header shared by all normal
(i.e., non-bootable) executable files.
The header format of bootable
executable files is defined by the manufacturer and
a custom function is almost always
required to decode it.
Header file
.CW /sys/include/bootexec.h
contains data structures defining the bootable
headers for all architectures.  If the new architecture
uses an existing format, the appropriate
decoding function should already be in
.CW executable.c .
If the header format is unique, then
a new function must be added to this file.
Usually the decoding function for an existing
architecture can be adopted with minor modifications.
.RE
.LP
.IP 4.
Write an object file parser and
store it in file
.CW /sys/src/libmach/\fIm\fPobj.c
where
.I m
is the identifier character assigned to the architecture.
Two functions are required: a predicate to identify an
object file for the architecture and a function to extract
symbol references from the object code.
The object code format is obscure but
it is often possible to adopt the
code of an existing architecture
with minor modifications.
When these
functions are in hand, insert their addresses
in the jump table at the beginning of file
.CW /sys/src/libmach/obj.c .
.LP
.IP 5.
Implement the required debugger support functions and
initialize the parameters and jump table of the
.CW Machdata
data structure for the architecture.
This code is conventionally stored in
a file named
.CW /sys/src/libmach/\fIm\fPdb.c
where
.I m
is the identifier character assigned to the architecture.
The fields of the
.CW Machdata
structure are:
.RS
.IP "\f(CWbpinst\fP and \f(CWbpsize\fP\ -\ "
These fields
contain the breakpoint instruction and the size
of the instruction, respectively.
.IP "\f(CWswab\fP\ -\ "
This field
contains the address of a function to
byte-swap a 16-bit value.  Choose
.CW leswab
or
.CW beswab
for little-endian or big-endian architectures, respectively.
.IP "\f(CWswal\fP\ -\ "
This field
contains the address of a function to
byte-swap a 32-bit value.  Choose
.CW leswal
or
.CW beswal
for little-endian or big-endian architectures, respectively.
.IP "\f(CWctrace\fP\ -\ "
This field
contains the address of a function to perform a
C-language stack trace.  Two general trace functions,
.CW risctrace
and
.CW cisctrace ,
traverse fixed-frame and relative-frame stacks,
respectively.  If the compiler for the
new architecture conforms to one of
these models, select the appropriate function.  If the
stack model is unique,
supply a custom stack trace function.
.IP "\f(CWfindframe\fP\ -\ "
This field
contains the address of a function to locate the stack
frame associated with a text address.
Generic functions
.CW riscframe
and
.CW ciscframe
process fixed-frame and relative-frame stack
models.
.IP "\f(CWufixup\fP\ -\ "
This field
contains the address of a function to adjust
the base address of the register save area.
Currently, only the
68020 requires this bias
to offset over the active
exception frame.
.IP "\f(CWexcep\fP\ -\ "
This field
contains the address of a function to produce a
text
string describing the
current exception.
Each architecture stores exception
information uniquely, so this code must always be supplied.
.IP "\f(CWbpfix\fP\ -\ "
This field
contains the address of a function to adjust an
address prior to laying down a breakpoint.
.IP "\f(CWsftos\fP\ -\ "
This field
contains the address of a function to convert a single
precision floating point value
to a string.  Choose
.CW leieeesftos
for little-endian
or
.CW beieeesftos
for big-endian architectures.
.IP "\f(CWdftos\fP\ -\ "
This field
contains the address of a function to convert a double
precision floating point value
to a string.  Choose
.CW leieeedftos
for little-endian
or
.CW beieeedftos
for big-endian architectures.
.IP "\f(CWfoll\fP, \f(CWdas\fP, \f(CWhexinst\fP, and \f(CWinstsize\fP\ -\ "
These fields point to functions that interpret machine
instructions.
They rely on disassembly of the instruction
and are unique to each architecture.
.CW Foll
calculates the follow set of an instruction.
.CW Das
disassembles a machine instruction to assembly language.
.CW Hexinst
formats a machine instruction as a text
string of
hexadecimal digits.
.CW Instsize
calculates the size in bytes, of an instruction.
Once the disassembler is written, the other functions
can usually be implemented as trivial extensions of it.
.LP
It is possible to provide support for a new architecture
incrementally by filling the jump table entries
of the
.CW Machdata
structure as code is written.  In general, if
a jump table entry contains a zero, application
programs requiring that function will issue an
error message instead of attempting to
call the function.  For example,
the
.CW foll ,
.CW das ,
.CW hexinst ,
and
.CW instsize
jump table slots can be zeroed until a
disassembler is written.
Other capabilities, such as
stack trace or variable inspection,
can be supplied and will be available to
the debuggers but attempts to use the
disassembler will result in an error message.
.RE
.IP 6.
Update the table named
.CW machines
near the beginning of
.CW /sys/src/libmach/setmach.c .
This table binds the
file type code and machine name to the
.CW Mach
and
.CW Machdata
structures of an architecture.
The names of the initialized
.CW Mach
and
.CW Machdata
structures built in steps 2 and 5
must be added to the list of
structure definitions immediately
preceding the table initialization.
If both Plan 9 and
native disassembly are supported, add
an entry for each disassembler to the table.  The
entry for the default disassembler (usually
Plan 9) must be first.
.IP 7.
Add an entry describing the architecture to
the table named
.CW trans
near the end of
.CW /sys/src/cmd/prof.c .
.RE
.IP 8.
Add an entry describing the architecture to
the table named
.CW objtype
near the start of
.CW /sys/src/cmd/pcc.c .
.RE
.IP 9.
Recompile and install
all application programs that include header file
.CW mach.h
and load with
.CW libmach.a .
