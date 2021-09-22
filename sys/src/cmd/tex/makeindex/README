% /u/sy/beebe/tex/makeindex/2-12/makeindex/src/README, Wed May 26 11:25:46 1993
% Edit by Nelson H. F. Beebe <beebe@plot79.math.utah.edu>
=================================================================
= As of the release of MakeIndex version 2.9, the original
= author, Pehong Chen, is no longer able to maintain it, and is
= inaccessible via electronic mail.  The maintenance job has been
= taken over for the TeX User's Group by
=
=  	Nelson H. F. Beebe
=  	Center for Scientific Computing
=  	Department of Mathematics
=  	University of Utah
=  	Salt Lake City, UT 84112
=  	USA
=  	Tel: +1 801 581 5254
=  	FAX: +1 801 581 4148
=  	Email: <beebe@math.utah.edu>
=
= to whom reports of problems, ports to new environments, and
= general correspondence about MakeIndex, should be addressed.
=================================================================


==================
Building MakeIndex
==================

Makefiles for numerous systems are provided in the distribution
as the files makefile.*; select the appropriate one, and copy it
to Makefile.  For systems that lack a suitable make
implementation, command files (*.bat and *.com) are provided.

In many cases, once the makefile has been selected, you should be
able to do

	make
	(cd ../test; make)

and if the acceptance tests in that second step all pass (there
should be no output from any of the files differences, except on
VAX VMS, where a couple of lines differing only in letter case,
are expected), then on many UNIX systems, you may be able to do

	make install

to install the executable and manual pages on your system.

For version 2.12, the code has been updated to support compilation
with numerous C++ compilers.  No features of the C++ language are used
that do not also exist in C, but the stricter compile-time checking
provided by C++ compilers provides additional confidence in the code.
Thanks go to P. J. Plauger for his efforts in the ANSI/ISO Standard C
and C++ environments, and for promoting the idea of using C++
compilers for C code development.

If you have a C++ compiler, please try a build of MakeIndex with a
command like (select the one that corresponds to the name of your C++
compiler)

	make CC=CC
	make CC=xlC
	make CC=g++

Any problems with C++ compilations should be reported back to the
address above.

Before doing so however, check that BINDIR and MANDIR are set to
your local conventions (you can override the values in the
Makefile by setting them on the command-line), and also verify
that you do not have a brain-damaged install implementation that
either requires root privileges to run (e.g. IBM AIX), or moves
(instead of copies) files (e.g. Stardent or DEC ULTRIX).  In the
latter case, you might be able to instead do

	make cp-install

If you want to undo the installation,

	make uninstall

should work.

There are additional documentation files in ../doc that you may
want to install manually (Emacs info, local help systems, VAX VMS
help).


====================
Compilation Problems
====================

The standardization of the C language by ANSI X3.159-1989
Programming Language--C (December 15, 1989), and ISO C 1990, has
resulted in C implementations gradually being adapted to conform
to the requirements of the Standard, but released for use before
Standard conformance has been reached.  For a lot of C code, this
affects several main areas:

	(1) system header files, and what symbols they define
	(2) function prototype declarations
	(3) new preprocessor statements, and more
	    precisely-defined preprocessor behavior
	(4) new features introduced by Standard C (const,
	    adjacent string concatenation, new style function
	    definitions, new string escape sequences, trigraphs,
	    new data types)

MakeIndex has successfully compiled and run on many different
operating systems and C implementations, but until Standard C
becomes uniformly implemented, the first two above are the most
troublesome.

The code in MakeIndex is written to conform to Standard C, which
is distinguished by the preprocessor symbol __STDC__ being
permanently defined to a non-zero value.  Unfortunately, some
implementations define this symbol without having
Standard-conforming header files, or without accepting some of
the new language features.

In MakeIndex, function definitions are done in the old Kernighan and
Ritchie style in a pre-Standard C environment, but in Standard C and
C++ style if __STDC__ or __cplusplus are defined.

All functions referenced are declared by function prototype
declarations of the form

static	void	check_idx ARGS((char *fn,int open_fn));

where ARGS is defined in mkind.h to expand such a declaration to

static	void	check_idx (char *fn,int open_fn);

or

static	void	check_idx ();

depending on whether the compiler supports function prototype
declarations or not.  If you get syntax errors from the new
style, you may need to revert to the old style by introducing a

#define STDC_PROTOTYPES 0

statement in the appropriate section of mkind.h.  Some compilers
partly support the function prototypes, but without variable
names; use the old-style for them, and complain to your compiler
supplier.

When STDC_PROTOTYPES is non-zero, it is assumed that the Standard
C type modifier "const" is recognized; if your compiler complains
about it, add a

#define const

definition in mkind.h, or use a compile-time option -Dconst=;
either of these make it expand to an empty string, so the
compiler doesn't see it.

Standard C specifies the data types of all library functions and
their arguments, and the system header files are required to
declare them appropriately.  Nevertheless, some implementations
have a few erroneous declarations in the header files and still
claim to be Standard conforming (by defining the symbol __STDC__
to a non-zero value).  On such systems, the extra compile-time
type checking provided by function prototype declarations may
raise compilation errors, and it may be necessary to adjust
declarations in any of the files that use the ARGS macro
(genind.c, mkind.c, mkind.h, qsort.c, scanid.c, scanst.c, and
sortid.c) to make the code conform to the local implementation's
erroneous declarations.

Numerous external variables and functions in MakeIndex require
long names (at least 12 unique characters). For older systems
that do not support long names, make sure the symbol SHORTNAMES
is defined to a non-zero value in mkind.h; this is already done
for supported systems that need it.  Standard C requires
uniqueness in the first 6 characters of external names, WITHOUT
distinction between upper- and lower-case letters, so
implementations that do not allow long external names are NOT in
violation of the Standard.

If you get compiler warnings about conversion of integers to
pointers, or find that rindex() or index() cannot be unresolved
by the linker, then check the settings in mkind.h.  The default
is to use the Standard C functions strchr() and strrchr(), but on
a few non-conforming systems, to remap these names to index() and
rindex() instead.  Any other integer-to-pointer conversion
warnings are likely to be traced to missing function prototype
declarations, which is either because of an omission in system
header files, or because the MakeIndex declaration in one of its
source files was not selected during the preprocessing step.
When the MakeIndex source code is compiled correctly, EVERY
function used by MakeIndex will have been declared in a function
prototype prior to its first use.


======================
VAX VMS Considerations
======================

Different versions of VAX VMS C have used different
interpretations of the arguments to return() and exit()
statements.  The convention on UNIX and many other systems is
that a 0 value denotes success, and a non-zero value, failure.
The EXIT() macro in mkind.h hides the remapping of this value for
VAX VMS; for other systems, it calls exit() directly.

For VAX VMS, MakeIndex should be declared to be a foreign command
symbol in a system-wide, or user, startup file that is executed
at each login.  Here is an example:

$ makeindex :== $public$disk:[beebe.MakeIndex.src]makeindex.exe

The leading dollar sign identifies this as a foreign command
symbol, and must be followed by an ABSOLUTE file specification
for the executable program; this regrettably makes it site
dependent, and the definitions in ../test/vmsdiff.com and
elsewhere must be adjusted accordingly.

The symbol SW_PREFIX in mkind.h can be redefined from '-' to '/'
for those VAX VMS sites that prefer to retain VMS-style
command-line options; the default value is '-' on all systems.
This redefinition can be done on the command line, or by changing
the setting in mkind.h.


===============
Machines Tested
===============

Here is a list of machines, operating systems, and compilers under
which MakeIndex 2.12 has been successfully tested.  Convenience targets
in the Makefile provide support for them on UNIX.  Command files are
provided for MS DOS and VAX VMS.

------------------------------------------------------------------------
Machine			O/S			Compiler(s)
------------------------------------------------------------------------

DEC Alpha 3000/500X	OSF 1.2			cc, c89, cxx

DECstation 3100		ULTRIX 4.2 and 4.3	cc, gcc, g++, lcc

DEC VAX			VMS 5.4-2		cc

HP 9000/735		HP-UX 9.01		cc, c89, CC, gcc, g++

HP 9000/850		HP-UX 9.0		cc, c89

IBM 3090/600S-VF	AIX 2.1			cc (Metaware High C
						2.1t), CC

IBM PS/2		AIX 2.1			cc (Metaware High C
						2.2g), CC

IBM RS/6000		AIX 3.2			cc, xlC

MIPS R6000		RISC/os 5.0		cc

NeXT			Mach 3.0		cc, Objective C, gcc

Sun SPARCstation	SunOS 4.1.3		apcc, cc, CC, gcc, g++

------------------------------------------------------------------------
