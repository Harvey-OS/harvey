.HTML "APE — The ANSI/POSIX Environment
.de XX
.IP \ \ \ \-
..
.TL
APE \(em The ANSI/POSIX Environment
.AU
Howard Trickey
howard@plan9.bell-labs.com
.SH
Introduction
.PP
When a large or frequently-updated program must be ported
to or from Plan 9, the ANSI/POSIX environment known as APE can be useful.
APE combines the set of headers and object code libraries specified by
the ANSI C standard (ANSI X3.159-1989) with the POSIX operating system
interface standard (IEEE 1003.1-1990, ISO 9945-1), the part of POSIX
defining the basic operating system functions.
Using APE will cause slower compilation and marginally slower execution speeds,
so if the importing or exporting happens only infrequently, due consideration
should be given to using the usual Plan 9 compilation environment instead.
Another factor to consider is that the Plan 9 header organization is
much simpler to remember and use.
.PP
There are some aspects of required POSIX behavior that are impossible or
very hard to simulate in Plan 9.  They are described below.
Experience has shown, however, that the simulation is adequate for the
vast majority of programs.  A much more common problem is that
many programs use functions or headers not defined by POSIX.
APE has some extensions to POSIX to help in this regard.
Extensions must be explicitly enabled with an appropriate
.CW #define ,
in order that the APE environment be a good aid for testing
ANSI/POSIX compliance of programs.
.SH
Pcc
.PP
The
.CW pcc
command acts as a front end to the Plan 9 C compilers and loaders.
It runs an ANSI C preprocessor over source files, using the APE
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
Psh and Cc
.PP
The
.CW pcc
command is intended for uses where the source code is
ANSI/POSIX, but the programs are built in the usual Plan 9
manner \(em with
.CW mk
and producing object files with names ending in
.CW .v ,
etc.
Sometimes it is best to use the standard POSIX
.CW make
and
.CW cc
(which produces object files with names ending in
.CW .o ,
and automatically calls the loader unless
.CW -c
is specified).
Under these circumstances, execute the command:
.DS
.CW "ape/psh"
.DE
This starts a POSIX shell, with an environment that
includes the POSIX commands
.CW ar89 ,
.CW c89 ,
.CW cc ,
.CW basename ,
.CW dirname ,
.CW expr ,
.CW false ,
.CW grep ,
.CW kill ,
.CW make ,
.CW rmdir ,
.CW sed ,
.CW sh ,
.CW stty ,
.CW true ,
.CW uname ,
and
.CW yacc .
There are also a few placeholders for commands that cannot be
implemented in Plan 9:
.CW chown ,
.CW ln ,
and
.CW umask .
.PP
The
.CW cc
command accepts the options mandated for
the POSIX command
.CW c89 ,
as specified in the C-Language Development Utilities Option
annex of the POSIX Shell and Utilities standard.
It also accepts the following nonstandard options:
.CW -v
for echoing the commands for each pass to stdout;
.CW -A
to turn on ANSI prototype warnings;
.CW -S
to leave assembly language in
.I file .s;
.CW -Wp,\fIargs\fP
to pass
.I args
to the
.CW cpp ;
.CW -W0,\fIargs\fP
to pass
.I args
to 2c, etc.;
and
.CW -Wl,\fIargs\fP
to pass
.I args
to 2l, etc.
.PP
The
.CW sh
command is pdksh, a mostly POSIX-compliant public domain Korn Shell.
The Plan 9 implementation does not include
the emacs and vi editing modes.
.PP
The
.CW stty
command only has effect if the
.CW ape/ptyfs
command has been started to interpose a pseudo-tty interface
between
.CW /dev/cons
and the running command.
None of the distributed commands do this automatically.
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
.CW #defines
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
To export a program, it helps to know whether it fits
in one of the following categories:
.IP 1.
Strictly conforming ANSI C program. It only uses features of the language,
libraries, and headers explicitly required by the C standard.  It does not
depend on unspecified, undefined, or implementation-dependent behavior,
and does not exceed any minimum implementation limit.
.IP 2.
Strictly conforming POSIX program. Similar, but for the POSIX standard as well.
.IP 3.
Some superset of POSIX, with extensions.  Each extension
is selected by a feature test macro, so it is clear which extensions
are being used.
.PP
With APE, if headers are always included to declare any library functions
used, then the set of feature test macros defined by a program will
show which of the above categories the program is in.
To accomplish this, no symbol is defined in a header if it is not required
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
is defined, in which case the symbols required by POSIX are also defined.
This means that a program that uses
.CW ENAMETOOLONG
cannot masquerade as a strictly conforming ANSI C program.
.PP
.CW Pcc
and
.CW cc
do not predefine any preprocessor symbols except those required by
the ANSI C standard:
.CW __STDC__ ,
.CW __LINE__ ,
.CW __FILE__ ,
.CW __DATE__ ,
and
.CW __TIME__ .
Any others must be defined in the program itself or by using
.CW -D
on the command line.
.SH
Extensions
.PP
The discipline enforced by putting only required
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
header to declare the needed types and functions.
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
.CW _BSD_EXTENSION .
This extension includes not only Berkeley Unix routines,
but also a grab bag of other miscellaneous routines often
found in Unix implementations.
The extension allows the inclusion of any of:
.CW <bsd.h>
for
.CW bcopy() ,
.CW bcmp() ,
and similar Berkeley functions;
.CW <netdb.h>
for
.CW gethostbyname() ,
etc.,
and associated structures;
.CW <select.h>
for the Berkeley
.CW select
function and associated types and macros
for dealing with multiple input sources;
.CW <sys/ioctl.h>
for the
.CW ioctl
function (minimally implemented);
.CW <sys/param.h>
for
.CW NOFILES_MAX ;
.CW <sys/pty.h>
for pseudo-tty support via the
.CW ptsname(int)
and
.CW ptmname(int)
functions;
.CW <sys/resource.h> ;
.CW <sys/socket.h>
for socket structures, constants, and functions;
.CW <sys/time.h>
for definitions of the
.CW timeval
and
.CW timezone
structures;
and
.CW <sys/uio.h>
for the
.CW iovec
structure and the
.CW writev
and
.CW readv
functions used for scatter/gather I/O.
Defining
.CW _BSD_EXTENSION
also enables various extra definitions in
.CW <ctype.h> ,
.CW <signal.h> ,
.CW <stdio.h> ,
.CW <unistd.h> ,
.CW <sys/stat.h> ,
and
.CW <sys/times.h> .
.XX
.CW _NET_EXTENSION .
This extension allows inclusion of
.CW <libnet.h> ,
which defines the networking functions described in the Plan 9 manual page
.I dial (2).
.XX
.CW _PLAN9_EXTENSION .
This extension allows inclusion of
.CW <u.h> ,
.CW <lock.h> ,
.CW <qlock.h> ,
.CW <utf.h> ,
.CW <fmt.h> ,
and
.CW <draw.h> .
These are pieces of Plan 9 source code ported into APE,
mostly from 
.CW <libc.h> .
.XX
.CW _REGEXP_EXTENSION .
This extension allows inclusion of
.CW <regexp.h> ,
which defines the regular expression matching functions described
in the Plan 9 manual page
.I regexp (2).
.XX
.CW _RESEARCH_SOURCE .
This extension enables a small library of functions from the Tenth Edition Unix
Research System (V10).
These functions and the types needed to use them are all defined in the
.CW <libv.h>
header.
The provided functions are:
.CW srand ,
.CW rand ,
.CW nrand ,
.CW lrand ,
and
.CW frand
(better random number generators);
.CW getpass ,
.CW tty_echoon ,
.CW tty_echooff
(for dealing with the common needs for mucking with terminal
characteristics);
.CW min
and
.CW max ;
.CW nap ;
and
.CW setfields ,
.CW getfields ,
and
.CW getmfields
(for parsing a line into fields).
See the Research Unix System Programmer's Manual, Tenth Edition, for a description
of these functions.
.XX
.CW _C99_SNPRINTF_EXTENSION .
This extension permits the use of the return values of
.I snprintf
and
.I vsnprintf .
Before C99, the 1999 C standard,
these functions usually returned the number of bytes,
excluding terminating NUL,
actually stored in the target string.
(GNU, as usual, had to be different and returned -1 if the target
string was too small.)
C99 requires them to instead return the number of bytes,
excluding terminating NUL,
that would have been written into the target string if it were infinitely large
or a negative value if an `encoding error' occurs,
so old programs compiled under C99 rules will be prone to overrunning
their buffers.
This extension is a way for the programmer to declare that he or she understands
the situation and has adjusted the code being compiled to compensate.
.SH
Common Problems
.PP
Some large systems, including X11, have been ported successfully
to Plan 9 using APE
(the X11 port is not included in the distribution, however,
because supporting it properly is too big a job).
The problems encountered fall into three categories:
(1) non-ANSI C/POSIX features used; (2) inadequate simulation of POSIX functions;
and (3) compiler/loader bugs.
By far the majority of problems are in the first category.
.PP
POSIX is just starting to be a target for programmers.
Most existing code is written to work with one or both of a BSD or a System V Unix.
System V is fairly close to POSIX, but there are some differences.
Also, many System V systems have imported some BSD features that are
not part of POSIX.
A good strategy for porting external programs is to first try using
.CW CFLAGS=-D_POSIX_SOURCE ;
if that doesn't work, try adding
.CW _D_BSD_EXTENSION
and perhaps include
.CW <bsd.h>
in source files.
Here are some solutions to problems that might remain:
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
.CW <varargs.h> .
Rewrite to use
.CW <stdarg.h> .
.PP
The second class of problems has to do with inadequacies in the Plan 9
simulation of POSIX functions.
These shortcomings have rarely gotten in the way
(except, perhaps, for the
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
The approximation used is only sometimes correct.
.XX
.CW link
always fails.
.XX
With
.CW open ,
the
.CW O_NOCTTY
option has no effect.
The concept of a controlling tty is foreign to Plan 9.
.XX
.CW setsid
forks the name space and note group,
which is only approximately the right behavior.
.XX
The functions dealing with stacking signals,
.CW sigpending ,
.CW sigprocmask
and
.CW sigsuspend ,
do not work.
.XX
.CW umask
has no effect, as there is no such concept in Plan 9.
.XX
code that does
.CW getenv("HOME")
should be changed to
.CW getenv("home")
on Plan 9.
