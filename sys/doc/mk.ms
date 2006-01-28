.HTML "Maintaining Files on Plan 9 with Mk
.TL
Maintaining Files on Plan 9 with Mk
.AU
Andrew G. Hume
andrew@research.att.com
Bob Flandrena
bobf@plan9.bell-labs.com
.AB
.PP
.CW Mk
is a tool
for describing and maintaining dependencies between
files.
It is similar to the
UNIX program
.CW make ,
but provides several extensions.
.CW Mk\fR'\fPs
flexible rule specifications, implied
dependency derivation, and parallel
execution of maintenance actions are
well-suited to the Plan 9 environment.
Almost all Plan 9 maintenance procedures
are automated using
.CW mk .
.AE
.NH 1
Introduction
.PP
This document describes how
.CW mk ,
a program functionally similar to
.CW make
[Feld79],
is used to maintain dependencies between
files in Plan 9.
.CW Mk
provides several extensions to the
capabilities of its predecessor that work
well in Plan 9's distributed, multi-architecture
environment.  It
exploits the power of multiprocessors by executing
maintenance actions in parallel and interacts with
the Plan 9 command interpreter
.CW rc
to provide a powerful set of maintenance tools.
It accepts pattern-based dependency specifications
that are not limited to describing
rules for program construction.
The result is a tool that is flexible enough to
perform many maintenance tasks including
database maintenance,
hardware design, and document production.
.PP
This document begins by discussing 
the syntax of the control file,
the pattern matching capabilities, and
the special rules for maintaining archives.
A brief description of
.CW mk\fR'\fPs
algorithm for deriving dependencies
is followed by a discussion
of the conventions used to resolve ambiguous
specifications.  The final sections
describe parallel execution
and special features.
.PP
An earlier paper [Hume87]
provides a detailed discussion of
.CW mk\fR'\fPs
design and an appendix summarizes
the differences between
.CW mk
and
.CW make .
.NH 1
The \f(CWMkfile\fP
.PP
.CW Mk
reads a file describing relationships among files
and executes commands to bring the files up to date.
The specification file, called a
.CW mkfile ,
contains three types of statements:
assignments, includes, and rules.
Assignment and include statements are similar
to those in C.
Rules specify dependencies between a
.I target
and its
.I prerequisites .
When the target and prerequisites are files, their
modification times determine if they
are out of date.  Rules often contain a
.I recipe ,
an
.I rc (1)
script that produces the target from
the prerequisites.
.PP
This simple
.CW mkfile
produces an executable
from a C source file:
.P1
CC=pcc
f1:	f1.c
	$CC -o f1 f1.c
.P2
The first line assigns the name of the portable ANSI/POSIX compiler
to the
.CW mk
variable
.CW CC ;
subsequent references of the form
.CW $CC
select this compiler.
The only rule specifies a dependence between the target file
.CW f1
and the prerequisite file
.CW f1.c .
If the target does not exist or if the
prerequisite has been modified more recently than
the target,
.CW mk
passes the recipe to
.CW rc
for execution.  Here,
.CW f1.c
is compiled and loaded to produce
.CW f1 .
.PP
The native Plan 9 environment
requires executables for
all architectures, not only the current one.
The Plan 9 version of the same
.CW mkfile
looks like:
.P1
</$objtype/mkfile

f1:	f1.$O
	$LD $LDFLAGS -o f1 f1.$O
f1.$O:	f1.c
	$CC $CFLAGS f1.c
.P2
The first line is an include statement
that replaces itself with the contents of the file
.CW /$objtype/mkfile .
The variable
.CW $objtype
is inherited from the environment and
contains the name of the target architecture.
The prototype
.CW mkfile
for that architecture defines architecture-specific variables:
.CW CC
and
.CW LD
are the names of the compiler and loader,
.CW O 
is the code character of the architecture.
The rules compile the source file into an object
file and invoke the loader to produce
.CW f1 .
Invoking
.CW mk
from the command line as follows
.P1
% objtype=mips mk
vc -w f1.c
vl $LDFLAGS -o f1 f1.k
%
.P2
produces the
.CW mips
executable of program
.CW f1
regardless of the current architecture type.
.PP
We can extend the
.CW mkfile
to build two programs:
.P1
</$objtype/mkfile
ALL=f1 f2

all:V:	$ALL

f1:	f1.$O
	$LD $LDFLAGS -o f1 f1.$O
f1.$O:	f1.c
	$CC $CFLAGS f1.c
f2:	f2.$O
	$LD $LDFLAGS -o f2 f2.$O
f2.$O:	f2.c
	$CC $CFLAGS f2.c
.P2
The target
.CW all ,
modified by the
.I attribute
.CW V ,
builds both programs.
The attribute identifies 
.CW all
as a dummy target that is
not related to a file of the same name;
its precise effect is explained later.
This example describes cascading dependencies:
the first target depends on another which depends on a third and
so on.
Here, individual rules build each
program; later we'll see how to do this with a
general rule.
.NH 1
Variables and the environment
.PP
.CW Mk
does not distinguish between its
internal variables and
.CW rc
variables in the environment.
When
.CW mk
starts, it imports each environment variable into a
.CW mk
variable of the same name.  Before executing a recipe,
.CW mk
exports all variables, including those
inherited from the environment,
to the environment in which
.CW rc
executes the recipe.
.PP
There are several ways for a
variable to take a value.
It can be set with an assignment statement,
inherited from the environment, or specified
on the command line.
.CW Mk
also maintains several special internal variables
that are described in
.I mk (1).
Assignments have the following decreasing order of precedence:
.LP
.in .7i
1)  Command line assignment
.br
2)  Assignment statement
.br
3)  Imported from the environment
.br
4)  Implicitly set by \f(CWmk\fP
.in 0
.LP
For example, a command line assignment overrides
a value imported from the environment.
.PP
All variable values are strings.  They can be
used for pattern matching and
comparison but not for arithmetic.
A
.I list
is a string containing several values separated by
white space.  Each member is
handled individually during pattern matching,
target selection, and prerequisite evaluation.
.PP
A
.I namelist
is a list produced by
transforming the members of an existing list.
The transform applies a pattern to each member,
replacing each matched string with a new string,
much as in the substitute command in
.I sam (1)
or
.I ed (1).
The syntax is
.P1
${\fIvar\fP:A%B=C%D}
.P2
where
.I var
is a variable.
The pattern
.CW A%B
matches a member beginning with the string
.I A
and ending with the string
.I B
with any string in between;
it behaves like the regular expression
.CW A.*B .
When a member of the
.I var
list
matches this pattern,
the string
.I C
replaces
.I A ,
.I D
replaces
.I B ,
and the matched string replaces itself.
Any of
.I A ,
.I B ,
.I C ,
or
.I D
may be the empty string.  In effect, a namelist is
generated by applying the
.I ed (1)
substitute command
.P1
	s/\fIA\fP(.*)\fIB\fP/\fIC\fP\e1\fID\fP/
.P2
to each member of a variable list.
.PP
Namelists are useful for generating
a list based on a predictable transformation.
For example,
.P1
	SRC=a.c b.c c.c
	OBJ=${SRC:%.c=%.v}
.P2
assigns the list \f(CW(a.v b.v c.v)\fP to
.CW OBJ .
A namelist may be used anywhere a variable is allowed
except in a recipe.
.PP
Command output is assigned to a variable
using the normal
.CW rc
syntax:
.P1
	var=`{rc command}
.P2
The command executes in an environment populated
with previously assigned variables, including those
inherited from
.CW mk\fR'\fPs
execution environment.
The command may
be arbitrarily complex; for example,
.P1
	TARG=`{ls -d *.[cy] | sed 's/..$//'}
.P2
assigns a list of the C and yacc source files in the current
directory, stripped of their suffix, to the variable
.CW TARG .
.NH 1
The include statement
.PP
The include statement
replaces itself with the contents of a file.
It is functionally similar to the C
.CW #include
statement but uses a different syntax:
.P1
	<\fIfilename\fP
.P2
The contents of the file are evaluated
as they are read.
An include statement may be used anywhere except
in a recipe.
.PP
Unlike
.CW make ,
.CW mk
has no built-in rules.  Instead,
the include statement allows generic rules
to be imported from a prototype
.CW mkfile ;
most Plan 9
.CW mkfiles
use this approach [Flan95].
.NH 1
Rules
.PP
A rule has four elements: targets,
prerequisites, attributes, and a recipe.
It has the form:
.P1
\fItargets\fP:\fIattributes\fP:\fIprerequisites\fP
	\fIrecipe\fP
.P2
The first line, containing the
targets, attributes, and prerequisites is
the
.I "rule header" ;
it
must begin at the left margin.
The recipe contains zero or more lines,
each of which begins with white space.
One or more targets must be specified but the
attributes, prerequisites, and recipe are optional.
A rule specifies
a dependency between the target(s) and its prerequisite(s),
the recipe brings the target(s)
up to date with the prerequisite(s) and
attributes modify
.CW mk\fR'\fPs
evaluation of the dependency.
.PP
Normally the target is a file that depends
on one or more prerequisite files.
.CW Mk
compares the modification times of each target
and each prerequisite; a target is considered out of date
when it does not exist or when a prerequisite has been modified
more recently.
When a target is out of date,
.CW mk
executes the
recipe to bring it up to date.
When the recipe completes,
the modification time of the target is checked and
used in later dependency evaluations.
If the recipe does not update the target,
evaluation continues with the out of date target.
.PP
A prerequisite of one rule
may be the target of another.  When
this happens, the rules cascade
to define a multi-step procedure.
For example,
an executable target depends on prerequisite
object files, each of which is a target
in a rule with a C source file as the prerequisite.
.CW Mk
follows a chain of dependencies until it encounters
a prerequisite that is not a target of another rule
or it finds a target that
is up to date.  It then
executes the recipes in reverse order to produce
the desired target.
.PP
The rule header is evaluated when the rule is read.
Variables are replaced by their values, namelists are
generated, and
commands are replaced by their
output at this time.
.PP
Most attributes modify
.CW mk\fR'\fPs
evaluation of a rule.
An attribute is usually a single letter but some
are more complicated.
This paper only discusses commonly used attributes;
see
.I mk (1)
for a complete list.
.PP
The
.CW V
attribute identifies a
.I virtual 
target;
that is, a target that is not a file.
For example,
.P1
clean:V:
	rm *.$O $O.out
.P2
removes executables and compiler intermediate files.
The target is virtual because it does not refer to a file named
.CW clean .
Without the attribute, the recipe would not be
executed if a file named
.CW clean 
existed.
The
.CW Q
attribute
silences the printing of a recipe before
execution.
It is useful when the output of a recipe is
similar to the recipe:
.P1
default:QV:
	echo 'No default target; use mk all or mk install'
.P2
.PP
The recipe is an
.CW rc
script.  It is optional but when it is
missing, the rule is handled specially, as described later.
Unlike
.CW make ,
.CW mk
executes recipes without interpretation.
After
stripping the first white space character from each line
it passes the entire recipe to
.CW rc 
on standard input.
Since
.CW mk
does not interpret a recipe,
escape conventions are exactly those of
.CW rc .
Scripts for
.CW awk
and
.CW sed
commands can be embedded exactly as they would
be entered from the command line.
.CW Mk
invokes
.CW rc
with the
.CW -e
flag, which causes
.CW rc
to stop if any command
in the recipe exits with a non-zero status; the
.CW E
attribute overrides this behavior and allows
.CW rc
to continue executing in the face of errors.
Before a recipe is executed, variables are exported
to the environment where they are available to
.CW rc .
Commands in the recipe may not read from
standard input because
.CW mk
uses it internally.
.PP
References to a variable can yield different
values depending on the location of the
reference in the
.CW mkfile .
.CW Mk
resolves variable references
in assignment statements and rule headers
when the statement is read.  Variable references
in recipes are evaluated by
.CW rc
when the recipe is executed; this
happens after the entire
.CW mkfile
has been read.  The value of a variable in a recipe
is the last value assigned in the file.  For example,
.P1
STRING=all

all:VQ:
	echo $STRING
STRING=none
.P2
produces the message
.CW none .
A variable assignment in a recipe
does not affect the value of the variable in the
.CW mkfile 
for two reasons.
First,
.CW mk
does not import values from
the environment when a recipe completes;
one recipe cannot pass a value through
the environment to another recipe.
Second, no recipe is executed until 
.CW mk
has completed its evaluation, so even if a variable
were changed,
it would not affect the dependency evaluation.
.NH 1
Metarules
.PP
A
.I metarule
is a rule based on a pattern.
The pattern selects a class of target(s) and 
identifies related prerequisites.
.CW Mk
metarules may select targets and prerequisites
based on any criterion that can be described by a pattern, not just
the suffix transformations associated with program
construction.
.PP
Metarule patterns are either
.I intrinsic
or regular expressions conforming to the
syntax of
.I regexp (6).
The intrinsic patterns are shorthand
for common regular expressions.
The intrinsic pattern
.CW %
matches one or more of anything; it is equivalent to
the regular expression
.CW `.+' .
The other intrinsic pattern,
.CW & ,
matches one or more of any characters except \f(CW`/'\fP
and \f(CW`.'\fP.
It matches a portion of a path and is
equivalent to the regular expression
.CW `[^./]+' .
An intrinsic pattern in a prerequisite references
the string matched by the same intrinsic pattern in the target.
For example, the rule
.P1
	%.v:	%.c
.P2
says that a file ending in
.CW .v
depends on a file of the same name with a
.CW .c
suffix:
.CW foo.v
depends on
.CW foo.c ,
.CW bar.v
depends on
.CW bar.c , 
and so on.
The string matched by an intrinsic pattern in the target
is supplied to the recipe in the variable
.CW $stem .
Thus the rule
.P1
%.$O:	%.c
	$CC $CFLAGS $stem.c
.P2
creates an object file for the target architecture from
a similarly named C source file.  If several object
files are out of date, the rule is applied repeatedly and
.CW $stem
refers to each file in turn.
Since there is only one
.CW stem
variable, there can only be one
.CW %
or
.CW &
pattern in a target;
the pattern
.CW %-%.c
is illegal.
.PP
Metarules simplify the
.CW mkfile
for building programs
.CW f1
and
.CW f2 :
.P1
</$objtype/mkfile

ALL=f1 f2

all:V:	$ALL

%:	%.$O
	$LD -o $target $prereq
%.$O:	%.c
	$CC $CFLAGS $stem.c
clean:V:
	rm -f $ALL *.[$OS]
.P2
(The variable
.CW $OS
is a list of code characters for all architectures.)
Here, metarules specify
compile and load steps for all C source files.
The loader rule relies on two internal variables
set by
.CW mk
during evaluation of the rule:
.CW $target
is the name of the target(s) and
.CW $prereq
the name of all prerequisite(s).
Metarules allow this
.CW mkfile
to be easily extended; a new program
is supported by adding its name to the third line.
.PP
A regular expression metarule must have an
.CW R
attribute.
Prerequisites may reference matching substrings in
the target using the form
.CW \e\fIn\fP
where
.I n
is a digit from 1 to 9 specifying the
.I n th
parenthesized sub-expression.  In a recipe,
.CW $stem\fIn\fP
is the equivalent reference.
For example, a compile rule could be
specified using regular expressions:
.P1
(.+)\e.$O:R:	\e1.c
	$CC $CFLAGS $stem1.c
.P2
Here,
.CW \e1
and
.CW $stem1
refer to the name of the target object file without the
suffix.  The variable
.CW $stem
associated with an intrinsic pattern is undefined
in a regular expression metarule.
.NH 1
Archives
.PP
.CW Mk
provides a special mechanism for maintaining an archive.
An archive member is referenced using the form
.CW \fIlib\fP(\fIfile\fP)
where
.I lib
is the name of the archive and 
.I file
is the name of the member.  Two rules define the
dependency between an object file and its membership
in an archive:
.P1
$LIB(foo.8):N:	foo.8
$LIB:	$LIB(foo.8)
	ar rv $LIB foo.8
.P2
The first rule establishes a dependency between the
archive member and the object file.
Normally,
.CW mk
detects an error when a target does not exist and the rule
contains no recipe; the
.CW N
attribute overrides this behavior because the subsequent rule
updates the member.
The second
rule establishes the dependency between the member and
the archive; its recipe inserts the member
into the archive.
This two-step specification allows the modification time
of the archive
to represent the state of its members.  Other rules
can then specify the archive as a prerequisite instead of
listing each member.
.PP
A metarule generalizes library maintenance:
.P1
LIB=lib.a
OBJS=etoa.$O atoe.$O ebcdic.$O

$LIB(%):N:	%
$LIB:	${OBJS:%=$LIB(%)}
	ar rv $LIB $OBJS
.P2
The namelist prerequisite of the
.CW $LIB
target generates archive member names for each object file name;
for example, 
.CW etoa.$O
becomes
.CW lib.a(etoa.$O) .
This formulation always updates all members.
This is acceptable for a small archive, but may 
be slow for a big one.
The rule
.P1
$LIB:	${OBJS:%=$LIB(%)}
	ar rv $LIB `{membername $newprereq}
.P2
only updates out of date object files.
The internal variable
.CW $newprereq
contains the names of the out of
date prerequisites.  The
.CW rc
script
.CW membername
transforms an archive member specification into a file name:
it translates
.CW lib.a(etoa.$O)
into
.CW etoa.$O .
.PP
The
.CW mkfile
.P1
</$objtype/mkfile
LIB=lib.a
OBJS=etoa.$O atoe.$O ebcdic.$O

prog:	main.$O $LIB
	$LD -o $target $prereq

$LIB(%):N:	%
$LIB:	${OBJS:%=$LIB(%)}
	ar rv $LIB $OBJS
.P2
builds a program by loading it with a library.
.NH 1
Evaluation algorithm
.PP
For each target of interest,
.CW mk
uses the rules in a
.CW mkfile
to build a data
structure called a dependency graph.  The nodes of
the graph represent targets and prerequisites;
a directed arc
from one node to another indicates that
the file associated with the first node depends
on the file associated with the second.
When the
.CW mkfile
has been completely read, the graph is analyzed.
In the first step, implied dependencies are resolved by
computing the
.I "transitive closure"
of the graph.
This calculation extends the graph to include all
targets that are potentially
derivable from the rules in the
.CW mkfile .
Next the graph is checked for cycles;
.CW make
accepts cyclic dependencies, but
.CW mk
does not allow them.
Subsequent steps
prune subgraphs that are irrelevant for producing the
desired target and verify that there is only one way
to build it.
The recipes associated with the
nodes on the longest path between the
target and an out of date prerequisite
are then executed in reverse order.
.PP
The transitive closure calculation is sensitive to
metarules; the patterns often select many potential targets
and cause the graph to grow rapidly.
Fortunately,
dependencies associated with the desired target
usually form a small part of the graph, so, after
pruning, analysis is tractable.
For example, the rules
.P1
%:	x.%
	recipe1
x.%:	%.k
	recipe2
%.k:	%.f
	recipe3
.P2
produce a graph with four nodes for each file in the
current directory.
If the desired target is
.CW foo ,
.CW mk
detects the dependency between it
and the original file
.CW foo.f
through intermediate dependencies on
.CW foo.k
and
.CW x.foo .
Nodes associated with other files are deleted during pruning because
they are irrelevant to the production of
.CW foo .
.PP
.CW Mk
avoids infinite cycles by evaluating
each metarule once.
Thus, the rule
.P1
%:	%.z
	cp $prereq $prereq.z
.P2
copies the prerequisite file once.
.NH 1
Conventions for evaluating rules
.PP
There must be only one
way to build each target.  However, during evaluation
metarule patterns often select potential targets that
conflict with the
targets of other rules.
.CW Mk
uses several conventions to resolve ambiguities
and to select the proper dependencies.
.PP
When a target selects more than one rule,
.CW mk
chooses a regular rule
over a metarule.
For example, the
.CW mkfile
.P1
</$objtype/mkfile

FILES=f1.$O f2.$O f3.$O

prog:	$FILES
	$LD -o $target $prereq

%.$O:	%.c
	$CC $CFLAGS $stem.c

f2.$O:	f2.c
	$CC f2.c
.P2
contains two rules that could build
.CW f2.$O .
.CW Mk
selects the last rule because its target,
.CW f2.$O ,
is explicitly specified, while the 
.CW %.$O
rule is a metarule.  In effect,
the explicit rule for
.CW f2.$O
overrides the general rule for building object files from
C source files.
.PP
When a rule has a target and prerequisites but no recipe,
those prerequisites are added to all other rules with
recipes that have the same target.
All prerequisites, regardless of where they were specified, are
exported to the recipe in variable
.CW $prereq .
For example, in
.P1
</$objtype/mkfile

FILES=f1.$O f2.$O f3.$O

prog:	$FILES
	$LD -o $target $prereq

%.$O:	hdr.h

%.$O:	%.c
	$CC $CFLAGS $stem.c
.P2
the second rule adds
.CW hdr.h
as a prerequisite of the compile metarule;
an object file produced from a C source file
depends on
.CW hdr.h
as well as the source file.  Notice that the recipe of 
the compile rule uses
.CW $stem.c
instead of
.CW $prereq
because the latter specification would attempt to compile
.CW hdr.h .
.PP
When a target is virtual and there is no other rule with
the same target,
.CW mk
evaluates each prerequisite.
For example, adding the rule
.P1
all:V:	prog
.P2
to the preceding example builds the executable
when either
.CW prog
or
.CW all
is the specified target.  In effect, the
.CW all
target is an alias for
.CW prog .
.PP
When two rules have identical rule headers and both have
recipes, the later rule replaces the former one.
For example,
if a file named
.CW mkrules
contains
.P1
$O.out:	$OFILES
	$LD $LFLAGS $OFILES
%.$O:	%.c
	$CC $CFLAGS $stem.c
.P2
the
.CW mkfile
.P1
OFILES=f1.$O f2.$O f3.$O

<mkrules

$O.out:	$OFILES
	$LD $LFLAGS -l $OFILES -lbio -lc
.P2
overrides the general loader rule with a special
rule using a non-standard library search sequence.
A rule is neutralized by overriding it with a rule
with a null recipe:
.P1
<mkrules

$O.out:Q:	$OFILES
	;
.P2
The
.CW Q
attribute suppresses the printing of the semicolon.
.PP
When a rule has no prerequisites, the recipe is executed
only when the target does not exist.  For example,
.P1
marker:
	touch $target
.P2
defines a rule to manage a marker file.
If the file exists, it is considered up to date
regardless of its modification time.
When a virtual target has no prerequisites the
recipe is always executed.
The
.CW clean
rule is of this type:
.P1
clean:V:
	rm -f [$OS].out *.[$OS]
.P2
When a rule without prerequisites has multiple targets, the
extra targets are aliases for the rule.
For example, in
.P1
clean tidy nuke:V:
	rm -f [$OS].out *.[$OS]
.P2
the
rule can be invoked by any of three names.
The first rule in a
.CW mkfile
is handled specially:
when
.CW mk
is invoked without a command line target
all targets of the first non-metarule are built.
If that rule has multiple targets, the recipe
is executed once for each target; normally, the recipe
of a rule with multiple targets is only executed once.
.PP
A rule applies to a target only when its prerequisites
exist or can be derived.  More than one rule may have the
same target as long as only one rule with a recipe
remains applicable after the dependency evaluation completes.
For example, consider a program built from C
and assembler source files.  Two rules produce
object files:
.P1
%.$O:	%.c
	$CC $CFLAGS $stem.c
%.$O:	%.s
	$AS $AFLAGS $stem.s
.P2
As long as there are not two source files with names like
.CW \fIfoo\fP.c
and
.CW \fIfoo\fP.s ,
.CW mk
can unambiguously select the proper rule.
If both files exist,
the rules are ambiguous
and
.CW mk
exits with an error message.
.PP
In Plan 9, many programs consist of portable code stored
in one directory and architecture-specific source stored in
another.
For example, the
.CW mkfile
.P1
</$objtype/mkfile

FILES=f1.$O f2.$O f3.$O f3.$O

prog:	$FILES
	$LD -o $target $prereq

%.$O:	%.$c
	$CC $CFLAGS $stem.c

%.$O:	../port/%.c
	$CC $CFLAGS ../port/$stem.c
.P2
builds the program named
.CW prog
using portable code in directory
.CW ../port
and architecture-specific code in the current directory.
As long as the
names of the C source files in 
.CW ../port
do not conflict with the names of files in the current directory,
.CW mk
selects the appropriate rule to build the object file.
If like-named files exist in both directories, the
specification is ambiguous and an explicit target
must be specified to resolve the ambiguity.
For example,
adding the rule
.P1
f2.$O:	f2.c
	$CC $CFLAGS $f2.c
.P2
to the previous
.CW mkfile
uses the architecture-specific version of
.CW f2.c
instead of the portable one.
Here, the explicit rule unambiguously
documents which of the
like-named source files is used to build the program.
.PP
.CW Mk\fR'\fP s
heuristics can produce unintended results
when rules are not carefully specified.
For example, the rules that build
object files from C or assembler source files
.P1
%.$O:	%.c
	$CC $CFLAGS $stem.c
%.$O:	%.s
	$AS $AFLAGS $stem.s
.P2
illustrate a subtle pratfall.
Adding a header file dependency to the compile rule
.P1
%.$O:	%.c hdr.h
	$CC $CFLAGS $stem.c
.P2
produces the error message
.P1
.CW "don't know how to make '\fIfile\fP.c'"
.P2
when \fIfile\fP.s is an assembler
source file.
This occurs because 
.CW \fIfile\fP.s
satisfies the assemble rule and
.CW hdr.h
satisfies the compile rule, so
either rule can potentially produce the target.
When a prerequisite exists or can be
derived,
all other prerequisites in that
rule header must exist or be derivable; here,
the existence of
.CW hdr.h
forces the evaluation of a C source file.
Specifying the dependencies in different
rules avoids this interpretation:
.P1
%.$O:	hdr.h
%.$O:	%.c
	$CC $CFLAGS $stem.c
.P2
Although
.CW hdr.h
is an additional prerequisite of the compile rule,
the two rules are evaluated independently and
the existence of the C source file is not linked
to the existence of the header file.
However, this specification describes a different
dependency.  Originally, only object
files derived from C files depended on
.CW hdr.h ;
now all object files, including those built
from assembler source, depend on the header file.
.PP
Metarule patterns should be as restrictive as possible to
prevent conflicts with other rules.
Consider the
.CW mkfile
.P1
</$objtype/mkfile
BIN=/$objtype/bin
PROG=foo

install:V:	$BIN/$PROG

%:	%.c
	$CC $stem.c
	$LD -o $target $stem.$O

$BIN/%:	%
	mv $stem $target
.P2
The first target builds an executable
in the local directory; the second
installs it in the directory
of executables for the architecture.
Invoking
.CW mk
with the
.CW install
target produces:
.P1 0
mk: ambiguous recipes for /mips/bin/foo:
/mips/bin/foo <-(mkfile:8)- /mips/bin/foo.c <-(mkfile:12)- foo.c
/mips/bin/foo <-(mkfile:12)- foo <-(mkfile:8)- foo.c
.P2
The prerequisite of the
.CW install
rule,
.CW $BIN/$PROG ,
matches both metarules because the
.CW %
pattern matches everything.
The
.CW &
pattern restricts the compile rule to files in the
current directory and avoids the conflict:
.P1
&:	&.c
	$CC $stem.c
	$LD -o $target $stem.$O
.P2
.NH 1
Missing intermediates
.PP
.CW Mk
does not build a missing intermediate file if a target
is up to date with the prerequisites of the intermediate.
For example,
when an executable is up to date with its source file,
.CW mk
does not compile the source to create a missing object file.
The evaluation only applies
when a target is considered up to date by pretending that the
intermediate exists.  Thus, it does not apply
when the intermediate is a command line target
or when it has no prerequisites.
.PP
This capability is useful for
maintaining archives.  We can modify the archive
update recipe to remove object files after
they are archived:
.P1
$LIB(%):N:	%
$LIB:	${OBJS:%=$LIB(%)}
	names=`{membername $newprereq}
	ar rv $LIB $names
	rm -f $names
.P2
A subsequent
.CW mk
does not remake the object files as long as the members
of the archive remain up to date with the source files.
The
.CW -i
command line option overrides this behavior
and causes all intermediates to be built.
.NH 1
Alternative out-of-date determination
.PP
Sometimes the modification time is not useful
for deciding when a target and prerequisite are out of date.
The
.CW P
attribute replaces the default mechanism with the result of
a command.  The command immediately follows the attribute
and is repeatedly executed with each
target and each prerequisite as its arguments;
if its exit status is non-zero, they are considered out of date
and the recipe is executed.  Consider the
.CW mkfile
.P1
foo.ref:Pcmp -s:	foo
	cp $prereq $target
.P2
The command
.P1
cmp -s foo.ref foo
.P2
is executed and if 
.CW foo.ref
differs from
.CW foo ,
the latter file is copied to the former.
.NH 1
Parallel processing
.PP
When possible,
.CW mk
executes recipes in parallel.
The variable
.CW $NPROC
specifies the maximum number of simultaneously executing
recipes.
Normally it is imported from the environment,
where the system has set it to the number of available processors.
It can be decreased by assigning a new
value and can be set to 1 to force single-threaded recipe execution.
This is necessary when several targets access
a common resource such as
a status file or data base.
When there is no dependency between targets,
.CW mk
assumes the
recipes can be
executed concurrently.
Normally, this allows
multiple prerequisites to be built simultaneously;
for example, the object file prerequisites of
a load rule can be produced by compiling the source files in parallel.
.CW Mk
does not define the order of execution of independent recipes.
When the prerequisites of a rule are not independent,
the dependencies between them should be specified in a rule or the
.CW mkfile
should be single-threaded.
For example, the archive update rules
.P1
$LIB(%):N:	%
$LIB:	${OBJS:%=$LIB(%)}
	ar rv $LIB `{membername $newprereq}
.P2
compile source files in parallel but update
all members of the archive at once.
It is a mistake to merge the two rules
.P1
$LIB(%):	%
	ar rv $LIB $stem
.P2
because an
.CW ar
command is executed for every
member of the library.  Not only is this
inefficient, but the archive is updated
in parallel, making interference likely.
.PP
The
.CW $nproc
environment variable contains a number associated
with the processor executing a recipe.
It can be used to create unique
names when the
recipe may be executing simultaneously on several processors.
Other maintenance tools provide mechanisms to control recipe
scheduling explicitly [Cmel86], but
.CW mk\fR'\fPs
general rules are sufficient for all but the most unusual cases.
.NH 1
Deleting target files on errors
.PP
The
.CW D
attribute
causes
.CW mk
to remove the target file when a
recipe terminates prematurely.
The error message describing the
termination condition warns
of the deletion.
A partially built file is doubly dangerous:
it is not only wrong, but is also
considered to be up to date so
a subsequent
.CW mk
will not rebuild it.  For example,
.P1
pic.out:D:	mk.ms
		pic $prereq | tbl | troff -ms > $target
.P2
produces the message
.P1
.CW "mk: pic mk.ms | ...  : exit status=rc 685: deleting 'pic.out'"
.P2
if any program in the recipe exits with an error status.
.NH 1
Unspecified dependencies
.PP
The
.CW -w
command line flag forces the
files following the flag to be treated
as if they were just modified.
We can use this flag with a command that selects files
to force a build based on the selection criterion.
For example, if the declaration of
a global variable named
.I var
is changed in a header file,
all source files that reference
it can be rebuilt with the command
.P1
$ mk -w`{grep -l \fIvar\fP *.[cyl]}
.P2
.NH 1
Conclusion
.PP
There are many programs related to
.CW make ,
each choosing a different balance between
specialization and generality.
.CW Mk
emphasizes generality but allows
customization through its pattern specifications and
include facilities.
.PP
Plan 9 presents a difficult maintenance environment
with its heterogeneous
architectures and languages.
.CW Mk\fR'\fPs
flexible specification language and simple
interaction with
.CW rc
work well in this environment.
As a result,
Plan 9 relies on
.CW mk
to automate almost all maintenance.
Tasks as diverse as updating the
network data base, producing the manual,
or building a release are expressed as
.CW mk
procedures.
.NH 1
References
.LP
[Cmel86] R. F. Cmelik,
``Concurrent Make: A Distributed Program in Concurrent C'',
AT&T Bell Laboratories Technical Report, 1986.
.LP
[Feld79] S. I. Feldman,
``Make \(em a program for maintaining computer programs'',
.I
Software Practice & Experience ,
.R
1979
Vol 9 #4,
pp. 255-266.
.LP
[Flan95] Bob Flandrena,
``Plan 9 Mkfiles'',
this volume.
.LP
[Hume87] A. G. Hume,
``Mk: A Successor to Make'',
.I
USENIX Summer Conf. Proc.,
.R
Phoenix, Az.
.NH 1
Appendix: Differences between
.CW make
and
.CW mk
.PP
The differences between
.CW mk
and
.CW make
are:
.IP \(bu 3n
.CW Make
builds targets when it needs them, allowing systematic use of side effects.
.CW Mk
constructs the entire dependency graph before building any target.
.IP \(bu
.CW Make
supports suffix rules and
.CW %
metarules.
.CW Mk
supports
.CW %
and regular expression metarules.
(Older versions of
.CW make
support only suffix rules.)
.IP \(bu
.CW Mk
performs transitive closure on metarules,
.CW make
does not.
.IP \(bu
.CW Make
supports cyclic dependencies,
.CW mk
does not.
.IP \(bu
.CW Make
evaluates recipes one line at a time, replacing variables by their values and
executing some commands internally.
.CW Mk
passes the entire recipe to the shell without
interpretation or internal execution.
.IP \(bu
.CW Make
supports parallel execution of single-line recipes when building
the prerequisites for specified targets.
.CW Mk
supports parallel execution of all recipes.
(Older versions of
.CW make
did not support parallel execution.)
.IP \(bu
.CW Make
uses special targets (beginning with a period)
to indicate special processing.
.CW Mk
uses attributes to modify rule evaluation.
.IP \(bu
.CW Mk
supports virtual
targets that are independent of the file system.
.IP \(bu
.CW Mk
allows non-standard out-of-date determination,
.CW make
does not.
.PP
It is usually easy to convert a
.CW makefile
to or from an equivalent
.CW mkfile .
