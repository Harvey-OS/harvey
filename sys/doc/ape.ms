.de XX
.IP \ \ \ \-
..
.TL
APE \(em The ANSI/POSIX Environment
.AU
Howard Trickey
.SH
Introduction
.PP
When a large or frequently-updated program must be imported or exported
to and from Plan 9, the ANSI/POSIX environment known as APE can be useful.
APE combines the set of headers and object code libraries specified by
the ANSI C standard (ANSI X3.159-1989) with the POSIX operating system
interface standard (IEEE 1003.1-1990, ISO 9945-1).
Using APE will cause slower compilation and marginally slower execution speeds,
so if the importing or exporting happens only infrequently, due consideration
should be given to using the usual Plan 9 compilation environment instead.
Another factor to consider is that the Plan 9 header organization is
much simpler to remember and use.
.PP
There are some aspectes of required POSIX behavior that are impossible or
very hard to simulate in Plan 9.  They are described below.
Experience has shown, however, that the simulation is adequate for the
vast majority of programs.  The much more common problem is that
many programs use functions or headers not defined by POSIX.
APE has some extensions to POSIX to help in this regard.
.SH
Pcc
.PP
The
.CW pcc
command acts as a front end to the Plan 9 C compilers and loaders.
It runs an ANSI C preprecessor over source files, using the APE
headers to satisfy
.CW "#include <\fIfile\fP>"
directives; then it runs a Plan 9 C compiler; finally, it may load
with APE libraries to produce an executable program.
The document
.I "How to Use the Plan 9 C Compiler"
explains how environment variables are used by convention to
handle compilation for differing architectures.
The environment variable
.CW $objtype
controls which Plan 9 compiler and loader are used by
.CW pcc ,
as well as the location of header and library files.
For example, if
.CW $objtype
is
.CW mips ,
then
.CW pcc
has
.CW cpp
look for headers in
.CW /mips/include/ape
followed by
.CW /sys/include/ape ;
then
.CW pcc
uses
.CW vc
to create
.CW .v
object files;
finally,
.CW vl
is used to create an executable using libraries in
.CW /mips/lib/ape .
.SH
Symbols
.PP
The C and POSIX standards require that certain symbols be
defined in headers.
They also require that certain other classes of symbols not
be defined in the headers, and specify certain other
symbols that may be defined in headers at the discretion
of the implementation.
POSIX defines
.I "feature test macros" ,
which are preprocessor symbols beginning with an underscore
and then a capital letter;  if the program
.CW #define s
a feature test macro before the inclusion of any headers,
then it is requesting that certain symbols be visible in the headers.
The most important feature test macro is
.CW _POSIX_SOURCE :
when it is defined, exactly the symbols required by POSIX are
visible in the appropriate headers.
Consider
.CW <signal.h>
for example:
ANSI defines some names that must be defined in
.CW <signal.h> ,
but POSIX defines others, such as
.CW sigset_t ,
which are not allowed according to ANSI.
The solution is to make the additional symbols visible only when
.CW _POSIX_SOURCE
is defined.
.PP
When you export a program, it helps to know whether the program fits
in one of the following categories:
.IP 1.
Strictly conforming ANSI C program. It only uses features of the language,
libraries, and headers explicitly required by the C standard.  It does not
depend on unspecified, undefined, or implementation-dependent behavior,
and does not exceed any minimum implementation limit.
.IP 2.
Strictly conforming POSIX program. Similar, but for the POSIX standard.
.IP 3.
Some superset of POSIX, with extensions.  Each extension
is selected by a feature test macro, so it is clear which extensions
are being used.
.PP
With APE, if headers are always included to declare any library functions
used, then the set of feature test macros defined by a program will
show which of the above categories the program is in.
To accomplish this, no symbol is defined in a header if it not required
by the C or POSIX standard, and those required by the POSIX standard
are protected by
.CW "#ifdef _POSIX_SOURCE" .
For example,
.CW <errno.h>
defines
.CW EDOM ,
.CW ERANGE ,
and
.CW errno ,
as required by the C standard.
The C standard allows more names beginning with
.CW E ,
but our header defines only those unless
.CW _POSIX_SOURCE
is defined, in which case the 35 more symbols required by POSIX are defined.
This means that a program which uses
.CW ENAMETOOLONG
cannot masquerade as a strictly conforming ANSI C program.
.PP
.CW Pcc
does not predefine any preprocessor symbols except those required by
the ANSI C standard:
.CW __STDC__ ,
.CW __LINE__ ,
.CW __FILE__ ,
.CW __DATE__ ,
and
.CW __TIME__ .
Any others must be defined in the program itself or using
.CW -D
on the command line.
.SH
Extensions
.PP
The pedagogy enforced by only putting required
names in the headers is useful for exporting programs,
but it gets in the way when importing programs.
The compromise is to allow additional symbols in headers,
additional headers, and additional library functions,
but only under control of extension feature test macros.
The following extensions are provided; unless otherwise
specified, the additional library functions are in the
default APE library.
.XX
.CW _LIBG_EXTENSION .
This allows the use of the Plan 9 graphics library.
The functions are as described in the Plan 9 manual (see
.I graphics (2))
except that
.CW div
had to be renamed
.CW ptdiv .
Include the
.CW <libg.h>
header to declare the needed types and functions,
and use
.CW -lg
on the
.CW pcc
command line to load the graphics functions.
.XX
.CW _LIMITS_EXTENSION .
POSIX does not require that names such as
.CW PATH_MAX
and
.CW OPEN_MAX
be defined in
.CW <limits.h> ,
but many programs assume they are defined there.
If
.CW _LIMITS_EXTENSION
is defined, those names will all be defined when
.CW <limits.h>
is included.
.XX
.CW _BSDTIME_EXTENSION .
The header
.CW <sys/time.h>
is not defined by POSIX.
This extension allows the inclusion of that header,
with its definitions of the
.CW timeval
and
.CW timezone
structures.
.XX
.CW _SELECT_EXTENSION .
This extension allows the inclusion of the
.CW <select.h>
header, which defines the Berkeley
.CW select
function and associated types and macros
for dealing with multiple input sources.
The
.CW <sys/time.h>
header must be included before
.CW <select.h> ,
so the
.CW _BSDTIME_EXTENSION
is needed too.
.XX
.CW _PCONS_EXTENSION .
This defines a ``pseudo-tty'' capability.
If
.CW _PCONS_EXTENSION
is defined,
.CW <pcons.h>
may be included; it defines the functions
.CW pcons(void)
and
.CW pushtty(void).
.CW Pcons
arranges for a Plan 9 file server to provide the
.CW /dev/cons
and
.CW /dev/consctl
files.
It also provides
.CW /dev/mcons ,
which is the ``master end'' of the pseudo-tty:
characters written to
.CW /dev/mcons
can be read from
.CW /dev/cons
(or
.CW /dev/tty ,
a synonym)
and vice versa.
POSIX tty line processing is supported:
the contents of a
.CW termios
structure can be written to or read from the
.CW /dev/pttyctl
file in the format
.P1
"stty %8ux %8ux %8ux %8ux "
.P2
(the numbers are the
.CW c_iflag ,
.CW c_oflag ,
.CW c_cflag ,
and
.CW c_lflag ),
followed by
.CW NCCS
bytes of
.CW c_cc
data.
The
.CW pushtty
function uses
.CW pcons
to arrange that the standard input, output, and error files
behave as ttys are supposed to according to POSIX.
(This should not have to be done explicitly, but the cost of supporting
.CW tcgetattr
and
.CW tcputattr
is sufficiently high that it seemed better to not make it the default.)
.XX
.CW _NET_EXTENSION .
This extension allows inclusion of
.CW <libnet.h> ,
which defines the networking functions described in the Plan 9 manual page
.I dial (2).
Programs using them must be loaded with
.CW -lnet .
.XX
.CW _REGEXP_EXTENSION .
This extension allows inclusion of
.CW <libregexp.h> ,
which defines the regular expression matching functions described
in the Plan 9 manual page
.I regexp (2X).
Programs using them must be loaded with
.CW -lregexp .
.XX
.CW _RESEARCH_SOURCE .
This extension enables a small library of functions from the Tenth Edition Unix
Research System (V10).
These functions and the types needed to use them are all defined in the
.CW <libv.h>
header, and programs using them must be loaded with
.CW -lv .
The provided functions are:
.CW popen
and
.CW pclose
(common extensions to ANSI C stdio; the
.CW <stdio.h>
header must have been included);
.CW telldir
and
.CW seekdir
(common extensions to the ANSI C
.CW readdir
suite;
.CW <dirent.h>
must have been included);
.CW lstat ,
.CW symlink ,
and
.CW readlink
(dummy functions for Plan 9, since there are no symbolic links);
.CW srand ,
.CW rand ,
.CW nrand ,
.CW lrand ,
and
.CW frand
(better random number generators);
.CW getopt
(program argument handler);
.CW getpass ,
.CW tty_echoon ,
.CW tty_echooff
(for dealing with the common needs for mucking with terminal
characteristics);
.CW min
and
.CW max ;
.CW nap ;
.CW setfields ,
.CW getfields ,
and
.CW getmfields
(for parsing a line into fields);
and
.CW ftw
(for walking a file tree).
See the Research Unix Programmer's Manual for a description
of these functions.
.SH
Common Problems
.PP
Some large systems, including X11, have been ported successfully
to Plan 9 using APE.
The problems encountered fall into three categories:
(1) non- ANSI C/POSIX features used; (2) inadequate simulation of POSIX functions;
and (3) compiler/loader bugs.
By far the majority of problems are in the first category.
.PP
POSIX is just starting to be a target for programmers.
Most existing code is written to work with one or both of a BSD or a System V Unix.
System V is fairly close to POSIX, but there are some differences.
Also, many System V systems have imported some BSD features that are
not part of POSIX.
Here are some common non-POSIX things used in current programs, and what to
do about them.
.XX
.CW sys_errlist
and
.CW sys_nerr
globals.  Use
.CW strerror
instead.
.XX
Third (environment) argument to
.CW main .
Use the
.CW environ
global instead.
.XX
.CW OPEN_MAX ,
.CW PATH_MAX ,
etc., assumed in
.CW <limits.h> .
Rewrite to call
.CW sysconf
or define
.CW _LIMITS_EXTENSION .
.XX
.CW isascii
assumed in
.CW <ctype.h> .
Let the dead codeset go: leave the test out, or better still,
convert to use multibyte characters (c.f. runes in Plan 9).
Or define your own
.CW isascii
macro.
.XX
.CW <strings.h> ,
.CW index ,
and
.CW rindex .
Use
.CW <string.h> ,
.CW strchr ,
and
.CW strrchr
instead.
.XX
.CW mktemp .
Rewrite to use ANSI C
.CW tempnam
or
.CW tmpfile .
.XX
.CW <varargs.h> .
Rewrite to use
.CW <stdarg.h> .
.XX
.CW popen
assumed in
.CW <stdio.h> .
Use
.CW _RESEARCH_SOURCE
extension, include
.CW <libv.h> ,
and load with
.CW -lv .
Similarly for
.CW telldir ,
.CW seekdir ,
.CW lstat ,
.CW srand ,
.CW rand ,
.CW nrand ,
.CW lrand ,
.CW frand ,
and
.CW getopt .
.XX
.CW select .
Define
.CW _BSDTIME_EXTENSION
and
.CW _SELECT_EXTENSION
and include
.CW <sys/times.h>
and
.CW <select.h> .
.XX
Socket or stream stuff.
Rewrite to use the Plan 9 networking functions, define
.CW _NET_EXTENSION ,
include
.CW <libnet.h>
and load with
.CW -lnet .
.PP
The second class of problems has to do with inadequacies in the Plan 9
simulation of POSIX functions.
As long as this list is, these shortcomings have rarely gotten in the way
(except, perhaps for the
.CW link
problem).
.XX
Functions for setting the userid, groupid, effective userid and effective groupid
do not do anything useful.  The concept is impossible to simulate in Plan 9.
.CW Chown
also does nothing.
.XX
.CW execlp
and the related functions do not look at the
.CW PATH
environment variable.  They just try the current directory and
.CW /bin
if the pathname is not absolute.
.XX
Advisory locking via
.CW fcntl
is not implemented.
.XX
.CW isatty
is hard to do correctly.
The approximation used is to see if the file is the same as
.CW /dev/cons .
Sometimes this cannot be determined, and sometimes this is not the right test.
.XX
With
.CW kill ,
the behavior mandated for negative arguments (killing process groups)
is not implemented.
.XX
.CW link
always fails.
.XX
With
.CW open ,
the
.CW O_NOCTTY
option has no effect.
The concept of a ``controlling'' tty is foreign to Plan 9.
.XX
.CW setsid
does a
.CW forkpgrp ,
which is only approximately the right behavior.
.XX
The functions dealing with stacking signals,
.CW sigpending ,
.CW sigprocmask
and
.CW sigsuspend ,
do not work.
.XX
.CW system
uses
.CW rc
instead of
.CW sh
to interpret the argument.
Use of
.CW system
is discouraged for security reasons anyway.
.XX
.CW tcgetattr
and
.CW tcsetattr
only work if
.CW pcons
or
.CW pushtty
has been executed
(see the discussion about the
.CW _PCONS_EXTENSION ,
above ).
.XX
.CW umask
has no effect, as there is no such concept in Plan 9.
.XX
.CW utime
has no effect, because the time can't be changed in Plan 9.
