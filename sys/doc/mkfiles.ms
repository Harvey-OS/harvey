.HTML "Plan 9 Mkfiles
.TL
Plan 9 Mkfiles
.AU
Bob Flandrena
bobf@plan9.bell-labs.com
.SH
Introduction
.LP
Every Plan 9 source directory contains a file, called
.CW mkfile ,
specifying the rules for building the executable or
library that is the product of the directory.
.I Mk (1)
interprets the rules in the file, calculates
the dependencies, and executes an
.I rc (1)
script to construct the product.
If necessary components are supplied by
neighboring directories or sub-directories, the mkfiles in those
directories are first executed to build the components
before the local construction proceeds.
.LP
Most application source directories produce one of
four types of product:
a single executable, several
executables, a local library, or
a system library.
Four generic
mkfiles
define the normal rules
for building each type of product.  The simplest
mkfiles need only
list the components
and include the appropriate
generic
mkfile 
to do the work.
More complex 
mkfiles
may supply additional rules
to augment, modify, or override the generic rules.
.SH
Using a Mkfile
.LP
To build a product, change to the directory containing
its source and invoke
.I mk
with the appropriate target as an argument.
All mkfiles provide the following standard targets:
.TS
lw(1i) lw(4.5i).
\f(CWall\fP	T{
Build a local version of the product or products for the
current architecture.  If the product is a single program,
the result is stored in file
.CW $O.out .
If the directory produces multiple executables, they are
stored in the files named
.CW $O.\fIprogname,\fP
where
.I progname
is the name of each executable.
A product may be built for a different architecture by
prefacing the
.CW mk
command with
\f(CWobjtype=\fP\fIarchitecture\fP,
where
.I architecture
is the name of the target architecture.
Directories producing system
libraries always operate directly on the installed version of the
library; in this case the target
.CW all
is equivalent to the target
.CW install .
T}
\f(CWinstall\fP	T{
Build and install the product or products for the current
architecture.
T}
\f(CWinstallall\fP	T{
Build and install the product or products for all architectures.
T}
\f(CWclean\fP	T{
Rid the directory and its subdirectories of the by-products of
the build process.  Intermediate files that are easily reproduced
(e.g., object files,
.CW yacc
intermediates, target executables) are always
removed.  Complicated intermediates, such as local libraries, are
usually preserved.
T}
\f(CWnuke\fP	T{
Remove all intermediates from the directory and any subdirectories.
This target guarantees that a subsequent build for the
architecture is performed
from scratch.
T}
.TE
.LP
If no target is specified on the
.CW mk
command line, the
.CW all
target is built by default.  In a directory
producing multiple executables, there is
no default target.
.LP
In addition to the five standard targets,
additional targets may be supplied by each
generic mkfile or by the directory's mkfile.
.LP
The environment variable
.CW NPROC
is set by the system to the number of
available processors.
Setting
this variable, either in the environment or in
a mkfile, controls the amount of parallelism in
the build.  For example, the command
.P1
	NPROC=1 mk
.P2
restricts a build to a single thread of execution.
.SH
Creating a Mkfile
.LP
The easiest way to build a new mkfile is to copy and modify
an existing mkfile of the same type.
Failing that, it is usually possible to create a new
mkfile with minimal effort, since the appropriate
generic mkfile predefines the rules that do all the work.
In the simplest and most common cases, the new mkfile
need only define a couple of variables and include the appropriate
architecture-specific
and generic mkfiles.
.SH The Generic Mkfiles
.LP
There are four generic mkfiles containing commonly
used rules for building a product:
.CW mkone ,
.CW mkmany ,
.CW mklib ,
and
.CW mksyslib .
These rules
perform such actions as compiling C source files,
loading object files, archiving libraries, and
installing executables in the
.CW bin
directory of the appropriate architecture.
The generic mkfiles are stored in directory
.CW /sys/src/cmd .
Mkfile
.CW mkone
builds a single executable,
.CW mkmany
builds several executables from the source in a single
directory, and
.CW mklib
and
\f(CWmksyslib\fP,
maintain local and system libraries, respectively.
The rules in the generic mkfiles are driven by
the values of variables, some of which must be
set by the product mkfile and some of which are
supplied by the generic mkfile.  Variables in the
latter class include:
.TS
center;
ri ci li
rw(1i) cw(0.5i) lw(2i).
Variable	Default	Meaning
.sp .5
\f(CWCFLAGS\fP	\f(CW-FVw\fP	C compiler flags
\f(CWLDFLAGS\fP		Loader flags
\f(CWYFLAGS\fP	\f(CW-d\fP	Yacc flags
\f(CWAFLAGS\fP		Assembler flags
.TE
.LP
The following variables are set by the product mkfile
and used by the generic mkfile.
Any may be empty depending on the specific product being
made.
.TS
center;
lw(1i) lw(2.5i).
\f(CWTARG\fP	Name(s) of the executable(s) to be built
\f(CWLIB\fP	Library name(s)
\f(CWOFILES\fP	Object files
\f(CWHFILES\fP	Header files included by all source files
\f(CWYFILES\fP	\f(CWYacc\fP input files
\f(CWBIN\fP	Directory where executables are installed
.TE
.SH
Mkfile Organization
.LP
All
mkfiles
share the following common structure:
.P1
</$objtype/mkfile	# \f1architecture-dependent definitions\fP
.sp
\fIvariable definitions\fP		# TARG\f1, \fPOFILES\f1, \fPHFILES\f1, etc.\fP
.sp
</sys/src/cmd/\fIgeneric\fP	# mkone\f1, \fPmkmany\f1, \fPmklib\f1, or \fPmksyslib
.sp
\fIvariable overrides\fP		# CFLAGS\f1, \fPobjtype\f1, etc.\fP
.sp
\fIextra rules\fP			# \f1overrides, augmented rules, additional targets\fP
.P2
Note that the architecture-dependent mkfiles include file
.CW /sys/src/mkfile.proto
for system-wide variables that are common to all architectures.
.LP
The variables driving the expansion of the generic mkfile
may be specified in any order as long as they are defined
before the inclusion of the generic mkfile.  The value
of a variable may be changed by assigning a new value
following the inclusion of the generic mkfile, but the
effects are sometimes counter-intuitive.
Such variable assignments do not apply to the target and
prerequisite portions of any previously defined rules;
the new values only apply to the recipes of rules preceding
the assignment statement and
to all parts of any rules following it.
.LP
The rules supplied by the generic mkfile may
be overridden or augmented.  The new rules must
be specified after the inclusion of the generic
mkfile.  If the target and prerequisite portion
of the rule exactly match the target and prerequisite
portion of a previously defined rule and the new rule contains
a recipe, the new rule replaces the old one.
If the target of a new rule exactly matches the
target of a previous rule and one or more new
prerequisites are specified and the new rule contains
no recipe, the new prerequisites are added to the prerequisites
of the old rule.
.LP
Following sections discuss
each generic mkfile in detail.
.SH
Mkone
.LP
The
.CW mkone
generic mkfile contains rules for building
a single executable from one or more files
in a directory.
The variable
.CW TARG
specifies the name of the executable and
variables
.CW OFILES
and
.CW YFILES
specify the object files and
.CW yacc
source files used to build it.
.CW HFILES
contains the names of the local header files
included in all source files.
.CW BIN
is the name of the directory where the executable
is installed.
.CW LIB
contains the names of local libraries used by the
linker.  This variable is rarely needed
as libraries referenced by a
.CW #pragma
directive in an associated header file, including
all system libraries, are automatically
searched by the loader.
.LP
If
.CW mk
is executed without a target, the
.CW all
target is built; it
produces an executable in
.CW $O.out .
Variable
.CW HFILES
identifies the header files that
are included in all or most or
the C source files.  Occasionally,
a program has other header files
that are only used in some
source files.  A
header can be added to the prerequisites for
those object files by adding a rule of
the following form following the inclusion of generic mkfile
.CW mkone :
.P1
file.$O:	header.h
.P2
.LP
The mkfile for a directory producing a single
executable using the normal set of rules is
trivial: a list of some files followed by the
inclusion of
.I mkone.
For example, 
.CW /sys/src/cmd/diff/mkfile
contains:
.P1
< /$objtype/mkfile

TARG=diff
OFILES=\e
	diffdir.$O\e
	diffio.$O\e
	diffreg.$O\e
	main.$O\e

HFILES=diff.h

BIN=/$objtype/bin
</sys/src/cmd/mkone
.P2
The more complex mkfile in
.CW /sys/src/cmd/awk
overrides compiler and loader variables to
select the ANSI/POSIX Computing Environment with appropriately
defined command line variables.  It also overrides
the default
.CW yacc
rule to place the output soure in file
.CW awkgram.c
and the
.CW clean
and
.CW nuke
rules, so it can remove the non-standard intermediate
files.  Finally, the last three rules build a version of
.CW maketab
appropriate for the architecture where the
.CW mk
is being
run and then executes it to create source file
.CW proctab.c :
.P1
</$objtype/mkfile

TARG=awk
OFILES=re.$O\e
	lex.$O\e
	main.$O\e
	parse.$O\e
	proctab.$O\e
	tran.$O\e
	lib.$O\e
	run.$O\e
	awkgram.$O\e

HFILES=awk.h\e
	y.tab.h\e
	proto.h\e

YFILES=awkgram.y

BIN=/$objtype/bin
</sys/src/cmd/mkone
CFLAGS=-c -D_REGEXP_EXTENSION -D_RESEARCH_SOURCE \e
	-D_BSD_EXTENSION -DUTF
YFLAGS=-S -d -v
CC=pcc
LD=pcc
cpuobjtype=`{sed -n 's/^O=//p' /$cputype/mkfile}

y.tab.h awkgram.c:	$YFILES
	$YACC -o awkgram.c $YFLAGS $prereq

clean:V:
	rm -f *.[$OS] [$OS].out [$OS].maketab y.tab.? y.debug\e
		 y.output $TARG

nuke:V:
	rm -f *.[$OS] [$OS].out [$OS].maketab y.tab.? y.debug\e
		 y.output awkgram.c $TARG

proctab.c:	$cpuobjtype.maketab
	./$cpuobjtype.maketab >proctab.c

$cpuobjtype.maketab:	y.tab.h maketab.c
	objtype=$cputype
	mk maketab.$cputype

maketab.$cputype:V:	y.tab.h maketab.$O
	$LD -o $O.maketab maketab.$O
.P2
.SH
Mkmany
.LP
The
.CW mkmany
generic mkfile builds several
executables from the files in a
directory.  It differs from the operation of
.CW mkone
in three respects:
.CW TARG
specifies the names of all executables,
there is no default command-line target,
and additional rules allow a single executable to
be built or installed.
.LP
The
.CW TARG
variable specifies the names of all
executables produced by the mkfile.  The
rules assume the name of each executable is also
the name of the file containing its
.CW main
function.
.CW OFILES
specifies files containing
common subroutines loaded with all executables.
Consider the mkfile:
.P1
</$objtype/mkfile

TARG=alpha beta
OFILES=common.$O
BIN=/$objtype/bin
</sys/src/cmd/mkmany
.P2
It assumes the main functions for executables
.CW alpha
and
.CW beta
are in files
.CW alpha.$O
and
.CW beta.$O
and that both programs use the subroutines
in file
.CW common.$O .
The
.CW all
target builds all executables, leaving each in
a file with a name of the form
.CW $O.\fIprogname\fP
where
.I progname
is the name of the executable.  In this
example the
.CW all
target produces executables
.CW $O.alpha
and 
.CW $O.beta .
.LP
The
.CW mkmany
rules provide additional
targets for building a single
executable:
.TS
lw(1i) lw(3.8i).
\f(CW$O.progname\fP	T{
Builds executable 
\f(CW$O.\fP\fIprogname\fP
in the current directory.  When the target
architecture is not the current architecture
the
.CW mk
command
must be prefixed with the customary
.CW objtype=\fIarchitecture\fP
assignment to select the proper compilers and loaders.
T}
\f(CWprogname.install\fP	T{
Installs executable
.I progname
for the target architecture.
T}
\f(CWprogname.installall\fP	T{
Installs executable
.I progname
for all architectures.
T}
.TE
.SH
Mklib
.LP
The
.CW mklib
generic mkfile builds a local library.
Since this form of mkfile constructs no
executable, the
.CW TARG 
and
.CW BIN
variables are not needed.  Instead, the
.CW LIB
variable specifies the library
to be built or updated.  Variable
.CW OFILES
contains the names of the object files to be archived
in the library.  The use of variables
.CW YFILES
and
.CW HFILES
does not change.  When possible, only the
out-of-date members of the library are updated.
.LP
The variable
.CW LIBDIR
contains the name of the directory where the
library is installed; by default it selects
the current directory.  It can be overridden
by assigning the new directory name after the
point where
.CW mklib
is included.
.LP
The
.CW clean
target removes object files and
.CW yacc
intermediate files but does not touch the
library.  The
.CW nuke
target removes the library as well as the
files removed by the
.CW clean
target.  The command
.RS
.CW "mk -s clean all"
.RE
causes the existing library to be updated, or
created if it doesn't already exist.  The command
.RS
.CW "mk -s nuke all"
.RE
forces the library to be rebuilt from scratch.
.LP
The mkfile from
.CW /sys/src/cmd/upas/libString
contains the following specifications to
build the local library
.CW libString.a$O
for the object architecture referenced by
.CW $O\fR\:\fP
.P1
</$objtype/mkfile

LIB=libString.a$O
OFILES=	s_alloc.$O\e
	s_append.$O\e
	s_array.$O\e
	s_copy.$O\e
	s_getline.$O\e
	s_grow.$O\e
	s_nappend.$O\e
	s_parse.$O\e
	s_read.$O\e
	s_read_line.$O\e
	s_tolower.$O\e

</sys/src/cmd/mklib

nuke:V:
	mk clean
	rm -f libString.a[$OS]
.P2
The override of the rule for target
.CW nuke
removes the libraries for all architectures as
opposed to the default recipe for this target
which removes the library for the current architecture.
.SH
Mksyslib
.LP
The
.CW mksyslib
generic mkfile is similar to the
.CW mklib
mkfile except that it operates on a system library
instead of a local library.
The
.CW install
and
.CW all
targets are the same; since there is no local copy of
the library, all updates are performed on the
installed library.
The rule for the
.CW nuke
target is identical to that of the
.CW clean
target; unlike the
.CW nuke
target for local libraries,
the library is never removed.
.LP
No attempt is made to determine if individual library
members are up-to-date; all members of a
library are always updated.
Special targets support manipulation of a single
object file; the target
.CW objfile
updates file
.CW objfile\f(CW.$O\fP
in the library of the current architecture and the target
.CW objfile.all
updates
.CW objfile\f(CW.$O\fP
in the libraries of all architectures.
.SH
Overrides
.LP
The rules provided by a generic mkfile or
the variables used to control the evaluation
of those rules may be overridden in most
circumstances.  Overrides
must be specified in the product mkfile
after the point where the generic
mkfile is included; in general, variable
and rule overrides occupy the end of a
product mkfile.
.LP
The value of a variable is overridden by
assigning a new value to the variable.
Most variable overrides modify the
values of flags or the names of commands executed
in recipes.  For example, the default value of
.CW CFLAGS
is often overridden or augmented and
the ANSI/POSIX Computing Environment is selected by
setting the
.CW CC
and
.CW LD
variables to
.CW pcc.
.LP
Modifying rules is trickier than modifying
variables.  Additional constraints can be added
to a rule by specifying the target and
the new prerequisite.  For example,
.P1
%.$O:	header.h
.P2
adds file
.CW header.h
the set of prerequisites for all object files.
There is no mechanism for adding additional
commands to an existing recipe; if a
recipe is unsatisfactory, the rule and its recipe
must be completely overridden.
A rule is overridden only when the replacement rule
matches the target and prerequisite portions
of the original rule exactly.  The recipe
associated with the new rule
then replaces the recipe of the original rule.
For example,
.CW /sys/src/cmd/lex/mkfile
overrides the default
.CW installall
rule to perform the normal loop on all
architectures and then copy a prototype file
to the system library directory.
.P1
</$objtype/mkfile

TARG=lex
OFILES=lmain.$O\e
	y.tab.$O\e
	sub1.$O\e
	sub2.$O\e
	header.$O\e

HFILES=ldefs.h\e

YFILES=parser.y\e

BIN=/$objtype/bin
</sys/src/cmd/mkone

installall:V:
	for(objtype in $CPUS)
		mk install
	cp ncform /sys/lib/lex
.P2
Another way to perform the same override is to
add a dependency to the default
.CW installall
rule that executes an additional rule to
install the prototype file:
.P1
installall:V:	ncform.install

ncform.install:V:
	cp ncform /sys/lib/lex
.P2
.SH
Special Tricks
.LP
Two special cases
require extra deviousness.
.LP
In the first, a file needed to build an
executable is generated by a program that,
in turn, is built from a source file that
is not part of the product.  In this case,
the
executable must be built for the
target architecture, but the intermediate
executable must be built for the architecture
.CW mk
is executing on.  The intermediate executable
is built by recursively invoking
.CW  mk
with the appropriate target and the
executing architecture as the target
architecture.  When that
.CW mk
completes, the intermediate is
executed to generate the source file to
complete the build for the target architecture.
The earlier example of
.CW /sys/src/cmd/awk/mkfile
illustrates this technique.
.LP
Another awkward situation
occurs when a directory contains
source to build an executable as
well as source for auxiliary executables
that are not to be installed.  In this case
the
.CW mkmany
generic rules are inappropriate, because
all executables would be built and installed.
Instead, use the
.CW mkone
generic file to build the primary executable
and provide extra targets to
build the auxiliary files.  This
approach is also useful when the auxiliary
files are not executables;
.CW /sys/src/cmd/spell/mkfile
augments the default rules to build and install the
.CW spell
executable with
elaborate rules to generate
and maintain the auxiliary spelling lists.
